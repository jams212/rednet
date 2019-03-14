#include <functional>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assembly/ucstart.h>
#include <uextern.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <ulock.h>

#include "gameevent.h"

#include <aws/core/Aws.h>
#include <aws/gamelift/GameLiftClient.h>

#define DEPOT_PACKET_START sizeof(int)
#define MAX_PLAY_NUM 6

using namespace std;
using namespace rednet;

struct UserInfo
{
    int userId;
    char name[32];
};

struct awsGameConfig
{
    const char *gameLiftAlias;
    const char *gameLiftRegion;
    const char *gameLiftPlayTableName;
};

class playSession : public uobject
{
  public:
    playSession()
    {
    }

    ~playSession()
    {
    }

  public:
    int userId;
    int socketId;
    char name[32];
    int score;
    int status;
};

class roomPlay
{
  public:
    roomPlay() : userId(0), dirc(0)
    {
    }

    ~roomPlay()
    {
    }

  public:
    int userId;
    char dirc;
};

class roomSession : public uobject
{
  public:
    roomSession()
    {
        status__ = 0;
        plays.resize(6, roomPlay());
    }

    ~roomSession()
    {
        plays__.clear();
    }

    int enter(const int userId, char dirc)
    {
        int i;
        ulocking k(&lock__);
        for (i = 0; i < plays__.size; i++)
        {
            if (plays__[i].userId == userId)
            {
                plays__[i].dirc = dirc;
                return 0;
            }
        }

        for (i = 0; i < plays__.size; i++)
        {
            if (plays__[i].userId == userId)
            {
                plays__[i].userId = userId;
                plays__[i].dirc = dirc;
                return 0;
            }
        }
        return -1;
    }

    int exit(const int userId)
    {
        int i;
        ulocking k(&lock__);
        for (i = 0; i < plays__.size; i++)
        {
            if (plays__[i].userId == userId)
            {
                plays__[i].userId = 0;
                plays__[i].socketId = 0;
                plays__[i].dirc = dirc;
                return 0;
            }
        }
        return -1;
    }

    int exitSocketId(const int socketId)
    {
        int i, result = 0;
        ulocking k(&lock__);
        for (i = 0; i < plays__.size; i++)
        {
            if (plays__[i].socketId == socketId)
            {
                result = plays__[i].userId;
                plays__[i].userId = 0;
                plays__[i].socketId = 0;
                plays__[i].dirc = dirc;
                break;
            }
        }
        return result;
    }

    int count()
    {
        int result = 0;
        ulocking k(&lock__);
        for (int i = 0; i < plays__.size; i++)
        {
            if (plays__[i].userId == userId)
            {
                ++result;
            }
        }

        return result;
    }

    void getPlays(vector<int> *plays)
    {
        ulocking k(&lock__);
        for (int i = 0; i < plays__.size; i++)
        {
            if (plays__[i].userId != 0)
            {
                plays->push(plays__[i].userId);
            }
        }
    }

    int getStatus() { return status__; }

  private:
    int status__;
    vector<struct roomPlay> plays__;
    ulock lock__;
};

class deptoRooms : public uobject
{
  public:
    deptoRooms()
    {
        rooms__.resize(128);
    }

    ~deptoRooms()
    {
    }

    int enterR(int userId, char dirc)
    {
        roomSession *prom = getRoom();
        return prom->enter(userId, dirc);
    }

    int exitR(int userId)
    {
        roomSession *prom = getRoom();
        return prom->exit(userId);
    }

    void getPlays(vector<int> *plays)
    {
        roomSession *prom = getRoom();
        prom->getPlays(plays);
    }

  private:
    roomSession *getRoom()
    {
        ulocking k(&roomlck__);
        return &rooms__[0];
    }

  private:
    vector<roomSession> rooms__;
    ulock roomlck__;
}

class depotPlays : public uobject
{
    struct cmpstr
    {
        bool operator()(const char *a, const char *b) const
        {
            return memcmp(a, b, 64) == 0;
        }
    };

    struct hash_func
    {
        int operator()(const char *str) const
        {
            int seed = 131;
            int hash = 0;
            hash = (hash * seed) + (*str);
            while (*(++str))
            {
                hash = (hash * seed) + (*str);
            }

            return hash & (0x7FFFFFFF);
        }
    };

