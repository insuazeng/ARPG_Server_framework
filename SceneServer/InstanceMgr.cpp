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

#include "stdafx.h"
#include "convert/strconv.h"

#include "Player.h"
#include "MapSpawn.h"
#include "MapMgr.h"

InstanceMgr::InstanceMgr() : m_nServerDefaultBornMapID(0)
{
}

InstanceMgr::~InstanceMgr()
{
}

void InstanceMgr::Initialize()
{
	// 读取地图配置
	LoadServerSceneConfigs();
	// 针对怪物出生点进行地图配置
	Load();
}

void InstanceMgr::Load()
{
	m_InstanceHigh = 1000;
	m_MonsterGuidHigh = 1000;
	set<uint32> creatureSpawnMapIDs;
	// load each map we have in the database.
	Json::Value j_root;
	Json::Reader j_reader;
	int32 ret = JsonFileReader::ParseJsonFile(strMakeJsonConfigFullPath("monster_born/monster_born.json").c_str(), j_reader, j_root);
	if (ret == 0)
	{
		for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
		{
			const uint32 mapID = (*it)["map_id"].asUInt();
			creatureSpawnMapIDs.insert(mapID);
		}
	}

	for (set<uint32>::iterator it = creatureSpawnMapIDs.begin(); it != creatureSpawnMapIDs.end(); ++it)
	{
		if (sSceneMapMgr.GetMapInfo(*it) != NULL)
		{
			_CreateMap(*it);
			sLog.outString("Create monster map for id %u", *it);
		}
	}

	map<uint32, MapInfo*>::iterator itr = sSceneMapMgr.m_MapInfos.begin();
	for (; itr != sSceneMapMgr.m_MapInfos.end(); ++itr)
	{
		uint32 mapid = itr->second->map_id;
		// 创建剩余场景
		if (m_mapSpawnConfigs.find(mapid) == m_mapSpawnConfigs.end())
		{
			_CreateMap(mapid);
			sLog.outString("Create map for id %u", mapid);
		}
	}
}

const uint32 InstanceMgr::GetHandledServerIndexBySceneID(const uint32 sceneID)
{
	uint32 uRet = ILLEGAL_DATA_32;
	if (HandleSpecifiedScene(sceneID))
		uRet = sGateCommHandler.GetSelfSceneServerID();
	else
	{
		for (map<uint32, set<uint32> >::iterator it = m_sceneControlConfig.begin(); it != m_sceneControlConfig.end(); ++it)
		{
			if (it->second.find(sceneID) != it->second.end())
			{
				uRet = it->first;
				break;
			}
		}
	}
	return uRet;
}

void InstanceMgr::Update(uint32 diff)
{
	// DEBUG_LOG("InstanceMgr", "Update");
	for (hash_map<uint32, MapMgr*>::iterator it = m_singleMaps.begin(); it != m_singleMaps.end(); ++it)
	{
		if ((*it).second != NULL)
			(*it).second->Update(diff);
	}

	InstanceMap::iterator itr;
	HM_NAMESPACE::hash_map<uint32, InstanceMap*>::iterator insiter = m_instances.begin();
	for ( ; insiter != m_instances.end(); ++insiter )
	{
		for ( itr = insiter->second->begin(); itr != insiter->second->end(); )
		{
			InstanceMap::iterator itr_temp = itr++;
			MapMgr * mgr = itr_temp->second->m_mapMgr;
			if (mgr == NULL)
				continue ;
			if (mgr->InactiveMoveTime != 0 && mgr->InactiveMoveTime < UNIXTIME)
			{
				mgr->Update(diff);
				mgr->Term(false);
			}
			else
			{
				mgr->Update(diff);
			}
		}
	}
}

void InstanceMgr::Shutdown()
{
	InstanceMap::iterator itr;
	HM_NAMESPACE::hash_map<uint32, InstanceMap*>::iterator insiter = m_instances.begin();
	for(;insiter!=m_instances.end(); ++insiter)
	{
		for(itr = insiter->second->begin(); itr != insiter->second->end();)
		{
			InstanceMap::iterator itr_temp = itr++;
			if(itr_temp->second->m_mapMgr)
				itr_temp->second->m_mapMgr->Term(true);
		}
		delete insiter->second;
	}
	m_instances.clear();

	HM_NAMESPACE::hash_map<uint32, MapMgr*>::iterator smapiter = m_singleMaps.begin();
	while(smapiter != m_singleMaps.end())
	{
		MapMgr* ptr = smapiter->second;
		if ( !ptr->GetMapInfo()->isInstance() )
		{
			m_singleMaps.erase(smapiter++);
			ptr->Term(true);
			continue ;
		}
		ptr->Term(true);
		++smapiter;
	}

	MapSpawnConfigMap::iterator mapiter = m_mapSpawnConfigs.begin();
	for(; mapiter != m_mapSpawnConfigs.end(); ++mapiter)
	{
		delete mapiter->second;
	}
	m_mapSpawnConfigs.clear();
}

