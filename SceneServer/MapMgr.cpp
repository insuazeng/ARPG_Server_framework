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
#include "ObjectPoolMgr.h"
#include "GateCommHandler.h"
#include "MapSpawn.h"

#define MAP_MGR_UPDATE_PERIOD		100
#define MAPMGR_INACTIVE_MOVE_TIME	10
#define DROP_ITEM_UPDATE_INTERVAL	1000		// 掉落物检测间隔时间

extern bool m_stopEvent;

#ifdef _WIN64
#pragma warning(disable:4355)
#endif

const uint32 MapMgr::ms_syncToCentreFreSegment[SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT] =
{
	60, 80, 100, 120, 140					// 分别对应update频率是3,4,5,6,7秒
};

MapMgr::MapMgr(MapSpawn *spawn, uint32 mapId, uint32 instanceid) :  CellHandler<MapCell>(spawn), 
		_mapId( mapId ), eventHolder( instanceid ), m_instanceID( instanceid ), iInstanceDungeonID(0),
		iInstanceDifficulty( 0 ), InactiveMoveTime( 0 ), pInstance( NULL ), m_nAddMonsterCount(0),
		lastObjectUpdateMsTime( 0 ), m_nNextDropItemUpdate(DROP_ITEM_UPDATE_INTERVAL)
{
	pMapInfo = sSceneMapMgr.GetMapInfo( mapId );
	m_holder = &eventHolder;
	m_event_Instanceid = eventHolder.GetInstanceID();
	//sLog.outDebug("MapMgr:m_event_instanceId = %d", m_event_Instanceid);
	ASSERT(m_event_Instanceid != WORLD_INSTANCE);

	_updates.clear();
	_processQueue.clear();
	_creationQueue.clear();
	_syncToMasterQueue.clear();
	_moveInfoQueue.clear();
	MapSessions.clear();

	m_processFre = 0;
	m_syncToMasterFre = ms_syncToCentreFreSegment[RandomUInt(SCENEMAP_SYNC_TO_CENTRE_FRE_SEGMENT)];
	activeMonsters.clear();
}

MapMgr::~MapMgr()
{
	SetThreadName("thread_proc");//free the name

	sEventMgr.RemoveEvents(this);
	sEventMgr.RemoveEventHolder(m_instanceID);

	_updates.clear();
	_processQueue.clear();
	_creationQueue.clear();
	_moveInfoQueue.clear();
	_syncToMasterQueue.clear();

	MapSessions.clear();
	pMapInfo = NULL;
	m_PlayerStorage.clear();
	m_MonsterStorage.clear();
	
	activeMonsters.clear();
}

void MapMgr::Init()
{

}

void MapMgr::Term(bool bShutDown)
{
    // 应该先对角色进行处理，之后对实例内部进行处理
    // Teleport any left-over players out.
	if(!m_objectinsertpool.empty())
	{
		sLog.outError("Error:m_objectinsertpool 残留未添加的对象 %u 个，有可能导致非法MapMgr引用", uint32(m_objectinsertpool.size()));
		for (ObjectSet::iterator it = m_objectinsertpool.begin(); it != m_objectinsertpool.end(); ++it)
		{
			Object *pObj = *it;
			pObj->OnMgrTerm();
		}
	}
	// 将玩家传送出本场景
    TeleportPlayers();	

	if ( pInstance != NULL )
	{
		// check for a non-raid instance, these expire after 10 minutes.
		switch( GetMapInfo()->type )
		{
		case Map_type_normal:
			// just null out the pointer
			pInstance->m_mapMgr = NULL;
			SAFE_DELETE(pInstance);
			break;
        default:
			// 单实例特殊场景的instance实例删除
            SAFE_DELETE(pInstance);
            break;
		}
	}
	RemoveAllObjects(bShutDown);
	// delete ourselves
	Destructor();
}

// call me to break the circular reference, perform cleanup
void MapMgr::Destructor()
{
	delete this;
}

void MapMgr::Update(uint32 timeDiff)
{
	uint64 exec_start = getMSTime64();
	//first push to world new objects
	m_objectinsertlock.Acquire();//<<<<<<<<<<<<<<<<
	if ( !m_objectinsertpool.empty() )
	{
		for ( ObjectSet::iterator i = m_objectinsertpool.begin(); i != m_objectinsertpool.end(); ++i )
		{
			Object *obj = *i;
			obj->PushToWorld( this );
			if (obj->isPlayer())
			{
				Player *plr = TO_PLAYER(obj);
				// 如果玩家存在宠物的话，将玩家宠物也推放进mapmgr中
			}
		}
		m_objectinsertpool.clear();
	}
	m_objectinsertlock.Release();//>>>>>>>>>>>>>>>>

#ifdef _WIN64
	uint32 insert_excute = getMSTime64() - exec_start;
#endif

	//Now update sessions of this map + objects
	_PerformObjectDuties();

#ifdef _WIN64
	uint64 dropitem_start = getMSTime64();
#endif

	_PerformDropItemUpdate( timeDiff );

#ifdef _WIN64
	uint64 script_start = getMSTime64();
	uint32 roller_excute = script_start - dropitem_start;
#endif

#ifdef _WIN64
	uint32 script_excute = getMSTime64() - script_start;
	uint32 exec_time = getMSTime64() - exec_start;
	if ( exec_time > MAP_MGR_UPDATE_PERIOD )
	{
		//sLog.outString("地图 %s(%u)(crSize=%u,plrSize=%u,petSize=%u)update时间过长[%u]ms (in=[%u]ms,drop=[%u]ms,roller=[%u]ms,script=[%u]ms)", GetMapInfo()->name.c_str(),
		//			GetInstanceID(), GetCreatureCount(), GetPlayerCount(), GetCPetCount(), exec_time, insert_excute, dropitem_excute, roller_excute, script_excute);
	}
#endif

}

void MapMgr::RemoveAllObjects(bool bShutDown)
{
	sLog.outString("[地图实例回收]flag=%u MapMgr(%u) Instance[%u] Shutdown.[%u]Monsters,[%u]Players,[%u]Lootitems", bShutDown?uint32(1):uint32(0), 
					GetMapId(), m_instanceID, uint32(m_MonsterStorage.size()), uint32(m_PlayerStorage.size()), uint32(0));

	// Remove objects
	if ( _cells != NULL )
	{
		for ( uint32 i = 0; i < _sizeX; ++i )
		{
			if ( _cells[i] != NULL )
			{
				for ( uint32 j = 0; j < _sizeY; ++j )
				{
					if ( _cells[i][j] != NULL )
					{
						_cells[i][j]->RemoveAllObjects(bShutDown);
					}
				}
			}
		}
	}

	// 不应该还在场景内存在怪物及玩家
	if (!(m_MonsterStorage.empty() && m_PlayerStorage.empty()))
		sLog.outError("Still Remaining [%u]Creatures and [%u]Players", (uint32)m_MonsterStorage.size(), (uint32)m_PlayerStorage.size());
}

