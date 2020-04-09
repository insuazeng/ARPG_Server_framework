/*
 * Ascent MMORPG Server
 * Copyright (C) 2005-2010 Ascent Team <http://www.ascentemulator.net/>
 *
 * This software is  under the terms of the EULA License
 * All title, including but not limited to copyrights, in and to the AscentNG Software
 * and any copies there of are owned by ZEDCLANS INC. or its suppliers. All title
 * and intellectual property rights in and to the content which may be accessed through
 * use of the AscentNG is the property of the respective content owner and may be protected
 * by applicable copyright or other intellectual property laws and treaties. This EULA grants
 * you no rights to use such content. All rights not expressly granted are reserved by ZEDCLANS INC.
 *
 */


#ifndef __MAPMGR_H
#define __MAPMGR_H

#include "MapInfo.h"
#include "MapCell.h"
#include "MapObjectStorage.h"
#include "CellHandler.h"

class MapCell;
class MapSpawn;
class Object;
class GameSession;
class Player;
class Monster;
class Instance;

enum MapMgrTimers
{
	MMUPDATE_OBJECTS		= 0,
	MMUPDATE_SESSIONS		= 1,
	MMUPDATE_FIELDS			= 2,
	MMUPDATE_IDLE_OBJECTS	= 3,
	MMUPDATE_ACTIVE_OBJECTS = 4,
	MMUPDATE_COUNT			= 5
};

enum ObjectActiveState
{
	OBJECT_STATE_NONE		= 0,
	OBJECT_STATE_INACTIVE	= 1,
	OBJECT_STATE_ACTIVE		= 2,
};

typedef hash_set<Object* > ObjectSet;
typedef hash_set<Object* > UpdateQueue;
typedef hash_set<Player*  > PUpdateQueue;
typedef hash_set<Player*  > PMoveInfoQueue;
typedef hash_set<Player*  > PCreationQueue;
typedef hash_set<Player*  > PlayerSet;
typedef hash_set<Monster* > MonsterSet;
typedef list<uint64> MonsterGuidList;
typedef list<Monster* > MonsterList;
typedef hash_map<uint64, Monster*> MonsterHashMap;

typedef HM_NAMESPACE::hash_map<uint64, Player*> PlayerStorageMap;

#define RESERVE_EXPAND_SIZE 1024

#define SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT		5

/************************************************************************/
/* 一个地图实例对应一个MapMgr                                           */
/************************************************************************/
class MapMgr : public CellHandler <MapCell>, public EventableObject
{
	friend class MapCell;

public:
		
	//This will be done in regular way soon
	Mutex m_objectinsertlock;
	ObjectSet m_objectinsertpool;
	void AddObject(Object *obj);

	//////////////////////////////////////////////////////////
	// Local (mapmgr) storage of players for faster lookup
	////////////////////////////////
	CMapObjectStorage<Player> m_PlayerStorage;
	Player* GetPlayer(const uint64 guid);

	/////////////////////////////////////////////////////////
	// Local (mapmgr) storage/generation of monster
	/////////////////////////////////////////////
	CMapObjectStorage<Monster> m_MonsterStorage;
	Monster* CreateMonster(uint32 entry);
	Monster* GetMonsterByEntry(uint32 entry);
	Monster* GetMonster(const uint64 guid);
	inline const uint32 GetAddMonsterCount()
	{
		return m_nAddMonsterCount;
	}
	uint32 m_nAddMonsterCount;

	//////////////////////////////////////////////////////////
	// Lookup Wrappers
	///////////////////////////////////
	Object* GetObject(const uint64 & guid);

	MapMgr(MapSpawn *spawn, uint32 mapid, uint32 instanceid);
	~MapMgr();

	void Init();
	void Term(bool bShutDown);
	void Update(uint32 timeDiff);
	void RemoveAllObjects(bool bShutDown);
	void Destructor();

	//添加object到场景中
	void PushObject(Object* obj);
	void RemoveObject(Object* obj, bool free_guid);

	void ChangeObjectLocation( Object* obj, bool same_map_teleport = false ); // update inrange lists

	//! Mark object as updated
	void ObjectUpdated(Object* obj);							// 放到要更新数据的队列中
	void UpdateCellActivity(uint32 x, uint32 y, int radius);
	bool GetMapCellActivityState(uint32 posx, uint32 posy);

	// 地形相关的接口，地面类型和是否能走可以考虑制作，地面高度就不考虑了吧
	inline uint8 GetTerrianType(float x, float y) { return 0; }
	inline uint8 GetWalkableState(float x, float y) { return 0; }

	void AddForcedCell(MapCell * c);
	void RemoveForcedCell(MapCell * c);

	void PushToMoveProcessed(Player* plr);
	void PushToProcessed(Player* plr);				// 放到要处理更新的队列中
	void PushToCreationProcessed(Player *plr);		// 将玩家对象推放至创建数据队列

	// 移除所有玩家
	uint32 GetInstanceLastTurnBackTime(Player *plr);// 获取玩家回到副本实例的最后时间
	void TeleportPlayers();