bool InstanceMgr::CanAddPlayerToSpecifiedWorld(Player *plr, uint32 targetMap, uint32 &targetInstance, uint32 targetDungeonData)
{
	bool bRet = true;
	// preteleport is where all the magic happens :P instance creation, etc.
	MapInfo * inf = sSceneMapMgr.GetMapInfo(targetMap);
	//is the map vaild?
	if ( inf == NULL )
		return false;

	//普通场景地图检查，只有唯一实例
	if ( inf->isNormalScene() )
	{
		if (m_singleMaps.find(targetMap) == m_singleMaps.end())
			bRet = false;
	}
	else
	{
		// 系统实例或副本实例(如果目标instance不为0的话，说明是断线重回)
		if (targetInstance != 0)
		{
			InstanceMap *instanceMap = GetInstanceMap(targetMap);
			if (instanceMap == NULL)
				bRet = false;
			else
			{
				InstanceMap::iterator it = instanceMap->find(targetInstance);
				if (it == instanceMap->end())
					bRet = false;
			}
		}
		else
		{
			// 此处有可能为玩家试图进入新进程的一个副本，所以要在此处创建这个副本或找出该副本，以便让玩家进入
			bool bError = true;

			if (bError)
			{
				sLog.outError("玩家 %s(guid="I64FMTD") 尝试断线重回实例id为0的非普通场景地图(%u)", plr->strGetGbkNameString().c_str(), plr->GetGUID(), targetMap);
				bRet = false;
			}
		}
	}
	return bRet;
}

uint32 InstanceMgr::PreTeleport(uint32 mapid, Player* plr, uint32 instanceid)
{
	// preteleport is where all the magic happens :P instance creation, etc.
	MapInfo * inf = sSceneMapMgr.GetMapInfo(mapid);

	//is the map vaild?
	if ( inf == NULL )
		return INSTANCE_ABORT_NOT_FOUND;

	//普通场景地图检查，只有唯一实例
	if ( inf->isNormalScene() )
	{
		/*plr->SetInstanceDifficulty(0);
		plr->SetInstanceDungeonID(0);*/
		return (m_singleMaps.find(mapid) != m_singleMaps.end()) ? INSTANCE_OK : INSTANCE_ABORT_NOT_FOUND;
	}

	Instance *in = NULL;
	InstanceMap * instancemap = GetInstanceMap(mapid);	// m_instances[mapid];
	if ( instancemap != NULL )
	{
		InstanceMap::iterator itr;
		if ( instanceid != 0 )
		{
			itr = instancemap->find( instanceid );
			if ( itr != instancemap->end() )
			{
				in = itr->second;
				//we have an instance,but can we enter it?
				uint8 owns =  PlayerOwnsInstance( in, plr );
				if ( owns >= OWNER_CHECK_OK )
				{
					//wakeup call for saved instances
					if ( in->m_mapMgr == NULL )
					{
						in->m_mapMgr = _CreateInstance( in );
					}
					return INSTANCE_OK;
				}
				else
					DEBUG_LOG( "InstanceMgr","Check failed %s, return code %u",plr->strGetGbkNameString().c_str(), owns );
			}
			return INSTANCE_ABORT_NOT_FOUND;
		}
		else
		{
			// search all active instances and see if we have one here.
			for ( itr = instancemap->begin(); itr != instancemap->end(); )
			{
				in = itr->second;
				++itr;

				//we have an instance,but do we own it?
				uint8 owns = PlayerOwnsInstance( in, plr );
				if ( owns >= OWNER_CHECK_OK )
				{
					//wakeup call for saved instances
					if ( !in->m_mapMgr )
						in->m_mapMgr = _CreateInstance(in);

					// found our instance, allow him in.
					plr->SetInstanceID( in->m_instanceId );
					return INSTANCE_OK;
				}
				else
				{
					DEBUG_LOG( "InstanceMgr","Check failed %s, return code %u", plr->strGetGbkNameString().c_str(), owns );
				}
			}
		}
	}
	else
	{
		if ( instanceid != 0 )
		{
			// wtf, how can we have an instance_id for a mapid which doesn't even exist?
			return INSTANCE_ABORT_NOT_FOUND;
		}
		// this mapid hasn't been added yet, so we gotta create the hashmap now.
		instancemap = CreateInstanceMap(mapid);
	}

    // 已改为上层逻辑创建副本，一般不会进入以下代码 by eeeo@2011.8.5
	if ( !inf->isPersonalInstance() )	// 只有玩家副本能创建后进入
	{
		return INSTANCE_ABORT_NOT_FOUND;
	}

	uint32 instanceGroupType = Instance_group_type_personal;
	/*switch (inf->type)
	{
	case Map_type_dungeon_team:
		instanceGroupType = Instance_group_type_team * INSTANCE_RECORD_FACTOR_FOR_CREATOR + plr->GetCurTeamID();
		break;
	case Map_type_guild_instance:
		instanceGroupType =	Instance_group_type_guild * INSTANCE_RECORD_FACTOR_FOR_CREATOR + plr->GetGuildID();
		break;
	default:
		break;
	}*/
    sLog.outError( "Warning:Create instance(%u) for player("I64FMTD") passively?", mapid, plr->GetGUID() );

	// if we're here, it means we need to create a new instance.
	in = newInstanceObject(mapid, inf, GenerateInstanceID());
	// in->InitData(plr->GetGUID(), instanceGroupType, plr->GetInstanceDifficulty(), plr->GetInstanceDungeonID());
	in->InitData(plr->GetGUID(), instanceGroupType, 0, 0);
	plr->SetInstanceID( in->m_instanceId );

	DEBUG_LOG("InstanceMgr", "Prepared new instance %u for player %u and group (type=%u,content=%u) on map %u. (%u)",
				in->m_instanceId, in->m_creatorGuid, in->GetInstanceCreatorType(), in->GetInstanceCreatorInfo(), in->m_mapId, in->m_instanceId);

	// apply it in the instance map
	instancemap->insert( make_pair( in->m_instanceId, in ) );
	// create the actual instance (if we don't GetInstance() won't be able to access it).
	in->m_mapMgr = _CreateInstance(in);

	// instance created ok, i guess? return the ok for him to transport.
	return INSTANCE_OK;
}

