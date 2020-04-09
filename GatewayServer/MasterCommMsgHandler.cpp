#include "stdafx.h"
#include "threading/UserThreadMgr.h"
#include "MasterCommClient.h"
#include "SceneCommHandler.h"
#include "GameSocket.h"
#include "UpdateThread.h"

extern uint32 m_ShutdownTimer;

void CMasterCommClient::HandlePong(WorldPacket *packet)
{
	latency_ms = getMSTime64() - ping_ms_time;
	if(latency_ms)
	{
		if(latency_ms>1000)
			sLog.outError("中央服务器ping时间太长, %u ms", latency_ms);
	}
	sLog.outDebug(">> masterserver latency: %ums", latency_ms);
	m_lastPongUnixTime = time(NULL);
}

void CMasterCommClient::HandleAuthResponse(WorldPacket *packet)
{
	uint32 result;
	(*packet) >> result;
	sLog.outColor(TYELLOW, "%d", result);
	if(result != 1)
		authenticated = 0xFFFFFFFF;
	else
	{
		authenticated = 1;
		uint32 error;
		string servername;
		(*packet) >> error >> m_nThisGateId >> servername;
		sLog.outColor(TNORMAL, "\n        >> 中央服务器 `%s` 注册的id为 ", servername.c_str());
		sLog.outColor(TGREEN, "%u", m_nThisGateId);
		// 在handler出注册该id
		sSceneCommHandler.AdditionAckFromCentre(m_nThisGateId);
	}
}

void CMasterCommClient::HandleCloseGameSocket(WorldPacket *packet)
{
	uint64 guid;
	uint32 sessionindex;
	bool direct;
	*packet >> guid >> sessionindex >> direct;
	sLog.outString("[会话关闭]中央服通知关闭GameSock,guid="I64FMTD",SessionIndex=%u,direct=%d", guid, sessionindex, direct);
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID(guid);
	if (client == NULL)
	{
		sLog.outError("未找到guid对应的GameSocket对象,会话关闭失败");
		return ;
	}
	if (sessionindex != client->m_SessionIndex)
	{
		sLog.outError("GameSocket当前sessionIndex=%u与参数index不匹配,会话关闭失败", client->m_SessionIndex);
		return ;
	}
	
	if(direct)		
	{
		// 直接关闭
		client->m_AuthStatus = GSocket_auth_state_ret_failed;
		client->Disconnect();
	}
	else
	{
		// 通知关闭
		if (client->m_uSceneServerID > 0)
		{
			sSceneCommHandler.SendCloseSessionNoticeToScene(client->m_uSceneServerID, guid, sessionindex);
			sSceneCommHandler.SendCloseServantNoticeToMaster(guid, sessionindex, false);
		}
		else
		{
			sLog.outError("[会话关闭]Warning:通知关闭会话时,用户guid="I64FMTD"并不处于任一场景服中", guid);
			// 直接关闭中央服的servant
			sSceneCommHandler.SendCloseServantNoticeToMaster(guid, sessionindex, true);
		}
	}
}

void CMasterCommClient::HandleChooseGameRoleResponse(WorldPacket *packet)
{
	uint64 tempGuid, realGuid = 0;
	uint32 targetSceneID = 0;
	(*packet) >> tempGuid >> realGuid >> targetSceneID;

	GameSocket * pSock = sSceneCommHandler.GetSocketByPlayerGUID( tempGuid );
	if ( pSock == NULL )
		return;
	
	// 临时guid不等于真正guid,才进行guid的替换
	if (tempGuid != realGuid)
		sSceneCommHandler.UpdateGameSocektGuid(tempGuid, realGuid, pSock);

	if (packet->GetErrorCode() != 0)
	{
		// 选角出错,直接返回给客户端
	}
	else
	{
		// 通知场景服进入游戏
		sSceneCommHandler.AddNewPlayerToSceneServer( pSock, packet, targetSceneID );
	}
}

void CMasterCommClient::HandlePlayerGameResponse(WorldPacket *packet)
{
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID(packet->m_nUserGUID);
	if (client != NULL && client->IsConnected())
	{
		// sLog.outDebug("[玩家响应包]发送包至玩家%s, op=%u, size=%u", client->m_strAccountName.c_str(), packet->GetOpcode(), packet->size());
		client->SendPacket(packet);
	}
}

void CMasterCommClient::HandleChangeSceneServerMasterNotice(WorldPacket *packet)
{
	uint64 plrGuid = 0;
	uint32 targetSceneServerID = 0, targetMapID = 0;
	float posX = 0.0f, posY = 0.0f, height = 0.0f;
	(*packet) >> plrGuid >> targetSceneServerID >> targetMapID >> posX >> posY >> height;

	GameSocket * pSock = sSceneCommHandler.GetSocketByPlayerGUID( plrGuid );
	if ( pSock == NULL )
		return;

	// 通知场景服进入游戏
	sSceneCommHandler.AddNewPlayerToSceneServer( pSock, packet, targetSceneServerID );

	// 通知玩家开始loading
	pSock->SendChangeSceneResult(0, targetMapID, posX, posY, height);
}

