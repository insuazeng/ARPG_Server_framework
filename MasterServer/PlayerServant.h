#pragma once

#include "MasterCommServer.h"

class CMasterCommServer;
class CPlayerServant;

struct OpcodeHandler
{
	uint16 status;
	void (CPlayerServant::*handler)(WorldPacket &recvPacket);
};

enum ServantStatus
{
	STATUS_NONE					= 0x00,		// 无
	STATUS_LOGIN				= 0x01,		// 已登录
	STATUS_GAMEING				= 0x02,		// 在游戏中
	STATUS_SERVER_SWITTHING		= 0x04,		// 服务端切换
};

enum RoleCreateError
{
	CREATE_ERROR_NONE,
	CREATE_ERROR_NAME			= 100801,
	CREATE_ERROR_LIMIT			= 100802,
	CREATE_ERROR_EXIST			= 100803,
	CREATE_ERROR_NAME_LEN		= 100804,
	CREATE_ERROR_CAREER			= 100805,
};

enum RoleLoginError
{
	LOGINERROR_NONE,
	LOGINERROR_ROLE,
	LOGINERROR_BANNED,
	LOGINERROR_TIMELIMIT,
};

struct tagSingleRoleInfo
{
	tagSingleRoleInfo() : m_nPlayerID(0), 
		m_strPlayerName(""), 
		m_nCareer(-1),
		m_nLevel(1)
	{

	}
	tagSingleRoleInfo(uint64 plrGuid, const char* plrName, uint32 level, int career, uint32 banned) : 
		m_nPlayerID(plrGuid),
		m_strPlayerName(plrName),
		m_nLevel(level),
		m_nCareer(career),
		m_nBanExpiredTime(banned)
	{
	}

	void Clear()
	{
		m_nPlayerID = 0;
		m_strPlayerName = "";
		m_nLevel = 0;
		m_nCareer = -1;
		m_nBanExpiredTime = 0;
	}

	uint64 m_nPlayerID;
	std::string m_strPlayerName;
	uint32 m_nLevel;
	int m_nCareer;				// 职业
	uint32 m_nBanExpiredTime;
};

#define MAX_ROLE_PER_ACCOUNT	1

struct tagRoleList			// 账户拥有所有角色信息
{
	tagRoleList() : m_strAccountName(""), m_strAgentName(""), m_nPlayerGuid(0)
	{
		m_vecRoles.clear();
	}
	void Clear()
	{
		m_strAccountName = "";
		m_strAgentName = "";
		m_nPlayerGuid = 0;
		m_vecRoles.clear();
	}

	const uint32 GetRolesNum() { return m_vecRoles.size(); }

	tagSingleRoleInfo* GetSingleRoleInfoByGuid(uint64 plrGuid)
	{
		tagSingleRoleInfo *ret = NULL;
		for (vector<tagSingleRoleInfo>::iterator it = m_vecRoles.begin(); it != m_vecRoles.end(); ++it)
		{
			if (it->m_nPlayerID == plrGuid)
			{
				ret = &(*it);
				break ;
			}
		}
		return ret;
	}

	void AddSingleRoleInfo(tagSingleRoleInfo &roleInfo)
	{
		m_vecRoles.push_back(roleInfo);
	}

	string m_strAccountName;				// 账户名
	string m_strAgentName;					// 代理名
	uint64 m_nPlayerGuid;					// 玩家guid（当玩家未选择角色进入时，该guid为网关根据平台/账号生成的一个哈希值，玩家选择角色进入游戏后，该值为角色的guid）
	std::vector<tagSingleRoleInfo> m_vecRoles;	// 保存角色信息的数组
};

class ServantCreateInfo;

class CPlayerServant
{
	friend class CPlayerMgr;
public:
	CPlayerServant(void);
	~CPlayerServant(void);

	void Init( ServantCreateInfo * info, uint32 sessionIndex );
	void Term();
	void DisConnect();