MapMgr* InstanceMgr::GetMapMgr(uint32 mapId)
{
	HM_NAMESPACE::hash_map< uint32, MapMgr * >::iterator itr = m_singleMaps.find( mapId );
	if ( itr != m_singleMaps.end() )
	{
		return itr->second;
	}
	return NULL;
}

InstanceMap* InstanceMgr::GetInstanceMap(uint32 mapId)
{
	InstanceMap *pRet = NULL;
	hash_map<uint32, InstanceMap*>::iterator it = m_instances.find(mapId);
	if (it != m_instances.end())
		pRet = it->second;
	return pRet;
}

InstanceMap* InstanceMgr::CreateInstanceMap(uint32 mapId)
{
	InstanceMap *ret=  new InstanceMap();
	m_instances.insert(make_pair(mapId, ret));
	return ret;
}

// getinstance函数，获取obj所在地图的mapmgr实例
MapMgr* InstanceMgr::GetNormalSceneMapMgr(Object* obj)
{
	MapInfo * inf = NULL;
	uint32 objMapID = obj->GetMapId();
	inf = sSceneMapMgr.GetMapInfo(objMapID);

	// we can *never* teleport to maps without a mapinfo.
	if( inf == NULL )
		return NULL;

	// 如果不是副本，则试图返回单体地图
	//if (!(isInstance() && inf->isSystemInstance()))
	//	return (m_singleMaps.find(objMapID) != m_singleMaps.end()) ? m_singleMaps[objMapID] : NULL;
	if (inf->isNormalScene())
		return (m_singleMaps.find(objMapID) != m_singleMaps.end()) ? m_singleMaps[objMapID] : NULL;

	return NULL;
}