void MapMgr::PushObject(Object* obj)
{
	ASSERT(obj);
	
	Player* plObj = NULL;

	if ( obj->isPlayer() )
	{
		plObj = dynamic_cast<Player*>( obj );
		if ( plObj == NULL || plObj->GetSession() == NULL)
		{
			sLog.outError("MapMgr Could not get a valid playerobject or session from object while trying to push to world");
			return;
		}
	}
	
	obj->ClearInRangeSet();

	///////////////////////
	// Get cell coordinates
	///////////////////////

	ASSERT(obj->GetMapId() == _mapId);
	ASSERT(_cells);

	uint32 mx = obj->GetGridPositionX();
	uint32 my = obj->GetGridPositionY();
	if(	mx > _maxX || my > _maxY || mx < _minX || my < _minY )
	{
		if ( plObj != NULL )
		{
			sLog.outError( "收到非法进入坐标(gridX=%u,gridY=%u,posX=%u,posY=%u), player %s("I64FMTD"),即将断开网络连接", mx, my, (uint32)obj->GetPositionX(), (uint32)obj->GetPositionY(),
							plObj->strGetGbkNameString().c_str(), plObj->GetGUID() );
			
			// 将玩家会话添加进park的会话列表中，并且让它断开
			plObj->GetSession()->SetInstance(0);
			// Add it to the global session set (if it's not being deleted).
			if ( !plObj->GetSession()->bDeleted )
				sPark.AddGlobalSession( plObj->GetSession() );
			plObj->GetSession()->Disconnect();
			return;
		}
		else
		{
			sLog.outError("Obj(type=%u,guid="I64FMTD") 被推送到非法格子坐标(%u,%u)", uint32(obj->GetTypeID()), obj->GetGUID(), mx, my);
			mx = 100;
			my = 100;
		}
    }

    uint32 cx = GetPosX( mx );
    uint32 cy = GetPosY( my );
    if ( cx >= _sizeX || cy >= _sizeY )
    {
        obj->GetPositionV()->Init( 200.0f, 200.0f );
        cx = GetPosX( 200 );
        cy = GetPosY( 200 );
    }

	MapCell *objCell = GetCell( cx,cy );
	if ( objCell == NULL )
	{
		objCell = Create( cx,cy );
		objCell->Init( cx, cy, this );
		if ( GetMapInfo()->isInstance() )
			AddForcedCell(objCell);
	}

	uint32 endX, endY, startX, startY;

	if ( GetMapInfo()->isInstance() )
	{
		endX = _sizeX - 1;
		endY = _sizeY - 1;
		startX = 0;
		startY = 0;
	}
	else
	{
		endX = (cx < _sizeX-1) ? cx + 1 : (_sizeX-1);
		endY = (cy < _sizeY-1) ? cy + 1 : (_sizeY-1);
		startX = cx > 0 ? cx - 1 : 0;
		startY = cy > 0 ? cy - 1 : 0;
	}

	uint32 count = 0;
	// 创建自己改为在gameloaded后就创建
	/*if ( plObj != NULL )
	{
		DEBUG_LOG("MapMgr", "Creating Player "I64FMT" Data for himself.", obj->GetGUID());
		plObj->BuildCreationDataBlockForPlayer( plObj );
	}*/

	////////////////////////
	//// Build in-range data
	////////////////////////

	MapCell * cell = NULL;
	for ( uint32 posX = startX; posX <= endX; ++posX )
	{
		for ( uint32 posY = startY; posY <= endY; ++posY )
		{
			cell = GetCell( posX, posY );
			if ( cell != NULL )
			{
				__UpdateInRangeSet( obj, plObj, cell );
			}
		}
	}

	////Add to the cell's object list
	objCell->AddObject( obj );
	obj->SetMapCell( objCell );
	
	// 如果该cell不激活，那么试图尝试激活一下
	if (!objCell->IsActive() && !objCell->IsForcedActive())
		UpdateCellActivity(cx, cy, 0);

	//Add to the mapmanager's object list
	if ( plObj != NULL )
	{
		m_PlayerStorage.AddObject(plObj->GetGUID(), plObj);
		sLog.outString("场景[%u]Mgr 添加Player:%s(guid="I64FMTD",Instance=%u)", GetMapId(), plObj->strGetGbkNameString().c_str(), plObj->GetGUID(), GetInstanceID());

		uint32 pID = plObj->GetPlatformID();
		_syncToMasterQueue[pID].insert(plObj);
		
		UpdateCellActivity( cx, cy, 2 );
	}
	else
	{
		switch ( obj->GetTypeID() )
		{
		case TYPEID_MONSTER:
			{

#ifdef _WIN64
				ASSERT((obj->GetGUID()) <= MAX_MONSTER_GUID);
#endif
				Monster *mObj = TO_MONSTER(obj);
				m_MonsterStorage.AddObject(mObj->GetGUID(), mObj);
				++m_nAddMonsterCount;

				//if ( crObj->m_spawn != NULL && !crObj->m_spawn->temporal )
				//{
				//	// sLog.outString("[sql怪物]向地图 %u 添加(%u->%u)", GetMapId(), crObj->GetSQL_id(), crObj->GetEntry());
				//	_sqlids_creatures.AddObject(crObj->GetSQL_id(), crObj);
				//}
			}
			break;
		case TYPEID_PARTNER:
			break;
		case TYPEID_FIGHTPET:
			break;
		default:
			break;
		}
	}

	//// Handle activation of that object.
	if ( ( objCell->IsActive() || objCell->IsForcedActive() ) && obj->isMonster() )
    {
		obj->Activate(this);
    }

	//// Add the session to our set if it is a player.
	if ( plObj != NULL )
	{
		MapSessions.insert(plObj->GetSession());
		sLog.outString("场景[%u]Mgr 添加Session: %s(guid="I64FMTD",Instance=%u)", GetMapId(), plObj->strGetGbkNameString().c_str(), plObj->GetGUID(), GetInstanceID());
		// Change the instance ID, this will cause it to be removed from the world thread (return value 1)
		plObj->GetSession()->SetInstance(GetInstanceID());
	}
}