  public:
    depotPlays()
    {
    }

    ~depotPlays()
    {
    }

    bool pushSession(int socketId, const char *name, int userId, int score, int status)
    {
        ulocking k(&lock__);
        if (!plays__.empty())
        {
            auto it = plays__.find(name);
            if (it != plays__.end())
            {
                it->second->socketId = socketId;
                it->second->userId = userId;
                it->second->score = score;
                it->second->status = status;
                return true;
            }
        }

        playSession *session = new playSession();
        session->socketId = socketId;
        session->userId = userId;
        session->score = score;
        session->status = status;
        memcpy(session->name, name, 16);

        shared_ptr<playSession> ptr(session);

        plays__.insert(pair < const char *, shared_ptr<playSession>(session->name, ptr));

        return true;
    }

    bool removeSession(int socketId)
    {
        ulocking k(&lock__);
        if (plays__.empty())
        {
            return true;
        }

        auto it = plays__.begin();
        for (; it != plays__.end(); it++)
        {
            if (it->second->socketId == socketId)
            {
                plays__.earse(it);
                return true;
            }
        }

        return false;
    }

    const char *getSessionName(int userId)
    {
        ulocking k(&lock__);
        if (plays__.empty())
        {
            return NULL;
        }

        auto it = plays__.begin();
        for (; it != plays__.end(); it++)
        {
            if (it->second->userId == 0 || it->second->socketId == 0)
            {
                continue;
            }

            if (it->second->userId != userId)
            {
                continue;
            }
            return it->second->userName;
        }
        return NULL;
    }

    int getSessionSocketId(int userId)
    {
        ulocking k(&lock__);
        if (plays__.empty())
        {
            return 0;
        }

        auto it = plays__.begin();
        for (; it != plays__.end(); it++)
        {
            if (it->second->userId == userId)
            {
                return it->second->socketId;
            }
        }
        return 0;
    }

  private:
    unordered_map<const char *, shared_ptr<playSession>, hash_func, cmpstr> plays__;
    ulock lock__;
};

class depotGames : public ucstart
{
  public:
    depotGames()
    {
        Aws::InitAPI(options__);
    }

    ~depotGames()
    {
        Aws::ShutdownAPI(options__);
    }

  protected:
    void onStart(ucontext *ctx, const char *param)
    {
        registerUser("test1", 1001);
        registerUser("test2", 1002);
        registerUser("test3", 1003);
        registerUser("test4", 1004);
        registerUser("test5", 1005);
        registerUser("test6", 1006);
        registerUser("test7", 1007);
        registerUser("test8", 1008);
        registerUser("test9", 1009);
        registerUser("test10", 1010);

        awsconf__.gameLiftAlias = UENVSTR("GAMELIFT_ALIAS", NULL);
        awsconf__.gameLiftRegion = UENVSTR("GAMELIFT_REGION", NULL);
        awsconf__.gameLiftPlayTableName = UENVSTR("GAMELIFT_PLAYER_TABLENAM", NULL);
    }

    int onCallback(void *ud, int type, uint32_t source, int session, void *data, size_t sz)
    {
        switch (type)
        {
        case PGE_LOGIN:
            loginProc(data);
            break;
        case PGE_ENTER:
            enterPorc(data);
            break;
        case PGE_EXIT:
            exitProc(data);
            break;
        case PGE_START:
            break;
        case PGE_UNLINE:
            unlineProc(data);
            break;
        default:
            break;
        }
        return 0;
    }

  private:
    int getUserID(const char *name)
    {
        for (int i = 0; i < userlist__.size(); i++)
        {
            if (strcmp(userlist__[i].name, name) == 0)
            {
                return userlist__[i].userId;
            }
        }
        return 0;
    }

    void registerUser(const char *name, int userId)
    {
        struct UserInfo *u = (struct UserInfo *)umemory::malloc(sizeof(*u));
        strcpy(u.name, name);
        u.userId = userId;
        userlist__.push(u);
    }

