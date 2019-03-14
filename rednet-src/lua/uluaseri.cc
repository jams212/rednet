#include "uluaseri.h"
#include <assert.h>

namespace rednet
{

namespace lualib
{

lWriteBlock::lWriteBlock(lua_State *l) : l__(l),
                                         head__(NULL),
                                         current__(NULL),
                                         len__(0),
                                         ptr__(0)
{
    packFrom();
    serialize();
}

lWriteBlock::~lWriteBlock()
{
    clear();
}

void lWriteBlock::packFrom()
{
    int nfrom = 0;
    int n = lua_gettop(l__) - nfrom;
    for (int i = 1; i <= n; i++)
    {
        packOne(nfrom + i, 0);
    }
}

void lWriteBlock::packOne(int index, int depth)
{
    if (depth > LS_MAX_DEPTH)
    {
        clear();
        luaL_error(l__, "serialize can't pack too depth table");
    }

    int type = lua_type(l__, index);
    switch (type)
    {
    case LUA_TNIL:
        pushNil();
        break;
    case LUA_TNUMBER:
    {
        if (lua_isinteger(l__, index))
        {
            lua_Integer x = lua_tointeger(l__, index);
            pushInt(x);
        }
        else
        {
            lua_Number n = lua_tonumber(l__, index);
            pushReal(n);
        }
        break;
    }
    case LUA_TBOOLEAN:
        pushBool(lua_toboolean(l__, index));
        break;
    case LUA_TSTRING:
    {
        size_t sz = 0;
        const char *str = lua_tolstring(l__, index, &sz);
        pushString(str, (int)sz);
        break;
    }
    case LUA_TLIGHTUSERDATA:
        pushPointer(lua_touserdata(l__, index));
        break;
    case LUA_TTABLE:
    {
        if (index < 0)
        {
            index = lua_gettop(l__) + index + 1;
        }
        pushTable(index, depth + 1);
        break;
    }
    default:
        luaL_error(l__, "Unsupport type %s to serialize", lua_typename(l__, type));
        break;
    }
}

void lWriteBlock::serialize()
{
    uint8_t *buffer = (uint8_t *)umemory::malloc(len__);
    uint8_t *ptr = buffer;
    int sz = len__;
    lBlock *b = head__;
    while (len__ > 0)
    {
        assert(b);
        if (len__ >= LS_BLOCK_SIZE)
        {
            memcpy(ptr, b->buffer__, LS_BLOCK_SIZE);
            ptr += LS_BLOCK_SIZE;
            len__ -= LS_BLOCK_SIZE;
            b = b->next__;
        }
        else
        {
            memcpy(ptr, b->buffer__, len__);
            break;
        }
    }

    lua_pushlightuserdata(l__, buffer);
    lua_pushinteger(l__, sz);
}

void lWriteBlock::clear()
{
    lBlock *blk = head__;
    while (blk)
    {
        lBlock *next = blk->next__;
        delete blk;
        blk = next;
    }

    ptr__ = 0;
    len__ = 0;
    head__ = NULL;
    current__ = NULL;
}

void lWriteBlock::pushNil()
{
    uint8_t n = luaValueType::lnil;
    push(&n, 1);
}

void lWriteBlock::pushInt(lua_Integer v)
{
    int type = luaValueType::lnumber;
    if (v == 0)
    {
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::lzero);
        push(&n, 1);
    }
    else if (v != (int32_t)v)
    {
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::lqword);
        int64_t v64 = v;
        push(&n, 1);
        push(&v64, sizeof(v64));
    }
    else if (v < 0)
    {
        int32_t v32 = (int32_t)v;
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::ldword);
        push(&n, 1);
        push(&v32, sizeof(v32));
    }
    else if (v < 0x100)
    {
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::lbyte);
        push(&n, 1);
        uint8_t byte = (uint8_t)v;
        push(&byte, sizeof(byte));
    }
    else if (v < 0x10000)
    {
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::lword);
        push(&n, 1);
        uint16_t word = (uint16_t)v;
        push(&word, sizeof(word));
    }
    else
    {
        uint8_t n = LS_COMBINE_TYPE(type, luaNumberType::ldword);
        push(&n, 1);
        uint32_t v32 = (uint32_t)v;
        push(&v32, sizeof(v32));
    }
}

void lWriteBlock::pushReal(double v)
{
    uint8_t n = LS_COMBINE_TYPE(luaValueType::lnumber, luaNumberType::lreal);
    push(&n, 1);
    push(&v, sizeof(v));
}

void lWriteBlock::pushBool(int boolean)
{
    uint8_t n = LS_COMBINE_TYPE(luaValueType::lboolean, boolean ? 1 : 0);
    push(&n, 1);
}

void lWriteBlock::pushPointer(void *v)
{
    uint8_t n = luaValueType::luserdata;
    push(&n, 1);
    push(&v, sizeof(v));
}

