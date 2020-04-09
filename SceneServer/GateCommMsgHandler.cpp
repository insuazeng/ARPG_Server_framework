#include "stdafx.h"
#include "SceneCommServer.h"
#include "UpdateThread.h"

extern bool m_ShutdownEvent;
extern uint32 m_ShutdownTimer;
extern bool g_sReloadStaticDataBase;

void CSceneCommServer::HandlePacket(WorldPacket *recvData)
{
	// 处理消息包的内容，包括网关服务器的验证，以及会话的创建等等
	/*if(recvData->GetServerOpCode() != CMSG_GATE_2_SCENE_SESSIONING && recvData->GetOpcode() == CMSG_GATE_2_SCENE_PING)
	{
		sLog.outColor(TYELLOW, "Gateway Ping delay %d sec\n", time(NULL) - m_lastPingRecvUnixTime);
		m_lastPingRecvUnixTime = time(NULL);
	}*/

	if(recvData->GetServerOpCode() != CMSG_GATE_2_SCENE_SESSIONING && 
		(recvData->GetOpcode() == CMSG_GATE_2_SCENE_ENTER_GAME_REQ || recvData->GetOpcode() == CMSG_GATE_2_SCENE_CLOSE_SESSION_REQ))
	{
		_lowrecvQueue.Push(recvData);
	}
	else
		_recvQueue.Push(recvData);
}

void CSceneCommServer::HandlePing(WorldPacket &recvData)
{
	WorldPacket pck(SMSG_SCENE_2_GATE_PONG, 20);
	uint32 SessionCount = sPark.GetSessionCount();
	pck << sPark.GetSessionCount();		// 返回本服务器在线人数
	SendPacket( &pck, 0,0);

	uint32 curTime = time(NULL);
	int32 diff = curTime - m_lastPingRecvUnixTime - SERVER_PING_INTERVAL;
	//本地数据包队列处理时间过长，PONG包返回是否直接放到网络线程接收的地方？
	if(diff >= 2)
		sLog.outString("GameServer Pong delay %d sec", diff);

	m_lastPingRecvUnixTime = curTime;
	// sLog.outDebug("SceneServer Pong back to %s at %d", registeradderss.c_str(), m_lastPingRecvUnixTime);
}

void CSceneCommServer::HandleGameServerConnectionStatus(WorldPacket &recvData)
{
	uint32 serverCounter;
	recvData >> serverCounter;
	for (int i = 0; i < serverCounter; ++i)
	{

	}
}

void CSceneCommServer::HandleGatewayAuthReq(WorldPacket &recvData)
{
	sLog.outColor(TGREEN, "游戏服务器 接收到 网关服务器 验证请求..packetSize=%d\n", recvData.size());
	unsigned char key[20];
	uint32 result = 1;
	recvData.read(key, 20);

	uint32 myid;
	recvData >> myid;

	sGateCommHandler.serverIDLock.Acquire();
	if(sGateCommHandler.m_sceneServerID != 0 && sGateCommHandler.m_sceneServerID != myid)
	{
		sLog.outError("网关服务器意图连接的游戏服务器编号出错（localSrvIndex=%u, recvIndex=%u）！", sGateCommHandler.m_sceneServerID, myid);
		result = 0;
	}
	sGateCommHandler.serverIDLock.Release();

	if(myid==0)
	{
		sLog.outError("游戏服务器的ID没有指定（必须大于1）");
		result = 0;
	}
	// check if we have the correct password
	SHA_1 sha1;
	int sha1Result = sha1.SHA1Result(&sGateCommHandler.m_gatewayAuthSha1Data, key);
	if(sha1Result != shaSuccess)
	{
		result = 0;
		sLog.outString("ERROR:网关的Key不匹配，拒绝连接,sha1Result=%d", sha1Result);
	}

	sLog.outString("Authentication request from %s, result %s.",(char*)inet_ntoa(m_peer.sin_addr), result ? "OK" : "FAIL");

	printf("Key: ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X", key[i]);
	}
	printf("\n");

	/* send the response packet */
	WorldPacket pck( SMSG_SCENE_2_GATE_AUTH_RESULT, 128 );
	pck << result;
	pck << (uint32)0;

	authenticated = result;

	if(authenticated)
	{
		recvData >> registername >> registeradderss >> registerPort >> src_platform_id;

		uint32 my_id = sGateCommHandler.GenerateRealmID();
		sLog.outString("注册网关服务器(平台id=%u,name=%s,address=%s,port=%u) 生成对应ID=%u", src_platform_id, registername.c_str(), registeradderss.c_str(), registerPort, my_id);

		pck << my_id;			// Realm ID
		pck << registername;
		SendPacket(&pck, 0, 0);

		associate_gateway_id = my_id;
		// 将被socket存入平台socket列表中
		sGateCommHandler.RemoveServerSocket(this);
		sGateCommHandler.AddPlatformGatewaySocket(src_platform_id, this);
	}
	else
	{
		SendPacket( &pck,0,0 );
	}
}

