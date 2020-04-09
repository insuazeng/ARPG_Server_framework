#include "stdafx.h"
#include "MasterCommServer.h"
#include "GateCommHandler.h"
#include "PlayerMgr.h"

void CMasterCommServer::HandlePing(WorldPacket &recvData)
{
	WorldPacket pck(SMSG_MASTER_2_GATE_PONG, 20);
	SendPacket(&pck, SMSG_MASTER_2_GATE_PONG, 0);

	int32 diff = time(NULL) - m_lastPingRecvUnixTime - SERVER_PING_INTERVAL;
	if (diff >= 2)
		sLog.outString("MasterServer Pong delay %d sec", diff);

	// printf("center server pong %d sec\n", diff);
	m_lastPingRecvUnixTime = time(NULL);
}

void CMasterCommServer::HandleSceneServerConnectionStatus(WorldPacket &recvData)
{
	sPark.GetSceneServerConnectionStatus(recvData);
}

void CMasterCommServer::HandleAuthChallenge(WorldPacket &recvData)
{
	// sLog.outColor(TGREEN, "中央服务器 接收到验证请求..\n");
	unsigned char key[20];
	uint32 result = 1;
	recvData.read(key, 20);

	sLog.outString("Authentication request from %s, result %s.",(char*)inet_ntoa(m_peer.sin_addr), result ? "OK" : "FAIL");

	printf("Key: ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X", key[i]);
	}
	printf("\n");
	/* send the response packet */
	WorldPacket pck( SMSG_MASTER_2_GATE_AUTH_RESULT, 128 );
	pck << result;

	authenticated = result;
	if(authenticated)
	{	
		uint32 gatewayPort, globalServerID;
		recvData >> registername >> registeradderss >> gatewayPort >> globalServerID;
		gateserver_id = sInfoCore.GenerateGateServerID();
		sLog.outString("注册网关[id=%u]服务器%s 地址%s,Port=%u", gateserver_id, registername.c_str(), registeradderss.c_str(), gatewayPort);
		pck << uint32(0);
		pck << gateserver_id;
		pck << registername;
		SendPacket( &pck, SMSG_MASTER_2_GATE_AUTH_RESULT, 0 );
	}
	else
	{
		SendPacket( &pck, SMSG_MASTER_2_GATE_AUTH_RESULT, 0 );
	}
}

void CMasterCommServer::HandleRequestRoleList(WorldPacket &recvData)
{
	uint64 guid = 0;
	uint32 sessionIndex = 0, gmFlag = 0;
	string agentName, accName;
	recvData >> guid >> agentName >> accName >> sessionIndex >> gmFlag;

	sLog.outString("[登陆]代理%s玩家账号%s(网关id=%u)请求获取人物列表", agentName.c_str(), accName.c_str(), GetGateServerId());

	bool bWaitAdd = false;				// 延迟加入的标记
	CPlayerServant *pExist = sPlayerMgr.FindServant(agentName, accName);
	
	if ( pExist != NULL )
	{
		// 这里处理的是来自不同网关的请求，因为同一网关的重复登陆请求已经在网关做掉了
		if (pExist->GetServerSocket() != this)		// 所以该判断一定为真
			pExist->DisConnect();
		else
		{
			if(pExist->GetServerSocket() != NULL && !pExist->IsWaitLastData())
			{
				sLog.outError("同一网关没有处理重复登陆的请求？！代理%s(账号名%s) 原网关%u 当前网关%u", agentName.c_str(), accName.c_str(), pExist->GetServerSocket()->GetGateServerId(), GetGateServerId());
				pExist->DisConnect();
				// return ;
			}
			sLog.outError("同一网关登录时,旧的servert(代理%s,账号名%s)还未被删除,来自网关%u", agentName.c_str(), accName.c_str(), GetGateServerId());
		}
		bWaitAdd  = true;
		sLog.outString("玩家(代理%s,账号名%s,guid="I64FMTD")已经存在游戏中,来自网关%u,需要延时登陆处理", agentName.c_str(), accName.c_str(), pExist->GetCurPlayerGUID(), GetGateServerId());
	}

    ServantCreateInfo info;
	info.Init(guid, gmFlag, agentName, accName);
	
	CPlayerServant* pNew = sPlayerMgr.CreatePlayerServant( &info, sessionIndex, this );
    if ( pNew == NULL )
    {
        sLog.outError( "Error: fail to create new servant(guid="I64FMTD"), out of memory?", guid );
        return;
    }

	if ( bWaitAdd )
	{
		pNew->m_nReloginStartTime = getMSTime64();
		sPlayerMgr.AddPendingServant( pNew );
	}
	else
    {
		sPlayerMgr.AddServant( pNew );
    }
}

