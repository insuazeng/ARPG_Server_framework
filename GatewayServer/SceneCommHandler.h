#pragma once

#include "ServerCommonDef.h"
#include "auth/Sha1.h"
#include "GameSocket.h"
#include "SceneCommClient.h"
#include "MasterCommClient.h"

struct tagGameSocketCacheData
{
	tagGameSocketCacheData() : plr_guid(0), gm_flag(0), relogin_valid_expire_time(0), session_index(0)
	{
	}
	uint64 plr_guid;
	uint32 gm_flag;
	uint32 session_index;
	uint32 relogin_valid_expire_time;
	string agent_name;
	string acc_name;
	string token_data;
};

typedef map<uint64, GameSocket*> GameSocketMap;

class CSceneCommHandler : public Singleton<CSceneCommHandler>
{
public:
	CSceneCommHandler(void);
	~CSceneCommHandler(void);

public:
	// Func
	CSceneCommClient* ConnectToSceneServer(string Address, uint32 Port, bool blockingConnect = true, int32 timeOutSec = 1);		// 使用网关服务器套接字去连接游戏服务器
	CMasterCommClient* ConnectToCentreServer(string Address, uint32 Port);	// 连接中央服务器

	void Startup();															// 本控制器启动（读取文件配置，连接至游戏服务器）
	void OnGameServerConnectionModified();									// 场景服被成功连入（网络连入+验证成功）
	void FillValidateGameServerInfo(WorldPacket &pck);						// 填充有效的场景服信息

	void ConnectionDropped(uint32 ID);										// 网关服务器与某场景服断开连接
	void CentreConnectionDropped();											// 网关服务器与中央服务器断开连接
	void AdditionAck(uint32 ID, uint32 ServID);								// 将一个网关服务器变成已注册
	void AdditionAckFromCentre(uint32 id);									// 网关服务器在中央注册成功
	
	//更新游戏服务器的包 add by tangquan 2011/11/11
	void UpdateSockets();													// 当某一网关服务器ping出错时便关闭它
	//更新客户端来的数据包 add by tangquan 2011/11/11
	void UpdateGameSockets();												// 轮询检查每个客户端套接字，如果超时便通知游戏服务器将该会话关闭
	void UpdatePendingGameSockets();										// 更新每个待进入游戏的gamesocket
		
	void ConnectSceneServer(SceneServer * server);							// 让某个网关服务器连接到场景服务器
	void ConnectCentre(MasterServer *server);								// 让该网关服连接到中央服务器
	
	void SendPacketToSceneServer(WorldPacket *data, uint32 uServerID);
	void SendPacketToMasterServer(WorldPacket *data);
	
	void LoadRealmConfiguration();											// 读取文件配置（创建游戏服务器对象以及网关服务器对象）

	inline GameSocket* GetSocketByPlayerGUID(uint64 guid)					// 通过角色guid找到一个客户端套接字
	{
		GameSocket* sock = NULL;
		onlineLock.Acquire();
		GameSocketMap::iterator itr = online_players.find(guid);
		if (itr != online_players.end())
		{
			sock = itr->second;
		}
		onlineLock.Release();
		return sock;
	}

	inline uint32 GetOnlinePlayersCount()
	{ 
		onlineLock.Acquire();
		uint32 count = online_players.size(); 
		onlineLock.Release();
		return count;
	}
	
	inline uint32 GenerateSessionIndex()
	{
		uint32 uRet = 0;
		sessionIndexLock.Acquire();
		uRet = ++m_nSessionIndexHigh;
		sessionIndexLock.Release();
		return uRet;
	}


	inline void SetIPLimit(uint32 limit) { m_nIPPlayersLimit = limit; }

	void UpdateGameSocektGuid(uint64 tempGuid, uint64 realGuid, GameSocket *sock);		// 更新GameSock的guid数据
	// 向在线玩家表中添加一个新的玩家
	void AddNewPlayerToSceneServer( GameSocket * playerSocket, ByteBuffer *enterData, uint32 targetSceneServerID );
	// 发送积压包
	void UpdateClientSendQueuePackets();
	// 处理客户端接收缓冲
	void UpdateClientRecvQueuePackets();
	// 关闭所有客户端套接字（贡文斌 9.10修改，按照所属游戏服务器关闭）
	void CloseAllGameSockets(uint32 serverid = 0);
	//关闭所有服务端端口
	void CloseAllServerSockets();
	//发送全局消息
	void SendGlobaleMsg(WorldPacket *packet);
	// 发送全局消息给部分的玩家
	void SendGlobalMsgToPartialPlayer(WorldPacket &rawPacket);

