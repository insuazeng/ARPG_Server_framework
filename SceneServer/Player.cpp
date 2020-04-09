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
#include "ObjectPoolMgr.h"
#include "convert/strconv.h"

UpdateMask Player::m_visibleUpdateMask;

Player::Player () : m_session(NULL), m_gamestates(STATUS_ANY), m_pbcMovmentDataBuffer(NULL), m_pbcRemoveDataBuffer(NULL), m_pbcRemoveDataCounter(0),
					m_pbcCreationDataBuffer(NULL), m_pbcPropUpdateDataBuffer(NULL), m_bNeedSyncPlayerData(false)
{
	m_uint64Values = NULL;
	m_valuesCount = 0;
	ResetSyncObjectDatas();
}

Player::~Player ()
{
	SAFE_DELETE_ARRAY(m_uint64Values);
	m_valuesCount = 0;
}

void Player::Init(GameSession *session, uint64 guid, const uint32 fieldCount)		//代替构造函数
{
	m_gamestates = STATUS_ANY;
	m_uint64Values = new uint64[fieldCount];
	m_valuesCount = fieldCount;
	m_updateMask.SetCount(fieldCount);
	
	memset(m_uint64Values, 0, fieldCount * sizeof(uint64));

	SetUInt64Value( OBJECT_FIELD_GUID, guid );
	SetUInt64Value( OBJECT_FIELD_TYPE, TYPEID_PLAYER );

	m_session = session;
	m_session->SetPlayer(this);

	ResetSyncObjectDatas();

	bProcessPending			= false;
	if (m_pbcPropUpdateDataBuffer == NULL)
		m_pbcPropUpdateDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002012");
	bMoveProcessPending		= false;
	if (m_pbcMovmentDataBuffer == NULL)
		m_pbcMovmentDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002004");
	bRemoveProcessPending	= false;
	if (m_pbcRemoveDataBuffer == NULL)
		m_pbcRemoveDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002010");
	bCreateProcessPending	= false;
	if (m_pbcCreationDataBuffer == NULL)
		m_pbcCreationDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002008");
	
	// 设置基础坐标
	if (sInstanceMgr.GetDefaultBornMap() != 0)
	{
		SetMapId(sInstanceMgr.GetDefaultBornMap());
		SetPosition(sInstanceMgr.GetDefaultBornPosX(), sInstanceMgr.GetDefaultBornPosY());
	}
	else
	{
		SetMapId(1001);
		SetPosition(100, 100);
	}

	Object::Init();
}

void Player::Term()//代替析构函数
{
	// 删除伙伴对象
	if (!m_Partners.empty())
	{
		Partner *p = NULL;
		for (PlayerPartnerMap::iterator it = m_Partners.begin(); it != m_Partners.end(); ++it)
		{
			p = it->second;
			p->Term();
			SAFE_DELETE(p);
		}
		m_Partners.clear();
	}

	memset(m_uint64Values, 0, m_valuesCount * sizeof(uint64));
	m_session = NULL;
	
	DEL_PBC_W(m_pbcMovmentDataBuffer);
	DEL_PBC_W(m_pbcCreationDataBuffer);
	DEL_PBC_W(m_pbcRemoveDataBuffer);
	m_pbcRemoveDataCounter = 0;
	DEL_PBC_W(m_pbcPropUpdateDataBuffer);

	ResetSyncObjectDatas();

	Object::Term();
}

string Player::strGetGbkNameString()
{
	if (m_NickName.length() <= 0)
		return m_NickName;

	CUTF82C *utf8_name = new CUTF82C(m_NickName.c_str());
	string str = utf8_name->c_str();
	delete utf8_name;

	return str;
}

uint32	Player::GetGameStates()
{ 
	//在状态改变的时候直接修改状态，然后就可以直接返回m_gamestates，目前实时获取状态
	/*m_gamestates = STATUS_ANY; 
	if(this->isDead())
	m_gamestates|=STATUS_DEAD;
	else
	m_gamestates|=STATUS_ALIVE;
	if(GetMapMgr()==NULL)
	m_gamestates|=STATUS_NOTINWORLD;*/
	return m_gamestates;
}

#define PLAYER_UPDATE_SPAN     50
void	Player::Update( uint32 p_time )
{
	if (!IsInWorld())		// 不在世界中
		return ;

	UpdateMoving(p_time);

	// Unit::Update(p_time);	// 此处回血(AI的控制)

	// 对数据属性的buffer进行同步
	if (((++m_nSyncDataLoopCounter % 10) == 0) && m_bNeedSyncPlayerData)
		SyncUpdateDataMasterServer();
}

