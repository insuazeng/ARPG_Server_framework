#include "stdafx.h"
#include "ObjectPoolMgr.h"
#include "Monster.h"
#include "EventDef.h"

UpdateMask Monster::m_visibleUpdateMask;
void Monster::InitVisibleUpdateBits()
{
	Monster::m_visibleUpdateMask.SetCount(UNIT_FIELD_END);
	Monster::m_visibleUpdateMask.SetBit(OBJECT_FIELD_GUID);
	Monster::m_visibleUpdateMask.SetBit(OBJECT_FIELD_PROTO);

	Monster::m_visibleUpdateMask.SetBit(UNIT_FIELD_MAX_HP);
	Monster::m_visibleUpdateMask.SetBit(UNIT_FIELD_CUR_HP);
}

Monster::Monster(void) : m_protoConfig(NULL), m_spawnData(NULL), m_corpseEvent(false), m_respawnCell(NULL), m_AIState(AI_IDLE)
{
	m_valuesCount = UNIT_FIELD_END;
	m_uint64Values = _fields;
	
	memset(m_uint64Values, 0, (UNIT_FIELD_END) * sizeof(uint64));
	m_updateMask.SetCount(UNIT_FIELD_END);
}


Monster::~Monster(void)
{
	// sLog.outString("第 %u 个creature 被[delete]", ++Creature::m_deleteCounter);
	sEventMgr.RemoveEvents(this);

	m_protoConfig = NULL;
	SAFE_DELETE(m_spawnData);

	if(m_respawnCell != NULL)
	{
		m_respawnCell->_respawnObjects.erase(this);
		m_respawnCell = NULL;
	}

	m_corpseEvent = false;
}

void Monster::Initialize(uint64 guid)
{
	m_valuesCount = UNIT_FIELD_END;
	m_uint64Values = _fields;
	m_updateMask.SetCount(UNIT_FIELD_END);
	memset(m_uint64Values, 0, (UNIT_FIELD_END) * sizeof(uint64));
	
	SetUInt64Value(OBJECT_FIELD_GUID, guid);
	SetUInt64Value(OBJECT_FIELD_TYPE, TYPEID_MONSTER);
	setDeathState(ALIVE);
	// sLog.outString("第 %u 个creature 被[初始化]", ++Creature::m_initCounter);
	Init();
}

void Monster::Terminate()
{
	SAFE_DELETE(m_spawnData);
	m_protoConfig = NULL;

	sEventMgr.RemoveEvents(this);
	if (m_respawnCell != NULL)
	{
		m_respawnCell->_respawnObjects.erase(this);
		m_respawnCell = NULL;
	}
	
	m_corpseEvent = false;
	
	Term();
}

void Monster::gcToMonsterPool()
{
	objPoolMgr.DeleteMonsterToBuffer(this);
}

void Monster::Update( uint32 time )
{
	if ( m_corpseEvent )
	{
		sEventMgr.RemoveEvents(this);

		// 给怪物添加移除尸体的事件
		// 之所以要区分一下是否“快速移除”，是因为客户端需要一段时间表现怪物实体的受击与死亡动作

		/*if(HasCreatureFlag(CREATURE_TYPE_DESPAWN_QUICKLY))
			sEventMgr.AddEvent( TO_CREATURE(this), &Creature::OnRemoveCorpse, EVENT_CREATURE_REMOVE_CORPSE, TIME_CREATURE_REMOVE_CORPSE_QUICKLY, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT );
		else*/
		sEventMgr.AddEvent( TO_MONSTER(this), &Monster::OnRemoveCorpse, EVENT_CREATURE_REMOVE_CORPSE, 1000, 1, EVENT_FLAG_DO_NOT_EXECUTE_IN_WORLD_CONTEXT );

		m_corpseEvent = false;
	}

	if (isAlive() && IsInWorld())
	{
		UpdateMoving(time);
	}
}

void Monster::OnPushToWorld()
{
	ScriptParamArray params;
	params << GetGUID() << GetMapId() << GetInstanceID();
	LuaScript.Call("OnMonsterPushToWorld", &params, NULL);
}