    void loginProc(void *data)
    {
        int socketId = getSocketId(data);
        if (socketId <= 0)
        {
            ucontext::error(parent__->id, "login data error:socket id %d", socketId);
            return;
        }

        //DEPOT_PACKET_START
        struct login *lPack = (struct login *)(data + DEPOT_PACKET_START);
        if (strlen(lPack->userName) > 16)
        {
            ucontext::error(parent__->id, "login account name len out");
            return;
        }

        int userId = getUser(lPack->userName);
        if (userId <= 0)
        {
            ucontext::error(parent__->id, "login account does not exist[%s]", lPack->userName);
            return;
        }

        playtable__.pushSession(socketId, lPakc->userName, userId, 0, 0);

        char *resultData = (char *)umemory::malloc(sizeof(int) * 2);
        assert(resultData);
        memcpy(resultData, (char *)&socketId, sizeof(int));
        memcpy(resultData + sizeof(int), (char *)&userId, sizeof(int));
        if (!ucall::sendEvent(parent__->id, "netgames", PGE_LOGIN, 0, resultData, sizeof(int) * 2))
        {
            ucontext::error(parent__->id, "login back errror");
            return;
        }
    }

    void enterPorc(void *data)
    {
        int socketId = getSocketId(data);
        if (socketId <= 0)
        {
            ucontext::error(parent__->id, "enter room data error:socket id %d", socketId);
            return;
        }

        struct enterRoom *lPack = (struct enterRoom *)data;
        if (lPack->RoomID < 0 || lPack->RoomID > 0)
        {
            ucontext::error(parent__->id, "enter room data error:param room id %d", lPack->RoomID);
            return;
        }

        int r = roomtable__.enterR(lPack->userId, 0);

        char *resultData = (char *)umemory::malloc(sizeof(int) * 2);
        assert(resultData);
        memcpy(resultData, (char *)&socketId, sizeof(int));
        memcpy(resultData + sizeof(int), (char *)&r, sizeof(int));

        if (!ucall::sendEvent(parent__->id, "netgames", PGE_ENTER, 0, resultData, sizeof(int) * 2))
        {
            ucontext::error(parent__->id, "enter room back errror");
            return;
        }
    }

    void exitProc(void *data)
    {
        int socketId = getSocketId(data);
        if (socketId <= 0)
        {
            ucontext::error(parent__->id, "exit room data error:socket id %d", socketId);
            return;
        }

        struct exitRoom *lPack = (struct exitRoom *)data;
        if (lPack->userId <= 0)
        {
            ucontext::error(parent__->id, "exit room data error:param user id %d", lPack->userId);
            return;
        }

        int r roomtable__->exitR(lPack->userId);

        char *resultData = (char *)umemory::malloc(sizeof(int) * 2);
        assert(resultData);
        memcpy(resultData, (char *)&socketId, sizeof(int));
        memcpy(resultData + sizeof(int), (char *)&r, sizeof(int));

        if (!ucall::sendEvent(parent__->id, "netgames", PGE_EXIT, 0, resultData, sizeof(int) * 2))
        {
            ucontext::error(parent__->id, "exit room back errror");
            return;
        }
    }

    void startProc()
    {
        Aws::GameLift::Model::SearchGameSessionsRequest searchReq;
        searchReq.SetAliasId(awsconf__.gameLiftAlias);
        searchReq.SetFilterExpression("playerSessionCount=0 AND hasAvailablePlayerSessions=true");
        auto searchResult = glclient__->SearchGameSessions(searchReq);
        if (searchResult.IsSuccess())
        {
            auto availableGs = searchResult.GetResult().GetGameSessions();
            if (availableGs.size() > 0)
            {
                auto ipAddress = availableGs[0].GetIpAddress();
                auto port = availableGs[0].GetPort();
                auto gameSessionId = availableGs[0].GetGameSessionId();

                ucontext::error(parent__->id, "create player sessions on searched game session");
                createPlaySession(ipAddress, gameSessionId, port);
            }
            else
            {
                Aws::GameLift::Model::CreateGameSessionRequest req;
                req.SetAliasId(awsconf__.gameLiftAlias);
                req.SetMaximumPlayerSessionCount(MAX_PLAY_NUM);
                auto outcome = glclient__->CreateGameSession(req);
                if (outcom.IsSuccess())
                {
                    auto gs = outcome.GetResult().GetGameSession();
                    auto port = gs.GetPort();
                    auto ipAddress = gs.GetIpAddress();
                    auto gameSessionId = gs.GetGameSessionId();
                    ucontext::error(parent__->id, "create player session on created game session");
                    createPlaySession(ipAddress, gameSessionId, port);
                }
                else
                {
                    ucontext::error(parent__->id, "CreateGameSession error: %s", outcome.GetError().GetExceptionName().c_str());
                }
            }
        }
        else
        {
            ucontext::error(parent__->id, "SearchGameSessions error: %s", searchResult.GetError().GetExceptionName().c_str());
        }
    }

