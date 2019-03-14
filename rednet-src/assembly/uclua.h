#ifndef REDNET_ASSEMBLY_UCLUA_H
#define REDNET_ASSEMBLY_UCLUA_H

/**
 * @brief open lua file base component.
 * 
 */

#include <ucomponent.h>

namespace rednet
{
namespace assembly
{
class uclua : public ucomponent
{
  public:
    uclua();
    ~uclua();
};
} // namespace assembly
} // namespace rednet

#endif