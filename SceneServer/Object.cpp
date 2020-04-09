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

#include "stdafx.h"
#include "GameSession.h"
#include "SkillHelp.h"

Object::Object() : m_mapId(0), m_objectUpdated(false), Active(false),
					m_mapMgr(NULL), m_mapCell(NULL), m_instanceId(0), m_direction(RandomUInt(255)), m_bMoving(false), m_nMovingUpdateTimer(0), m_nStartMovingTime(0)
{
	m_uint64Values = NULL;
	m_valuesCount = 0;
	m_objectsInRange.clear();
	m_inRangePlayers.clear();
	m_position.Init(1.0f,1.0f);
	m_moveDestPosition.Init();
	m_moveDirection.Init();
}

Object::~Object( )
{
	// for linux
	if (IsInWorld())
	{
		RemoveFromWorld(false);
	}
	m_instanceId = -1;

	sEventMgr.RemoveEvents(this);
}

bool Object::isInRange(Object* target, float range)
{
	float dist = CalcDistance( target );
	return( dist <= range );
}

float Object::StartMoving(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 moveMode)
{
	static const char* szDirection16Name[GDirection16_Count] = 
	{
		"上", "右上偏上", "右上", "右上偏右",
		"右", "右下偏右", "右下", "右下偏下",
		"下", "左下偏下", "左下", "左下偏左",
		"左", "左上偏左", "左上", "左上偏上"
	};

	// 开始移动，将移动标记设置成true，并且推送至附近的player中
	m_bMoving = true;

	m_position.Init(srcPosX, srcPosY);
	m_moveDestPosition.Init(destPosX, destPosY);
	m_moveDirection = m_moveDestPosition - m_position;
	m_moveDirection.Norm();

	BoardCastMovingStatus(srcPosX, srcPosY, destPosX, destPosY, moveMode);
	
	float costTime = CalcDistance(destPosX, destPosY) / GetMoveSpeed();
	m_nMovingUpdateTimer = 0;
	m_nStartMovingTime = getMSTime64();
	// 重新计算方向
	float dir = SkillHelp::CalcuDirection(m_position.m_fX, m_position.m_fY, m_moveDestPosition.m_fX, m_moveDestPosition.m_fY);
	SetDirection(dir);
	uint32 dir16 = SkillHelp::CalcuDireciton16(dir);

	//if (isPlayer())
	//	sLog.outDebug("玩家当前方向: %.2f -> %s", dir, szDirection16Name[dir16]);

	sLog.outString("[开始移动]对象"I64FMTD" 开始移动,当前坐标(%.1f,%.1f),目标坐标(%.1f,%.1f),预期移动时间%.2f秒", GetGUID(), GetPositionX(), GetPositionY(), 
					m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, costTime);

	return costTime;
}

void Object::StopMoving(bool systemStop, const FloatPoint &curPos)
{
	/*if (!m_bMoving)
		return ;*/

	if (systemStop)
	{
		sLog.outString("[停止移动]sys 对象"I64FMTD",当前坐标(%.1f,%.1f),目标坐标(%.1f,%.1f),耗时%u ms", GetGUID(), GetPositionX(), GetPositionY(), 
						m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, getMSTime64() - m_nStartMovingTime);
	}
	else
	{
		sLog.outString("[停止移动]user 对象"I64FMTD",当前坐标(%.1f,%.1f),目标坐标(%.1f,%.1f),耗时%u ms", GetGUID(), GetPositionX(), GetPositionY(),
						m_moveDestPosition.m_fX, m_moveDestPosition.m_fY, getMSTime64() - m_nStartMovingTime);
	}

	float curDist = CalcDistance(curPos.m_fX, curPos.m_fY);

	if (curDist > 1.0f)
	{
		sLog.outError("对象 "I64FMTD" 客户端坐标(%.1f, %.1f)与服务端坐标(%.1f, %.1f)有差异(dist=%.1f)", GetGUID(), curPos.m_fX, curPos.m_fY, 
						GetPositionX(), GetPositionY(), curDist);
	}
	
	// 设置最新坐标
	m_position = curPos;

	// 清理移动数据
	m_bMoving = false;
	m_moveDestPosition.Init();
	m_moveDirection.Init();
	m_nStartMovingTime = 0;

	BoardCastMovingStatus(m_position.m_fX, m_position.m_fY, m_position.m_fX, m_position.m_fY, OBJ_MOVE_TYPE_STOP_MOVING);

	// 调用lua的停止移动函数
	ScriptParamArray params;
	params << GetGUID() << m_position.m_fX << m_position.m_fY;
	LuaScript.Call("OnMovingStopped", &params, NULL);
}