void CSceneCommServer::HandleRequestEnterGame(WorldPacket &recvData)
{
	cLog.Success("CSceneCommServer", "收到请求进入游戏的消息..\n");
	sPark.CreateSession(this, recvData);
}

void CSceneCommServer::HandleCloseSession(WorldPacket &recvData) // gateserver要求移除对话
{
	uint64 plrGuid;
	uint32 platformID, sessionindex;
	recvData >> platformID >> plrGuid >> sessionindex;
	sPark.InvalidSession(plrGuid, sessionindex);

	sLog.outString("gate服务器通知关闭来自平台%u的玩家会话SessionIndex=%u,guid="I64FMTD"", platformID, sessionindex, plrGuid);
}

void CSceneCommServer::HandleGameSessionPacket(WorldPacket* pRecvData)
{
	uint32 opCode = pRecvData->GetOpcode();
	sPark.SessioningProc(pRecvData, pRecvData->GetUserGUID());
}

//void CSceneCommServer::HandleObjectDataSyncNotice(WorldPacket &recvData)
//{
//	uint32 undef, platformServerID;
//	recvData >> undef >> undef >> platformServerID;		// >> 场景服id >> accountid
//	uint16 playerCounter, dataEndPos = 0;
//	recvData >> playerCounter;
//	uint32 plrGuid, syncCount;
//	uint8 syncTypeID;
//}

void CSceneCommServer::HandleServerCMD( WorldPacket& packet )
{
	std::string cmd;
	int32 p1,p2,p3;
	packet>>cmd>>p1>>p2>>p3;
	if(cmd=="SHUTDOWN")
	{
		m_ShutdownTimer = p1;
		m_ShutdownEvent = true;
		sLog.outString("%u秒后关闭服务器",m_ShutdownTimer/1000);
	}
	else if(cmd=="CANCLE_SHUTDOWN")
	{
		m_ShutdownTimer = p1>10000?p1:m_ShutdownTimer;
		m_ShutdownEvent = false;
		sLog.outString("关闭服务器指令中止！",m_ShutdownTimer/1000);
	}
	else if(cmd=="INPUT")
	{
		CUpdateThread* prun = (CUpdateThread*)sThreadMgr.GetThreadByType(THREADTYPE_CONSOLEINTERFACE);
		if(prun!=NULL)
			prun->m_input=p3;
	}
	else if (cmd=="RELOAD")
	{
		g_sReloadStaticDataBase = true;
	}
}

//void CSceneCommServer::HandleMasterServerPlayerMsgEvent(WorldPacket &recvData)
//{
//	uint32 account_id, platformServerID, player_id, op_code;
//	recvData >> account_id >> account_id >> platformServerID;
//	GameSession *pSession = sPark.FindSession(player_id);
//	if (pSession == NULL)
//		return ;
//}
//
//void CSceneCommServer::HandleMasterServerSystemMsgEvent(WorldPacket &recvData)
//{
//	uint32 serverIndex;
//	recvData >> serverIndex;
//	uint16 opCode;
//	recvData >> opCode;
//
//}
//
//void CSceneCommServer::HandleSceneServerSystemMsgEventFromOtherScene(WorldPacket &recvData)
//{
//	uint32 serverIndex, opCode;
//	recvData >> serverIndex >> serverIndex >> opCode;
//	// 不处理自身传回的消息
//	if (serverIndex == sGateCommHandler.GetSelfSceneServerID())
//		return ;
//
//}