void MapMgr::RemoveObject(Object* obj, bool free_guid)
{
	/////////////
	// Assertions
	/////////////

	ASSERT(obj != NULL && obj->GetMapId() == _mapId);
	ASSERT(_cells);

	_updates.erase(obj);
	obj->ClearUpdateMask();
	Player* plObj = NULL;

	/////////////////////////////////////////
	//// Remove object from all needed places
	/////////////////////////////////////////
 //
	switch(obj->GetTypeID())
	{
	case TYPEID_MONSTER:
		{
#ifdef _WIN64
			ASSERT(obj->GetGUID() <= MAX_MONSTER_GUID);
#endif
			//Creature *crObj = TO_CREATURE(obj);
			//if ( crObj->m_spawn != NULL && !crObj->m_spawn->temporal )
			//{
			//	// sLog.outString("[sql怪物]向地图 %u 移除(%u->%u)", GetMapId(), crObj->GetSQL_id(), crObj->GetEntry());
			//	_sqlids_creatures.RemoveObject(crObj->GetSQL_id());
			//}

			/*if ( free_guid )
				_reusable_guids_creature.push_back( obj->GetGUID() );*/

			m_MonsterStorage.RemoveObject(obj->GetGUID());
		}
		break;
	case TYPEID_PLAYER:
		{
			// __player_iterator似乎是保存m_PlayerStorage的begin,但没什么用途 by eeeo@2011.7.11
				// 我错了，__player_iterator是用于某些特殊循环，可能在循环体中改变当前迭代器，同理有__creature_iterator等	\
					// 个人认为这种实现技巧复杂了点		by eeeo@2011.7.11
			// check iterator
			plObj = TO_PLAYER( obj );
			if( __player_iterator != m_PlayerStorage.end() && __player_iterator->second == plObj )
				++__player_iterator;
		}
		break;
	case TYPEID_PARTNER:
		break;
	default:
		break;
	}

	if ( obj->Active )
		obj->Deactivate( this );

	if ( obj->GetMapCell() == NULL )
	{
		/* set the map cell correctly */
		if ( obj->GetGridPositionX() >= _maxX || obj->GetGridPositionX() <= _minY ||
			obj->GetGridPositionY() >= _maxY || obj->GetGridPositionY() <= _minY )
		{
			// do nothing
		}
		else
		{
			obj->SetMapCell(this->GetCellByCoords(obj->GetGridPositionX(), obj->GetGridPositionY()));
		}		
	}

	if ( obj->GetMapCell() != NULL )
	{
		// Remove object from cell
		obj->GetMapCell()->RemoveObject( obj );

		// Unset object's cell
		obj->SetMapCell( NULL );
	}

	//// Clear any updates pending
	if ( plObj != NULL )
	{
		_moveInfoQueue.erase( plObj );
		_processQueue.erase( plObj );
		_creationQueue.erase( plObj );
		_syncToMasterQueue[plObj->GetPlatformID()].erase(plObj);
		plObj->ClearAllPendingUpdates();
	}

	//// Remove object from all objects 'seeing' him
	for ( InRangeSet::iterator iter = obj->GetInRangeSetBegin();
		iter != obj->GetInRangeSetEnd(); ++iter )
	{
		if ( (*iter) != NULL )
		{
			if ( (*iter)->isPlayer() )
				obj->BuildRemoveDataBlockForPlayer(TO_PLAYER(*iter));

			(*iter)->RemoveInRangeObject( obj );
		}
	}

	// Clear object's in-range set
	obj->ClearInRangeSet();

	if ( plObj != NULL )
	{
		// If it's a player and he's inside boundaries - update his nearby cells
		if ( obj->GetGridPositionX() < _maxX && obj->GetGridPositionX() >= _minX &&
			obj->GetGridPositionY() < _maxY && obj->GetGridPositionY() >= _minY )
		{
			uint32 x = GetPosX( obj->GetGridPositionX() );
			uint32 y = GetPosY( obj->GetGridPositionY() );
			UpdateCellActivity( x, y, 2 );
		}
		uint64 plrGuid = plObj->GetGUID();
		m_PlayerStorage.RemoveObject(plrGuid);
		sLog.outString("场景[%u]Mgr 移除Player %s(guid="I64FMTD",Instance=%u)", GetMapId(), plObj->strGetGbkNameString().c_str(), plrGuid, plObj->GetInstanceID());

		// Setting an instance ID here will trigger the session to be removed
		// by MapMgr::run(). :)
		plObj->GetSession()->SetInstance(0);

		// Add it to the global session set (if it's not being deleted).
		if ( !plObj->GetSession()->bDeleted )
			sPark.AddGlobalSession( plObj->GetSession() );
	}

	// 这里在玩家全部离开副本后，强制再设置副本回收的时间，提高复用效率，暂时先去掉
	/*
    static const uint32 TimeForReturn = 150;
	if ( GetMapInfo()->isDungeon() && !HasPlayers() )
	{
        // Modified by eeeo 2012.3.30
        if (InactiveMoveTime > UNIXTIME + TimeForReturn)
        {
            InactiveMoveTime = UNIXTIME + TimeForReturn;
        }
	}*/
}

