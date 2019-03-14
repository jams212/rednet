#include "uframework.h"
#include "ucontext.h"
#include "udaemon.h"
#include "uenv.h"
#include "usignal.h"
#include "uglobal.h"
#include "ugroup.h"
#include "uini.h"
#include "umodule.h"
#include "utimer.h"
#include "uwork.h"
#include "network/usocket_server.h"
#include <assert.h>
#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IS_ABORT                     \
    if (INST(uglobal, contextEmpty)) \
        break;

using namespace rednet;
using namespace rednet::network;
using namespace rednet::server;

uframework::uframework()
{
}

uframework::~uframework()
{
}

void uframework::init()
{
    INST(uglobal, init);
    sig__ = new usignal(SA_RESTART, SIGHUP, std::bind(&uframework::onSignal, this));
}

void uframework::destory()
{
    RELEASE(sig__);
    //足个释放
    INST(uglobal, exit);
}

int uframework::onWorkSocket(void *param)
{
    INST(uglobal, bindWorker, WS_SOCKET);
    //
    for (;;)
    {
        int r = INST(usocket_server, onWait);
        if (r == 0)
            break;
        if (r < 0)
        {
            IS_ABORT
            continue;
        }

        INST(uworks, wakeup, 0);
    }
    return 0;
}

int uframework::onWorkTimer(void *param)
{
    INST(uglobal, bindWorker, WS_TIMER);
    for (;;)
    {
        INST(utimer, tick);
        INST(usocket_server, onUpdateTime, INST(utimer, now));
        IS_ABORT
        INST(uworks, wakeup, INST(uworks, getCount) - 1);
        usleep(2500);
        sig__->wait();
    }

    usocket_server::socketExit();
    INST(uworks, stop);
    return 0;
}

int uframework::onWorkLogic(void *param)
{
    eq_ptr q_ptr = nullptr;
    while (!INST(uworks, isShutdown))
    {
        q_ptr = onDispatchEvent(q_ptr, -1);
        if (q_ptr == nullptr)
        {
            INST(uworks, wait);
        }
    }
    return 0;
}

void uframework::onSignal()
{
    uint32_t loggerId = INST(uctxMgr, convertId, "logger");
    if (!loggerId)
        return;
    struct uevent ev;
    ev.source = 0;
    ev.session = 0;
    ev.type = EP_OPER;
    ev.data = NULL;
    ucall::sendEvent(loggerId, &ev);
}

eq_ptr uframework::onDispatchEvent(eq_ptr q, int weight)
{
    if (q == nullptr)
    {
        q = INST(uglobal_queue, pop);
        if (q == nullptr)
            return nullptr;
    }

    uint32_t id = q->id;
    ctx_ptr ptr = INST(uctxMgr, grab, id);

    if (ptr == nullptr)
    {
        return INST(uglobal_queue, pop);
    }

    int i, n = 1;
    uevent e;
    for (i = 0; i < n; i++)
    {
        if (!q->pop(&e))
            return INST(uglobal_queue, pop);
        else if (i == 0 && weight >= 0)
        {
            n = q->length();
            n >>= weight;
        }

        int overload = q->overload();
        if (overload)
        {
            ucontext::error(0, "May overload, event queue length %d", overload);
        }

        ptr->execute(&e);
    }

    ptr->isAssert(q.get());
    eq_ptr nq = INST(uglobal_queue, pop);
    if (nq)
    {
        INST(uglobal_queue, push, q);
        q = nq;
    }

    return q;
}

bool uframework::openService(const char *cmdline, ucontext **out)
{
    int sz = strlen(cmdline);
    char name[sz + 1];
    char args[sz + 1];
    sscanf(cmdline, "%s %s", name, args);

    ucontext *ctx = new ucontext();
    if (ctx->init(name, args) == 0)
    {
        RELEASE(ctx);
        return false;
    }

    *out = ctx;
    return true;
}

void uframework::bootstrap(const char *logparm, const char *cmdline)
{
    ucontext *logctx = NULL;
    ucontext *bootctx = NULL;

    if (!logparm)
    {
        fprintf(stderr, "Logger error : is null\n");
        exit(1);
        return;
    }

    if (!openService(logparm, &logctx))
    {
        fprintf(stderr, "Bootstarp error : %s\n", logparm);
        exit(1);
        return;
    }

    if (!openService(cmdline, &bootctx))
    {
        ucontext::error(0, "Bootstrap error : %s", cmdline);
        logctx->disponseAll();
        exit(1);
        return;
    }
}

bool uframework::loadDeploy(const char *filename)
{
    rednet::uini *hini = new rednet::uini();
    assert(hini);
    if (hini->load(filename) != 0)
    {
        fprintf(stderr, "Need a configuration file: load fail ------->>>> %s.\n", filename);
        return false;
    }

    rednet::uini::iterator it = hini->begin();
    while (it != hini->end())
    {
        INST(uenv, setEnv, it->first.c_str(), it->second.c_str());
        ++it;
    }

    RELEASE(hini);
    return true;
}

void uframework::startup(const char *file)
{
    int i;
    udaemon *hdaemon = NULL;
    if (file == NULL)
    {
        fprintf(stderr, "Need a configuration file ------->>>> rednet [filename].\n");
        return;
    }

    init();

    if (!INST(usocket_server, isInvalid))
    {
        goto _failed;
    }

    if (!loadDeploy(file))
    {
        goto _failed;
    }

    dep__.workerNumber = UENVINT("thread", 4);
    dep__.serverGroupID = UENVINT("server_group_id", 1);
    dep__.daemon = UENVSTR("daemon", NULL);
    dep__.modulePath = UENVSTR("module_path", "./service_cxx/?.so");
    dep__.proFile = UENVBOL("profile", false);
    dep__.bootstrap = UENVSTR("bootstrap", "bootstrap");
    dep__.logger = UENVSTR("logger", NULL);
    dep__.luaPath = UENVSTR("lua_path", "./lualib/?.lua;./lualib/?/init.lua");
    dep__.luaCPath = UENVSTR("lua_cpath", "./lib_cxx/?.so");
    dep__.luaService = UENVSTR("lua_service", "./script/?.lua");

    INST(uglobal, setProFile, dep__.proFile);
    INST(ugroup, init, dep__.serverGroupID);
    INST(uctxMgr, init, dep__.serverGroupID);
    INST(umodMgr, init, dep__.modulePath);

    if (dep__.daemon)
    {
        hdaemon = new udaemon(dep__.daemon);
        if (hdaemon->init() != 0)
        {
            goto _failed;
        }
    }

    bootstrap(dep__.logger, dep__.bootstrap);

    INST(uworks, append, std::bind(&uframework::onWorkTimer, this, std::placeholders::_1), NULL);
    INST(uworks, append, std::bind(&uframework::onWorkSocket, this, std::placeholders::_1), NULL);

    for (i = 0; i < dep__.workerNumber; i++)
    {
        INST(uworks, append, std::bind(&uframework::onWorkLogic, this, std::placeholders::_1), NULL);
    }

    INST(uworks, join);

    if (dep__.daemon)
    {
        hdaemon->exit();
    }
_failed:
    destory();
    RELEASE(hdaemon);
}