void Object::BoardCastMovingStatus(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 mode)
{
	if (!IsInWorld())
		return ;

	uint64 guid = GetGUID();
	const uint32 mapID = GetMapId();
	float srcHeight = sSceneMapMgr.GetPositionHeight(mapID, srcPosX, srcPosY);
	float destHeight = sSceneMapMgr.GetPositionHeight(mapID, destPosX, destPosY);

	// 推送给自己
	if (isPlayer())
	{
		TO_PLAYER(this)->PushMovementData(guid, srcPosX, srcPosY, srcHeight, destPosX, destPosY, destHeight, mode);
	}
	
	if (!HasInRangePlayers())
		return ;

	Player * plr = NULL;
	uint32 sendcount = 0;
	
	hash_set< Player * > * in_range_players = GetInRangePlayerSet();
	for ( hash_set< Player * >::iterator itr = in_range_players->begin(); itr != in_range_players->end() && sendcount < 150; ++itr )
	{
		plr = *itr;
		ASSERT( plr->GetSession() != NULL );
		if ( plr->CanSee( this ) )
		{
			++sendcount;
			plr->PushMovementData( guid, srcPosX, srcPosY, srcHeight, destPosX, destPosY, destHeight, mode );
		}
	}
}

void Object::UpdateMoving(uint32 timeDiff)
{
	if (!m_bMoving)
		return ;

	if (m_nMovingUpdateTimer > timeDiff)
	{
		m_nMovingUpdateTimer -= timeDiff;
		return ;
	}
	
	// 获取本次完整的update时间(不一定准确，获取可以用lastMovetimer这样的变量来记录)
	uint32 updateDelta = timeDiff - m_nMovingUpdateTimer + OBJ_MOVE_UPDATE_TIMER;
	m_nMovingUpdateTimer = OBJ_MOVE_UPDATE_TIMER;

	float disWithDest = CalcDistance(m_moveDestPosition);

	// 根据update的时间计算出速度
	float dist = updateDelta * GetMoveSpeed() / 1000.0f;
	float newPosX = GetPositionX() + m_moveDirection.m_fX * dist;
	float newPosY = GetPositionY() + m_moveDirection.m_fY * dist;

	// 如果格子本身不可走,那么对玩家对象来说,就要停止移动了
	if (!sSceneMapMgr.IsCurrentPositionMovable(m_mapId, newPosX, newPosY))
	{
		if (isPlayer())
		{
			StopMoving(true, m_position);
			return ;
		}
	}

	float disWithNextPos = CalcDistance(newPosX, newPosY);
	if (disWithDest <= disWithNextPos)
	{
		// 已经到达目的地
		SetPosition(m_moveDestPosition.m_fX, m_moveDestPosition.m_fY);
		StopMoving(true, m_moveDestPosition);
	}
	else
	{
		// 继续移动
		SetPosition(newPosX, newPosY);
		// sLog.outDebug("[移动中]对象("I64FMTD")移动中,当前坐标位置[%.2f,%.2f]", GetGUID(), newPosX, newPosY);
		m_moveDirection = m_moveDestPosition - m_position;
		m_moveDirection.Norm();
	}
}

float Object::CalcDistance(Object* Ob)
{
	return CalcDistance(GetPositionX(), GetPositionY(), Ob->GetPositionX(), Ob->GetPositionY());
}

float Object::CalcDistance(float ObX, float ObY)
{
	return CalcDistance(GetPositionX(), GetPositionY(), ObX, ObY);
}

float Object::CalcDistance(Object* Oa, Object* Ob)
{
	return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), Ob->GetPositionX(), Ob->GetPositionY());
}

float Object::CalcDistance(Object* Oa, float ObX, float ObY)
{
	return CalcDistance(Oa->GetPositionX(), Oa->GetPositionY(), ObX, ObY);
}

