#ifndef REDNET_UGROUP_H
#define REDNET_UGROUP_H

#include "usingleton.h"
#include <stdint.h>
#include <stdlib.h>

#define GLOBALNAME_LENGTH 16

namespace rednet
{
class ucontext;
namespace server
{
struct remoteServer
{
  char name[GLOBALNAME_LENGTH];
  uint32_t id;
};

struct remoteEvent
{
  struct remoteServer destination;
  const void *event;
  size_t sz;
  int type;
};

class ugroup : public usingleton<ugroup>
{
public:
  ugroup();
  ~ugroup();

  void init(int groupID);
  bool isRemote(uint32_t id);

private:
  unsigned int groupID__;
  ucontext *remote__;
};
} // namespace server
} // namespace rednet

#endif