void Monster::OnPreRemoveFromWorld(MapMgr *mgr)
{

}

const float Monster::GetMoveSpeed()
{
	if (m_protoConfig != NULL)
		return m_protoConfig->m_BaseMoveSpeed;
	
	return sDBConfLoader.GetUInt32Value("object_base_speed") / 100.0f;
}

bool Monster::Load( tagMonsterBornConfig *bornConfig, MapMgr *bornMapMgr )
{
	if (m_spawnData != NULL)
		return false;
	m_protoConfig = objmgr.GetMonsterProto(bornConfig->proto_id);
	if (m_protoConfig == NULL)
		return false;

	// 设置entryid
	SetUInt64Value(OBJECT_FIELD_PROTO, m_protoConfig->m_ProtoID);

	uint32 ackType = Monster_ack_passive;
	if (bornConfig->active_ack_percentage > 0 && RandomUInt(100) < bornConfig->active_ack_percentage)
		ackType = Monster_ack_active;

	uint32 moveType= Monster_move_none;
	if (bornConfig->rand_move_percentage > 0 && RandomUInt(100) < bornConfig->rand_move_percentage)
		moveType = Monster_move_random;

	const uint32 mapID = bornMapMgr->GetMapId();

	// 设置一下最大血量（临时）
	SetUInt64Value(UNIT_FIELD_MAX_HP, m_protoConfig->m_HP);
	SetUInt64Value(UNIT_FIELD_CUR_HP, m_protoConfig->m_HP);
	
	m_spawnData = new tagMonsterSpwanData();
	m_spawnData->InitData(bornConfig->conf_id, 0, m_protoConfig->m_ProtoID);
	m_spawnData->config_id = bornConfig->conf_id;
	m_spawnData->born_dir = RandomUInt(360);
	if (bornConfig->born_direction != 0xFF)
		m_spawnData->born_dir = bornConfig->born_direction;

	// 只出生一只怪的话,按照配表的坐标去出生,否则按照指定半径的点位随机设置一个出生坐标
	if (bornConfig->born_count == 1)
	{
		m_spawnData->born_pos = bornConfig->born_center_pos;
	}
	else
	{
		float randRange = bornConfig->born_radius * 2;
		float xDiff = RandomUInt(uint32(randRange * 100)) / 100.0f - bornConfig->born_radius;
		float yDiff = RandomUInt(uint32(randRange * 100)) / 100.0f - bornConfig->born_radius;
		m_spawnData->born_pos.m_fX = bornConfig->born_center_pos.m_fX + xDiff;
		m_spawnData->born_pos.m_fY = bornConfig->born_center_pos.m_fY + yDiff;

		// 如果随机出来的格子无法站立,那么就在中央位置
		if (!sSceneMapMgr.IsCurrentPositionMovable(mapID, m_spawnData->born_pos.m_fX, m_spawnData->born_pos.m_fY))
		{
			m_spawnData->born_pos = bornConfig->born_center_pos;
		}
	}

	_Create(mapID, m_spawnData->born_pos.m_fX, m_spawnData->born_pos.m_fY, m_spawnData->born_dir);

	// 调用lua函数创建lua怪物对象
	ScriptParamArray params;
	params << GetGUID() << m_protoConfig->m_ProtoID << ackType << moveType;
	LuaScript.Call("CreateMonster", &params, NULL);

	return true;
}

void Monster::Load( tagMonsterProto *proto_, float x, float y, float direction, MapMgr *bornMapMgr )
{
	if ( proto_ == NULL )
		return;

	SetProto(proto_);
	_Create(bornMapMgr->GetMapId(), x, y, direction);

	// 如果该怪物是临时设置出生的,那么该怪在死亡后不会进行复活
}

