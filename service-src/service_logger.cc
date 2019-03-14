#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../rednet-src/uextern.h"

using namespace rednet;
using namespace std;
class logger : public ucomponent
{
  public:
    logger() : handle__(NULL),
               filename__(NULL),
               close__(0)
    {
    }

    ~logger()
    {
    }

    int init(ucontext *ctx, const char *parm) override
    {
        if (parm && strcmp(parm, "") != 0)
        {
            handle__ = fopen(parm, "w");
            if (!handle__)
                return 1;
            filename__ = umemory::strdup(parm);
            strcpy(filename__, parm);
            close__ = 1;
        }
        else
            handle__ = stdout;
        if (handle__)
        {

            ctx->setCallback(EVBIND(&logger::execute, this), ctx);
            return 0;
        }
        return 1;
    }

    int signal(int signal) override
    {
        return 0;
    }

    int execute(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        switch (type)
        {
        case EP_OPER:
            if (filename__)
                handle__ = freopen(filename__, "a", handle__);
            break;
        case EP_TEXT:
            fprintf(handle__, "[:%08x]", source);
            fwrite(data, sz, 1, handle__);
            fprintf(handle__, "\n");
            fflush(handle__);
            break;
        }
        return 0;
    }

  private:
    FILE *handle__;
    char *filename__;
    int close__;
};

extern "C" logger *logger_create(void)
{
    logger *inst = new logger();
    return inst;
}

extern "C" void logger_release(logger *inst)
{
    delete inst;
    inst = NULL;
}