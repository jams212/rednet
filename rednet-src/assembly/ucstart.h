#ifndef REDNET_ASSEMBLY_UCSTART_H
#define REDNET_ASSEMBLY_UCSTART_H

#include "uevent.h"
#include "ucomponent.h"
#include "ucontext.h"
#include <functional>

using namespace rednet;
using namespace std;

namespace rednet
{
namespace assembly
{
class ucstart : public ucomponent
{
  public:
    virtual ~ucstart() {}

    int init(ucontext *ctx, const char *parm) override
    {
        parent__ = ctx;
        uint32_t ctxid = ctx->id;
        callback__ = EVBIND(&ucstart::launch, this);
        ud__ = (void *)ctx;

        int sz = ((parm == NULL) ? 0 : strlen(parm));
        char *tmp = NULL;
        if (parm)
        {
            tmp = (char *)umemory::malloc(sz);
            memcpy(tmp, parm, sz);
        }
        ucall::sendEvent(ctxid, ctxid, EP_DONT, 0, tmp, sz);
        return 0;
    }

    virtual int signal(int signal)
    {
        return 0;
    }

  private:
    int launch(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        callback__ = EVBIND(&ucstart::onCallback, this);
        onStart((ucontext *)ud, (const char *)data);
        ucall::sendEvent(source, source, EP_DONT, 0, NULL, 0);
        return 0;
    }

  protected:
    virtual void onStart(ucontext *ctx, const char *param)
    {
    }

    virtual int onCallback(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        return 0;
    }
};
} // namespace assembly
} // namespace rednet

#endif