bool Player::SafeTeleport(uint32 MapID, uint32 InstanceID, float X, float Y)
{
	// Lookup map info
	MapInfo * mi = sSceneMapMgr.GetMapInfo( MapID );
	if ( mi == NULL )
		return false;

	//are we changing instance or map?
	bool force_new_world = ( m_mapId != MapID || ( InstanceID != 0 && (uint32)m_instanceId != InstanceID ) );	//用于切换到副本的情况
	return _Relocate( MapID, FloatPoint(X, Y), force_new_world, InstanceID );
}

bool Player::CanEnterThisScene(uint32 mapId)
{
	// 场景能否进入，留待用脚本实现
	return true;
}

bool Player::HandleSceneTeleportRequest(uint32 destServerIndex, uint32 destMapID, FloatPoint destPos)
{
	uint32 curMapID = GetMapId();
	uint32 changeRet = TRANSFER_ERROR_NONE;
	do 
	{
		if (!IsInWorld() )
		{
			changeRet = TRANSFER_ERROR_NOT_IN_WORLD;
			break;
		}
		//if (IsInCombat())
		//{
		//	changeRet = TRANSFER_ERROR_BADSTATE_COMBATING;
		//	break;
		//}
		if (destPos.m_fX >= mapLimitWidth || destPos.m_fY >= mapLimitLength)
		{
			changeRet = TRANSFER_ERROR_COORD;
			break;
		}
		if(!CanEnterThisScene(destMapID))
		{
			//GetSession()->SendPopUpMessage(POPUP_AREA_4, sLang.GetTranslate("目标场景未开放"));
			changeRet = TRANSFER_ERROR_SCENE_NOT_OPEN;
			break;
		}
	} while (false);

	if (changeRet == TRANSFER_ERROR_NONE)
	{
		if (curMapID == destMapID)														// 如果是同地图传送，则网关不进行锁定
		{
			if (InSpecifiedScene(destMapID) && !destPos.IsSamePoint(0.0f, 0.0f))		// 应该检测目标点是否可以到达
			{
				if ( CalcDistance( destPos.m_fX, destPos.m_fY ) < 3.0f )	// 3米内的距离以内不传送
					return false;
			}
			else
				return false;
		}
		else																	// 网关进行锁定，无论如何都要返回让网关继续保持锁定或解锁
		{
			// 检测是否在本场景服内的场景切换
			if (destPos.IsSamePoint(0.0f, 0.0f))		// 获取传送点的目标场景坐标
				changeRet = sSceneMapMgr.TransferPlayer( curMapID, GetPositionNC(), destMapID, destPos );

			if (changeRet == TRANSFER_ERROR_NONE)
			{
				// 检查是否为有效的地图
				MapInfo * pminfo = sSceneMapMgr.GetMapInfo( destMapID );
				if ( pminfo == NULL || !pminfo->isNormalScene() )
				{
					changeRet = TRANSFER_ERROR_MAP;
				}
				else if(pminfo->enter_min_level > GetLevel())
				{
					//GetSession()->SendPopUpMessage(POPUP_AREA_4, sLang.GetTranslate("场景等级限制"));
					changeRet = TRANSFER_ERROR_LEVEL;
				}
				else
				{
					// 检测是否为有效坐标
					if (!sSceneMapMgr.IsCurrentPositionMovable(destMapID, destPos.m_fX, destPos.m_fY))
						changeRet = TRANSFER_ERROR_COORD;
				}
			}
		}
	}

	if (changeRet == TRANSFER_ERROR_NONE)
	{
		if (destServerIndex == ILLEGAL_DATA_32)
		{
			if (sInstanceMgr.HandleSpecifiedScene(destMapID))
			{
				// 在本场景服进行传送
				if ( !SafeTeleport( destMapID, 0, destPos.m_fX, destPos.m_fY ) )
				{
					sLog.outError( "玩家 %s 传送到 %u(%.1f,%.1f) 失败", strGetGbkNameString().c_str(), destMapID, destPos.m_fX, destPos.m_fY );
					changeRet = TRANSFER_ERROR_MAP;
				}
			}
			else
			{
				uint32 targetServerIndex = sInstanceMgr.GetHandledServerIndexBySceneID(destMapID);
				sLog.outString("玩家 %s 准备去往场景服 %u 管理的场景%u (%.1f,%.1f)", strGetGbkNameString().c_str(), targetServerIndex, destMapID, destPos.m_fX, destPos.m_fY);
				changeRet = TRANSFER_ERROR_NONE_BUT_SWITCH;
			}
		}
		else
		{
			// 如果目标场景进程是本进程也不需要特殊处理
			if (destServerIndex == sGateCommHandler.GetSelfSceneServerID())
			{
				// 在本场景服进行传送
				if ( !SafeTeleport( destMapID, 0, destPos.m_fX, destPos.m_fY ) )
				{
					sLog.outError( "玩家 %s 传送到 %u(%.1f,%.1f) 失败", strGetGbkNameString().c_str(), destMapID, destPos.m_fX, destPos.m_fY );
					changeRet = TRANSFER_ERROR_MAP;
				}
			}
			else
			{
				sLog.outString("玩家 %s 准备去往场景服 %u 管理的场景%u (%.1f,%.1f)", strGetGbkNameString().c_str(), destServerIndex, destMapID, destPos.m_fX, destPos.m_fY);
				changeRet = TRANSFER_ERROR_NONE_BUT_SWITCH;
			}
		}
	}

	// 通知回去
	sPark.SceneServerChangeProc(GetSession(), destMapID, destPos, changeRet, destServerIndex);
	return true;
}

