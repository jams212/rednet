#ifndef REDNET_ULUASERI_H
#define REDNET_ULUASERI_H

#include <lua.hpp>
#include <uobject.h>
#include <string.h>

#define LS_MAX_COOKIE 32
#define LS_BLOCK_SIZE 128
#define LS_MAX_DEPTH 32

#define LS_COMBINE_TYPE(t, v) ((t) | (v) << 3)

namespace rednet
{
namespace lualib
{

enum luaValueType
{
  lnil = 0,
  lboolean = 1,
  lnumber = 2,
  luserdata = 3,
  lsstring = 4,
  llstring = 5,
  ltable = 6,
};

enum luaNumberType
{
  lzero = 0,
  lbyte = 1,
  lword = 2,
  ldword = 4,
  lqword = 6,
  lreal = 8,
};

class lBlock : public uobject
{
public:
  struct lBlock *next__;
  char buffer__[LS_BLOCK_SIZE];

public:
  lBlock() : next__(NULL)
  {
  }
};

class lWriteBlock : public uobject
{
public:
  lWriteBlock(lua_State *l);
  ~lWriteBlock();

private:
  void packFrom();
  void packOne(int index, int depth);
  void serialize();
  void clear();

private:
  void pushNil();
  void pushInt(lua_Integer v);
  void pushBool(int boolean);
  void pushReal(double v);
  void pushPointer(void *v);
  void pushString(const char *str, int len);
  int pushTableArray(int index, int depth);
  void pushTableHash(int index, int depth, int arraySize);
  void pushTableMetapairs(int index, int depth);
  void pushTable(int index, int depth);

private:
  inline void push(const void *buf, int sz)
  {
    const char *buffer = (const char *)buf;

    if (head__ == NULL || current__ == NULL)
    {
      head__ = current__ = new lBlock();
    }

    if (ptr__ == LS_BLOCK_SIZE)
    {
    _again:
      current__ = current__->next__ = new lBlock();
      ptr__ = 0;
    }
    if (ptr__ <= LS_BLOCK_SIZE - sz)
    {
      memcpy(current__->buffer__ + ptr__, buffer, sz);
      ptr__ += sz;
      len__ += sz;
    }
    else
    {
      int cpysz = LS_BLOCK_SIZE - ptr__;
      memcpy(current__->buffer__ + ptr__, buffer, cpysz);
      buffer += cpysz;
      len__ += cpysz;
      sz -= cpysz;
      goto _again;
    }
  }

private:
  lua_State *l__;
  lBlock *head__;
  lBlock *current__;
  int len__;
  int ptr__;
};

class lReadBlock
{
public:
  lReadBlock(lua_State *l, void *buf, int size);
  ~lReadBlock();

private:
  void *read(int sz);
  void pushValue(int type, int cookie);
  void unPackOne();

private:
  lua_Integer readInt(int cookie);
  double readReal();
  void *readPointer();
  void readBuffer(int len);
  void readTable(int arraySize);

private:
  lua_State *l__;
  char *buffer__;
  int len__;
  int ptr__;

  friend void invalidStreamLine(lua_State *l, lReadBlock *rb, int line);
};

extern "C" int luaseri_pack(lua_State *L);

extern "C" int luaseri_unpack(lua_State *L);

} // namespace lualib
} // namespace rednet

#endif