float Object::CalcDistance(float OaX, float OaY, float ObX, float ObY)
{
	float xdest = OaX - ObX;
	float ydest = OaY - ObY;
	return sqrtf( ydest*ydest + xdest*xdest );
}

float Object::CalcDistance(FloatPoint &pos)
{
	return pos.Dist(m_position);
}

float Object::CalcDistanceSQ(Object *obj)
{
	if(obj->GetMapId() != m_mapId) 
		return 250000.0f;				// enough for out of range
	return m_position.DistSqr(obj->GetPosition());
}

float Object::CalcDistanceWithModelSize(Object *obj)
{
	float modelDist = GetModelRadius() + obj->GetModelRadius();
	float dist = CalcDistance(obj);
	if(dist < modelDist)
		return 0.0f;

	return dist - modelDist;
}

float Object::CalcDistanceWithModelSize(float x, float y)
{
	float dist = CalcDistance(x, y);
	if(dist < GetModelRadius())
		return 0.0f;

	return dist - GetModelRadius();
}

void Object::SetPosition( float newX, float newY)
{
	bool updateMap = false;

	FloatPoint newPos(newX, newY);
	if(m_lastMapUpdatePosition.Dist(newPos) > 1.5f)	// 超过1.5米的距离，update范围
		updateMap = true;

	m_position.Init(newX, newY);

#ifdef _WIN64
	ASSERT(m_position.m_fX >= 0.0f && m_position.m_fY >= 0.0f && m_position.m_fX < mapLimitWidth && m_position.m_fY < mapLimitLength);
#else
	if (m_position.m_fX < 0.0f)
		m_position.m_fX = 1.0f;
	if (m_position.m_fX >= mapLimitWidth)
		m_position.m_fX = mapLimitWidth - 1;
	if (m_position.m_fY < 0.0f)
		m_position.m_fY = 1.0f;
	if (m_position.m_fY >= mapLimitLength)
		m_position.m_fY = mapLimitLength - 1;
#endif

	if (IsInWorld() && updateMap)
	{
		m_lastMapUpdatePosition.Init(newX, newY);
		m_mapMgr->ChangeObjectLocation( this, true );
		
		//sLog.outDebug("Obj "I64FMTD" 移动中 当前坐标(%u, %u), 目标坐标(%u, %u)", GetGUID(), (uint32)GetPositionX(), (uint32)GetPositionY(), 
		//			uint32(m_moveDestPosition.m_fX), uint32(m_moveDestPosition.m_fY));
	}

	//同步宠物坐标
	if(isPlayer())
	{
		//CPet *pet = TO_PLAYER(this)->GetCurPet(); 
		//if(pet != NULL && !pet->IsServerControl())
		//{
		//	TO_PLAYER(this)->GetCurPet()->SetPosition(newX, newY);
		//}
	}
}

void Object::SetRespawnLocation(const FloatPoint &spawnPos, float spawnDir)
{
	m_spawnLocation = spawnPos;
	m_spawndir = spawnDir;
}

void Object::ModifyPosition(float modX, float modY)
{
	m_position.m_fX += modX;
	m_position.m_fY += modY;
	bool updateMap = false;

	FloatPoint newPos(m_position);
	if(m_lastMapUpdatePosition.DistSqr(newPos) > 4.0f)		/* 2.0f */
		updateMap = true;

	if (IsInWorld() && updateMap)
	{
		m_lastMapUpdatePosition = newPos;
		m_mapMgr->ChangeObjectLocation( this , false );
	}
}

bool Object::CheckPosition(uint16 x, uint16 y)
{
	return ((uint16)m_position.m_fX==x) && ((uint16)m_position.m_fY==y);
}

void Object::_Create( uint32 mapid, float x, float y, float dir )
{
	m_mapId = mapid;
	m_position.Init(x, y);
	m_spawnLocation.Init(x, y);
	m_lastMapUpdatePosition.Init(x,y);
	m_spawndir = dir;
	m_direction = dir;
}

// 把本对象的创建数据写入目标玩家的缓冲区中
void Object::BuildCreationDataBlockForPlayer( Player* target )
{
	pbc_wmessage *creationBuffer = target->GetObjCreationBuffer();
	pbc_wmessage *data = pbc_wmessage_message(creationBuffer, "createObjectList");
	if (data == NULL)
		return ;

	BuildCreationDataBlockToBuffer(data, target);
	// 将target放入地图的待推送队列
	target->PushCreationData();
}

