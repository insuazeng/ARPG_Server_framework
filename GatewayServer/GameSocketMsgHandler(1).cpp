#include "stdafx.h"
#include "GameSocket.h"
#include "LogonCommHandler.h"

void GameSocket::_HandleUserAuthSession(WorldPacket* recvPacket)
{
	cLog.Notice("测试", "接收到来自客户端的登录请求..\n");

	// 接收账号，sessionkey
	uint32 loginType;
	string accountName, authKey, clientVersion, agentName, netType, sysVersion, sysModel, phoneMac;
	
	pbc_rmessage *msg1003 = sPbcRegister.Make_pbc_rmessage("UserProto001003", (void*)recvPacket->contents(), recvPacket->size());
	if (msg1003 == NULL)
		return ;

	loginType = pbc_rmessage_integer(msg1003, "loginType", 0, NULL);
	accountName = pbc_rmessage_string(msg1003, "accountName", 0, NULL);
	authKey = pbc_rmessage_string(msg1003, "gateServerAuthKey", 0, NULL);
	clientVersion = pbc_rmessage_string(msg1003, "clientVersion", 0, NULL);
	agentName = pbc_rmessage_string(msg1003, "agentName", 0, NULL);
	netType = pbc_rmessage_string(msg1003, "netType", 0, NULL);
	sysVersion = pbc_rmessage_string(msg1003, "sysVersion", 0, NULL);
	sysModel = pbc_rmessage_string(msg1003, "sysModel", 0, NULL);
	phoneMac = pbc_rmessage_string(msg1003, "phoneMac", 0, NULL);

	sLog.outString("用户请求会话验证,accName=%s,loginType=%u,authKey=%s", accountName.c_str(), loginType, authKey.c_str());

	switch (loginType)
	{
	case 1:			// 初次登陆
		{
			m_strAuthSessionKey = authKey;
			// Send out a request for this account.
			m_nAuthRequestID = sLogonCommHandler.GenerateRequestID();
			if(m_nAuthRequestID == ILLEGAL_DATA_32 || !sLogonCommHandler.ClientConnected(accountName, this))
			{
				sLog.outError("无法发送客户端验证请求,即将断开(帐号=%s,authReqID=%u", accountName.c_str(), m_nAuthRequestID);
				Disconnect();
				return;
			}
		}
		break;
		//case 2:			// 断线重登
		//	{
		//		string strToken;
		//		try
		//		{
		//			*recvPacket >> strToken;
		//		}
		//		catch(ByteBuffer::error &)
		//		{
		//			sLog.outError("Incomplete copy of AUTH_SESSION recieved. accName=%s,login_type=1", account_name.c_str());
		//			Disconnect();
		//			return;
		//		}

		//		sLog.outString("[会话验证] 账号 %s 请求进行断线重登,recv_token=%s", account_name.c_str(), strToken.c_str());
		//		// 请求网关核心模块进行处理
		//		int32 nRet = sSceneCommHandler.CheckAccountReloginValidation(account_name, strToken);
		//		if (nRet == 0)
		//		{
		//			tagGameSocketCacheData *data = sSceneCommHandler.GetCachedGameSocketData(account_name);
		//			ASSERT(data != NULL);

		//			uint32 index = data->session_index;
		//			uint32 platformID = data->platform_id;
		//			uint32 accID = data->acc_id;
		//			uint32 gmFlag = data->gm_flag;
		//			string existToken = data->token_data;
		//			// 获取后将该信息移除掉
		//			sSceneCommHandler.RemoveReloginAccountData(account_name);

		//			SessionAuthedSuccessCallback(index, platformID, accID, account_name, gmFlag, existToken);
		//		}
		//		else
		//		{
		//			sLog.outError("[会话令牌]验证失效(accName=%s),RET=%d", account_name.c_str(), nRet);
		//			// 断线重登失败，需要重走正式的登陆流程
		//			SessionAuthedFailCallback(GameSock_auth_ret_err_need_relogin);
		//		}
		//	}
		//	break;
	default:
		{
			sLog.outError("Incomplete copy of AUTH_SESSION recieved.accName=%s,login_type=%u", accountName.c_str(), loginType);
			Disconnect();
			return;
		}
		break;
	}

	// 此处赋值一下账号名,代理渠道
	m_strAccountName = accountName;
	m_strClientAgent = agentName;

	DEL_PBC_R(msg1003);
}

