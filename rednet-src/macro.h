#ifndef REDNET_MACRO_H
#define REDNET_MACRO_H

#define R_CONSTRUCTOR(className) \
    className() {}               \
    ~className() {}

#define PRIVATE_MACRO_VAR_ARGS_IMPL_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, N, ...) N
#define PRIVATE_MACRO_VAR_ARGS_IMPL(args) PRIVATE_MACRO_VAR_ARGS_IMPL_COUNT args
#define COUNT_MACRO_VAR_ARGS(...) PRIVATE_MACRO_VAR_ARGS_IMPL((__VA_ARGS__, 10, 9, 8, 7, 6, 5 4, 3, 2, 1, 0))
#define PRIVATE_MACRO_CHOOSE_HELPER2(M, count) M##count
#define PRIVATE_MACRO_CHOOSE_HELPER1(M, count) PRIVATE_MACRO_CHOOSE_HELPER2(M, count)
#define PRIVATE_MACRO_CHOOSE_HELPER(M, count) PRIVATE_MACRO_CHOOSE_HELPER1(M, count)

#define INST_8(className, funName, ar1, ar2, ar3, ar4, ar5, ar6, ar7, ar8) className::instance()->funName(ar1, ar2, ar3, ar4, ar5, ar6, ar7, ar8)
#define INST_7(className, funName, ar1, ar2, ar3, ar4, ar5, ar6, ar7) className::instance()->funName(ar1, ar2, ar3, ar4, ar5, ar6, ar7)
#define INST_6(className, funName, ar1, ar2, ar3, ar4, ar5, ar6) className::instance()->funName(ar1, ar2, ar3, ar4, ar5, ar6)
#define INST_5(className, funName, ar1, ar2, ar3, ar4, ar5) className::instance()->funName(ar1, ar2, ar3, ar4, ar5)
#define INST_4(className, funName, ar1, ar2, ar3, ar4) className::instance()->funName(ar1, ar2, ar3, ar4)
#define INST_3(className, funName, ar1, ar2, ar3) className::instance()->funName(ar1, ar2, ar3)
#define INST_2(className, funName, ar1, ar2) className::instance()->funName(ar1, ar2)
#define INST_1(className, funName, ar1) className::instance()->funName(ar1)
#define INST_0(className, funName) className::instance()->funName()
#define INST(className, funName, ...)                                     \
    PRIVATE_MACRO_CHOOSE_HELPER(INST_, COUNT_MACRO_VAR_ARGS(__VA_ARGS__)) \
    (className, funName, __VA_ARGS__)
#define INSTGET(className) className::instance()

#define STAT_8(className, funName, ar1, ar2, ar3, ar4, ar5, ar6, ar7, ar8) className::funName(ar1, ar2, ar3, ar4, ar5, ar6, ar7, ar8)
#define STAT_7(className, funName, ar1, ar2, ar3, ar4, ar5, ar6, ar7) className::funName(ar1, ar2, ar3, ar4, ar5, ar6, ar7)
#define STAT_6(className, funName, ar1, ar2, ar3, ar4, ar5, ar6) className::funName(ar1, ar2, ar3, ar4, ar5, ar6)
#define STAT_5(className, funName, ar1, ar2, ar3, ar4, ar5) className::funName(ar1, ar2, ar3, ar4, ar5)
#define STAT_4(className, funName, ar1, ar2, ar3, ar4) className::funName(ar1, ar2, ar3, ar4)
#define STAT_3(className, funName, ar1, ar2, ar3) className::funName(ar1, ar2, ar3)
#define STAT_2(className, funName, ar1, ar2) className::funName(ar1, ar2)
#define STAT_1(className, funName, ar1) className::funName(ar1)
#define STAT_0(className, funName) className::funName()
#define STAT(className, funName, ...)                                     \
    PRIVATE_MACRO_CHOOSE_HELPER(STAT_, COUNT_MACRO_VAR_ARGS(__VA_ARGS__)) \
    (className, funName, __VA_ARGS__)

#define EVBIND(funAddr, objec) bind(funAddr, objec, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, placeholders::_5, placeholders::_6)
#define SCKBIND(funAddr, objec) bind(funAddr, objec, placeholders::_1)

#define RELEASE(p) \
    if (p != NULL) \
    {              \
        delete p;  \
        p = NULL;  \
    }

#define RELEASE_PTR(p) \
    if (p != nullptr)  \
    {                  \
        delete p;      \
        p = nullptr;   \
    }

#include <cstdio>
#include <functional>

using namespace std;
namespace rednet
{

} // namespace rednet

#endif