void Object::BuildCreationDataBlockToBuffer( pbc_wmessage *data, Player *target )
{
	sPbcRegister.WriteUInt64Value(data, "objectGuid", GetGUID());
	const uint32 objType = GetTypeID();
	pbc_wmessage_integer(data, "objectType", objType, 0);

	switch (objType)
	{
	case TYPEID_PLAYER:
		{
			Player *plr = TO_PLAYER(this);
			pbc_wmessage *plrData = pbc_wmessage_message(data, "plrData");
			if (plrData != NULL)
			{
				pbc_wmessage_string(plrData, "plrName", plr->strGetUtf8NameString().c_str(), 0);
			}
		}
		break;
	case TYPEID_MONSTER:
		{
			Monster *m = TO_MONSTER(this);
			pbc_wmessage *monsterData = pbc_wmessage_message(data, "monsterData");
			if (monsterData != NULL)
			{
				char szName[64] = { 0 };
				snprintf(szName, 64, "id:"I64FMTD"", m->GetGUID());
				// monsterData->set_monstername(m->GetProto()->m_MonsterName);
				pbc_wmessage_string(monsterData, "monsterName", szName, 0);
			}
		}
		break;
	case TYPEID_PARTNER:
		{
			Partner *p = TO_PARTNER(this);
			pbc_wmessage *partnerData = pbc_wmessage_message(data, "partnerData");
			if (partnerData != NULL)
			{
				pbc_wmessage_integer(partnerData, "partnerProtoID", p->GetEntry(), 0);
			}
		}
		break;
	default:
		break;
	}

	pbc_wmessage_real(data, "curPosX", m_position.m_fX);
	pbc_wmessage_real(data, "curPosY", m_position.m_fY);
	pbc_wmessage_real(data, "curPosHeight", sSceneMapMgr.GetPositionHeight(GetMapId(), m_position.m_fX, m_position.m_fY));
	pbc_wmessage_integer(data, "curDirection", (uint32)m_direction, 0);
	uint32 movingFlag = m_bMoving ? 1 : 0;
	pbc_wmessage_integer(data, "isMoving", movingFlag, 0);

	if (m_bMoving)
	{
		pbc_wmessage_real(data, "moveDestPosX", m_moveDestPosition.m_fX);
		pbc_wmessage_real(data, "moveDestPosY", m_moveDestPosition.m_fY);
		pbc_wmessage_real(data, "moveDestHeight", sSceneMapMgr.GetPositionHeight(GetMapId(), m_moveDestPosition.m_fX, m_moveDestPosition.m_fY));
		pbc_wmessage_integer(data, "moveType", uint32(OBJ_MOVE_TYPE_START_MOVING), 0);
	}

	// sLog.outDebug("[MSG2008]添加一个创建对象数据,当前队列%u个,byteSize=%u", creationBuffer->createobjectlist_size(), creationBuffer->ByteSize());
	// we have dirty data, or are creating for ourself.
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetCreateBits( &updateMask, target );
	// this will cache automatically if needed
	_BuildValuesCreation( data, &updateMask );
}

bool Object::BuildValuesUpdateBlockToBuffer( pbc_wmessage *msg, Player *target )
{
	uint32 uRet = 0;
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetUpdateBits( &updateMask, target );
	if (updateMask.GetSetBitCount() <= 0)
		return false;

	pbc_wmessage *data = pbc_wmessage_message(msg, "UpdateObjects");
	if (data == NULL)
		return false;

	sPbcRegister.WriteUInt64Value(data, "objGuid", GetGUID());
	_BuildValuesUpdate(data, &updateMask);
	
	return true;
}

bool Object::BuildValuesUpdateBlockForPlayer( Player *target )
{
	UpdateMask updateMask;
	updateMask.SetCount( m_valuesCount );
	_SetUpdateBits( &updateMask, target );
	if (updateMask.GetSetBitCount() <= 0)
		return false;

	pbc_wmessage *buffer = target->GetObjUpdateBuffer();
	pbc_wmessage *data = pbc_wmessage_message(buffer, "updateObjects");
	if (data == NULL)
		return false;

	sPbcRegister.WriteUInt64Value(data, "objGuid", GetGUID());
	_BuildValuesUpdate(data, &updateMask);

	return true;
}