bool Player::_Relocate(uint32 mapid, const FloatPoint &v, bool force_new_world, uint32 instance_id)
{
	//are we changing maps?
	if ( force_new_world || m_mapId != mapid )	// 本函数基本由SafeTeleprot调用，force_new_world中已包含异图判断
	{
		//Preteleport will try to find an instance (saved or active), or create a new one if none found.
		uint32 status = sInstanceMgr.PreTeleport( mapid, this, instance_id );
		if ( status != INSTANCE_OK )
		{
			// 未找到合适的副本进行传送 by eeeo@2011.7.14
			// 或许应该在上层逻辑做更多的判断
			return false;
		}

		//did we get a new instanceid?
		if ( instance_id != 0 )
			SetInstanceID(instance_id);

		if ( IsInWorld() )  // 备份坐标
		{
			bool savePos = true;
			if(savePos)
			{
				m_lastMapData.SetData(GetMapId(), GetPositionX(), GetPositionY());
			}
		}
		else
		{
			m_lastMapData.ClearData();
		}

		//remove us from this map
		if ( IsInWorld() )
			RemoveFromWorld();

		SetMapId( mapid );

		//if(GetPlayerTransferStatus() == TRANSFER_PENDING)
		//	sLog.outError("when plr %s safeteleport to map %u in state transpending", strGetGbkNameString().c_str(), mapid);

		////trandsfer_pending表示玩家正在传送中，还没成功
		//SetPlayerTransferStatus( TRANSFER_PENDING );
	}

	// 修改坐标
	SetPosition(v.m_fX, v.m_fY);

	return true;
}

void Player::EjectFromInstance()
{
	// 从副本内退出，调用脚本处理
}

void Player::AddToWorld()
{
	// If we join an invalid instance and get booted out, TO_PLAYER(this) will prevent our stats from doubling :P
	if ( IsInWorld() )
	{
		// sLog.outError( "Player %s is already InWorld(%u) when he is calling AddToWorld() !", strGetGbkNameString().c_str(), GetMapId());
		return;
	}

	Object::AddToWorld();

	/// Add failed.
	if ( m_mapMgr == NULL )
	{
		sLog.outError( "Adding player %s to (map=%u,instance=%u) failed.", strGetGbkNameString().c_str(), GetMapId(), GetInstanceID() );
		// eject from instance
		// EjectFromInstance();		// 回到原先的场景内
		return;
	}
	else
	{
		if ( GetSession() != NULL )
			GetSession()->SetInstance( m_mapMgr->GetInstanceID() );
	}
}

