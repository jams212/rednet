
#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assembly/ucstart.h>

#include <uextern.h>
#include <unetdrive.h>
#include <network/usocket_server.h>

#include "gameevent.h"

using namespace rednet;
using namespace rednet::network;
using namespace rednet::assembly;
using namespace std;

#define PACKET_SIZE 2
#define PACKET_CMD_SIZE 2

class netGame : public ucstart
{
  public:
    netGame() : netDrive__(NULL)
    {
    }

    virtual ~netGame()
    {
        RELEASE(netDrive__);
    }

    int signal(int signal)
    {
        return 0;
    }

  protected:
    void onStart(ucontext *ctx, const char *param)
    {
        netDrive__ = new unetdrive(ctx->id);
        assert(netDrive__);
        int socketId = netDrive__->beginListen(param);
        netDrive__->beginAccept(socketId, SCKBIND(&netGame::onAccept, this));
    }

    int onCallback(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        if (type == EP_SOCKET)
            return netDrive__->onEvent(ud, type, source, session, data, sz);
        else
        {
            switch (type)
            {
            case PGE_LOGIN:
                loginBackProc(data);
                break;
            case PGE_ENTER:
                enterRoomBackProc(data);
                break;
            case PGE_EXIT:
                exitRoomBackProc(data);
                break;
            case PGE_START:
                startBackProc(data);
                break;
            default:
                break;
            }
        }
        return 0;
    }

  private:
    void onAccept(struct asynResult *result)
    {
        if (result->type != NT_SOCKET_ACCEPT)
        {
            if (result->type == NT_SOCKET_OPEN)
            {
                ucontext::error(parent__->id, "Listen finish.");
            }
            else
            {
                ucontext::error(parent__->id, "Listen error:%s.", result->buffer ? (const char *)result->buffer : "");
            }
            return;
        }

        //连接数过多
        if (netDrive__->size() > 2000)
        {
            usocket_server::socketClose(parent__->id, result->ud);
            return;
        }

        netDrive__->endAccept(result->ud, SCKBIND(&netGame::onClient, this));
        netDrive__->beginRecv(result->ud, SCKBIND(&netGame::onRecv, this));
    }

    void onClient(struct asynResult *result)
    {
        if (result->type == NT_SOCKET_OPEN)
        {
            ucontext::error(parent__->id, "open client.");
        }
        else //其它得
        {
            switch (result->type)
            {
            case NT_SOCKET_ERR:
                //通知用户管理器，某用户连接错误
                break;
            case NT_SOCKET_CLOSE:
                unLineProc((const int)result->id);
                break;
            default:
                break;
            }
        }
    }

    void onRecv(int socketId) //protobuf
    {
        const int bufferSize = 2048;
        char bufferData[bufferSize];

        int dataSz = 0;
        const char *data = NULL;
        for (;;)
        {
            data = netDrive__->check(socketId, dataSz);
            if (data == NULL)
            {
                return;
            }

            int n = protoAnalyze(data, (const int)dataSz);
            if (n == 0)
            {
                return;
            }
            else if (n == -1)
            {
                netDrive__->clone(socketId);
                usocket_server::socketClose(parent__->id, socketId);
                return;
            }

            netDrive__->read(socketId, bufferData, n);
            const uint16_t *len = (const uint16_t *)(bufferData);
            const uint16_t *cmd = (const uint16_t *)(bufferData + PACKET_SIZE);
            switch (*cmd)
            {
            case NC_LOGIN:
                if (sizeof(struct login) != *len)
                {
                    ucontext::error(parent__->id, "login data length error:%d-%d.", sizeof(struct login), *len);
                    break;
                }
                loginProc((const int)socketId, (const struct login *)(bufferData + PACKET_SIZE + PACKET_CMD_SIZE));
                break;
            case NC_ENTERROOM:
                if (sizeof(struct enterRoom) != *len)
                {
                    ucontext::error(parent__->id, "enter room data length error:%d-%d.", sizeof(struct enterRoom), *len);
                    break;
                }
                enterRoomProc((const int)socketId, (const struct enterRoom *)(bufferData + PACKET_SIZE + PACKET_CMD_SIZE));
                break;
            case NC_EXITROOM:
                if (sizeof(struct exitRoom) != *len)
                {
                    ucontext::error(parent__->id, "exit room data length error:%d-%d.", sizeof(struct exitRoom), *len);
                    break;
                }
                exitRoomProc((const int)socketId, (const struct exitRoom *)(bufferData + PACKET_SIZE + PACKET_CMD_SIZE));
                break;
            case NC_STARTROOM:
                if (sizeof(struct startRoom) != *len)
                {
                    ucontext::error(parent__->id, "start room data length error:%d-%d.", sizeof(struct startRoom), *len);
                    break;
                }
                startRoomProc((const int)socketId, (const struct startRoom *)(bufferData + PACKET_SIZE + PACKET_CMD_SIZE));
                break;
            default:
                ucontext::error(parent__->id, "network comman unknow");
                break;
            }
        }
    }

  private:
    int protoAnalyze(const char *data, const int size)
    {
        if (size < PACKET_SIZE + PACKET_CMD_SIZE)
        {
            return 0;
        }

        uint16_t *packLen = (uint16_t *)data;
        //uint16_t *packCmd = (uint16_t *)(data + PACKET_SIZE);

        uint16_t packCount = PACKET_SIZE + PACKET_CMD_SIZE + (*packLen);
        if (packCount > SOCKET_PACK_MAX)
        {
            return -1;
        }

        if (packCount > size)
        {
            return 0;
        }

        return packCount;
    }

