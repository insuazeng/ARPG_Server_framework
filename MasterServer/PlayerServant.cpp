#include "stdafx.h"
#include "PlayerServant.h"
#include "MasterCommServer.h"
#include "md5.h"

CPlayerServant::CPlayerServant(void) : m_nGameState(0), m_pServerSocket(NULL),
	m_nCurSceneServerID(0), m_lastTimeSetServerIndex(0), m_bLogoutFlag(false)
{
	m_deleteTime = 0;
	m_newPlayer = false;
	ResetSyncObjectDatas();
}

CPlayerServant::~CPlayerServant(void)
{
	WorldPacket *packet = NULL;
	deleteMutex.Acquire();
	while (packet = _recvQueue.Pop())
	{
		DeleteWorldPacket(packet);
	}
	deleteMutex.Release();
}

void CPlayerServant::Init( ServantCreateInfo * info, uint32 sessionIndex )
{
	if ( info == NULL )
	{
		sLog.outError( "Error: Null create info param for CPlayerServant::Init(), sessionIdx=%u", sessionIndex );
		return;
	}
	m_nGameState = STATUS_LOGIN;
	m_bHasPlayer = false;
	m_bDeleted = false;
	m_bPass = false;
	m_bLogoutFlag = false;
	m_bVerified = false;
	m_nSessionIndex = sessionIndex;
	m_RoleList.Clear();
	m_RoleList.m_nPlayerGuid = info->m_guid;
	m_RoleList.m_strAgentName = info->m_agentname;
	m_RoleList.m_strAccountName = info->m_accountname;
	m_nGmFlag = info->m_gmflags;
	m_nReloginStartTime = 0;
	m_nCurSceneServerID = 0;
	m_lastTimeSetServerIndex = 0;
	m_deleteTime = 0;
	m_newPlayer = false;
	ResetSyncObjectDatas();
	m_gm = false;
}

void CPlayerServant::Term()
{
	m_nGameState = STATUS_NONE;
	m_bDeleted = true;
	m_bLogoutFlag = true;
	m_RoleList.Clear();

	m_nSessionIndex = 0;
	m_nReloginStartTime = 0;
	m_deleteTime = 0;

	ResetSyncObjectDatas();

	//残留数据包删除？
	WorldPacket *packet = NULL;
	while(packet = _recvQueue.Pop())
	{
		DeleteWorldPacket(packet);
	}
}

void CPlayerServant::DisConnect()
{
	//这里要判断玩家是否在场景中，如果不在，可以直接删除，如果在，则要等场景服返回最终信息
	if(m_nCurSceneServerID > 0)
	{	
		if(m_deleteTime==0)//防止重复调用
		{
			m_deleteTime = UNIXTIME;
			if(m_pServerSocket && m_pServerSocket->IsConnected())
				m_pServerSocket->DeletePlayerSocket(m_RoleList.m_nPlayerGuid, m_nSessionIndex, false);
		}
	}
	else
	{
		if(m_pServerSocket && m_pServerSocket->IsConnected())
			m_pServerSocket->DeletePlayerSocket(m_RoleList.m_nPlayerGuid, m_nSessionIndex, true);
		m_pServerSocket = NULL;
	}
}

void CPlayerServant::FillRoleListInfo()
{
	// 从lua处获取角色列表数据
	ScriptParamArray params;
	params << m_RoleList.m_nPlayerGuid;
	params << m_RoleList.m_strAgentName.c_str();
	params << m_RoleList.m_strAccountName.c_str();
	LuaScript.Call("FillAccountRoleListData", &params, NULL);

	switch (m_RoleList.GetRolesNum())
	{
	case 0:
		m_newPlayer = true;
		// 还没有创建角色
		break;
	case MAX_ROLE_PER_ACCOUNT:
		{
			// 只有一个角色，那么直接进入游戏就可以了，进入游戏也交给客户端做
		}
		break;
	default:
		// 角色数量大于1，需要玩家选择角色
		sLog.outString("[登陆]账号%s的玩家处于合服后多角色状态,本次共读取角色信息%u条", szGetAccoundName(), m_RoleList.GetRolesNum());
		break;
	}
}

