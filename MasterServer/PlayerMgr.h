#pragma once

#include "threading/RWLock.h"
#include "PlayerServant.h"
#include "ObjectPool.h"

const static uint32 FORBID_LEVELLOW = 0;
const static uint32 FORBID_LEVELHIGH = 1;

extern const uint32 ILLEGAL_DATA_32;
extern const uint64 ILLEGAL_DATA_64;

struct BlockChatRecord
{
	uint32 id;
	uint32 accountid;
	uint32 block_state;
	uint32 start_time;
	uint32 end_time;
};

enum ChatBlockState
{
	CHAT_STATE_FREE			= 0,		// 自由状态
	CHAT_STATE_BLOCKED		= 1			// 被禁状态
};

enum ChatSqlState
{
	CHAT_SQL_NEW		= 0,
	CHAT_SQL_BLOCKING	= 1,
	CHAT_SQL_EXPIRE		= 2,
	CHAT_SQL_UNBLOCK	= 3,
};

typedef std::map< uint32, BlockChatRecord * > BlockMap;

class ServantCreateInfo
{
public:
    ServantCreateInfo() : m_guid( 0 ), m_gmflags( 0 ), 
        m_agentname( "" ), m_accountname( "" )
    {
    }
	void Init(uint64 guid, uint32 gmFlag, string &agent, string &name)
	{
		m_guid = guid;
		m_gmflags = gmFlag;
		m_agentname = agent;
		m_accountname = name;
	}

	uint64 m_guid;
    uint32 m_gmflags;
	string m_agentname;
    string m_accountname;
};

class CPlayerMgr : public Singleton <CPlayerMgr>
{
public:
	CPlayerMgr(void);
	~CPlayerMgr(void);

public:
	void Initialize();
	void Terminate();

public:
	CPlayerServant* FindServant( uint64 guid );									// 根据guid找到会话实例
	CPlayerServant* FindServant( const string &agent, const string &account );	// 根据平台+账号名找到会话实例

	uint32 GetPlayerCount()
	{
		uint32 totalcount = 0;
		m_playersLock.AcquireWriteLock();
		totalcount = m_onlinePlayerList.size();
		m_playersLock.ReleaseWriteLock();
		return totalcount;
	}

	void OnGateCommServerDisconnect(CMasterCommServer *pServer);
	CPlayerServant* CreatePlayerServant( ServantCreateInfo *info, uint32 sessionIndex, CMasterCommServer *pSocket );
	void AddServant(CPlayerServant *pServant);
	void ReAddServantWithRealGuid(CPlayerServant *pServant, uint64 tempGuid, uint64 realGuid);
	void InvalidServant(uint64 guid, uint32 sessionIndex, CMasterCommServer *pFromServer, bool ingoreIndex = false, bool ingoreServerSocket = false,bool direct = true);		// 让一个servant失效

public:
	void SessioningProc(WorldPacket *recvData, uint64 guid);
	void SaveAllOnlineServants();

	void UpdatePlayerServants(float diff);

    inline void AddPendingServant( CPlayerServant * servant )
    {
        _queueServants.push_back( servant );
    }
	void UpdateQueueServants(float diff);		// 处理重复登陆的一些servant

	void UpdateBlockChat( time_t diff );
	uint32 GetBlockPlayer( uint32 accoutid );
	void LoadBlockPlayers( bool first = false );

    uint32 AnalysisWords( const char* pcName, const uint32 uGuid, const string & strText );
	void ReLoadFrbidWords();

	//给在线玩家发送消息
	void FillSpecifiedConditonAccountIDs(vector<uint64> &vecGUIDs, bool onlineFlag, uint32 minLevel = 0, uint32 maxLevel = ILLEGAL_DATA_32);
	void SendPacketToOnlinePlayers(WorldPacket &packet);
	void SendPacketToOnlinePlayersWithSpecifiedLevel(WorldPacket &packet, uint32 minLevel = 0, uint32 maxLevel = ILLEGAL_DATA_32);
	void SendPacketToPartialPlayers(WorldPacket &packet, vector<uint64> &vecUserGUIDs);

public:
	// 导出给lua用的函数
	static int findPlayerServant(lua_State *l);
	static int updatePlayerServantRealGuid(lua_State *l);
	static int invalidServant(lua_State *l);

	LUA_EXPORT(CPlayerMgr)

public:
	std::list<CPlayerServant*> _queueServants;		// 用于保存延迟加入servant的列表,因为只在一个线程中使用，所以改成list，不再使用fastqueue
	void Save_ChatLog( const char* pcName, const uint32 uGuid, const std::string & pcText ,bool toalllog,uint32 chanel, uint32 group);

private:
    void load_ForbidWord();
    void DailyClearForbidLog(); 

private:
	time_t m_BlockChatInterval;

    typedef std::map< uint64, CPlayerServant* > ServantMap;
	ServantMap m_onlinePlayerList;
	RWLock m_playersLock;

	BlockMap m_BlockPlayers;	// 禁言记录

	// 服务端保存的过滤词
	std::vector<string> m_forbidWord;
	std::vector<string> m_forbidWordlevel1;
};

#define sPlayerMgr CPlayerMgr::GetSingleton()
