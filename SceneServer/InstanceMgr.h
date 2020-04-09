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

#ifndef __WORLDCREATOR_H
#define __WORLDCREATOR_H

#define MAX_INSTANCE_ID		90000000				// 通过instancemgr分配的instanceid最大为9千万，超过了从1000开始编号
#define MAX_MONSTER_GUID	(0x00000002540BE400)	// 最大的怪物id（十进制:10001000000）不能超过玩家的最小id

enum InstanceGroupSymbolType
{
	Instance_group_type_none	        = 0,		// 无
	Instance_group_type_personal        = 1,		// 单人副本
	Instance_group_type_team	        = 2,		// 组队副本
	Instance_group_type_guild	        = 3,		// 公会实例
};

enum INSTANCE_ABORT_ERROR
{
	INSTANCE_ABORT_ERROR_ERROR			= 0x00,
	INSTANCE_ABORT_FULL					= 0x01,
	INSTANCE_ABORT_NOT_FOUND			= 0x02,
	INSTANCE_ABORT_TOO_MANY				= 0x03,
	INSTANCE_ABORT_ENCOUNTER			= 0x04,
	INSTANCE_ABORT_NON_CLIENT_TYPE		= 0x05,
	INSTANCE_ABORT_NOT_IN_RAID_GROUP	= 0x07,

	INSTANCE_OK = 0x10,
};

enum OWNER_CHECK
{
	OWNER_CHECK_ERROR		= 0,
	OWNER_CHECK_EXPIRED		= 1,
	OWNER_CHECK_NOT_EXIST	= 2,
	OWNER_CHECK_NO_GROUP	= 3,
	OWNER_CHECK_DIFFICULT	= 4,
	OWNER_CHECK_MAX_LIMIT	= 5,
	OWNER_CHECK_MIN_LEVEL	= 6,
	OWNER_CHECK_WRONG_GROUP	= 7,
	OWNER_CHECK_OK			= 10,
	OWNER_CHECK_GROUP_OK	= 11,
	OWNER_CHECK_SAVED_OK	= 12,
};

class MapSpawn;
class MapMgr;
class Object;
class Player;

class Instance
{
public:
	Instance() : m_instanceId( 0 ), m_mapId( 0 ), m_mapMgr( NULL ), m_creatorGuid( 0 ), m_creatorGroup( Instance_group_type_none ),
		m_difficulty( 0 ), m_dungeonID(0), m_mapInfo( NULL )
	{
	}
	inline void InitData(uint64 creatorGUID, uint32 creatorGroup, uint32 difficulty, uint32 dungeonID)
	{
		m_creatorGuid = creatorGUID;
		m_creatorGroup = creatorGroup;
		m_difficulty = difficulty;
		m_dungeonID = dungeonID;
	}
	inline uint32 GetInstanceCreatorType()			// 创建者类型（玩家，公会，队伍，系统）
	{
		return 0;
	}
	inline uint32 GetInstanceCreatorInfo()			// 创建者id（玩家id，公会id，等…）
	{
		return 0;
	}
	
	uint32 m_instanceId;
	uint32 m_mapId;
	MapMgr *m_mapMgr;
	uint64 m_creatorGuid;
	uint32 m_creatorGroup;
	uint32 m_difficulty;
	uint32 m_dungeonID;
	MapInfo *m_mapInfo;
};