    void unlineProc(void *data)
    {
        int socketId = getSocketId(data);
        if (socketId <= 0)
        {
            ucontext::error(parent__->id, "unline data error:socket id %d", socketId);
            return;
        }

        int userId = playtable__.exitSocketId(socketId);
    }

    int getSocketId(void *data)
    {
        int reuslt = 0;
        memcpy(&result, data, sizeof(int));
        return result;
    }

  private:
    int getUser(const char *name)
    {
        int result = 0;
        for (int i = 0；i < userlist__->size(); i++)
        {
            if (strcom(userlist__[i].userName, name) == 0)
            {
                result = userlist__[i].userId;
                break;
            }
        }
        return result;
    }

    void createPlaySession(const std::string &ipAddress, const std::string &gsId, int port)
    {
        Aws::GameLift::Model::CreatePlayerSessionsRequest req;
        req.SetGameSessionId(gsId);
        std::vector<int> playerIds;
        roomtable__.getPlays(&playerIds);
        if (playerIds.empty())
        {
            ucontext::error(parent__->id, "room is emtpy game address:%s port:%d, gs:%s", ipAddress.c_str(), port, gsid.c_str());
            return;
        }

        int i;
        std::vector<std::string> playerNames;
        std::map<std::string, std::string> userIdMap;
        for (i = 0; i < playerIds.size(); i++)
        {
            const char *lname = playtable__.getSessionName(playerIds[i]);
            if (lname == NULL)
            {
                continue;
            }

            playerNames.push_back(lname);
            userIdMap[lname] = std::to_string(playerIds[i]);
        }
        //user play name
        req.SetPlayerIds(playerNames);
        req.SetPlayerDataMap(userIdMap);

        auto result = glclient__->CreatePlayerSessions(req);
        if (result.IsSuccess())
        {
            //
            int j = 0;
            auto playerSessionList = result.GetResult().GetPlayerSessions();
            for (i = 0; i < playerIds.size(); i++)
            {
                if (playerIds[i] <= 0)
                {
                    continue;
                }
                auto playSessionId = playerSessionList[j].GetPlayerSessionId();
                backUser(playerIds[i], port, ipAddress, playSessionId, 0);
                ++j;
            }
        }
        else
        {
            ucontext::error(parent__->id, "CreatePlayerSessions error: %s", result.GetError().GetExceptionName().c_str());
            for (i = 0; i < playerIds.size(); i++)
            {
                if (playerIds[i] <= 0)
                {
                    continue;
                }
                backUser(playerIds[i], port, ipAddress, std::string("psess-error"), 1);
            }
        }
    }

    void backUser(int userId, int port, const std::string &ipAddr, const std::string &psessId, int code)
    {
        int socketId = playtable__.getSessionSocketId(userId);
        if (socketId <= 0)
        {
            ucontext::error(parent__->id, "back User fail socketId:%d, userId:%d", socketId, userId);
            return;
        }

        //startBack
        char *data = (char *)umemory::malloc(128);
        struct startBack *lback = (struct startBack *)data;
        lback->socketId = socketId;
        strcpy(lback->ip, ipAddr.c_str());
        lback->port = port;
        lback->code = code;
        strcpy(lback->sessionId, psessId.c_str());

        if (!ucall::sendEvent(parent__->id, "netgames", PGE_START, 0, data, 128))
        {
            ucontext::error(parent__->id, "(free)back User fail socketId:%d, userId:%d", socketId, userId);
            umemory::free(data);
            return;
        }
    }

  private:
    depotPlays playtable__;
    deptoRooms roomtable__;
    vector<UserInfo *> userlist__;

    Aws::SDKOptions options__;
    std::shared_ptr<Aws::GameLift::GameLiftClient> glclient__;
    awsGameConfig awsconf__;
};

extern "C" depotGames *depotgames_create(void)
{
    depotGames *inst = new depotGames();
    return inst;
}

extern "C" void depotgames_release(depotGames *inst)
{
    delete inst;
    inst = NULL;
}