void CPlayerServant::WriteRoleListDataToBuffer(pbc_wmessage *msg)
{
	for (int i = 0; i < m_RoleList.m_vecRoles.size(); ++i)
	{
		tagSingleRoleInfo& data = m_RoleList.m_vecRoles[i];
		pbc_wmessage *roleData = pbc_wmessage_message(msg, "roleList");
		sPbcRegister.WriteUInt64Value(roleData, "roleGuid", data.m_nPlayerID);
		pbc_wmessage_string(roleData, "roleName", data.m_strPlayerName.c_str(), 0);
		pbc_wmessage_integer(roleData, "career", data.m_nCareer, 0);
		pbc_wmessage_integer(roleData, "level", data.m_nLevel, 0);
		pbc_wmessage_integer(roleData, "banExpireTime", data.m_nBanExpiredTime, 0);
	}
}

void CPlayerServant::LogoutServant(bool saveData)
{
	static const uint32 sOnlineFlag = STATUS_GAMEING | STATUS_SERVER_SWITTHING;

	if (m_bLogoutFlag)
		return ;

	m_bLogoutFlag = true;

	ScriptParamArray params;
	params << GetCurPlayerGUID();
	params << m_nGameState;
	params << saveData;
	LuaScript.Call("LogoutPlayer", &params, NULL);
}

void CPlayerServant::ResetCharacterInfoBeforeTransToNextSceneServer()
{

}

void CPlayerServant::SendServerPacket(WorldPacket *packet)
{
	packet->SetUserGUID(m_RoleList.m_nPlayerGuid);
	deleteMutex.Acquire();
	if (m_pServerSocket && m_pServerSocket->IsConnected())
	{
		m_pServerSocket->SendPacket(packet, packet->GetServerOpCode(), packet->GetUserGUID());
	}
	deleteMutex.Release();
}

void CPlayerServant::SendResultPacket(WorldPacket *packet)
{
	deleteMutex.Acquire();
	packet->SetServerOpcodeAndUserGUID(SMSG_MASTER_2_GATE_SESSIONING_RET, GetCurPlayerGUID());
	SendServerPacket(packet);
	deleteMutex.Release();
}

int CPlayerServant::Update(uint32 p_time)
{
	OpcodeHandler * Handler = NULL;
	WorldPacket *packet = NULL;
	if(m_deleteTime > 0)
	{
		//这里的时间差不能大于15秒，即socket内存回收的时间。否则会引用的已经删除的m_pServerSocket
		if(UNIXTIME > m_deleteTime + 5)
		{
			sLog.outError("Servant (guid="I64FMTD") WaitLastData TimeOut", GetCurPlayerGUID());
			m_pServerSocket = NULL;
		}
	}

	if (!m_pServerSocket)
	{
		// 玩家下线登陆
		LogoutServant(true);
		return -1;
	}

	bool inGamingState = IsInGameStatus(STATUS_GAMEING);
	while(packet = _recvQueue.Pop())
	{
		ASSERT(packet != NULL);

		// 把网络数据交给lua层处理
		ScriptParamArray params;
		params << packet->GetOpcode();
		params << packet->GetUserGUID();
		if (!packet->empty())
		{
			size_t sz = packet->size();
			params.AppendUserData((void*)packet->contents(), sz);
			params << sz;
		}
		LuaScript.Call("HandleWorldPacket", &params, &params);
		
		/*Handler = &WorldPacketHandlers[packet->GetOpcode()];
		if (inGamingState && !m_bHasPlayer)
		{
			DeleteWorldPacket(packet);
			continue;
		}
		if (Handler->handler != 0 && IsInGameStatus(Handler->status))
		{
			(this->*Handler->handler)(*packet);
		}*/

		DeleteWorldPacket(packet);
	}

	// 同步对象数据
	if ((m_nSyncDataObjectCount > 0) && ((++m_nSyncDataLoopCounter % 10) == 0))
	{
		if (m_nCurSceneServerID > 0)
		{
			// ASSERT(m_nCurSceneServerIndex != 0 && "CenterServer SyncData to SceneServer when curIndex == 0");
			//WorldPacket packet(CENTER_TO_GAME_SYNC_OBJECT_DATAS);
			//packet << m_nCurSceneServerIndex;						// 场景服索引
			//packet << (uint32)0;									// 账号id
			//packet << sPark.GetServerID();
			//packet << (uint16)1;
			//packet << GetPlayerId();
			//packet << m_nSyncDataObjectCount;
			//packet.append(m_syncObjectDataBuffer);
			//SendServerPacket(&packet);
			ResetSyncObjectDatas();
		}
	}

	//update中有可能主动断开玩家？
	if (!m_pServerSocket)
	{
		LogoutServant(true);
		return -1;
	}

	return 0;
}