	//获取接口
	inline bool HasPlayers() { return !m_PlayerStorage.empty(); }
	inline uint32 GetMapId() { return _mapId; }
	inline uint32 GetInstanceID() { return m_instanceID; }
	inline uint32 GetDungeonID() { return iInstanceDungeonID; }
	inline MapInfo *GetMapInfo() { return pMapInfo; }

	virtual int32 event_GetInstanceID() { return m_instanceID; }

	void LoadAllCells();

	//一看就明白
	inline size_t GetPlayerCount() { return m_PlayerStorage.size(); }
	inline size_t GetMonsterCount() { return m_MonsterStorage.size(); }

    inline bool HasMapInfoFlag( uint32 flag )
    {
        if ( pMapInfo != NULL )
        {
            return pMapInfo->HasFlag( flag );
        }
        return false;
	}

	void _PerformObjectDuties();	//各种物体的功能性update
	void _PerformUpdateMonsters(uint32 f_time);
	void _PerformUpdateMonstersForWin32(uint32 f_time, map<uint32, uint32> &update_times);
	void _PerformUpdateEventHolders(uint32 f_time);
	void _PerformUpdateFightPets(uint32 f_time);
	void _PerformUpdatePlayers(uint32 f_time);
	void _PerformUpdateGameSessions();
	void _PerformDropItemUpdate( uint32 diff );

	uint64 lastObjectUpdateMsTime;	// 怪物的update计时
	uint32 m_nNextDropItemUpdate;	// 掉落物品的update计时

	time_t InactiveMoveTime;		// 副本的不活动计时，超过这个时间没东西进去就会卸载
    uint32 iInstanceDifficulty;		// 副本难度
	uint32 iInstanceDungeonID;		// 副本id

	//怪物和物体的刷新
	void EventRespawnMonster(Monster* m, MapCell * p);
	void ForceRespawnMonsterByEntry(uint32 monsterEntry);
	void ForceRespawnMonsterByGUID(uint64 monsterGUID);
	void ForceRespawnAllMonster();

	//两个消息发送的接口
	void SendMessageToCellPlayers(Object* obj, WorldPacket * packet, uint32 cell_radius = 2);

	//实例信息的指针
	Instance * pInstance;

	// better hope to clear any references to us when calling this :P
	void InstanceShutdown()
	{
		pInstance = NULL;
	}

	static const uint32 ms_syncToCentreFreSegment[SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT];

protected:
	//! Collect and send updates to clients
	void _UpdateObjects();
	// Sync Map Object data to master server
	void _UpdateSyncDataProcess();

private:
	//! Objects that exist on map
 	uint32 _mapId;//场景的id
	bool __isCellActive(uint32 x, uint32 y);							// 返回这个格子是否是活跃的
	void __UpdateInRangeSet(Object* obj, Player* plObj, MapCell* cell);	// 更新在范围内的object集合
	void __DeactiveMonsters();				// 取消队列中怪物的活跃状态
	void __ActiveMonsters();				// 设置队列中怪物的活跃状态

private:
	/* Update System */
	Mutex m_updateMutex;				// use a user-mode mutex for extra speed
	UpdateQueue _updates;				// 需要更新数据的object会插入到此队列
	PUpdateQueue _processQueue;			// 玩家的数据会在这里集中一起发送？好像是这样的意思
	PMoveInfoQueue _moveInfoQueue;		// 有哪些人需要被通知附近有object移动
	PCreationQueue _creationQueue;		// 创建数据的队列（位于集合中的玩家会在update过程中被发送创建信息协议包）
	uint32 m_processFre;	     

	int32 m_syncToMasterFre;						// 向中央服同步数据的频率
	map<uint32, PUpdateQueue > _syncToMasterQueue;	// 需要向中央服同步信息的玩家集合map<平台id,玩家队列>

	/* Sessions */
	SessionSet MapSessions;				// 玩家的session列表，玩家进入地图后，session的update就在这个地图的update里面执行了

	/* Map Information */
	MapInfo *pMapInfo;				// 地图的简要信息
	uint32 m_instanceID;			// 此地图的实例id，普通场景有唯一的实例，像副本之类，如果要创建多个实例，区别就是这个。

	PlayerStorageMap::iterator __player_iterator;

public:
	EventableObjectHolder eventHolder;

	MonsterHashMap activeMonsters;		// 活跃怪物列表
	MonsterGuidList pendingActiveMonsterGuids;		// 等待添加活跃的怪物id列表
	MonsterGuidList pendingDeactiveMonsterGuids;	// 等待取消活跃的怪物id列表

public:
	void SendPacketToPlayers( WorldPacket * pData, uint64 ignoreGuid = 0 );

public:
	PlayerStorageMap::iterator GetPlayerStorageMapBegin()
	{
		return m_PlayerStorage.begin();
	}
	PlayerStorageMap::iterator GetPlayerStorageMapEnd()
	{
		return m_PlayerStorage.end();
	}
};

#endif