void Object::BuildRemoveDataBlockForPlayer( Player *target )
{
	uint64 guid = GetGUID();
	uint32 tp = GetTypeID();

	target->PushOutOfRange(guid, tp);
}

void Object::ClearUpdateMask()
{
	m_updateMask.Clear();
	m_objectUpdated = false;
}

void Object::SetUpdatedFlag()
{
	if (!m_objectUpdated && m_mapMgr != NULL)
	{
		m_mapMgr->ObjectUpdated(this);
		m_objectUpdated = true;
	}
}

void Object::CheckErrorOnUpdateRange(uint32 cellX, uint32 cellY, uint32 mapId)
{
	// sLog.outError("(OBJ type=%u,guid=%u)无法获得对应坐标(%u,%u)的地图cell(mapid=%u)", (uint32)GetTypeID(), GetGUID(), cellX, cellY, mapId);
}

void Object::OnValueFieldChange(const uint32 index)
{
	if(IsInWorld())
	{
		m_updateMask.SetBit( index );

		if(!m_objectUpdated)
			SetUpdatedFlag();
	}
}
//=======================================================================================
//  Creates an update block with the values of this object as
//  determined by the updateMask.
//=======================================================================================
void Object::_BuildValuesCreation( pbc_wmessage *createData, UpdateMask *updateMask )
{
	WPAssert( updateMask && updateMask->GetCount() == m_valuesCount );
	uint32 bc;
	uint32 values_count;
	if( m_valuesCount > ( 2 * 0x20 ) )//if number of blocks > 2->  unit and player+item container
	{
		bc = updateMask->GetUpdateBlockCount();
		values_count = (uint32)min( bc * 32, m_valuesCount );
	}
	else
	{
		bc = updateMask->GetBlockCount();
		values_count = m_valuesCount;
	}

	for( uint32 index = 0; index < values_count; ++index )
	{
		if( updateMask->GetBit( index ) )
		{
			pbc_wmessage *data = pbc_wmessage_message(createData, "propList");
			if (data != NULL)
			{
				pbc_wmessage_integer(data, "propIndex", index, 0);
				sPbcRegister.WriteUInt64Value(data, "propValue", m_uint64Values[index]);
				// sLog.outDebug("BuildCreateValue,objID="I64FMTD", index=%u,value="I64FMTD".", GetGUID(), index, m_uint64Values[index]);
			}
			updateMask->UnsetBit( index );
		}
	}
}

void Object::_BuildValuesUpdate( pbc_wmessage *propData, UpdateMask *updateMask )
{
	WPAssert( updateMask && updateMask->GetCount() == m_valuesCount );
	uint32 bc;
	uint32 values_count;
	if( m_valuesCount > ( 2 * 0x20 ) )//if number of blocks > 2->  unit and player+item container
	{
		bc = updateMask->GetUpdateBlockCount();
		values_count = (uint32)min( bc * 32, m_valuesCount );
	}
	else
	{
		bc = updateMask->GetBlockCount();
		values_count = m_valuesCount;
	}

	for( uint32 index = 0; index < values_count; ++index )
	{
		if( updateMask->GetBit( index ) )
		{
			pbc_wmessage *data = pbc_wmessage_message(propData, "updateProps");
			if (data != NULL)
			{
				pbc_wmessage_integer(data, "propIndex", index, 0);
				sPbcRegister.WriteUInt64Value(data, "propValue", m_uint64Values[index]);
			}

			updateMask->UnsetBit( index );
		}
	}
}

void Object::_SetUpdateBits(UpdateMask *updateMask, Player* target) const
{
	*updateMask = m_updateMask;
}

void Object::_SetCreateBits(UpdateMask *updateMask, Player* target) const
{
	for(uint32 i = 0; i < m_valuesCount; ++i)
	{
		if(m_uint64Values[i] != 0)
			updateMask->SetBit(i);
	}
}