typedef HM_NAMESPACE::hash_map<uint32, Instance*> InstanceMap;
typedef HM_NAMESPACE::hash_map<uint32, MapSpawn*> MapSpawnConfigMap;
/************************************************************************/
/*世界类，所有野外地图，副本等都是在“世界”里                          */
/************************************************************************/
class InstanceMgr : public Singleton<InstanceMgr>
{
	friend class MapMgr;   //为了使MapMgr能调用InstanceMgr里的一些私有函数

public:
	InstanceMgr();	
	~InstanceMgr();

public:
	inline bool HandleSpecifiedScene(uint32 sceneID)
	{
		return m_selfServerHandleScenes.find(sceneID) == m_selfServerHandleScenes.end() ? false : true;
	}
	inline void AddHandleScene(const uint32 sceneID)
	{
		m_selfServerHandleScenes.insert(sceneID);
	}
	inline const uint32 GetDefaultBornMap()
	{
		return m_nServerDefaultBornMapID;
	}
	inline const float GetDefaultBornPosX()
	{
		return m_ptServerDefaultBornPos.m_fX;
	}
	inline const float GetDefaultBornPosY()
	{
		return m_ptServerDefaultBornPos.m_fY;
	}

public:
	void Initialize();		// 初始化
	bool CanAddPlayerToSpecifiedWorld(Player *plr, uint32 targetMap, uint32 &targetInstance, uint32 targetDungeonData);
	uint32 PreTeleport(uint32 mapid, Player* plr, uint32 instanceid);   //传送函数，玩家通过“传送”进入地图(包括副本)
	
	MapMgr* GetNormalSceneMapMgr(Object* obj);
	MapMgr* GetInstanceMapMgr(uint32 MapId, uint32 InstanceId);

	Instance* GetInstance(uint32 mapID, uint32 instanceID);
	Instance* FindGuildInstance(uint32 mapID, uint32 guildID);
	Instance* FindSystemInstance(uint32 mapID);

	uint32 GenerateInstanceID();
	uint64 GenerateMonsterGuid();

	void LoadServerSceneConfigs();		// 读取本服务器场景配置
	void Load();						// 加载所有地图(地下城副本不加载)
	
	const uint32 GetHandledServerIndexBySceneID(const uint32 sceneID);
	// Has instance expired? Can player join?
	uint8 PlayerOwnsInstance(Instance * pInstance, Player* pPlayer);
	// update all mapmgrs
	void Update(uint32 diff);
	// delete all instances
	void Shutdown();

	MapMgr* GetMapMgr(uint32 mapId);
	InstanceMap* GetInstanceMap(uint32 mapId);
	InstanceMap* CreateInstanceMap(uint32 mapId);

	uint32 GetMapPlayerCount( uint32 mapid );
	Instance* CreateUndercity( uint32 mapid, Player *plr, uint32 dungeonID, uint32 difficulty );          // 由NPC对话激活的副本，针对单一玩家或队伍
	MapMgr* CreateInstance(Instance *instance);
	Instance* newInstanceObject(uint32 mapId, MapInfo *mapInfo, uint32 instanceId, MapMgr *mgr = NULL);

private:
	void _SetServerDefaultBornConfig(uint32 mapID, float posX, float posY);
	void _CreateMap(uint32 mapid);       //创建地图
	MapMgr* _CreateInstance(Instance * in);
	MapMgr* _CreateInstanceForSingleMap(uint32 mapid, uint32 instanceid);		// only used on main maps!
	bool _DeleteInstance(Instance * in);

	uint32 m_InstanceHigh;											// 创建副本实例的唯一guid
	uint64 m_MonsterGuidHigh;										// 怪物唯一guid
	Mutex m_monsterGuidLock;

	Mutex m_mapLock;
	MapSpawnConfigMap								m_mapSpawnConfigs;	// 地图出生配置
	HM_NAMESPACE::hash_map<uint32, MapMgr*>			m_singleMaps;	// 这里是普通的世界场景，一个mapid只有一个mapmgr
	HM_NAMESPACE::hash_map<uint32, InstanceMap*>	m_instances;	// 玩家创建的副本、系统活动实例等会用到多实例

	set<uint32> m_selfServerHandleScenes;						// 本场景服管理的地图id
	map<uint32, set<uint32> > m_sceneControlConfig;				// 地图管理配置<游戏服id，set<场景id> >

	uint32 m_nServerDefaultBornMapID;		// 本场景服默认的地图id（安全的让玩家默认进入的地图id）
	FloatPoint m_ptServerDefaultBornPos;	// 默认的出生点
};

#define sInstanceMgr InstanceMgr::GetSingleton()

#endif
