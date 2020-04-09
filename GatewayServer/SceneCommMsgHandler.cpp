#include "stdafx.h"
#include "SceneCommClient.h"
#include "SceneCommHandler.h"
#include "FloatPoint.h"

void CSceneCommClient::HandleAuthResponse(WorldPacket *pPacket)
{
	uint32 result;
	(*pPacket) >> result;
	sLog.outColor(TYELLOW, "%d", result);

	if(result != 1)
	{
		authenticated = 0xFFFFFFFF;
	}
	else
	{
		uint32 realmlid;
		uint32 error;
		string realmname;
		(*pPacket) >> error >> realmlid >> realmname;

		sLog.outColor(TNORMAL, "\n        >> 场景服务器 `%s` 注册的id为 ", realmname.c_str());
		sLog.outColor(TGREEN, "%u", realmlid);

		sSceneCommHandler.AdditionAck(_id, realmlid);//_id为游戏服务器的id，realmlid是本gateserver在游戏服务器上注册的id。

		authenticated = 1;
	}
}

void CSceneCommClient::HandlePong(WorldPacket *pPacket)
{
	uint32 count;
	*pPacket >> count;		// 从游戏服得到它的在线人数

	sSceneCommHandler.SetGameServerOLPlayerNums(_id, count);
	latency_ms = getMSTime64() - ping_ms_time;

	if(latency_ms > 1000)
		sLog.outError("游戏服务器%d线ping时间太长,%ums", _id, latency_ms);

	m_lastPongUnixtime = time(NULL);
	sLog.outDebug(">> scene server %d latency: %ums", _id, latency_ms);
}

void CSceneCommClient::HandleGameSessionPacketResult(WorldPacket *pPacket)
{
	// 在此函数中，获取游戏服务器反馈给某玩家的消息
	// 并且将该玩家找出来，发送游戏消息给他
	// 可以取出用户的AccountID，找出对应的GameSocket套接字
	// 而后直接调用GameSocket的SendPacket即可
	uint64 userGuid = pPacket->GetUserGUID();
	GameSocket *pSocket = sSceneCommHandler.GetSocketByPlayerGUID(userGuid);
	if (pSocket != NULL && pSocket->m_uSceneServerID == this->_id)//贡文斌 3.30 防止数据包发给没进入这线的玩家
	{
		pSocket->SendPacket(pPacket);
	}
	else
	{
		if (pSocket != NULL)
		{
			// 查看其他玩家信息的包，是可以跨线发送的
			/*if (pPacket->GetOpcode() == SMSG_FRIEND_ROLE_PANEL)
			pSocket->SendPacket(pPacket);
			else*/
				sLog.outError("客户端socket 用户id="I64FMTD" 不在 %d 线, 而在 %d 线,无法接收消息包 pck_OP=%u", userGuid, _id, pSocket->m_uSceneServerID, pPacket->GetOpcode());
		}
		else
			sLog.outDebug("客户端socket已经断开 用户id="I64FMTD" 不在%d线, 无法接收消息包 pck_OP=%u", userGuid, _id, pPacket->GetOpcode());
	}
}

void CSceneCommClient::HandleSendGlobalMsg(WorldPacket *pPacket)		// gameserver要求发送全局消息
{
	WorldPacket pck(*pPacket);
	sSceneCommHandler.SendGlobaleMsg(&pck);		
}