void MapMgr::ChangeObjectLocation( Object* obj, bool same_map_teleport /* = false */ )
{
	if(obj->GetMapMgr() != this)
	{
		/* Something removed us. */
		return;
	}


	Player* const plObj = obj->isPlayer() ? TO_PLAYER( obj ) : NULL;
	Object* curObj = NULL;
	float fRange = pMapInfo->update_outdistSqr; // normal distance

	///////////////////////////////////////
	// Update in-range data for old objects 处理范围内的object
	///////////////////////////////////////
	if ( obj->HasInRangeObjects() ) 
	{
		InRangeSet::iterator it = obj->GetInRangeSetBegin(), it2;
		for ( ; it != obj->GetInRangeSetEnd(); )
		{
			it2 = it++;
			curObj = *it2;

			//特殊情况下的可见距离可能就不限，可以设置range为0

			//If we have a update_distance, check if we are in range. 
			// if ( fRange > 0.0f && curObj->GetDistanceSq(obj) > fRange )
			if ( curObj->CalcDistanceSQ(obj) > fRange )
			{
				// 不进行可见判断，由客户端处理，坏处是玩家有可能获取退出自己范围隐身物体，进而推测其位置 
				// 当然，前提是玩家已经破解了客户端 by eeeo@2011.7.11
				if( plObj != NULL /*&& plObj->CanSee( curObj ) */)
				{
					curObj->BuildRemoveDataBlockForPlayer(plObj);
				}

				if( curObj->isPlayer() /*&& TO_PLAYER(curObj)->CanSee( obj )*/ )
				{
					obj->BuildRemoveDataBlockForPlayer(TO_PLAYER(curObj));
				}
				curObj->RemoveInRangeObject(obj);

				if( obj->GetMapMgr() != this )
				{
					/* Something removed us. */
					return;
				}
				// 这里不能用RemoveInRangeObject移除对象,否则会破坏迭代器
				obj->GetInRangeSetObejct()->erase(it2);
				obj->OnRemoveInRangeObject(curObj);
			}
		}
	}

	///////////////////////////
	// Get new cell coordinates
	///////////////////////////
	if(obj->GetMapMgr() != this)
	{
		/* Something removed us. */
		return;
	}

	int32 objGridPosX = obj->GetGridPositionX();
	int32 objGridPosY = obj->GetGridPositionY();

	if ( objGridPosX >= _maxX || objGridPosX < _minX || objGridPosY >= _maxY || objGridPosY < _minY )
	{
		if ( plObj != NULL )
		{
			//把玩家传送到安全位置
			sLog.outError( "Player "I64FMTD" is trying to get a illegal position", plObj->GetGUID() );
			if (sInstanceMgr.GetDefaultBornMap() != 0)
			{
				plObj->SafeTeleport(sInstanceMgr.GetDefaultBornMap(), 0, sInstanceMgr.GetDefaultBornPosX(), sInstanceMgr.GetDefaultBornPosY());
			}
			else
			{
				plObj->GetSession()->Disconnect();
			}
			return;
		}
		else
		{
            obj->GetPositionV()->Init(200, 200);
		}
	}

	uint32 cellX = GetPosX(obj->GetGridPositionX());
	uint32 cellY = GetPosY(obj->GetGridPositionY());

	if ( cellX >= _sizeX || cellY >= _sizeY )
	{
		return;
	}

	MapCell *objCell = GetCell(cellX, cellY);
	MapCell * pOldCell = obj->GetMapCell();
	if ( objCell == NULL )
	{
		objCell = Create( cellX,cellY );
		objCell->Init( cellX, cellY, this );
	}

	// If object moved cell
	if ( objCell != obj->GetMapCell() )
	{
		// THIS IS A HACK!
		// Current code, if a creature on a long waypoint path moves from an active
		// cell into an inactive one, it will disable itself and will never return.
		// This is to prevent cpu leaks. I will think of a better solution very soon :P
		
		// 尝试update这块新获取的cell
		if (!objCell->IsActive() && !objCell->IsForcedActive())
			UpdateCellActivity( cellX, cellY, 0 );

		// 如果还不是激活的，那么有可能要将里面的object都设置成不激活
		if ( !objCell->IsActive() && !objCell->IsForcedActive() && !plObj && obj->Active )
        {
            bool can_deatctive = true;
            if ( obj->isMonster() )
            {
				/*Creature * creature = TO_CREATURE(obj);
				if (creature->IsWaypointMovementCreature())
				can_deatctive = false;
				else if (creature->GetAIInterface() != NULL && creature->GetAIInterface()->GetAiTarget() != NULL )
				can_deatctive = false;*/
            }
            if ( can_deatctive )
                obj->Deactivate( this );
        }

		if ( obj->GetMapCell() )
			obj->GetMapCell()->RemoveObject( obj );

		objCell->AddObject( obj );
		obj->SetMapCell( objCell );

		// if player we need to update cell activity
		// radius = 2 is used in order to update both
		// old and new cells
		if ( plObj != NULL )
		{
			// have to unlock/lock here to avoid a deadlock situation.
			UpdateCellActivity( cellX, cellY, 2 );
			if( pOldCell != NULL )
			{
				// only do the second check if theres -/+ 2 difference
				if( abs( (int)cellX - (int)pOldCell->_cell_posX ) > 2 ||
					abs( (int)cellY - (int)pOldCell->_cell_posY ) > 2 )
				{
					UpdateCellActivity( pOldCell->_cell_posX, pOldCell->_cell_posY, 2 );
				}
			}
		}
	}

	if (plObj != NULL && same_map_teleport)
	{
		uint32 pID = plObj->GetPlatformID();
		_syncToMasterQueue[pID].insert(plObj);
	}

	//////////////////////////////////////
	// Update in-range set for new objects
	//////////////////////////////////////
	uint32 endX = (cellX < _sizeX -1)? cellX + 1 : (_sizeX-1);
	uint32 endY = (cellY < _sizeY -1)? cellY + 1 : (_sizeY-1);
	uint32 startX = cellX > 0 ? cellX - 1 : 0;
	uint32 startY = cellY > 0 ? cellY - 1 : 0;
	uint32 posX = 0, posY = 0;
	MapCell *cell = 0;

	// sLog.outString("对obj %u 进行(%u,%u) -> (%u,%u)的cell检测", obj->GetGUID(), startX, startY, endX, endY);
	//DEBUG_LOG("MapMgr", "Here is cell(%u, %u), update range now", cellX, cellY);
	for ( posX = startX; posX <= endX; ++posX )
	{
		for ( posY = startY; posY <= endY; ++posY )
		{
			cell = GetCell( posX, posY );
			if ( cell != NULL )
			{
				__UpdateInRangeSet( obj, plObj, cell );
			}
			else
			{
				obj->CheckErrorOnUpdateRange(posX, posY, GetMapId());
			}
		}
	}
}

void MapMgr::__UpdateInRangeSet( Object* obj, Player* plObj, MapCell* cell )
{
	Object *curObj = NULL;
	Player *plObj2 = NULL;
	int count = 0;
	ObjectSet::iterator iter = cell->Begin();
	float fRange = pMapInfo->update_indistSqr;// normal distance

	// 在推送给玩家推送玩家信息的时候，如果被推送的玩家携带了宠物的话，则将宠物信息一起推送。
	// 是宠物的时候，就continue

	while ( iter != cell->End() )
	{
		curObj = *iter;
		++iter;
		//某些条件下，可以修正range，保证可见

		float distSq = curObj->CalcDistanceSQ(obj);
		if ( distSq <= fRange && curObj != obj )
		{
			if ( !obj->IsInRangeSet(curObj) )//不在可见集里面的加入可见集
			{
				bool addObjSet = true;
				bool addCurObjSet = true;

				// 如果curObj或者obj列表中的obj是怪物并且是最愚蠢的，那么其他creature不添加进它的inrangeset里
				/*if (obj->isMonster() && TO_CREATURE(obj)->UseStupidAI())
				{
					if (curObj->isMonster())
						addObjSet = false;
				}
				if (curObj->isMonster() && TO_CREATURE(curObj)->UseStupidAI())
				{
					if (obj->isMonster())
						addCurObjSet = false;
				}*/

				// 任一条件满足，都进行相互加入inrangeset的行为
				if (addObjSet || addCurObjSet)
				{
					obj->AddInRangeObject(curObj);
					curObj->AddInRangeObject(obj);
				}

				if ( curObj->isPlayer() )
				{
					plObj2 = TO_PLAYER(curObj);
					if ( plObj2->CanSee(obj) )	// 简化的隐形检测
					{
						obj->BuildCreationDataBlockForPlayer(plObj2);
					}
				}

				if ( plObj != NULL )
				{
					if ( plObj->CanSee(curObj) )
					{
						curObj->BuildCreationDataBlockForPlayer(plObj);
					}
				}
			}
			else//在可见集里面的应该都看的见
					// 单位隐身及反隐等级改变不走此流程 by eeeo@2011.7.11
			{

			}
		}
	}
}

