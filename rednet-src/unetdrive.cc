#include "unetdrive.h"
#include "uevent.h"
#include "ucontext.h"
#include "network/usocket_server.h"
#include <string.h>

using namespace rednet;

unetdrive::unetdrive(uintptr_t opaque) : opaque__(opaque)
{
}

unetdrive::~unetdrive()
{
}

void unetdrive::insert(unetsocket *s)
{
	if (!netSocketMap__.empty())
	{
		auto pit = netSocketMap__.find(s->socketId);
		if (pit != netSocketMap__.end())
		{
			netSocketMap__[s->socketId] = *s;
			return;
		}
	}

	netSocketMap__.insert(pair<int, unetsocket>(s->socketId, *s));
}

unetsocket *unetdrive::grab(int socketId)
{
	if (netSocketMap__.empty())
		return NULL;
	auto pit = netSocketMap__.find(socketId);
	if (pit != netSocketMap__.end())
		return &pit->second;
	return NULL;
}

void unetdrive::earse(int socketId)
{
	if (netSocketMap__.empty())
		return;
	auto pit = netSocketMap__.find(socketId);
	if (pit != netSocketMap__.end())
	{
		netSocketMap__.erase(pit);
	}
}

int unetdrive::beginListen(const char *address)
{
	int sz = strlen(address);
	char addrs[sz + 1];
	char prots[sz + 1];

	sscanf(address, "%[^:]:%[0-9]", addrs, prots);

	return doListen(addrs, atoi(prots), 1024);
}

int unetdrive::beginListen(const char *host, const int port, const int backlog)
{
	return doListen(host, port, backlog);
}

void unetdrive::beginAccept(int socketId, netAsynCallback asynFun)
{
	unetsocket ns;
	ns.socketId = socketId;
	memset(&ns.buffer, 0, sizeof(ns.buffer));
	ns.connected = false;
	ns.protocol = PROTOCOL_TCP;
	ns.onCallback = asynFun;
	ns.onWarning = nullptr;
	insert(&ns);
	usocket_server::socketStart(opaque__, socketId);
}

void unetdrive::endAccept(int socketId, netAsynCallback asynFun)
{
	unetsocket ns;
	ns.socketId = socketId;
	memset(&ns.buffer, 0, sizeof(ns.buffer));
	ns.connected = false;
	ns.protocol = PROTOCOL_TCP;
	ns.onCallback = asynFun;
	ns.onWarning = nullptr;
	insert(&ns);
}

int unetdrive::beginRecv(int socketId, netAsynRecv asynRecv)
{
	unetsocket *s = grab(socketId);
	if (s == NULL)
	{
		ucontext::error(opaque__, "socket error: unfound socket[%d]", socketId);
		return -1;
	}

	s->onRecv = asynRecv;
	usocket_server::socketStart(opaque__, socketId);
	return 0;
}

void unetdrive::setWarning(int socketId, netAsynCallback asynWaring)
{
	unetsocket *s = grab(socketId);
	if (s != NULL)
	{
		s->onWarning = asynWaring;
		return;
	}

	return;
}

const char *unetdrive::check(int socketId, int &outSz)
{
	unetsocket *s = grab(socketId);
	if (s == NULL)
	{
		return NULL;
	}
	else if (!s->connected)
	{
		return NULL;
	}
	else if (s->buffer.sz == 0)
	{
		return NULL;
	}

	outSz = s->buffer.sz;
	return (const char *)s->buffer.data;
}

int unetdrive::clone(int socketId)
{
	unetsocket *s = grab(socketId);
	if (s == NULL)
	{
		return 0;
	}
	else if (!s->connected)
	{
		return 0;
	}

	s->buffer.sz = 0;
	return 0;
}

int unetdrive::read(int socketId, char *outBuffer, const int maxBuffer)
{
	unetsocket *s = grab(socketId);
	assert(s && maxBuffer > 0);
	if (s->buffer.sz == 0)
		return -1;
	int rdsz = (maxBuffer - s->buffer.sz) > 0 ? s->buffer.sz : maxBuffer;
	memcpy(outBuffer, s->buffer.data, rdsz);
	s->buffer.sz -= rdsz;
	if (s->buffer.sz > 0)
	{
		memmove(s->buffer.data, s->buffer.data + rdsz, s->buffer.sz);
	}

	return rdsz;
}