	void FillRoleListInfo();		// 填充角色列表信息
	void WriteRoleListDataToBuffer(pbc_wmessage *msg);
	void LogoutServant(bool saveData);						// 玩家下线，登出（保存）
	void ResetCharacterInfoBeforeTransToNextSceneServer();	// 传送到下一个场景服前要做的事

public:
	inline const uint32 GetGMFlag() { return m_nGmFlag; }
	inline bool IsBeDeleted() { return m_bDeleted; }
	inline bool IsWaitLastData(){ return m_deleteTime>0;}
	inline void SetWaitLastData(){ if(m_deleteTime==0) m_deleteTime=UNIXTIME;}
	inline bool HasPlayer() { return m_bHasPlayer; }
	inline const uint32 GetCurSceneServerID() { return m_nCurSceneServerID; }
	inline const time_t GetLastSetServerIndexTime() { return m_lastTimeSetServerIndex; }
    inline tagRoleList& GetPlayerRoleList() { return m_RoleList; }
	inline const uint64 GetCurPlayerGUID() { return m_RoleList.m_nPlayerGuid; }
	inline void ResetCurPlayerGUID(uint64 realGuid) { m_RoleList.m_nPlayerGuid = realGuid; }
	inline std::string strGetAccountName() { return m_RoleList.m_strAccountName; }
	inline const char* szGetAccoundName() { return m_RoleList.m_strAccountName.c_str(); }
	inline std::string strGetAgentName() { return m_RoleList.m_strAgentName; }
	inline const char* szGetAgentName() { return m_RoleList.m_strAgentName.c_str(); }
	inline bool IsInGameStatus(uint32 status) { return (m_nGameState & status) > 0; }

	inline void QueuePacket(WorldPacket *packet) { _recvQueue.Push(packet); }
	inline void SetCommSocketPtr(CMasterCommServer *pSock) { m_pServerSocket = pSock; }
	inline CMasterCommServer* GetServerSocket() { return m_pServerSocket; }

	inline bool HasGMFlag() { return m_gm; }

public:
	int Update(uint32 p_time);
	bool SetSceneServerID(uint32 curSceneServerIndex, const uint32 setIndexTimer, bool forceSet);
	void OnPlayerChangeSceneServer(uint32 changeIndexTimer);
	void ResetSyncObjectDatas();

	void SendServerPacket(WorldPacket *packet);//为了防止歧义
	void SendResultPacket(WorldPacket *packet);
	
public:
	// 导出给Lua用的函数
	int getAccountName(lua_State *l);
	int getAgentName(lua_State *l);
	int getRoleCount(lua_State *l);
	int getSessionIndex(lua_State *l);
	int getCurSceneServerID(lua_State *l);
	int addNewRole(lua_State *l);
	int setRoleListInfo(lua_State *l);
	int onEnterSceneNotice(lua_State *l);

	int sendServerPacket(lua_State *l);
	int sendResultPacket(lua_State *l);

	LUA_EXPORT(CPlayerServant)

public:
	bool m_bHasPlayer;						// 是否有player在游戏中（进入游戏前、切换地图服务器时此标记都为false
	uint32 m_nCurSceneServerID;				// 当前所在的场景服索引（不在任何场景服时（进入游戏前，切换场景服时）为0）
	time_t m_lastTimeSetServerIndex;		// 最后上次设置场景服索引的时间

	uint32 m_nGameState;					// 游戏状态（无，已登录，游戏中）
	uint64 m_nReloginStartTime;				// 重复登陆的时间（不同网关发送来的重复登陆请求，必须在三秒钟之后才能再次进来登陆）

	bool m_bVerified;						// 是否已经进行过认证
	bool m_bPass;							// 是否已通过身份认证
	bool m_bLogoutFlag;						// 登出标记

	uint32 m_nGmFlag;
	uint32 m_nSessionIndex;					// 会话id（中央、网关、游戏三者一致）

	// 对象数据同步
	ByteBuffer m_syncObjectDataBuffer;		// 缓冲区
	uint32 m_nSyncDataObjectCount;			// 数量
	uint32 m_nSyncDataLoopCounter;			// 循环计数器（并非每次update都同步数据）

	bool	m_newPlayer;					// 只有新角色才需要保存流失日志

protected:
	bool m_bDeleted;						// 是否被删除掉了
	uint32 m_deleteTime;					// 删除计时，用来做等待游戏服返回下线包的超时检测
    bool   m_gm;                            // 内部人员标识

	// 网络接收包
	FastQueue<WorldPacket*, Mutex> _recvQueue;	// 所有玩家发出来要中央服务器处理的消息包都会存在这里
	Mutex deleteMutex;
	
	CMasterCommServer *m_pServerSocket;
	tagRoleList m_RoleList;
};