MapMgr* InstanceMgr::GetInstanceMapMgr(uint32 MapId, uint32 InstanceId)
{
	//Instance * in;
	InstanceMap::iterator itr;
	InstanceMap * instancemap;
	MapInfo * inf = sSceneMapMgr.GetMapInfo(MapId);

	// we can *never* teleport to maps without a mapinfo.
	if( inf == NULL )
		return NULL;

	// single-instance maps never go into the instance set.
	if ( inf->isNormalScene() )
		return (m_singleMaps.find(MapId) != m_singleMaps.end()) ? m_singleMaps[MapId] : NULL; 

	//处理副本的搜索
	//m_mapLock.Acquire();
	instancemap = GetInstanceMap(MapId);	// m_instances[MapId];
	if ( instancemap != NULL )
	{
		// check our saved instance id. see if its valid, and if we can join before trying to find one.
		itr = instancemap->find(InstanceId);
		if ( itr != instancemap->end() )
		{
			if ( itr->second->m_mapMgr != NULL )
			{
				//m_mapLock.Release();
				return itr->second->m_mapMgr;
			}
		}
	}
	return NULL;
}

Instance* InstanceMgr::GetInstance(uint32 mapID, uint32 instanceID)
{
	Instance *pRet = NULL;
	InstanceMap *instanceMap = GetInstanceMap(mapID);
	if (instanceMap != NULL)
	{
		if (instanceMap->find(instanceID) != instanceMap->end())
			pRet = (*instanceMap)[instanceID];
	}
	return pRet;
}

Instance* InstanceMgr::FindGuildInstance(uint32 mapID, uint32 guildID)
{
	Instance *pRet = NULL;
	InstanceMap *inMap = GetInstanceMap(mapID);
	if (inMap != NULL)
	{
		Instance* pTemp = NULL;
		for (InstanceMap::iterator it = inMap->begin(); it != inMap->end(); ++it)
		{
			pTemp = it->second;
			uint32 instanceCreatorType = pTemp->GetInstanceCreatorType();
			uint32 instanceCreatorInfo = pTemp->GetInstanceCreatorInfo();
#ifdef _WIN64
			ASSERT(instanceCreatorType == Instance_group_type_guild);
#endif
			if((instanceCreatorType == Instance_group_type_guild) && (instanceCreatorInfo == guildID))
			{
				pRet = pTemp;
				break;
			}
		}
	}
	return pRet;
}

Instance* InstanceMgr::FindSystemInstance(uint32 mapID)
{
	Instance *pRet = NULL;
	switch (mapID)
	{
	default:
		break;
	}
	return pRet;
}

MapMgr* InstanceMgr::_CreateInstanceForSingleMap(uint32 mapid, uint32 instanceid)
 {
	MapInfo *inf = NULL;
	inf = sSceneMapMgr.GetMapInfo(mapid);
	ASSERT( inf != NULL && !inf->isInstance() && !inf->isSystemInstance() );
	
	CUTF82C *utf_8_name = new CUTF82C(inf->name.c_str());
	cLog.Notice("InstanceMgr", "Creating continent Map %s(id=%u)", utf_8_name->c_str(), inf->map_id);
	SAFE_DELETE(utf_8_name);

#ifdef _WIN64
	ASSERT(m_mapSpawnConfigs.find(mapid) != m_mapSpawnConfigs.end());
#endif // _WIN64

	MapMgr* ret = NULL;
	ret = new MapMgr(m_mapSpawnConfigs[mapid], mapid, instanceid);
	ret->Init();

	// 只有非多实例非副本场景才加入singlemap
	m_singleMaps[mapid] = ret;

	ret->pInstance = newInstanceObject(mapid, inf, instanceid, ret);
	ret->iInstanceDifficulty = 0;
	ret->iInstanceDungeonID = 0;
	ret->InactiveMoveTime = 0;
	ret->pInstance->InitData(0, Instance_group_type_none, 0, 0);

	return ret;
}

MapMgr* InstanceMgr::_CreateInstance(Instance * in)
{
	ASSERT(in->m_mapMgr==NULL);

	cLog.Notice("InstanceMgr", "Creating instance %u (%u)", in->m_instanceId, in->m_mapId );

	//// we don't have to check for world map info here, since the instance wouldn't have been saved if it didn't have any.
	in->m_mapMgr = (new MapMgr(m_mapSpawnConfigs[in->m_mapId], in->m_mapId, in->m_instanceId));
	in->m_mapMgr->Init();
	in->m_mapMgr->pInstance = in;
	in->m_mapMgr->iInstanceDifficulty = in->m_difficulty;
	in->m_mapMgr->iInstanceDungeonID = in->m_dungeonID;
	in->m_mapMgr->InactiveMoveTime = 120 + UNIXTIME;			// 空的实例给120秒时间让玩家loading进入
	return in->m_mapMgr;
}