void Monster::RemoveFromWorld(bool addrespawnevent, bool free_guid)
{
	if ( IsInWorld() )
	{
		uint32 delay = 0;
		if ( addrespawnevent && m_spawnData != NULL && m_protoConfig != NULL && m_protoConfig->m_RespawnTime > 0 )    // 增加m_spawn非空限制，避免特殊怪刷新
		{
			delay = m_protoConfig->m_RespawnTime;
		}
		Despawn(0, delay);
	}
}

void Monster::OnRemoveCorpse()
{
	if ( !isDead() )
		return;

	// time to respawn!
	if (IsInWorld() && (int32)m_mapMgr->GetInstanceID() == m_instanceId)
	{
		setDeathState(DEAD);
		m_position = m_spawnLocation;
		// DEBUG_LOG("Creature","OnRemoveCorpse Removing corpse of "I64FMT"...", GetGUID()
		if (!isInNormalScene())
		{
			RemoveFromWorld( false, false );    // 副本处理
			return ;
		}
		else
		{
			if(m_protoConfig != NULL && m_protoConfig->m_RespawnTime > 0)
				RemoveFromWorld(true, false);
			else
				RemoveFromWorld(false, true);
		}
	}
}

void Monster::OnRespawn(MapMgr* m)
{
	if ( m_protoConfig != NULL )
	{
		// 设置一下最大血量（临时）
		SetUInt64Value(UNIT_FIELD_MAX_HP, m_protoConfig->m_HP);
		SetUInt64Value(UNIT_FIELD_CUR_HP, m_protoConfig->m_HP);
	}

	if (m_spawnData != NULL)
	{
		SetDirection(m_spawnData->born_dir);
	}

	setDeathState(ALIVE);

	PushToWorld( m );
}

void Monster::Despawn(uint32 delay, uint32 respawntime)
{
	if ( delay > 0 )
	{
		sEventMgr.AddEvent( TO_MONSTER(this), &Monster::Despawn, (uint32)0, respawntime, EVENT_CREATURE_RESPAWN, delay, 1, 0 );
		return;
	}

	if ( !IsInWorld() )
		return;

	if ( respawntime != 0 )
	{
		/* get the cell with our SPAWN location. if we've moved cell this might break :P */
		MapCell * pCell = m_mapMgr->GetCellByCoords(GetGridSpwanX(), GetGridSpwanY());
		if ( pCell == NULL )
			pCell = m_mapCell;

		ASSERT(pCell != NULL);
		pCell->_respawnObjects.insert( TO_OBJECT(this) );
		sEventMgr.RemoveEvents( this );
		sEventMgr.AddEvent( m_mapMgr, &MapMgr::EventRespawnMonster, TO_MONSTER(this), pCell, EVENT_CREATURE_RESPAWN, respawntime, 1, 0 );
		Object::RemoveFromWorld( false );
		m_position = m_spawnLocation;
		m_respawnCell = pCell;
	}
	else
	{
		// 不考虑副本中的怪物重生，故只在此分支中调用 by eeeo@2011.8.31
		sEventMgr.RemoveEvents( this );	
		Object::RemoveFromWorld( isInNormalScene() );
		SafeDelete();
	}
}

void Monster::SafeDelete()
{
	sEventMgr.RemoveEvents( this );
	// gcToCreaturePool();
	sEventMgr.AddEvent( TO_MONSTER(this), &Monster::DeleteMe, EVENT_CREATURE_SAFE_DELETE, 1000, 1, EVENT_FLAG_DELETES_OBJECT );
}

void Monster::DeleteMe()
{
	if ( IsInWorld() )
	{
		RemoveFromWorld( false, true );		
	}
	gcToMonsterPool();
}

void Monster::_SetCreateBits(UpdateMask *updateMask, Player* target) const
{
	for(uint32 index = 0; index < m_valuesCount; index++)
	{
		if(m_uint64Values[index] != 0 && Monster::m_visibleUpdateMask.GetBit(index))
			updateMask->SetBit(index);
	}
}

void Monster::_SetUpdateBits(UpdateMask *updateMask, Player* target) const 
{
	Object::_SetUpdateBits(updateMask, target);
	*updateMask &= Monster::m_visibleUpdateMask;
}