void MapMgr::__DeactiveMonsters()
{
	const uint32 guidCount = pendingDeactiveMonsterGuids.size();
	uint64 monsterGuidArray[64] = { 0 };
	uint64 *pMonsterGuid = monsterGuidArray;
	if (guidCount > 64)
		pMonsterGuid = new uint64[guidCount];
	uint32 index = 0;

	for (MonsterGuidList::iterator it = pendingDeactiveMonsterGuids.begin(); it != pendingDeactiveMonsterGuids.end(); ++it)
	{
		pMonsterGuid[index++] = *it;
		activeMonsters.erase(*it);
	}

	// 脚本批量反激活
	ScriptParamArray params;
	params.AppendArrayData<uint64>(pMonsterGuid, guidCount);
	LuaScript.Call("DeactiveMonsters", &params, NULL);

	if (guidCount > 64)
	{
		SAFE_DELETE_ARRAY(pMonsterGuid);
	}
	// 清除list
	pendingDeactiveMonsterGuids.clear();
}

void MapMgr::__ActiveMonsters()
{
	Monster* ptr = NULL;
	const uint32 guidCount = pendingActiveMonsterGuids.size();
	uint64 monsterGuidArray[64] = { 0 };
	uint64 *pMonsterGuid = monsterGuidArray;
	if (guidCount > 64)
		pMonsterGuid = new uint64[guidCount];

	uint32 count = 0;
	for (MonsterGuidList::iterator it = pendingActiveMonsterGuids.begin(); it != pendingActiveMonsterGuids.end(); ++it)
	{
		ptr = GetMonster(*it);
		if (ptr != NULL && ptr->Active)
		{
			pMonsterGuid[count++] = *it;
			activeMonsters.insert(make_pair(*it, ptr));
		}
	}

	// 脚本批量激活
	if (count > 0)
	{
		ScriptParamArray params;
		params.AppendArrayData<uint64>(pMonsterGuid, count);
		LuaScript.Call("ActiveMonsters", &params, NULL);
	}

	if (guidCount > 64)
	{
		SAFE_DELETE_ARRAY(pMonsterGuid);
	}

	pendingActiveMonsterGuids.clear();
}

void MapMgr::_UpdateObjects()
{
	if ( _updates.empty() && _processQueue.empty() && _moveInfoQueue.empty() && _creationQueue.empty() )
		return;

	hash_set<Player*>::iterator itr;
	Object * pObj = NULL;
	Player * plr = NULL;
	uint32 count = 0;

	UpdateQueue::iterator iter = _updates.begin();
	while ( iter != _updates.end() )
	{
		pObj = *iter;
		++iter;
		if ( pObj == NULL )
			continue;

		if ( pObj->IsInWorld() )
		{
			// players have to receive their own updates ;)
			if ( pObj->isPlayer() )
			{
				Player *self = TO_PLAYER(pObj);
				if (pObj->BuildValuesUpdateBlockForPlayer(self))
					self->PushSingleObjectPropUpdateData();
			}

			// build the update(构造一个buffer,逐个拷贝至对应的玩家缓冲区内)
			//count = pObj->BuildValuesUpdateBlockToBuffer(m_singleObjPropUpdateBuffer, NULL);
			//if ( count > 0 )
			//{
			//	for ( itr = pObj->GetInRangePlayerSetBegin(); itr != pObj->GetInRangePlayerSetEnd(); ++itr )
			//	{
			//		plr = *itr;
			//		// Make sure that the target player can see us.
			//		if( plr != NULL && plr->CanSee( pObj ) )
			//			plr->PushSingleObjectPropUpdateData(&m_singleObjPropUpdateBuffer);
			//	}
			//	m_singleObjPropUpdateBuffer.Clear();
			//}
			
			for ( itr = pObj->GetInRangePlayerSetBegin(); itr != pObj->GetInRangePlayerSetEnd(); ++itr )
			{
				plr = *itr;
				// Make sure that the target player can see us.
				if( plr != NULL && plr->CanSee( pObj ) )
				{
					if (pObj->BuildValuesUpdateBlockForPlayer(plr))
						plr->PushSingleObjectPropUpdateData();
				}
			}
		}
		pObj->ClearUpdateMask();
	}
	_updates.clear();

	++m_processFre;
	if ( m_processFre >= 3 )	
	{
		if ( m_PlayerStorage.size() > 300 )	//发送频率减为三分之一
			m_processFre = 0;
		else								//发送频率减为一半
			m_processFre = 1;

		PUpdateQueue::iterator it;
		for ( it = _creationQueue.begin(); it != _creationQueue.end(); )
		{
			plr = *it;
			_creationQueue.erase(it++);
			if (plr->GetMapMgr() == this)
			{
				plr->ProcessObjectCreationUpdates();
			}
		}

		for ( it = _processQueue.begin(); it != _processQueue.end(); )
		{
			plr = *it;
			_processQueue.erase( it++ );
			if ( plr->GetMapMgr() == this )
			{
				plr->ProcessObjectPropUpdates();
				plr->ProcessObjectRemoveUpdates();
			}
		}
		
		for ( it = _moveInfoQueue.begin(); it != _moveInfoQueue.end(); )
		{
			plr = *it;
			_moveInfoQueue.erase( it++ );
			if ( plr->GetMapMgr() == this )
			{
				plr->ProcessObjectMovmentUpdates();
			}
		}
	}
}