void InstanceMgr::_SetServerDefaultBornConfig(uint32 mapID, float posX, float posY)
{
	m_nServerDefaultBornMapID = mapID;
	m_ptServerDefaultBornPos.m_fX = posX;
	m_ptServerDefaultBornPos.m_fY = posY;
	sLog.outString("场景服 %u 默认出生信息设置为(map=%u, posx=%.1f posy=%.1f)", sPark.GetServerID(), mapID, posX, posY);
}

void InstanceMgr::_CreateMap(uint32 mapid)
{
	MapInfo *inf = sSceneMapMgr.GetMapInfo(mapid);

	m_mapSpawnConfigs[mapid] = new MapSpawn(mapid, inf);
	// 此处只建立系统场景有关的mapmgr(玩家实例与系统实例在别处创建)
	if (inf->isNormalScene())
	{
		MapMgr *mgr = _CreateInstanceForSingleMap(mapid, GenerateInstanceID());
		ASSERT(mgr != NULL);
	}
}

uint32 InstanceMgr::GenerateInstanceID()
{
	uint32 iid;
	m_mapLock.Acquire();
	iid = m_InstanceHigh++;
	if (iid > MAX_INSTANCE_ID)
	{
		m_InstanceHigh = 1000;
		iid = m_InstanceHigh++;
	}
	m_mapLock.Release();
	return iid;
}

uint64 InstanceMgr::GenerateMonsterGuid()
{
	uint64 iid = 0;
	m_monsterGuidLock.Acquire();
	iid = m_MonsterGuidHigh++;
	if (iid > MAX_MONSTER_GUID)
	{
		m_MonsterGuidHigh = 1000;
		iid = m_MonsterGuidHigh++;
	}
	m_monsterGuidLock.Release();
	return iid;
}

void InstanceMgr::LoadServerSceneConfigs()
{
	m_sceneControlConfig.clear();
	m_selfServerHandleScenes.clear();

	// 获取本平台id
	uint32 selfSceneServerID = sGateCommHandler.GetSelfSceneServerID();
	Json::Value j_root;
	Json::Reader j_reader;
	int32 ret = JsonFileReader::ParseJsonFile(strMakeJsonConfigFullPath("map_control_config.json").c_str(), j_reader, j_root);

#ifdef _WIN64
	ASSERT(ret == 0 && "读取map_control_config信息出错");
#else
	if (ret != 0)
	{
		sLog.outError("读取map_control_config表数据出错");
		return ;
	}
#endif // _WIN64

	for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
	{
		uint32 sceneServerID = (*it)["scene_server_id"].asUInt();
		ASSERT((*it)["map_ids"].isArray());
		Json::Value mapIDs = (*it)["map_ids"];
		for (int i = 0; i < mapIDs.size(); i++)
		{
			if (!mapIDs[i].isUInt())
			{
				sLog.outError("配置的场景id(%s)不正确", mapIDs[i].asString().c_str());
				continue;
			}
			uint32 sceneID = mapIDs[i].asUInt();
			if (m_sceneControlConfig.find(sceneServerID) == m_sceneControlConfig.end())
				m_sceneControlConfig.insert(make_pair(sceneServerID, set<uint32>()));
			m_sceneControlConfig[sceneServerID].insert(sceneID);

			// 如果是本场景服管理的还要添加入另一个列表
			if (sceneServerID == selfSceneServerID)
				AddHandleScene(sceneID);
		}
	}

	// 每个场景服都默认添加新手村作为自己管理的场景服
	AddHandleScene(Scene_map_test1001);
	
	// 设置1001为默认地图和坐标(mapInfo肯定是可以拿到的)
	MapInfo *mapInfo = sSceneMapMgr.GetMapInfo(Scene_map_test1001);
	_SetServerDefaultBornConfig(Scene_map_test1001, mapInfo->born_pos.m_fX, mapInfo->born_pos.m_fY);

	sLog.outString("本场景服(id %u)共管理 %u 个场景, 默认场景及坐标%u(%.1f,%.1f)", selfSceneServerID, (uint32)m_selfServerHandleScenes.size(), 
					GetDefaultBornMap(), GetDefaultBornPosX(), GetDefaultBornPosY());
}

bool InstanceMgr::_DeleteInstance(Instance * in)
{
	m_mapLock.Acquire();
	InstanceMap * instancemap;
	InstanceMap::iterator itr;

	if(in->m_mapMgr != NULL)
	{
		in->m_mapMgr->InstanceShutdown();
	}

	// remove the instance from the large map.
	instancemap = GetInstanceMap(in->m_mapId);
	if(instancemap)
	{
		itr = instancemap->find(in->m_instanceId);
		if(itr != instancemap->end())
			instancemap->erase(itr);
	}

	delete in;
	m_mapLock.Release();
	return true;
}