bool CPlayerServant::SetSceneServerID(uint32 curSceneServerIndex, const uint32 setIndexTimer, bool forceSet)
{
	bool bRet = true;
	// 玩家切换了场景服务器
	if (forceSet)
	{
		if (m_nCurSceneServerID != curSceneServerIndex)
		{
			m_nCurSceneServerID = curSceneServerIndex;
			m_lastTimeSetServerIndex = UNIXTIME;
			sLog.outString("玩家 guid="I64FMTD" 当前场景服索引被设置为 %u", GetCurPlayerGUID(), curSceneServerIndex);
		}
	}
	else
	{
		// 检测设置的时间（后到的设置时间无法再设置此次的场景服id）
		if (m_lastTimeSetServerIndex > setIndexTimer)
		{
			bRet = false;
		}
		else
		{
			m_lastTimeSetServerIndex = setIndexTimer;
			m_nCurSceneServerID = curSceneServerIndex;
			sLog.outString("玩家 guid="I64FMTD" 当前场景服索引被设置为 %u", GetCurPlayerGUID(), curSceneServerIndex);
		}
	}

	// 被设置为0，需要清理当前的同步数据
	if (m_nCurSceneServerID == 0)
		ResetSyncObjectDatas();

	return bRet;
}

void CPlayerServant::OnPlayerChangeSceneServer(uint32 changeIndexTimer)
{
	bool setRet = SetSceneServerID(0, changeIndexTimer, false);
	// 成功将场景索引清零
	if (setRet)
	{
		m_bHasPlayer = false;
		m_nGameState = STATUS_SERVER_SWITTHING;
	}
}

//void CPlayerServant::OnPlayerFirstLoadedScene(uint32 curSceneIndex)
//{
//	m_bFirstLoadedScene = true;
//	PlayerEntity *plr = GetPlayer();
//	PlayerInfo *info = GetPlayerInfo();
//
//	// 玩家第一次进入到游戏场景服
//	if (info != NULL && info->first_enter_scene_time == 0)
//	{
//		info->first_enter_scene_time = UNIXTIME;
//		// 保存到数据库
//		// GameDatabase.Execute("UPDATE character_base_info set =%u where player_guid="I64FMTD";", uint32(UNIXTIME), plr->GetGUID());
//		BSONObjBuilder b, bSet;
//		b.appendIntOrLL("player_guid", plr->GetGUID());
//		bSet.append("first_enter_scene_time", uint32(UNIXTIME));
//		MongoGameDatabase.update(sDBConfLoader.GetMongoTableName("character_datas"), mongo::Query(b.obj()), BSON("$set" << bSet.obj()), true);
//		// 是否获得第一个称号
//		// ...
//	}
//
//	// 玩家在每一个场景服进程都第一次进入场景
//	sLog.outDebug("玩家 %s(%u) 在场景进程[%u]第一次进入场景", plr->strGetGbkName().c_str(), plr->GetGUID(), curSceneIndex);
//}
//
//void CPlayerServant::OnPlayerGotCharInfo(uint32 curSceneIndex)
//{
//	PlayerEntity *plr = GetPlayer();
//
//	sLog.outDebug("玩家 %s(%u) 在场景进程[%u]获取角色数据", plr->strGetGbkName().c_str(), plr->GetGUID(), curSceneIndex);
//	m_bGotCharacterInfo = true;
//
//}
//
//void CPlayerServant::OnPlayerChangedMap(WorldPacket &recvData)
//{
//}
//
//void CPlayerServant::FillEnterGameDatas(WorldPacket &packet)
//{
//	// 填充玩家进入游戏所需要的数据
//	packet << sPark.GetServerFullID();
//	packet << strGetAgentName();
//	packet << strGetAccountName();
//	GetPlayerInfo()->WriteEnterGameData(packet);
//	GetPlayer()->WriteEnterGameData(packet);
//}

void CPlayerServant::ResetSyncObjectDatas()
{
	m_nSyncDataLoopCounter = 0;
	m_nSyncDataObjectCount = 0;
	m_syncObjectDataBuffer.clear();
	m_syncObjectDataBuffer.reserve(4096);
}

//void CPlayerServant::OnCurrentPlayerLevelUp(uint32 plrGuid, uint32 newLevel)
//{
//	PlayerInfo *plrInfo = GetPlayerInfo();
//	ASSERT(plrInfo != NULL && plrInfo->guid == plrGuid);
//	plrInfo->level = newLevel;
//	sPlayerMgr.PlayerLevelUp(plrInfo, newLevel);
//}