void lWriteBlock::pushString(const char *str, int len)
{
    if (len < LS_MAX_COOKIE)
    {
        uint8_t n = LS_COMBINE_TYPE(luaValueType::lsstring, len);
        push(&n, 1);
        if (len > 0)
        {
            push(str, len);
        }
    }
    else
    {
        uint8_t n;
        if (len < 0x10000)
        {
            n = LS_COMBINE_TYPE(luaValueType::llstring, 2);
            push(&n, 1);
            uint16_t x = (uint16_t)len;
            push(&x, 2);
        }
        else
        {
            n = LS_COMBINE_TYPE(luaValueType::llstring, 4);
            push(&n, 1);
            uint32_t x = (uint32_t)len;
            push(&x, 4);
        }
        push(str, len);
    }
}

void lWriteBlock::pushTable(int index, int depth)
{
    luaL_checkstack(l__, LUA_MINSTACK, NULL);
    if (index < 0)
    {
        index = lua_gettop(l__) + index + 1;
    }
    else if (luaL_getmetafield(l__, index, "__pairs") != LUA_TNIL)
    {
        pushTableMetapairs(index, depth);
    }
    else
    {
        int array_size = pushTableArray(index, depth);
        pushTableHash(index, depth, array_size);
    }
}

int lWriteBlock::pushTableArray(int index, int depth)
{
    int arraySize = lua_rawlen(l__, index);
    if (arraySize >= LS_MAX_COOKIE - 1)
    {
        uint8_t n = LS_COMBINE_TYPE(luaValueType::ltable, LS_MAX_COOKIE - 1);
        push(&n, 1);
        pushInt(arraySize);
    }
    else
    {
        uint8_t n = LS_COMBINE_TYPE(luaValueType::ltable, arraySize);
        push(&n, 1);
    }

    int i;
    for (i = 1; i <= arraySize; i++)
    {
        lua_rawgeti(l__, index, i);
        packOne(-1, depth);
        lua_pop(l__, 1);
    }

    return arraySize;
}

void lWriteBlock::pushTableHash(int index, int depth, int arraySize)
{
    lua_pushnil(l__);
    while (lua_next(l__, index) != 0)
    {
        if (lua_type(l__, -2) == LUA_TNUMBER)
        {
            if (lua_isinteger(l__, -2))
            {
                lua_Integer x = lua_tointeger(l__, -2);
                if (x > 0 && x <= arraySize)
                {
                    lua_pop(l__, 1);
                    continue;
                }
            }
        }
        packOne(-2, depth);
        packOne(-1, depth);
        lua_pop(l__, 1);
    }
    pushNil();
}

void lWriteBlock::pushTableMetapairs(int index, int depth)
{
    uint8_t n = LS_COMBINE_TYPE(luaValueType::ltable, 0);
    push(&n, 1);
    lua_pushvalue(l__, index);
    lua_call(l__, 1, 3);
    for (;;)
    {
        lua_pushvalue(l__, -2);
        lua_pushvalue(l__, -2);
        lua_copy(l__, -5, -3);
        lua_call(l__, 2, 2);
        int type = lua_type(l__, -2);
        if (type == LUA_TNIL)
        {
            lua_pop(l__, 4);
            break;
        }
        packOne(-2, depth);
        packOne(-1, depth);
        lua_pop(l__, 1);
    }
    pushNil();
}

/*#############################################################*/
void invalidStreamLine(lua_State *l, lReadBlock *rb, int line)
{
    int len = rb->len__;
    luaL_error(l, "Invalid serialize stream %d (line:%d)", len, line);
}

#define invalidStream(L, rb) invalidStreamLine(L, rb, __LINE__)

lReadBlock::lReadBlock(lua_State *l, void *buf, int size) : l__(l),
                                                            buffer__((char *)buf),
                                                            len__(size),
                                                            ptr__(0)
{
    for (int i = 0;; i++)
    {
        if (i % 8 == 7)
        {
            luaL_checkstack(l__, LUA_MINSTACK, NULL);
        }
        uint8_t type = 0;
        uint8_t *t = (uint8_t *)read(sizeof(type));
        if (t == NULL)
            break;
        type = *t;
        pushValue(type & 0x7, type >> 3);
    }
}

lReadBlock::~lReadBlock()
{
}

void *lReadBlock::read(int sz)
{
    if (len__ < sz)
    {
        return NULL;
    }

    int ptr = ptr__;
    ptr__ += sz;
    len__ -= sz;
    return buffer__ + ptr;
}