void MapMgr::_UpdateSyncDataProcess()
{
	--m_syncToMasterFre;
	if (m_syncToMasterFre <= 0)
	{
		// 设置下次同步的时间
		m_syncToMasterFre = ms_syncToCentreFreSegment[0];
		if (_syncToMasterQueue.empty())
			return ;

		map<uint32, PUpdateQueue>::iterator pIt = _syncToMasterQueue.begin();
		for (; pIt != _syncToMasterQueue.end(); ++pIt)
		{
			if (pIt->second.empty())
				continue;

			// 同步本平台玩家的位置数据
			WorldPacket packet(SCENE_2_MASTER_SYNC_OBJ_POS_DATA, 256);
			packet << uint32(0);		// 标记是发往中央服的
			packet << uint16(1);
			packet << GetMapId();
			uint16 curPos = packet.size(), plrCounter = 0;
			packet << uint16(0);
			// 向中央服同步本服对象的数据（玩家位置）
			PUpdateQueue::iterator it = pIt->second.begin(), it2;
			for (; it != pIt->second.end();)
			{
				it2 = it;
				++it;
				packet << (*it2)->GetGUID();
				(*it2)->WriteCurPos(packet);
				
				/*sLog.outDebug("[位置同步]向平台%u中央服同步场景%u中玩家:%s(guid="I64FMTD")位置数据(%u,%u)", pIt->first, GetMapId(), 
								(*it2)->strGetGbkNameString().c_str(), (*it2)->GetGUID(), (uint32)(*it2)->GetPositionX(), (uint32)(*it2)->GetPositionY());*/

				pIt->second.erase(it2);
				if (++plrCounter >= 150)		// 最多同步150玩家数据
					break ;
			}
			packet.put(curPos, (const uint8*)&plrCounter, sizeof(uint16));
			// 如果是跨服的场景服务器,此处可能要分发给不同平台的网关,转至不同平台的中央服
			sGateCommHandler.TrySendPacket(pIt->first, &packet, SCENE_2_MASTER_SYNC_OBJ_POS_DATA);
			// sLog.outDebug("[位置同步]向平台 %u 的中央服同步场景%u中%u个玩家的位置数据", pIt->first, GetMapId(), (uint32)plrCounter);
		}
	}
}

void MapMgr::LoadAllCells()
{
	MapCell * cellInfo;
	CellSpawns * spawns;

	for( uint32 x = 0 ; x < _sizeX ; x ++ )
	{
		for( uint32 y = 0 ; y < _sizeY ; y ++ )
		{
			cellInfo = GetCell( x , y );
			if( !cellInfo )
			{
				cellInfo = Create( x , y );
				cellInfo->Init( x , y , this );
				DEBUG_LOG("MapMgr","Created cell [%u,%u] on map %d (instance %d)." , x , y , _mapId , m_instanceID );
				cellInfo->SetActivity( true );
				ASSERT( !cellInfo->IsLoaded() );
				spawns = m_mapSpawn->GetSpawnsList( x , y );
				if( spawns != NULL )
					cellInfo->LoadObjects( spawns );
			}
			else
			{
				// Cell exists, but is inactive
				if ( !cellInfo->IsActive() )
				{
					DEBUG_LOG("MapMgr","Activated cell [%u,%u] on map %d (instance %d).", x, y, _mapId, m_instanceID );
					cellInfo->SetActivity( true );

					if (!cellInfo->IsLoaded())
					{
						//DEBUG_LOG("MapMgr","Loading objects for Cell [%d][%d] on map %d (instance %d)...", 
						//	posX, posY, this->_mapId, m_instanceID);
						spawns = m_mapSpawn->GetSpawnsList( x , y );
						if( spawns != NULL )
							cellInfo->LoadObjects( spawns );
					}
				}
			}
		}
	}
}

void MapMgr::UpdateCellActivity(uint32 x, uint32 y, int radius)
{
	CellSpawns * sp = NULL;
	uint32 endX = (x + radius) < _sizeX ? x + radius : (_sizeX-1);
	uint32 endY = (y + radius) < _sizeY ? y + radius : (_sizeY-1);
	uint32 startX = ((int32)x - radius > 0) ? x - radius : 0;
	uint32 startY = ((int32)y - radius > 0) ? y - radius : 0;
	uint32 posX = 0, posY = 0;

	MapCell *objCell = NULL;

	//DEBUG_LOG("MapMgr","Here is Cell [%d,%d] on map %d (instance %d).", x, y, this->_mapId, m_instanceID);
	for (posX = startX; posX <= endX; posX++ )
	{
		for (posY = startY; posY <= endY; posY++ )
		{
			objCell = GetCell(posX, posY);
			if ( objCell == NULL )
			{
				if (__isCellActive(posX, posY))
				{
					objCell = Create(posX, posY);
					objCell->Init(posX, posY, this);

					// DEBUG_LOG("MapMgr","1.Cell [%d,%d] on map %d (instance %d) is now active.", posX, posY, this->_mapId, m_instanceID);
					objCell->SetActivity(true);

					ASSERT(!objCell->IsLoaded());

					// DEBUG_LOG("MapMgr","Loading objects for Cell [%d][%d] on map %d (instance %d)...",posX, posY, this->_mapId, m_instanceID);

					sp = m_mapSpawn->GetSpawnsList(posX, posY);
					if ( sp != NULL ) 
						objCell->LoadObjects(sp);
				}
			}
			else
			{
				//Cell is now active
				if (__isCellActive(posX, posY) && !objCell->IsActive())
				{
					//DEBUG_LOG("MapMgr","2.Cell [%d,%d] on map %d (instance %d) is now active.", posX, posY, this->_mapId, m_instanceID);
					objCell->SetActivity(true);

					if (!objCell->IsLoaded())
					{
						//DEBUG_LOG("MapMgr","Loading objects for Cell [%d][%d] on map %d (instance %d)...", posX, posY, this->_mapId, m_instanceID);
						sp = m_mapSpawn->GetSpawnsList(posX, posY);
						if ( sp != NULL )
							objCell->LoadObjects(sp);
					}
				}
				//Cell is no longer active
				else if (!__isCellActive(posX, posY) && !objCell->IsForcedActive() && objCell->IsActive())
				{
					//DEBUG_LOG("MapMgr","3.Cell [%d,%d] on map %d (instance %d) is now idle.",	posX, posY, this->_mapId, m_instanceID);
					objCell->SetActivity(false);
				}
			}
		}
	}
}

bool MapMgr::GetMapCellActivityState(uint32 posx, uint32 posy)
{
	uint32 cx = GetPosX( posx );
	uint32 cy = GetPosY( posy );
	if ( cx >= _sizeX || cy >= _sizeY )
		return false;

	MapCell *objCell = GetCell( cx,cy );
	return (objCell != NULL) ? objCell->IsActive() : false;
}

void MapMgr::AddForcedCell(MapCell * c)
{
	c->SetPermanentActivity(true);
	UpdateCellActivity(c->GetPositionX(), c->GetPositionY(), 1);
}