//void CMasterCommServer::HandleEnterSceneNotice(WorldPacket &recvData)
//{
//	uint64 guid = 0;
//	uint32 sceneServerID = 0, mapid = 0, curPos = 0, enterTime = 0;
//	string loginIpAddr;
//
//	recvData >> guid >> sceneServerID >> enterTime >> mapid >> curPos >> loginIpAddr;
//	
//	CPlayerServant *pServant = sPlayerMgr.FindServant(guid);
//	if (pServant == NULL || pServant->GetPlayer() == NULL)
//	{
//		sLog.outError("未找到合法的servant或entity对象(guid="I64FMTD"),设置登陆成功信息失败", guid);
//		return ;
//	}
//
//	PlayerEntity *plr = pServant->GetPlayer();
//	pServant->m_bHasPlayer = true;
//	pServant->m_nGameState = STATUS_GAMEING;
//
//	uint32 posX = Get_CoordPosX(curPos);
//	uint32 posY = Get_CoordPosY(curPos);
//
//	// 绑定该玩家的playerinfo
//	if (pServant->GetPlayerInfo() == NULL)
//	{
//		sLog.outError("玩家 %s("I64FMTD") 进入游戏前未设置playerinfo", plr->strGetGbkName().c_str(), guid);
//		PlayerInfo *info = sPlayerMgr.GetPlayerInfo(guid);
//		ASSERT(info != NULL);
//		pServant->SetPlayerInfo(info);
//	}
//
//	PlayerInfo *info = pServant->GetPlayerInfo();
//	ASSERT(info != NULL && "登陆时servant找不到playerinfo");
//
//	// 这里检测一下场景服的索引
//	if (pServant->GetCurSceneServerID() != 0)
//		sLog.outError("玩家 %s("I64FMTD") 进入场景服时,原场景服索引不为0(%u)", plr->strGetGbkName().c_str(), guid, pServant->GetCurSceneServerID());
//
//	// 设置本次进入场景服的时间
//	pServant->SetSceneServerID(sceneServerID, enterTime, true);
//	
//	// 当login_time为0时才记录本次登陆时间，换线不重新计时
//	if (plr->GetPlaytimeValue(PLAYTIME_CUR_LOGIN) == 0)
//		plr->SetPlaytimeValue(PLAYTIME_CUR_LOGIN, UNIXTIME);
//
//	// 为info设置servant指针
//	info->SetServant(pServant);
//
//	sLog.outString("[登陆成功]中央服确认玩家 %s("I64FMTD")进入场景服[%u](map%u,pos %u,%u)", plr->strGetGbkName().c_str(), guid, sceneServerID, mapid, posX, posY);
//
//}

void CMasterCommServer::HandleChangeSceneServerNotice(WorldPacket &recvData)
{
	// 玩家请求切换场景服
	uint64 guid;
	uint32 changeTimer;
	recvData >> guid >> changeTimer;
	CPlayerServant *servant = sPlayerMgr.FindServant(guid);
	if (servant != NULL)
		servant->OnPlayerChangeSceneServer(changeTimer);
}

void CMasterCommServer::HandlePlayerGameRequest(WorldPacket &recvData)
{
	// 塞入到每个servant的queue中
	sPlayerMgr.SessioningProc(&recvData, recvData.GetUserGUID());
}

void CMasterCommServer::HandleCloseServantRequest(WorldPacket &recvData)
{
	uint64 guid;
	uint32 sessionIndex;
	bool direct;
	recvData >> guid >> sessionIndex >> direct;

	// direct如果为false，表示网关已经通知游戏服退出。
	sLog.outString("[会话关闭]收到网关关闭玩家通知(guid="I64FMTD"),sessionindex=%u,direct=%d", guid, sessionIndex, direct);
	sPlayerMgr.InvalidServant(guid, sessionIndex, this, false, false, direct);
}

void CMasterCommServer::HandleSceneServerPlayerMsgEvent(WorldPacket &recvData)
{
	uint32 scene_index, acc_id, player_guid, op_code;
	recvData >> scene_index >> scene_index >> acc_id >> player_guid >> op_code;

	CPlayerServant *pServant = sPlayerMgr.FindServant(acc_id);
	if (pServant == NULL)
		return ;
}

void CMasterCommServer::HandleSceneServerSystemMsgEvent(WorldPacket &recvData)
{
	uint32 serverID;
	uint16 opCode;
	recvData >> serverID >> opCode;
	switch (opCode)
	{
	default:
		break;
	}
}