void CMasterCommClient::HandleEnterInstanceNotice(WorldPacket *packet)
{
	// 玩家请求进入副本或系统实例，需要在此处让gamesocket进入包积压状态
	//uint32 curSceneIndex, accID;
	//(*packet) >> curSceneIndex >> accID;
	////
	//GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( accID );
	//if (client != NULL)
	//{
	//	client->TryToEnterBackLogState();
	//	packet->SetServerAccountID(accID);
	//	// 将数据包发送至对应的场景服
	//	sSceneCommHandler.SendPacketToGameServer( packet, curSceneIndex );
	//}
}

void CMasterCommClient::HandleExitInstanceNotice(WorldPacket *packet)
{
	//uint32 curSceneIndex, accID;
	//(*packet) >> curSceneIndex >> accID;
	////
	//GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( accID );
	//if (client != NULL)
	//{
	//	client->TryToEnterBackLogState();
	//	packet->SetServerAccountID(accID);
	//	// 将数据包发送至对应的场景服
	//	sSceneCommHandler.SendPacketToGameServer( packet, curSceneIndex );
	//}
}

void CMasterCommClient::HandleSendGlobalMsg(WorldPacket *packet)
{
	sSceneCommHandler.SendGlobaleMsg(packet);
}

void CMasterCommClient::HandleSendPartialGlobalMsg(WorldPacket *packet)
{
	sSceneCommHandler.SendGlobalMsgToPartialPlayer(*packet);
}

void CMasterCommClient::HandleTranferPacket( WorldPacket * packet )
{
	uint64 userguid = 0;
	uint32 curLine = 0;
	*packet  >> curLine >> userguid;

	if ( curLine != 0 )
	{
		if (curLine == 0xffffffff)
		{
			DEBUG_LOG("转发消息", "转发消息到所有游戏服");
		}
		packet->SetUserGUID(userguid);
		sLog.outDebug("GateWay Trans Server Packet(%u, size=%u) to LineID %u", uint32(packet->GetServerOpCode()), (uint32)(packet->size()), curLine);
		sSceneCommHandler.SendPacketToSceneServer( packet, curLine );
	}
}

void CMasterCommClient::HandleBlockChat( WorldPacket * packet )
{
	uint64 userguid = 0;
	uint32 block = 0;
	*packet >> userguid >> block;
	GameSocket * client = sSceneCommHandler.GetSocketByPlayerGUID( userguid );
	if ( client == NULL )
		return;

	client->m_nChatBlockedExpireTime = block;
}

void CMasterCommClient::HandleServerCMD( WorldPacket * packet )
{
	std::string cmd,s1,s2;
	int32 p1,p2,p3;
	(*packet)>>cmd>>p1>>p2>>p3>>s1>>s2;
	//if(cmd=="SHUTDOWN")
	//{
	//	m_ShutdownTimer = p1>=10000?p1+15000:m_ShutdownTimer+15000;
	//	m_ShutdownEvent = true;

	//}
	//if(cmd=="CANCLE_SHUTDOWN")
	//{
	//	m_ShutdownTimer = p1>10000?p1:m_ShutdownTimer;
	//	m_ShutdownEvent = false;
	//	WorldPacket data(SMSG_SERVER_MESSAGE,20);
	//	data << uint32(SERVER_MSG_SHUTDOWN_CANCELLED);
	//	sSceneCommHandler.SendGlobaleMsg(&data);
	//}
	//if ( cmd == "OPCODEBAN" )
	//{
	//	if ( p1 < NUM_MSG_TYPES )
	//	{
	//		GameSocket::m_packetforbid[p1] = 1;
	//		sLog.outString( "数据包%u被屏蔽", p1 );
	//	}
	//	return;
	//}
	//if(cmd=="INPUT")
	//{
	//	if(p1==1)
	//	{
	//		if(p2==sSceneCommHandler.GetStartParmInt("id"))
	//		{
	//			CUpdateThread* prun = (CUpdateThread*)sThreadMgr.GetThreadByType(THREADTYPE_CONSOLEINTERFACE);
	//			if(prun!=NULL)
	//				prun->m_input=p3;
	//		}
	//	}
	//	else if(sSceneCommHandler.GetStartParmInt("id")==0)
	//	{
	//		sSceneCommHandler.SendPacketToGameServer(packet, p2);
	//	}
	//	return;
	//	//此命令不需要转发给所有服务器
	//}
	//if(sSceneCommHandler.GetStartParmInt("id")==0)
	//{
	//	sSceneCommHandler.SendPacketToGameServer(packet, 0xFFFFFFFF);
	//}
}

