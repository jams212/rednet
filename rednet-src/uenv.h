#ifndef REDNET_UENV_H
#define REDNET_UENV_H

#include "uobject.h"
#include "usingleton.h"
#include "uspinlock.h"
#include <lua.hpp>

namespace rednet
{
class uenv : public usingleton<uenv>
{
public:
  uenv();
  ~uenv();

  const char *getEnv(const char *key);

  void setEnv(const char *key, const char *value);

public:
  static int __getInt(const char *key, int opt = 0);

  static bool __getBoolean(const char *key, bool opt = false);

  static const char *__getString(const char *key, const char *opt = NULL);

private:
  lua_State *l__;
  uspinlock lk__;
};

} // namespace rednet

#define UENV(funName, ...) STAT(uenv, funName, __VA_ARGS__)
#define UENVSTR(...) UENV(__getString, __VA_ARGS__)
#define UENVINT(...) UENV(__getInt, __VA_ARGS__)
#define UENVBOL(...) UENV(__getBoolean, __VA_ARGS__)

#endif