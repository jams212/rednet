#ifndef PACT_GAMEEVENT_H
#define PACT_GAMEEVENT_H

#include <uextern.h>

enum pactGameEvent
{
    PGE_LOGIN = EP_USER + 1,
    PGE_ENTER = EP_USER + 2,
    PGE_EXIT = EP_USER + 3,
    PGE_START = EP_USER + 4,
    PGE_UNLINE = EP_USER + 5,
};

enum netcommand
{
    NC_LOGIN = 1,
    NC_ENTERROOM = 2,
    NC_EXITROOM = 3,
    NC_STARTROOM = 4,
};

#pragma pack(push, 1)
struct login
{
    char userName[16];
    char userPass[16];
};

struct enterRoom
{
    int userID;
    uint16_t RoomID;
};

struct exitRoom
{
    int userID;
    uint16_t RoomID;
};

struct startRoom
{
    int userID;
    uint16_t RoomID;
};

struct startBack
{
    int socketId;
    char ip[19];
    int port;
    char sessionId[64];
    int code;
};

struct Reponse
{
    uint16_t cmd;
    uint16_t code;  //0.成功 1.失败 2.服务器忙
    char data[128]; //结果数据
};

struct ReponseStartData
{
    char ip[19];
    int port;
    char sessionId[64];
};

#pragma pack(pop)

#endif
