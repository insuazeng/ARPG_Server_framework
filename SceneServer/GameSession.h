/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

//
//
// GameSession.h
//

#ifndef __GAMESESSION_H
#define __GAMESESSION_H

class WorldPacket;
class GameSession;

#include "Player.h"
#include "SceneCommServer.h"

struct OpcodeHandler
{
	uint16 status;
	void (GameSession::*handler)(WorldPacket& recvPacket);
};

//检查角色在世界中
#define CHECK_INWORLD_RETURN if(m_player->GetGUID()==0 || !m_player->IsInWorld()) { return; }

class GameSession
{
public:
	GameSession();
	~GameSession();

public:
	void InitSession(string &accName, uint32 srcPlatform, string &agentName, CSceneCommServer *sock, uint32 index, uint64 plrGuid, uint32 gmflag = 0, bool internalPlayer = false);
	void TermSession();

	void SendPacket(WorldPacket* packet, ENUMMSGPRIORITY msgPriority = HIGH_PRIORITY);
	void SendServerPacket(WorldPacket* packet);

public:
	inline const uint64 GetUserGUID() { return _guid; }
	inline Player* GetPlayer() { return m_player; }
	inline void SetPlayer(Player *plr) { m_player = plr; }
	inline const uint32 GetPlatformID() { return _srcPlatformID; }

	inline void SetGateServerSocket(CSceneCommServer *sock)
	{
		deleteMutex.Acquire();
		m_pServerSocket = sock;
		deleteMutex.Release();
	}
	inline CSceneCommServer* GetGateServerSocket()
	{ 
		CSceneCommServer *ret = NULL;
		deleteMutex.Acquire();
		if (m_pServerSocket != NULL && m_pServerSocket->IsConnected())
			ret = m_pServerSocket;
		deleteMutex.Release();

		return ret;
	}
	inline void SetNextSceneServerID(uint32 nextID) { m_NextSceneServerID = nextID; }

	inline void SetLogoutTimer(uint32 ms)
	{
		if(ms)  _logoutMsTime = getMSTime64() + ms;
		else	_logoutMsTime = 0;
	}

	inline void QueuePacket(WorldPacket* packet)
	{
		_recvQueue.Push(packet);
	}

	inline void Disconnect()
	{
		//如果disconnect和socket的OnDisconnect事件同时发生，则此处还是会出错2010.2.4 贡文斌
		//if(m_pServerSocket && m_pServerSocket->IsConnected())
		//	m_pServerSocket->DeletePlayerSocket(this->_accountId);
		//m_pServerSocket = NULL;

		//这里设置后，会在update的时候向网关发送保存并退出的数据包
		SetNextSceneServerID(0);
		SetLogoutTimer(1);			// 1ms后登出
	}
	inline void SetInstance(uint32 Instance) 
	{ 
		instanceId = Instance; 
		// sLog.outString("Player %s Session Instance Set to %u", GetPlayer()->strGetGbkNameString().c_str(), Instance);
	}
	inline std::string& GetAccountName() { return _accountName; }
    inline uint32 GetInstance() { return instanceId; }

	// void SaveBackDataToMasterServer();
	// 填充需要保存回中央服的信息
	// void FillPlayerSaveDatas(WorldPacket &data);
	// 发送会话创建成功结果包
	void SendCreateSuccessPacket();

	// 登出相关
	void LogoutPlayer(bool SaveToCentre);
	int  Update(uint32 InstanceID);

	bool bDeleted;
	Mutex deleteMutex;

public:
	// lua导出相关
	int getSessionIndex(lua_State *l);
	int getNextSceneServerID(lua_State *l);
	int setNextSceneServerID(lua_State *l);
	int getSrcPlatformID(lua_State *l);
	int onSceneLoaded(lua_State *l);
	int sendServerPacket(lua_State *l);

	LUA_EXPORT(GameSession)

public:
	// 玩家移动
	void HandleSessionPing(WorldPacket &recvPacket);
	void HandlePlayerMove(WorldPacket & recvPacket);
	void HandlePlayerSceneTransfer(WorldPacket & recvPacket);
	// void HandleGameLoadedOpcode(WorldPacket & recvPacket);

	// 副本活动
	void HandleExitInstance( WorldPacket & recvPacket );

public:
	uint32 m_SessionIndex;				// 每次登陆都不同的索引
	uint32 m_GMFlags;
	uint32 m_NextSceneServerID;			// 下一个场景服索引
	bool m_internal_player_flag;		// 内部玩家标记

private:
	friend class Player;
	Player *m_player;

	CSceneCommServer *m_pServerSocket;	// 每个会话所持有的对应的游戏服务器的套接字
	string _accountName;				// 账号名
	string _accountAgentName;			// 平台代理名
	uint32 _srcPlatformID;				// 源平台id
	uint64 _guid;						// 用户guid,与player对象中的guid一致,新添加一个是为了适应一些特殊场合,而player对象数据此时又被析构掉
	uint64 _logoutMsTime;				// time we received a logout request -- wait 20 seconds, and quit

	FastQueue<WorldPacket*, Mutex>	_recvQueue;

	bool _loggingOut;						// 正在登出

	uint32 instanceId;
};

typedef std::set<GameSession*> SessionSet;

#endif