// 处理玩家ping消息的函数
void GameSocket::_HandlePing(WorldPacket* recvPacket)
{
	pbc_rmessage *msg1011 = sPbcRegister.Make_pbc_rmessage("UserProto001011", (void*)recvPacket->contents(), recvPacket->size());
	if (msg1011 == NULL)
		return ;
	
	uint64 pingSendTime = sPbcRegister.ReadUInt64Value(msg1011, "pingSendTime");
	m_lastPingRecvdMStime = getMSTime64();

	pbc_wmessage *msg1012 = sPbcRegister.Make_pbc_wmessage("UserProto001012");
	sPbcRegister.WriteUInt64Value(msg1012, "pingSendTime", pingSendTime);
	sPbcRegister.WriteUInt64Value(msg1012, "pingRecvTime", m_lastPingRecvdMStime);

	WorldPacket packet(SMSG_001012, 16);
	sPbcRegister.EncodePbcDataToPacket(packet, msg1012);
	SendPacket(&packet);
	
	//ping的时候频率检测计数清零
	m_checkcount = 0;

	DEL_PBC_R(msg1011);
	DEL_PBC_W(msg1012);
}

void GameSocket::_HandlePlayerSpeek(WorldPacket *recvPacket)
{
	//if ( m_nChatBlockedExpireTime > 0 && UNIXTIME < m_nChatBlockedExpireTime )
	//{
	//	WorldPacket packet( SMSG_NOTIFICATION );
	//	packet << "";//禁言发空消息
	//	SendPacket( &packet );
	//	return;
	//}

	//uint8 chat_type;
	//(*recvPacket) >> chat_type;

	//if (chat_type < CHAT_TYPE_COUNT)
	//{
	//	if (m_uSceneServerID > 0)	// 玩家确实处于某个场景服内，发送数据
	//	{
	//		recvPacket->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + recvPacket->GetOpcode());
	//		sSceneCommHandler.SendPacketToGameServer(recvPacket, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//	}
	//}
}

void GameSocket::_HookRecvedWorldPacket(WorldPacket *recvPacket)
{
	const uint32 op = recvPacket->GetOpcode();
	switch (op)
	{
	case CMSG_002005:		// 请求进行场景传送（需要进入包阻塞队列）
		{
			/*pbc_rmessage *msg2005 = sPbcRegister.Make_pbc_rmessage("UserProto002005", (void*)recvPacket->contents(), recvPacket->size());
			if (msg2005 == NULL)
				return ;
			uint32 destScene = pbc_rmessage_integer(msg2005, "destSceneID", 0, NULL);
			uint32 destPosX = pbc_rmessage_integer(msg2005, "destPosX", 0, NULL);
			uint32 destPosY = pbc_rmessage_integer(msg2005, "destPosY", 0, NULL);*/
		}
		break;
	default:
		break;
	}
}

bool GameSocket::_HookSendWorldPacket(WorldPacket* recvPacket)
{
	//switch(recvPacket->GetOpcode())
	//{
	//	//客户端的这两个包有线程访问冲突的问题，未修改好之前不使用
	//case SMSG_ENTERGAME:
	//	{
	//		mClientStatus = eCSTATUS_ENTERGAME_SUCESS;
	//		break;
	//	}
	//case SMSG_CHARINFO:
	//	{
	//		mClientStatus = eCSTATUS_SEND_CHARINFO;
	//		break;
	//	}
	//case CMSG_CHAR_CREATE:
	//	{
	//		mClientStatus = eCSTATUS_SEND_CREATECHAR;
	//		break;
	//	}
	//case MSG_NULL_ACTION://用于接收客户端发送的状态
	//	{
	//		(*recvPacket) >> mClientLog;
	//		if(mClientLog.find("exploreos")!=string::npos)
	//		{
	//			mClientEnv=mClientLog;
	//		}
	//		else if(mClientLog.find("error")!=string::npos)
	//		{
	//			Crash_Log->AddLineFormat("账号%u 客户端错误:%s",m_PlayerGUID,mClientLog.c_str());
	//		}
	//		break;
	//	}
	//default:
	//	return false;
	//}

	return true;
}