void CSceneCommClient::HandleEnterGameResult(WorldPacket *pPacket)
{
	uint64 guid = 0;
	uint32 enterRet = 0;
	
	(*pPacket) >> guid >> enterRet;
	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( guid );
	if ( client == NULL )
		return ;

	if ( enterRet != 0 )
	{
		// 登陆失败,写错误码发送给玩家
		pPacket->SetOpcode( SMSG_001010 );
		client->SendPacket( pPacket );
	}
	else
	{
		string roleName;
		uint32 sceneServerID = 0, mapId = 0, enterTime = 0, career = 0, level = 0, maxHp = 0, curHp = 0;
		FloatPoint curPos;
		float height = 0.0f;
		(*pPacket) >> sceneServerID >> enterTime >> roleName >> career >> level >> maxHp >> curHp >> mapId >> curPos.m_fX >> curPos.m_fY >> height;

		// 找出该客户端套接字并发送出去
		pbc_wmessage *msg1010 = sPbcRegister.Make_pbc_wmessage("UserProto001010");
		sPbcRegister.WriteUInt64Value(msg1010, "roleGuid", guid);
		pbc_wmessage_string(msg1010, "roleName", roleName.c_str(), 0);
		pbc_wmessage_integer(msg1010, "career", career, 0);
		pbc_wmessage_integer(msg1010, "level", level, 0);
		pbc_wmessage_integer(msg1010, "curHP", curHp, 0);
		pbc_wmessage_integer(msg1010, "maxHp", maxHp, 0);
		pbc_wmessage_integer(msg1010, "mapID", mapId, 0);
		pbc_wmessage_real(msg1010, "curPosX", curPos.m_fX);
		pbc_wmessage_real(msg1010, "curPosY", curPos.m_fY);
		pbc_wmessage_real(msg1010, "curPosHeight", height);

		WorldPacket packet(SMSG_001010, 128);
		sPbcRegister.EncodePbcDataToPacket(packet, msg1010);
		client->SendPacket( &packet );
		DEL_PBC_W(msg1010);

		sLog.outString("通知 玩家guid="I64FMTD" 成功进入场景服 %u(地图%u: %.1f,%.1f)", guid, sceneServerID, mapId, curPos.m_fX, curPos.m_fY);
	
		//// 通知中央服玩家进入了游戏
		WorldPacket sendData( CMSG_GATE_2_MASTER_ENTER_SCENE_NOTICE, 128 );
		sendData << guid << sceneServerID << enterTime << mapId << curPos.m_fX << curPos.m_fY << client->GetPlayerIpAddr();
		sSceneCommHandler.SendPacketToMasterServer( &sendData );

		// 如果处于包积压状态，则退出
		if (client->InPacketBacklogState())
			client->TryToLeaveBackLogState();
	}
}

void CSceneCommClient::HandleChangeSceneServerNotice(WorldPacket *pPacket)
{
	uint64 guid = 0;
	uint32 ret = 0;
	(*pPacket) >> guid >> ret;

	GameSocket *client = sSceneCommHandler.GetSocketByPlayerGUID( guid );
	if (client == NULL)
		return ;

	switch (ret)
	{
	case TRANSFER_ERROR_NONE:
		{
			// 切换成功, 直接通知客户端进行loading
			uint32 targetMapId = 0;
			float posX = 0.0f, posY = 0.0f, height = 0.0f;
			(*pPacket) >> targetMapId >> posX >> posY >> height;
			
			client->SendChangeSceneResult(0, targetMapId, posX, posY, height);
			client->TryToLeaveBackLogState();
		}
		break;
	case TRANSFER_ERROR_NONE_BUT_SWITCH:
		{
			// 需要切换场景服进程,暂时不通知客户端,先通知中央服进程
			WorldPacket centrePacket(CMSG_GATE_2_MASTER_CHANGE_SCENE_NOTICE, 32);
			centrePacket << guid << uint32(UNIXTIME);
			sSceneCommHandler.SendPacketToMasterServer(&centrePacket);
			client->TryToEnterBackLogState();
		}
		break;
	default:
		{
			// 场景切换有错,直接返回给客户端
			client->SendChangeSceneResult(ret);
			client->TryToLeaveBackLogState();
		}
		break;
	}
}

void CSceneCommClient::HandleSavePlayerData(WorldPacket *pPacket)
{
	//uint32 srcPlatformID, srcServerIndex, accountID, sessionIndex,nextServerIndex;
	//(*pPacket) >> srcPlatformID >> srcServerIndex >> srcServerIndex >> accountID >> sessionIndex>>nextServerIndex;

	////下个场景为0表示下线
	//if (nextServerIndex == 0)
	//{
	//	sLog.outString("player logout account=%u sessionindex=%u", accountID, sessionIndex);	
	//	GameSocket *client = sSceneCommHandler.GetSocketByAccountID(accountID);
	//	if(client != NULL&&client->_SessionIndex==sessionIndex)
	//	{
	//		client->Authed = false;
	//		client->Disconnect();
	//		sLog.outDebug("receive Save And Logout,accountid＝%u index=%u",accountID,sessionIndex);
	//	}
	//}
	////转发到中央服
	//sSceneCommHandler.SendPacketToMasterServer( pPacket, 0, pPacket->GetServerOpCode() );
}

void CSceneCommClient::HandleTransferPacket(WorldPacket *pPacket)//处理游戏服务器转发的包
{
	sSceneCommHandler.g_transpacketnum++;;
	sSceneCommHandler.g_transpacketsize += pPacket->size();

	uint32 targetserver = 0;
	(*pPacket) >> targetserver;

	if ( targetserver == MASTER_SERVER_ID ) //发到中央服务器的
	{
		sSceneCommHandler.SendPacketToMasterServer( pPacket );
	}
	else
	{
		sSceneCommHandler.SendPacketToSceneServer( pPacket, targetserver );
	}
}