void MapMgr::RemoveForcedCell(MapCell * c)
{
	c->SetPermanentActivity(false);
	UpdateCellActivity(c->GetPositionX(), c->GetPositionY(), 1);
}

bool MapMgr::__isCellActive(uint32 x, uint32 y)
{

	uint32 endX = ((x+1) < _sizeX) ? x + 1 : (_sizeX-1);
	uint32 endY = ((y+1) < _sizeY) ? y + 1 : (_sizeY-1);
	uint32 startX = x > 0 ? x - 1 : 0;
	uint32 startY = y > 0 ? y - 1 : 0;
	uint32 posX, posY;

	MapCell *objCell;

	for (posX = startX; posX <= endX; posX++ )
	{
		for (posY = startY; posY <= endY; posY++ )
		{
			objCell = GetCell(posX, posY);

			if (objCell)
			{
				if (objCell->HasPlayers() || objCell->IsForcedActive() || objCell->HaveWaypointMovementCreatureNpc() || objCell->HaveActivityMonsters())
				{
					return true;
				}
			}
		}
	}

	return false;
}

void MapMgr::ObjectUpdated(Object* obj)
{
	// set our fields to dirty
	// stupid fucked up code in places.. i hate doing this but i've got to :<
	// - burlex
	_updates.insert(obj);
}

void MapMgr::PushToProcessed(Player* plr)
{
	_processQueue.insert(plr);
}

void MapMgr::PushToCreationProcessed(Player *plr)
{
	_creationQueue.insert(plr);
}

void MapMgr::PushToMoveProcessed(Player* plr)
{
	_moveInfoQueue.insert(plr);
}

void MapMgr::AddObject(Object* obj)
{
	m_objectinsertlock.Acquire();
	m_objectinsertpool.insert(obj);
	m_objectinsertlock.Release();
}

Object* MapMgr::GetObject(const uint64 & guid)
{
	if (guid >= PLATFORM_MIN_PLAYER_GUID)
	{
		return GetPlayer(guid);
	}
	else
	{
		return GetMonster(guid);
	}
	return NULL;
}

void MapMgr::_PerformObjectDuties()
{
	uint64 mstime = getMSTime64();
	uint32 difftime = mstime - lastObjectUpdateMsTime;

	if(difftime > 500)
		difftime = 500;

#ifdef _WIN64
	map<uint32, uint32> updateTimes;
	_PerformUpdateMonstersForWin32(difftime, updateTimes);
#else
	_PerformUpdateMonsters(difftime);
#endif

#ifdef _WIN64
	uint64 event_start = getMSTime64();
	uint32 cr_excute = event_start - mstime;
#endif

	_PerformUpdateEventHolders(difftime);

#ifdef _WIN64
	uint64 plr_start = getMSTime64();
	uint32 event_excute = plr_start - event_start;
#endif

	_PerformUpdatePlayers(difftime);

#ifdef _WIN64
	uint64 pet_start = getMSTime64();
	uint32 plr_excute = pet_start - plr_start;
#endif

	_PerformUpdateFightPets(difftime);

#ifdef _WIN64
	uint64 session_start = getMSTime64();
	uint32 pet_excute = session_start - pet_start;
#endif

	lastObjectUpdateMsTime = mstime;

	_PerformUpdateGameSessions();

#ifdef _WIN64
	uint64 sync_start = getMSTime64();
	uint32 session_excute = sync_start - session_start;
#endif

	// Finally, A9 Building/Distribution
	_UpdateObjects();
	_UpdateSyncDataProcess();

#ifdef _WIN64
	uint64 finish_time = getMSTime64();
	uint32 sync_excute = finish_time - sync_start;
	if (finish_time - mstime > 100)
	{
		sLog.outString("[场景update] cr=[%u]ms,event=[%u]ms,plr=[%u]ms,pet=[%u]ms,session=[%u]ms,sync=[%u]ms", cr_excute, 
			event_excute, plr_excute, pet_excute, session_excute, sync_excute);
		if (cr_excute > 100)
		{
			stringstream ss;
			for (map<uint32, uint32>::iterator it = updateTimes.begin(); it != updateTimes.end(); ++it)
			{
				ss << it->first << ":[" << it->second << "]ms,";
			}
			sLog.outString("怪物update细节:%s", ss.str().c_str());
		}
	}
#endif

}

void MapMgr::_PerformUpdateMonsters(uint32 f_time)
{
	// Update our objects.
	Monster* ptr = NULL;
	MonsterHashMap::iterator it = activeMonsters.begin();
	for ( ; it != activeMonsters.end(); it++)
	{
		ptr = it->second;
		ptr->Update(f_time);
	}

	if (!pendingDeactiveMonsterGuids.empty())
		__DeactiveMonsters();

	if (!pendingActiveMonsterGuids.empty())
		__ActiveMonsters();
}

void MapMgr::_PerformUpdateMonstersForWin32(uint32 f_time, map<uint32, uint32> &update_times)
{
	Monster* ptr = NULL;
	uint32 crEntry = 0;
	uint64 excute_start_time = 0;
	
	MonsterHashMap::iterator it = activeMonsters.begin();
	for ( ; it != activeMonsters.end(); it++)
	{
		excute_start_time = getMSTime64();
		ptr = it->second;
		crEntry = ptr->GetTypeID();
		ptr->Update(f_time);
		update_times[crEntry] += (getMSTime64() - excute_start_time);
	}

	if (!pendingDeactiveMonsterGuids.empty())
		__DeactiveMonsters();

	if (!pendingActiveMonsterGuids.empty())
		__ActiveMonsters();
}

void MapMgr::_PerformUpdateEventHolders(uint32 f_time)
{
	// Update any events.
	eventHolder.Update(f_time);
}

void MapMgr::_PerformUpdateFightPets(uint32 f_time)
{
	// Update Pets
	/*CPet* pet = NULL;
	__pet_iterator = m_PetStorage.begin();

	for (; __pet_iterator != m_PetStorage.end(); )
	{
	pet = __pet_iterator->second;
	++__pet_iterator; 
	pet->Update(f_time);
	}*/
}

void MapMgr::_PerformUpdatePlayers(uint32 f_time)
{
	// Update players.
	Player* ptr4 = NULL;
	__player_iterator = m_PlayerStorage.begin();
	for(; __player_iterator != m_PlayerStorage.end();)
	{
		ptr4 = __player_iterator->second;;
		++__player_iterator;
		ptr4->Update( f_time );
	}
}