  private:
    void loginProc(const int socketId, const struct login *request)
    {
        char *data = (char *)umemory::malloc(128);
        memcpy(data, (const void *)&socketId, sizeof(int));
        memcpy(data + sizeof(int), (char *)request, sizeof(struct login));
        if (!ucall::sendEvent(parent__->id, "gamemanage", PGE_LOGIN, 0, data, 128))
        {
            umemory::free(data);
            ucontext::error(parent__->id, "login submit task fail.");
            //不能直接回复数据
            return;
        }
    }

    void enterRoomProc(const int socketId, const struct enterRoom *request)
    {
        char *data = (char *)umemory::malloc(64);
        memcpy(data, (const void *)&socketId, sizeof(int));
        memcpy(data + sizeof(int), (char *)request, sizeof(struct enterRoom));
        if (!ucall::sendEvent(parent__->id, "gamemanage", PGE_ENTER, 0, data, 64))
        {
            umemory::free(data);
            ucontext::error(parent__->id, "enter Room  submit task fail.");
            return;
        }
    }

    void exitRoomProc(const int socketId, const struct exitRoom *request)
    {
        char *data = (char *)umemory::malloc(64);
        memcpy(data, (const void *)&socketId, sizeof(int));
        memcpy(data + sizeof(int), (char *)request, sizeof(struct exitRoom));
        if (!ucall::sendEvent(parent__->id, "gamemanage", PGE_EXIT, 0, data, 64))
        {
            umemory::free(data);
            ucontext::error(parent__->id, "exit Room submit task fail.");
            return;
        }
    }

    void startRoomProc(const int socketId, const struct startRoom *request)
    {
        char *data = (char *)umemory::malloc(64);
        memcpy(data, (const void *)&socketId, sizeof(int));
        memcpy(data + sizeof(int), (char *)request, sizeof(struct startRoom));
        if (!ucall::sendEvent(parent__->id, "gamemanage", PGE_START, 0, data, 64))
        {
            umemory::free(data);
            ucontext::error(parent__->id, "enter Room submit task fail.");
            return;
        }
    }

    void unLineProc(const int socketId)
    {
        char *data = (char *)umemory::malloc(16);
        memcpy(data, (const void *)&socketId, sizeof(int));
        if (!ucall::sendEvent(parent__->id, "gamemanage", PGE_UNLINE, 0, data, 16))
        {
            umemory::free(data);
            ucontext::error(parent__->id, "unLine process submit task fail.");
            return;
        }
    }

    //#########################################
    void loginBackProc(void *data)
    {
        int socketId = getSocketId(data);
        int userId = 0;
        memcpy((char *)&userId, ((char *)data) + sizeof(int), sizeof(int));

        char *buffer = (char *)umemory::malloc(256);
        struct Reponse *reponse = (struct Reponse *)buffer;
        reponse->cmd = NC_LOGIN;
        if (userId <= 0)
            reponse->code = 1;
        else
            reponse->code = 0;
        memcpy(reponse->data, &userId, sizeof(int));

        if (netDrive__->send(socketId, buffer, sizeof(*reponse)) != 0)
        {
            ucontext::error(parent__->id, "login back proc send error:%d", socketId);
            umemory::free(buffer);
        }
    }

    void enterRoomBackProc(void *data)
    {
        int socketId = getSocketId(data);
        char *buffer = (char *)umemory::malloc(256);
        struct Reponse *reponse = (struct Reponse *)buffer;
        reponse->cmd = NC_ENTERROOM;
        reponse->code = 0;

        if (netDrive__->send(socketId, buffer, sizeof(*reponse)) != 0)
        {
            ucontext::error(parent__->id, "enter Room  back proc send error:%d", socketId);
            umemory::free(buffer);
        }
    }

    void exitRoomBackProc(void *data)
    {
        int socketId = getSocketId(data);
        char *buffer = (char *)umemory::malloc(256);
        struct Reponse *reponse = (struct Reponse *)buffer;
        reponse->cmd = NC_EXITROOM;
        reponse->code = 0;

        if (netDrive__->send(socketId, buffer, sizeof(*reponse)) != 0)
        {
            ucontext::error(parent__->id, "exit Room  back proc send error:%d", socketId);
            umemory::free(buffer);
        }
    }

    void startBackProc(void *data)
    {
        struct startBack *lstart = (struct startBack *)data;
        char *buffer = (char *)umemory::malloc(256);
        struct Reponse *reponse = (struct Reponse *)buffer;
        reponse->cmd = PGE_START;
        reponse->code = lstart->code;

        struct ReponseStartData *startData = (struct ReponseStartData *)reponse->data;
        strcpy(startData->ip, lstart->ip);
        startData->port = lstart->port;
        strcpy(startData->sessionId, lstart->sessionId);

        if (netDrive__->send(lstart->socketId, buffer, sizeof(*reponse)) != 0)
        {
            ucontext::error(parent__->id, "exit Room  back proc send error:%d", lstart->socketId);
            umemory::free(buffer);
        }
    }

    int getSocketId(void *data)
    {
        int result = 0;
        memcpy(&result, data, sizeof(int));
        return result;
    }

  private:
    unetdrive *netDrive__;
};

extern "C" netGame *netgames_create(void)
{
    netGame *inst = new netGame();
    return inst;
}

extern "C" void netgames_release(netGame *inst)
{
    delete inst;
    inst = NULL;
}