MapMgr* Player::OnObjectGetInstanceFailed()
{
	MapMgr *ret = Object::OnObjectGetInstanceFailed();

	// 需要为玩家创建单人副本或找到新的系统实例让玩家进入
	//if (ret == NULL)
	//{
	//	Instance *instance = NULL;
	//	MapInfo *mapInfo = sSceneMapMgr.GetMapInfo(GetMapId());
	//	if (mapInfo == NULL)
	//		return NULL;

	//	if (mapInfo->isGuildScene())
	//		instance = sInstanceMgr.FindGuildInstance(GetMapId(), GetGuildID());
	//	else if (mapInfo->isSystemInstance())
	//		instance = sInstanceMgr.FindSystemInstance(GetMapId());
	//	else
	//		instance = sInstanceMgr.GetInstance(GetMapId(), GetInstanceID());

	//	if (instance != NULL)
	//	{
	//		// 已经存在该实例，检测玩家是否能进入
	//		uint8 checkRet = sInstanceMgr.PlayerOwnsInstance(instance, this);
	//		if (checkRet >= OWNER_CHECK_OK)
	//		{
	//			// 这个实例可能需要系统再创建
	//			if (instance->m_mapMgr == NULL && instance->m_mapInfo->isInstance())
	//				instance->m_mapMgr = sInstanceMgr.CreateInstance(instance);
	//			ret = instance->m_mapMgr;
	//		}
	//		
	//		else
	//			sLog.outError("玩家 %s 不满足进入场景 %u 的条件(ret=%u)", strGetGbkNameString().c_str(), GetInstanceID(), uint32(checkRet));
	//	}

	//	// 不应该不存在玩家副本实例
	//	// ASSERT(ret != NULL);
	//}

	return ret;
}

void Player::OnPushToWorld()
{
	// Process create packet
	if( m_mapMgr != NULL )
		ProcessObjectPropUpdates();

	// Unit::OnPushToWorld();

	// 调用脚本的进入场景
	// CALL_INSTANCE_SCRIPT_EVENT( m_mapMgr, OnPlayerEnter )( this );

	// 通知中央服自己进入某场景
	WorldPacket centrePacket(SCENE_2_MASTER_PLAYER_MSG_EVENT, 128);
	centrePacket << uint32(0) << sGateCommHandler.GetSelfSceneServerID();
	centrePacket << GetGUID();
	centrePacket << uint32(STM_plr_event_enter_map);
	uint16 mapID = GetMapId(), posX = GetPositionX(), posY = GetPositionY();
	centrePacket << mapID << posX << posY << GetInstanceID();
	GetSession()->SendServerPacket(&centrePacket);

	sPark.OnPlayerEnterMap(GetMapId());
}

void Player::RemoveFromWorld()
{
	if(IsInWorld())
	{
		sPark.OnPlayerLeaveMap(GetMapId());
		Object::RemoveFromWorld(false);
	}
}

void Player::OnValueFieldChange(const uint32 index)
{
	// sLog.outDebug("玩家:%s 设置value,index=%u, val="I64FMTD"..", strGetGbkNameString().c_str(), index, GetUInt64Value(index));
	if(IsInWorld() && m_visibleUpdateMask.GetBit(index))
	{
		m_updateMask.SetBit( index );
		if(!m_objectUpdated)
			SetUpdatedFlag();
	}
}

void Player::SyncUpdateDataMasterServer()
{
	ScriptParamArray params;
	params << GetGUID();

	LuaScript.Call("SyncPlayerDataToMaster", &params, NULL);

	// 重置同步数据
	ResetSyncObjectDatas();
}

void Player::ResetSyncObjectDatas()
{
	m_nSyncDataLoopCounter = 0;
	m_bNeedSyncPlayerData = false;
}

//===================================================================================================================
//  Set Create Player Bits -- Sets bits required for creating a player in the updateMask.
//  Note:  Doesn't set Quest or Inventory bits
//  updateMask - the updatemask to hold the set bits
//===================================================================================================================
void Player::_SetCreateBits(UpdateMask *updateMask, Player* target) const
{
	if(target == this)
	{
		Object::_SetCreateBits(updateMask, target);
	}
	else
	{
		for(uint32 index = 0; index < m_valuesCount; index++)
		{
			if(m_uint64Values[index] != 0 && Player::m_visibleUpdateMask.GetBit(index))
				updateMask->SetBit(index);
		}
	}
}

void Player::_SetUpdateBits(UpdateMask *updateMask, Player* target) const
{
	if(target == this)
	{
		Object::_SetUpdateBits(updateMask, target);
	}
	else
	{
		Object::_SetUpdateBits(updateMask, target);
		*updateMask &= Player::m_visibleUpdateMask;
	}
}