int unetdrive::send(int socketId, const char *inBuffer, const int inSz)
{
	unetsocket *s = grab(socketId);
	if (s == NULL || !s->connected)
	{
		return -1; //closed
	}

	return usocket_server::socketSend(socketId, (const void *)inBuffer, (int)inSz);
}

bool unetdrive::invalid(int socketId)
{
	unetsocket *s = grab(socketId);
	if (s == NULL)
		return false;
	return true;
}

int unetdrive::size()
{
	return (int)netSocketMap__.size();
}

void unetdrive::close(int socketId)
{
	unetsocket *s = grab(socketId);
	if (s == NULL)
	{
		return;
	}

	if (s->connected)
	{
		usocket_server::socketClose(opaque__, socketId);
		/*noties: waiting for onClose callbacks to delete resources*/
	}
}

void unetdrive::shutdown(int socketId)
{
	unetsocket *s = grab(socketId);
	if (s != NULL)
	{
		usocket_server::socketShutdown(opaque__, socketId);
	}
}

void unetdrive::abandon(int socketId)
{
	unetsocket *s = grab(socketId);
	if (s != NULL)
	{
		s->connected = false;
		this->earse(socketId);
	}
}

int unetdrive::doListen(const char *host, const int port, const int backlog)
{
	int socketId;
	if ((socketId = usocket_server::socketListen(opaque__, host, port, backlog)) < 0)
	{
		return -1;
	}

	return socketId;
}

void unetdrive::onData(asynResult *r)
{
	int sz = r->ud;
	char *data = r->buffer;
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		ucontext::error(opaque__, "socket: drop package from %d", r->id);
		return;
	}

	int space = 0, read = 0, write = 0;
	for (;;)
	{
		if (!s->connected)
			break;
		/*With single-edge triggers, each read requires the buffer to be read out*/
		space = SOCKET_BUFFER_MAX - s->buffer.sz;
		read = ((space - sz - write) > 0) ? (sz - write) : space;
		if (read > 0)
		{
			memcpy(s->buffer.data + s->buffer.sz, data + write, read);
			s->buffer.sz += read;
			write += read;
		}

		if (s->onRecv != nullptr)
		{
			s->onRecv(r->id);
		}

		if (write >= sz)
			break;
	}
}

void unetdrive::onClose(asynResult *r)
{
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		return;
	}
	s->connected = false;

	if (s->onCallback != nullptr)
	{
		s->onCallback(r);
	}

	/*Consider whether to remove objects?*/
	earse(r->id);
}

void unetdrive::onError(asynResult *r)
{
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		ucontext::error(opaque__, "socket: errror no unknown %d, %s", r->id, r->buffer);
		return;
	}

	if (s->connected)
		ucontext::error(opaque__, "socket: error on %d %s", r->id, r->buffer);

	usocket_server::socketShutdown(opaque__, r->id);
	if (s->onCallback != nullptr)
	{
		s->onCallback(r);
	}
}

void unetdrive::onOpen(asynResult *r)
{
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		return;
	}

	s->connected = true;
	if (s->onCallback != nullptr)
	{
		s->onCallback(r);
	}
}

void unetdrive::onAccept(asynResult *r)
{
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		usocket_server::socketClose(opaque__, r->id);
		return;
	}

	if (s->onCallback != nullptr && s->connected)
	{
		s->onCallback(r);
	}
}

void unetdrive::onWarn(struct asynResult *r)
{
	unetsocket *s = grab(r->id);
	if (s == NULL)
	{
		return;
	}

	if (s->onWarning == NULL)
	{
		ucontext::error(opaque__, "WARING: %d K bytes need to send out(socket = %d)", r->id, r->ud);
	}
	else
	{
		s->onWarning(r);
	}
}