	CSceneCommClient* GetServerSocket(uint32 serverid);
	void SetGameServerOLPlayerNums(uint32 serverId, uint32 playerNum);
	void WriteGameServerOLNumsToPacket(WorldPacket &pack);

	// 添加压缩包数据的使用统计
	void AddCompressedPacketFrenquency(uint16 opCode, uint32 length, uint32 compressed_length);
	void PrintCompressedPacketCount();

	bool OnIPConnect(const char* ip);
	void OnIPDisconnect(const char* ip);

	// 断线重连相关
	void RegisterReloginAccountTokenData(string agent_name, string &acc_name, uint64 guid, string &token_data, uint32 index, uint32 gm_flag);
	void RemoveReloginAccountData(string &acc_name);
	int32 CheckAccountReloginValidation(string &acc_name, string &recv_token);
	tagGameSocketCacheData* GetCachedGameSocketData(string &acc_name);

	// 等待登陆列表
	void AddPendingGameList(GameSocket *playerSocket, uint64 tempGuid);
	void RemoveDisconnectedSocketFromPendingGameList(uint64 guid);
	void SendCloseSessionNoticeToScene(uint32 scene_server_id, uint64 guid, uint32 session_index);
	void SendCloseServantNoticeToMaster(uint64 guid, uint32 session_index, bool direct);

public:
	SHA1Context m_sha1AuthKey;								// 用于网关服在场景服处验证注册时用到的sha1哈希值

	uint32 g_transpacketnum;
	uint32 g_transpacketsize;
	uint32 g_totalSendBytesToClient;
	uint32 g_totalRecvBytesFromClient;
	//uint32 m_clientPacketCounter[NUM_MSG_TYPES];

	// 网关和游戏服务器交互包统计
	uint32 g_totalSendDataToSceneServer;
	uint32 g_totalRecvDataFromSceneServer;
	uint32 m_sceneServerPacketCounter[SERVER_MSG_OPCODE_COUNT];
	// 网关和中央服务器交互包统计
	uint32 g_totalSendDataToMasterServer;
	uint32 g_totalRecvDataFromMasterServer;
	uint32 m_masterServerPacketCounter[SERVER_MSG_OPCODE_COUNT];
	void OutPutPacketLog();

private:
	std::vector<SceneServer*> m_SceneServerConfigs;				// 保存场景服务器配置的集合
	std::map<SceneServer*, CSceneCommClient*> m_sceneServers;	// 保存与场景服务器通讯的套接字映射表

	MasterServer* m_pMasterServerInfo;							// 用来保存中央服务器的一些信息
	CMasterCommClient* m_pMasterServer;							// 连接中央服务器的套接字表

	// 数据结构修改为<映射，AccountID>
	GameSocketMap	online_players;								// 保存在线玩家的套接字映射表
	HM_NAMESPACE::hash_map<string, tagGameSocketCacheData> m_gamesock_login_token;	// 缓存起来的gamesocket令牌数据，用于重登陆

	std::map<uint32, uint32>	m_ipcount;						// 同ip连接数控制
	Mutex connectionLock;										// 连接和断开的锁

	GameSocketMap	m_pendingGameSockets;					// 验证成功准备登陆游戏的socket<角色guid，gamesocket对象>

	Mutex mapLock;											// 游戏服务器通讯的套接字映射表的锁
	Mutex onlineLock;										// 在线玩家表锁
	Mutex reloginGameSocketLock;							// 断线重登的socket锁
	Mutex sessionIndexLock;									// sessionIndex的生成锁
	Mutex pendingGameLock;									// 等待登陆的玩家
	
	uint32 m_nSessionIndexHigh;								// sessionIndex
	uint32 m_nIPPlayersLimit;								// 所允许最大在线数量

	map<uint16, map<uint32, uint32> > m_compressPacketFrenc;// 压缩数据包的统计 
	Mutex m_compressPackLock;

private:
	//只能由updategamesocket本线程调用
	void __RemoveOnlineGameSocket(uint64 guid);												// 通过账户ID找到一个客户端套接字并且移除它
	void __AddGameSocketToOnlineList(uint64 guid, GameSocket *sock);						// 添加一个gamesock对象到在线列表
	bool __CheckGameSocketOnlineAndDisconnect(const string &agent, const string &accName);	// 确认socket是否存在，如果存在则断开

public:
	bool LoadStartParam(int argc, char* argv[]);
	void SetStartParm(const char* key,std::string val);
	const char* GetStartParm(const char* key);
	int GetStartParmInt(const char* key);

private:
	std::map<std::string, std::string> m_startParms;
};

#define sSceneCommHandler CSceneCommHandler::GetSingleton()