void Object::AddToWorld()
{
	// 仅仅是寻找普通场景的mapmgr让对象加入
	MapMgr* mapMgr = sInstanceMgr.GetNormalSceneMapMgr(this);
	// 并非加入普通场景，那么寻找玩家副本与系统实例副本让对象加入
	if ( mapMgr == NULL )
		mapMgr = OnObjectGetInstanceFailed();
	
	// 如果依然找不到mapmgr说明出错。
	if (mapMgr == NULL)
	{
		sLog.outError( "Couldn't get (map=%u, instance=%u) for Object "I64FMTD"", GetMapId(), GetInstanceID(), GetGUID() );
		return ;
	}

	m_mapMgr = mapMgr;

	mapMgr->AddObject(this);

	//// correct incorrect instance id's
	m_instanceId = m_mapMgr->GetInstanceID();
}

MapMgr* Object::OnObjectGetInstanceFailed()
{
	// 试图从副本实例中寻找到本object所属的地图
	return sInstanceMgr.GetInstanceMapMgr(GetMapId(), GetInstanceID());
}

//Unlike addtoworld it pushes it directly ignoring add pool
//this can only be called from the thread of mapmgr!!!
void Object::PushToWorld(MapMgr* mgr)
{
	ASSERT(mgr != NULL);

	if(mgr == NULL)
	{
		// Reset these so session will get updated properly.
		if(isPlayer())
		{
			cLog.Error("Object","Kicking Player %s due to empty MapMgr;",TO_PLAYER(this)->strGetGbkNameString().c_str());
			GameSession *session = TO_PLAYER(this)->GetSession();
			session->LogoutPlayer(false);
			session->Disconnect();
		}
		return; //instance add failed
	}

	m_mapId = mgr->GetMapId();
	m_instanceId = mgr->GetInstanceID();
	m_mapMgr = mgr;
	m_event_Instanceid = mgr->event_GetInstanceID();
	OnPrePushToWorld();

	mgr->PushObject(this);
	
    event_Relocate();
	OnPushToWorld();
}

void Object::Init()
{
	// 初始化一下行走的对象
	m_bMoving = false;
	m_moveDestPosition.Init();
	m_moveDirection.Init();
	m_nStartMovingTime = 0;
	
	m_instanceId = 0;
	m_deathState = ALIVE;
}

void Object::Delete()
{
	if(IsInWorld())
		RemoveFromWorld(true);
	delete this;
}

void Object::RemoveFromWorld(bool free_guid)
{
	ASSERT(m_mapMgr != NULL);
	MapMgr* m = m_mapMgr;
	m_mapMgr = NULL;

	if (isMoving())
		StopMoving(true, GetPositionNC());

	OnPreRemoveFromWorld(m);
	sEventMgr.RemoveEvents(this);	// 将身上所有定时器杀掉
	m_event_Instanceid = -1;	// 变成worldupdate的
	m->RemoveObject(this, free_guid);
	//// update our event holder
	event_Relocate();
}

bool Object::CanActivate()
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		{
			return true;
		}
		break;
	default:
		break;
	}
	return false;
}

void Object::Activate(MapMgr* mgr)
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		// sLog.outDebug("activeMonsters 添加怪物id="I64FMTD"", GetGUID());
		mgr->pendingActiveMonsterGuids.push_back(GetGUID());
		break;
	default:
		break;
	}

	Active = true;
}

void Object::Deactivate(MapMgr* mgr)
{
	switch(GetTypeID())
	{
	case TYPEID_MONSTER:
		{
			mgr->pendingDeactiveMonsterGuids.push_back(GetGUID());
			// sLog.outDebug("activeMonsters 移除怪物id="I64FMTD"", GetGUID());
		}
		break;
	default:
		break;
	}

	Active = false;
}

void Object::SendMessageToSet(WorldPacket *data, bool bToSelf)
{
	if(bToSelf && isPlayer())
	{
		Player *plr = TO_PLAYER(this);
		if (plr->GetSession() != NULL)
			plr->GetSession()->SendPacket(data);		
	}

	if(!IsInWorld() || !HasInRangePlayers())
		return;

	hash_set<Player*>::iterator itr = m_inRangePlayers.begin();
	hash_set<Player*>::iterator it_end = m_inRangePlayers.end();
	for(; itr != it_end; ++itr)
	{
		if ((*itr)->GetSession())
			(*itr)->GetSession()->SendPacket(data);
	}
}

const char* Object::szGetUtf8Name() const
{
	static const char *szUnknowObject = "Unknow Object";
	return szUnknowObject;
}