void MapMgr::_PerformUpdateGameSessions()
{
	int result = 0;
	GameSession * session = NULL;
	Player *plr = NULL;
	SessionSet::iterator itr = MapSessions.begin();
	SessionSet::iterator it2;

	while ( itr != MapSessions.end() )
	{
		session = (*itr);
		plr = session->GetPlayer();
		it2 = itr;
		++itr;

		//we have teleported to another map, remove us here.
		if ( session->GetInstance() != m_instanceID )
		{
			MapSessions.erase(it2);
			sLog.outString("场景%u(Instance=%u)Mgr 移除Session(guid="I64FMTD",instance=%u)", GetMapId(), GetInstanceID(), session->GetUserGUID(), session->GetInstance());
			continue;
		}

		// Don't update players not on our map.
		// If we abort in the handler, it means we will "lose" packets, or not process this.
		// .. and that could be diasterous to our client :P
		if( plr != NULL && plr->GetMapMgr() != this )
			continue;

		result = session->Update( m_instanceID );
		if (result > 0)
		{
			const uint64 guid = session->GetUserGUID();
			if (result == 1)	// 网络连接断掉
			{	
				sLog.outString("场景%u Mgr(Instance=%u) 移除Session(guid="I64FMTD")", GetMapId(), GetInstanceID(), guid);
				sPark.DeleteSession( guid, session );
				MapSessions.erase(it2);
			}
			else
			{
				sLog.outString("场景%u Mgr(Instance=%u) 移除Session(guid="I64FMTD",ret=%d)", GetMapId(), GetInstanceID(), guid, result);
				MapSessions.erase(it2);
			}
		}
	}
}

void MapMgr::_PerformDropItemUpdate( uint32 diff )
{

}

uint32 MapMgr::GetInstanceLastTurnBackTime(Player *plr)
{
	uint32 uRet = UNIXTIME;

	return uRet;
}

void MapMgr::TeleportPlayers()
{
	Player * plr = NULL;
	__player_iterator = m_PlayerStorage.begin();
	if ( !m_stopEvent )
	{
		// Update all players on map.
		while ( __player_iterator != m_PlayerStorage.end() )
		{
			plr = __player_iterator->second;
			++__player_iterator;
			if ( plr->GetSession() )
				plr->EjectFromInstance();
		}
	}
	else
	{
		//关闭服务器时应该已经没有session了，在update的时候应该已经删除
		// Update all players on map.
		while ( __player_iterator != m_PlayerStorage.end() )
		{
			plr = __player_iterator->second;
			++__player_iterator;

			if ( plr->GetSession() )
			{
				sLog.outError( "plr %s("I64FMTD") still exist in map when mapmgr terminate", plr->strGetGbkNameString().c_str(), plr->GetGUID() );
				GameSession *session = plr->GetSession();
				session->LogoutPlayer( false );
				session->Disconnect();
			}
			else
			{
				//m_PlayerStorage.erase(__player_iterator);			
				m_PlayerStorage.RemoveObject(plr->GetGUID());
			}
		}
	}
}

void MapMgr::EventRespawnMonster(Monster* m, MapCell * p)
{
	ObjectSet::iterator itr = p->_respawnObjects.find( TO_OBJECT(m) );
	if(itr != p->_respawnObjects.end())
	{
		m->m_respawnCell = NULL;
		p->_respawnObjects.erase(itr);
		m->OnRespawn(this);
	}
	EventMap::iterator itr2 = m_events.begin();
	for(; itr2 != m_events.end();)
	{
		if(itr2->second->deleted )
		{
			itr2->second->DecRef();
			m_events.erase(itr2++);
		}
		else
		{
			++itr2;
		}
	}
}

void MapMgr::ForceRespawnMonsterByEntry(uint32 monsterEntry)
{

}

void MapMgr::ForceRespawnMonsterByGUID(uint64 monsterGUID)
{

}

void MapMgr::ForceRespawnAllMonster()
{

}

void MapMgr::SendMessageToCellPlayers(Object* obj, WorldPacket * packet, uint32 cell_radius /* = 2 */)
{
	int cellX = GetPosX(obj->GetGridPositionX());
	int cellY = GetPosY(obj->GetGridPositionY());
	uint32 endX = (((int)cellX+(int)cell_radius) < _sizeX) ? cellX + cell_radius : (_sizeX-1);
	uint32 endY = (((int)cellY+(int)cell_radius) < _sizeY) ? cellY + cell_radius : (_sizeY-1);
	uint32 startX = ((int)cellX-(int)cell_radius) > 0 ? cellX - cell_radius : 0;
	uint32 startY = ((int)cellY-(int)cell_radius) > 0 ? cellY - cell_radius : 0;

	uint32 posX = 0, posY = 0;
	MapCell *cell = NULL;
	MapCell::ObjectSet::iterator iter, iend;
	for ( posX = startX; posX <= endX; ++posX )
	{
		for ( posY = startY; posY <= endY; ++posY )
		{
			cell = GetCell( posX, posY );
			if ( cell != NULL && cell->HasPlayers() )
			{
				iter = cell->Begin();
				iend = cell->End();
				for ( ; iter != iend; ++iter )
				{
					if ( (*iter)->isPlayer() )
					{
						TO_PLAYER(*iter)->GetSession()->SendPacket( packet );
					}
				}
			}
		}
	}
}

Player* MapMgr::GetPlayer(const uint64 guid)
{
	return m_PlayerStorage.GetObject(guid);
}

Monster* MapMgr::CreateMonster(uint32 entry)
{
	Monster *m = NULL;

	m = objPoolMgr.newMonster(sInstanceMgr.GenerateMonsterGuid());

	return m;
}

Monster* MapMgr::GetMonsterByEntry(uint32 entry)
{
	HM_NAMESPACE::hash_map< uint64, Monster * >::iterator itr = m_MonsterStorage.begin();
	for (; itr != m_MonsterStorage.end(); ++itr)
	{
		Monster *m = itr->second;
		if (m != NULL && m->GetTypeID() == entry)
			return m;
	}
	return NULL;
}

Monster* MapMgr::GetMonster(const uint64 guid)
{
	return m_MonsterStorage.GetObject(guid);
}

void MapMgr::SendPacketToPlayers( WorldPacket * pData, uint64 ignoreGuid /* = 0 */ )
{
	// Update all players on map.
	Player * plr = NULL;
	PlayerStorageMap::iterator itr = m_PlayerStorage.begin();
	for ( ; itr != m_PlayerStorage.end(); ++itr )
	{
		plr = itr->second;
		if ( plr->GetSession() != NULL && plr->GetGUID() != ignoreGuid)
		{
			plr->GetSession()->SendPacket( pData );
		}
	}
}