void lReadBlock::pushValue(int type, int cookie)
{
    switch (type)
    {
    case luaValueType::lnil:
        lua_pushnil(l__);
        break;
    case luaValueType::lboolean:
        lua_pushboolean(l__, cookie);
        break;
    case luaValueType::lnumber:
        if (cookie == luaNumberType::lreal)
        {
            lua_pushnumber(l__, readReal());
        }
        else
        {
            lua_pushinteger(l__, readInt(cookie));
        }
        break;
    case luaValueType::luserdata:
        lua_pushlightuserdata(l__, readPointer());
        break;
    case luaValueType::lsstring:
        readBuffer(cookie);
        break;
    case luaValueType::llstring:
    {
        if (cookie == 2)
        {
            uint16_t *plen = (uint16_t *)read(2);
            if (plen == NULL)
            {
                invalidStream(l__, this);
            }
            uint16_t n;
            memcpy(&n, plen, sizeof(n));
            readBuffer(n);
        }
        else
        {
            if (cookie != 4)
            {
                invalidStream(l__, this);
            }
            uint32_t *plen = (uint32_t *)read(4);
            if (plen == NULL)
            {
                invalidStream(l__, this);
            }
            uint32_t n;
            memcpy(&n, plen, sizeof(n));
            readBuffer(n);
        }
        break;
    }
    case luaValueType::ltable:
    {
        readTable(cookie);
        break;
    }
    default:
    {
        invalidStream(l__, this);
        break;
    }
    }
}

lua_Integer lReadBlock::readInt(int cookie)
{
    switch (cookie)
    {
    case luaNumberType::lzero:
        return 0;
    case luaNumberType::lbyte:
    {
        uint8_t n;
        uint8_t *pn = (uint8_t *)read(sizeof(n));
        if (pn == NULL)
            invalidStream(l__, this);
        n = *pn;
        return n;
    }
    case luaNumberType::lword:
    {
        uint16_t n;
        uint16_t *pn = (uint16_t *)read(sizeof(n));
        if (pn == NULL)
            invalidStream(l__, this);
        memcpy(&n, pn, sizeof(n));
        return n;
    }
    case luaNumberType::ldword:
    {
        int32_t n;
        int32_t *pn = (int32_t *)read(sizeof(n));
        if (pn == NULL)
            invalidStream(l__, this);
        memcpy(&n, pn, sizeof(n));
        return n;
    }
    case luaNumberType::lqword:
    {
        int64_t n;
        int64_t *pn = (int64_t *)read(sizeof(n));
        if (pn == NULL)
            invalidStream(l__, this);
        memcpy(&n, pn, sizeof(n));
        return n;
    }
    default:
        invalidStream(l__, this);
        return 0;
    }
}

double lReadBlock::readReal()
{
    double n;
    double *pn = (double *)read(sizeof(n));
    if (pn == NULL)
        invalidStream(l__, this);
    memcpy(&n, pn, sizeof(n));
    return n;
}

void *lReadBlock::readPointer()
{
    void *userdata = 0;
    void **v = (void **)read(sizeof(userdata));
    if (v == NULL)
    {
        invalidStream(l__, this);
    }
    memcpy(&userdata, v, sizeof(userdata));
    return userdata;
}

void lReadBlock::readTable(int arraySize)
{
    if (arraySize == LS_MAX_COOKIE - 1)
    {
        uint8_t type;
        uint8_t *t = (uint8_t *)read(sizeof(type));
        if (t == NULL)
        {
            invalidStream(l__, this);
        }
        type = *t;
        int cookie = type >> 3;
        if ((type & 7) != luaValueType::ltable || cookie == luaNumberType::lreal)
        {
            invalidStream(l__, this);
        }
        arraySize = readInt(cookie);
    }
    luaL_checkstack(l__, LUA_MINSTACK, NULL);
    lua_createtable(l__, arraySize, 0);
    int i;
    for (i = 1; i <= arraySize; i++)
    {
        unPackOne();
        lua_rawseti(l__, -2, i);
    }
    for (;;)
    {
        unPackOne();
        if (lua_isnil(l__, -1))
        {
            lua_pop(l__, 1);
            return;
        }
        unPackOne();
        lua_rawset(l__, -3);
    }
}

void lReadBlock::unPackOne()
{
    uint8_t type;
    uint8_t *t = (uint8_t *)read(sizeof(type));
    if (t == NULL)
    {
        invalidStream(l__, this);
    }
    type = *t;
    pushValue(type & 0x7, type >> 3);
}

void lReadBlock::readBuffer(int len)
{
    char *p = (char *)read(len);
    if (p == NULL)
    {
        invalidStream(l__, this);
    }
    lua_pushlstring(l__, p, len);
}

extern "C" int luaseri_pack(lua_State *L)
{
    lWriteBlock wb(L);
    return 2;
}

extern "C" int luaseri_unpack(lua_State *L)
{
    if (lua_isnoneornil(L, 1))
    {
        return 0;
    }
    void *buffer;
    int len;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        size_t sz;
        buffer = (void *)lua_tolstring(L, 1, &sz);
        len = (int)sz;
    }
    else
    {
        buffer = lua_touserdata(L, 1);
        len = luaL_checkinteger(L, 2);
    }
    if (len == 0)
    {
        return 0;
    }
    if (buffer == NULL)
    {
        return luaL_error(L, "deserialize null pointer");
    }

    lua_settop(L, 1);
    lReadBlock rb(L, buffer, len);

    return lua_gettop(L) - 1; // Need not free buffer
}

} // namespace lualib

} // namespace rednet