uint32 InstanceMgr::GetMapPlayerCount( uint32 mapid )
{
	if ( m_singleMaps.find( mapid ) == m_singleMaps.end() )
	{
		return ILLEGAL_DATA_32;
	}

	return m_singleMaps[mapid]->GetPlayerCount();
}

uint8 InstanceMgr::PlayerOwnsInstance(Instance * pInstance, Player* pPlayer)
{
	////Valid map?
	//if( !pInstance->m_mapInfo )
	//	return OWNER_CHECK_NOT_EXIST;

	//// Matching the requested mode?
	///*if( pInstance->m_difficulty != pPlayer->GetInstanceDifficulty() )
	//	return OWNER_CHECK_DIFFICULT;*/

	////Reached player limit?
	//if( pInstance->m_mapMgr != NULL && pInstance->m_mapInfo->PlayerLimited( pInstance->m_mapMgr->GetPlayerCount() ) )
	//	return OWNER_CHECK_MAX_LIMIT;

	////Meet level requirements?
	//if( pInstance->m_mapMgr && pPlayer->GetLevel() < pInstance->m_mapInfo->minlevel )
	//	return OWNER_CHECK_MIN_LEVEL;

	//// 副本mgr被移除前15秒内都无法进入该副本
	//if (pInstance->m_mapMgr->InactiveMoveTime != 0)
	//{
	//	if ((UNIXTIME + 15) >= pInstance->m_mapMgr->InactiveMoveTime)
	//		return OWNER_CHECK_NOT_EXIST;
	//}

	//uint32 creatorType = pInstance->GetInstanceCreatorType();
	//switch(creatorType)
	//{
	//case Instance_group_type_none:
	//	break;
	//case Instance_group_type_personal:
 //       {
	//	    if ( pInstance->m_creatorGuid != 0 && pInstance->m_creatorGuid != pPlayer->GetGUID() )
	//		    return OWNER_CHECK_WRONG_GROUP;
 //       }
	//	break;
	//case Instance_group_type_team:
	//	break;
	//case Instance_group_type_guild:
	//	break;
	//default:
	//	break;
	//}

	//nothing left to check, should be OK then
	return OWNER_CHECK_OK;
}

Instance* InstanceMgr::CreateUndercity( uint32 mapid, Player * plr, uint32 dungeonID, uint32 difficulty )
{
	MapInfo * mapinfo = sSceneMapMgr.GetMapInfo( mapid );
	if ( mapinfo == NULL || mapinfo->isNormalScene() )
		return NULL;

	InstanceMap * instancemap = GetInstanceMap(mapid);	// m_instances[mapid];
	if ( instancemap == NULL )
		instancemap = CreateInstanceMap(mapid);

	Instance * instance = newInstanceObject(mapid, mapinfo, GenerateInstanceID());
	uint32 instanceGroupType = Instance_group_type_none;
	switch (mapinfo->type)
	{
	default:
		break;
	}
	instance->InitData(plr->GetGUID(), instanceGroupType, difficulty, dungeonID);
	DEBUG_LOG("InstanceMgr", "Prepared new instance %u for player "I64FMTD" and group (type=%u,content=%u) on map %u. (%u)", instance->m_instanceId, instance->m_creatorGuid, 
				instance->GetInstanceCreatorType(), instance->GetInstanceCreatorInfo(), instance->m_mapId, instance->m_instanceId);
	
	// apply it in the instance map
	instancemap->insert( make_pair( instance->m_instanceId, instance ) );

	// create the actual instance (if we don't GetInstance() won't be able to access it).
	instance->m_mapMgr = _CreateInstance(instance);
	
	// instance created ok, i guess? return the ok for him to transport.
	return instance;
}

MapMgr* InstanceMgr::CreateInstance(Instance *instance)
{
	return _CreateInstance(instance);
}

Instance* InstanceMgr::newInstanceObject(uint32 mapId, MapInfo *mapInfo, uint32 instanceId, MapMgr *mgr /* = NULL */)
{
	Instance *pRet = new Instance();
	pRet->m_mapId = mapId;
	pRet->m_mapInfo = mapInfo;
	pRet->m_instanceId = instanceId;
	pRet->m_mapMgr = mgr;
	return pRet;
}
