#include "stdafx.h"

#include "md5.h"
#include "Util.h"
#include "zlib/zlib.h"
#include "ServerCommonDef.h"
#include "GameSocket.h"
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"
#include "OpDispatcher.h"

extern bool m_ShutdownEvent;
extern GatewayServer g_local_server_conf;

#define WORLDSOCKET_SENDBUF_SIZE 32768
#define WORLDSOCKET_RECVBUF_SIZE 4096
#define MAX_CLIENTPACKET 2048

enum PacketTransferDest
{
	Transfer_dst_gateway	= 0x01,
	Transfer_dst_master		= 0x02,
	Transfer_dst_scene		= 0x04
};

//#define WORLDSOCKET_SENDBUF_SIZE 65535
//#define WORLDSOCKET_RECVBUF_SIZE 65535
//#define MAX_CLIENTPACKET 65535

//uint8 GameSocket::m_transpacket[NUM_MSG_TYPES];
//uint32 GameSocket::m_packetfrequency[NUM_MSG_TYPES];
//uint8 GameSocket::m_packetforbid[NUM_MSG_TYPES];
//uint32 GameSocket::m_disconnectcount = 0;

void GameSocket::InitTransPacket(void)
{
	//memset(m_transpacket, PROC_GAME, sizeof(uint8) * NUM_MSG_TYPES);
	//memset(m_packetforbid, 0, sizeof(uint8) * NUM_MSG_TYPES);

	////这几个数据包需要根据玩家状态来决定如何分发
	//m_transpacket[MSG_NULL_ACTION] = PROC_GATEWAY;

	////收包频率设置，操作复杂的数据包，特别是对数据库有操作，间隔需要很大，每次登陆只有一次的，设置成0xFFFFFFFF
	//memset(m_packetfrequency, 0, sizeof(m_packetfrequency));

	//m_packetfrequency[CMSG_PING]			= 2000;
	//m_packetfrequency[CMSG_AUTH_SESSION]	= 5000;
	//m_packetfrequency[CMSG_MSG]				= 1000;
	//m_packetfrequency[CMSG_ENTERGAME]		= 0xFFFFFFFF;

	//m_packetfrequency[CMSG_CHAR_CREATE]		= 2000;
	//m_packetfrequency[CMSG_GET_CHARINFO]	= 2000;
	//m_packetfrequency[CMSG_PLAYER_PANEL_VIEW_REQ]= 1000;

}

string GameSocket::GenerateGameSocketLoginToken(const string &agent, const string &accName)
{
	char szData[512] = { 0 };
	uint32 rand_val = (rand() % 0xFF) * (rand() % 0xFF) * (rand() % 0xFF);
	uint32 time_val = UNIXTIME + getMSTime32();
	snprintf(szData, 512, "%s:%s:%u:%u", agent.c_str(), accName.c_str(), rand_val, time_val);
	string strData = szData;
	MD5 md5;
	md5.update(strData);

	return md5.toString();
}

uint64 GameSocket::GeneratePlayerHashGUID(const string& agent, const string& accName)
{
	string str = agent + accName;
	return BRDR_Hash(str.c_str());
}

GameSocket::GameSocket(SOCKET fd,const sockaddr_in * peer) : TcpSocket(fd, WORLDSOCKET_RECVBUF_SIZE, WORLDSOCKET_SENDBUF_SIZE, true, peer, 
																	RecvPacket_head_user2server, SendPacket_head_server2user, PacketCrypto_none, PacketCrypto_none)
{
	m_AuthStatus = GSocket_auth_state_none;
	
	m_lastPingRecvdMStime = getMSTime64();
	m_nSocketConnectedTime = m_lastPingRecvdMStime;
	m_ReloginNextMsTime = 0;
	m_SessionIndex = 0;
	m_nAuthRequestID = 0;
	m_PlayerGUID = 0;
	m_GMFlag = 0;
	m_uSceneServerID = 0;
	m_allowConnect = true;
	m_inPckBacklogState = false;

	m_nChatBlockedExpireTime = 0;

	if(!sSceneCommHandler.OnIPConnect(GetIP()))
	{
		m_allowConnect = false;
	}
#ifdef USE_FSM
	_sendFSM.setup(2059198199, 1501);
	_receiveFSM.setup(2059198199, 1501);
#endif
}

GameSocket::~GameSocket(void)
{

}

void GameSocket::SendPacket(WorldPacket * data)
{
	if(!data)
		return;

	if(!IsConnected())
		return;

	_HookSendWorldPacket(data);
	const uint32 opCode = data->GetOpcode();
	const uint32 packSize = data->size();
	
	//++sSceneCommHandler.m_clientPacketCounter[opCode];

	if (packSize >= 10240)		// 大于10KB的话，需要压缩后进行发送
	{
		if (_OutPacketCompress(opCode, packSize, data->GetErrorCode(), (const void*)data->contents()))
			return ;
	}

	const uint32 ret = SendingData((packSize > 0 ? (void*)data->contents() : NULL), packSize, opCode, 0, 0, data->GetErrorCode(), 0);
	switch (ret)
	{
	case Send_tcppck_err_none:
		sLog.outString("[GameSocket发包] 实际发送 (plrGuid="I64FMTD"),opcode=%u,len=%u,err=%u)", m_PlayerGUID, opCode, packSize, data->GetErrorCode());
		sSceneCommHandler.g_totalSendBytesToClient += packSize;
		if (m_beStuck)
		{
			m_beStuck = false;
			sLog.outString("[GameSock]角色(accName=%s)发包缓冲区恢复可用,可用空间%u", m_strAccountName.c_str(), GetWriteBufferSpace());
		}
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			if(!m_beStuck)
			{
				sLog.outError("[GameSock]角色(accName=%s)发包缓冲区用完,剩余空间%u,数据包op=%u 包长%u", m_strAccountName.c_str(), GetWriteBufferSpace(), opCode, packSize);
				m_beStuck = true;
			}
			WorldPacket *pck = NewWorldPacket(data);
			if (packSize > 0)
				pck->append((const uint8*)data, packSize);
			_queue.Push(pck);
		}
		break;
	default:
		sLog.outError("[GameSock]向客户端(accName=%s)发包遇到其他严重问题,ret=%u", m_strAccountName.c_str(), ret);
		break;
	}
}

bool GameSocket::_OutPacketCompress(uint32 opcode, uint32 packSize, uint32 errCode, const void* data)
{
	uint32 destsize = compressBound(packSize);
	uint8 *buffer = new uint8[destsize];
	int cresult = compress(buffer, (uLongf*)&destsize, (const Bytef*)data, packSize);
	if(cresult!= Z_OK)
	{
		delete [] buffer;
		sLog.outError("压缩出错, Opcode =%u Error=%u",opcode,cresult);
		return false;
	}
	else
	{
		sLog.outDebug("包(opcode=%u) 原长%u压缩后长%u", opcode, packSize, destsize);
		sSceneCommHandler.AddCompressedPacketFrenquency(opcode, packSize, destsize);
	}

	// 压缩数据后发送的包，需要在opcode处加一个标记，方便客户端区分
	const uint32 ret = SendingData(buffer, destsize, 1000000 + opcode, 0, 0, errCode, 0);

	//printf("压缩%d---%d\n",len,destsize + 4);
	delete [] buffer;

	return ret == Send_tcppck_err_none ? true : false;
}

void GameSocket::UpdateQueuedPackets()
{
	//queueLock.Acquire();
	if(!_queue.HasItems())
	{
		//queueLock.Release();
		return;
	}

	WorldPacket * pck = NULL;
	while((pck = _queue.front()))
	{
		const uint32 pckSize = pck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pck->contents() : NULL), pckSize, pck->GetOpcode(), 0, 0, pck->GetErrorCode(), 0);
		/* try to push out as many as you can */
		switch(ret)
		{
		case Send_tcppck_err_none:
			{
				sSceneCommHandler.g_totalSendBytesToClient += pckSize;
				DeleteWorldPacket(pck);
				_queue.pop_front();
				if (m_beStuck)
				{
					m_beStuck = false;
					sLog.outString("[GameSock]角色(accName=%s)缓冲区恢复可用,剩余空间:%u", m_strAccountName.c_str(), GetWriteBufferSpace());
				}
			}
			break;
		case Send_tcppck_err_no_enough_buffer:
			{
				if(!m_beStuck)
				{
					sLog.outError("[GameSock]角色(accName=%s)发包缓冲区用完,剩余空间%u,数据包op=%u 包长%u", m_strAccountName.c_str(), GetWriteBufferSpace(), pck->GetOpcode(), pckSize);
					m_beStuck = true;
				}
				sLog.outError("[GameSock]角色(accName=%s)发包缓冲区用完,剩余空间%u,剩余%u个包等下次发", m_strAccountName.c_str(), GetWriteBufferSpace(), _queue.GetElementCount());
				return;
			}
			break;
		default:
			{
				sLog.outError("[GameSock]向客户端(accName=%s)发包遇到其他严重问题,ret=%u,准备清除发送缓冲队列(包%u个)", m_strAccountName.c_str(), ret, _queue.GetElementCount());
				/* kill everything in the buffer */
				while((pck == _queue.Pop()))
				{
					DeleteWorldPacket(pck);
					pck = NULL;
				}
				//queueLock.Release();
				return;
			}
			break;
		}
	}
	//queueLock.Release();
}

void GameSocket::OnUserPacketRecved(WorldPacket *packet)
{
	// 如果同意ip连接太多，该客户端可能不会被注册成功，所以要断开连接
	if(!m_allowConnect)
	{
		DeleteWorldPacket(packet);
		Disconnect();
		return;
	}

	// 对客户端数据包进行处理
	const uint32 op_code = packet->GetOpcode();
	map<uint32, uint32>::iterator it = OpDispatcher::ms_OpcodeTransferDestServer.find(op_code);
	if (it != OpDispatcher::ms_OpcodeTransferDestServer.end())
	{
		if (it->second == Transfer_dst_gateway)
		{
			switch (op_code)
			{
			case CMSG_001003:
				_HandleUserAuthSession(packet);
				break;
			case CMSG_001011:
				_HandlePing(packet);
				break;
			default:
				break;
			}
			DeleteWorldPacket(packet);
		}
		else if (it->second == Transfer_dst_master)
		{
			_HookRecvedWorldPacket(packet);
			// 将包转发至中央服处理
			if (hadSessionAuthSucceeded())
			{
				packet->SetServerOpcodeAndUserGUID(CMSG_GATE_2_MASTER_SESSIONING, GetPlayerGUID());
				sSceneCommHandler.SendPacketToMasterServer(packet);
			}
			else
			{
				sLog.outError("收到未成功验证的账号数据包accName=%s, opcode=%u, size=%u", m_strAccountName.c_str(), op_code, packet->size());
			}
			DeleteWorldPacket(packet);
		}
		else if (it->second == Transfer_dst_scene)
		{
			_HookRecvedWorldPacket(packet);
			// 将包转发至对应的场景服处理
			if (hadSessionAuthSucceeded())
			{
				if (m_uSceneServerID > 0)
				{
					packet->SetServerOpcodeAndUserGUID(CMSG_GATE_2_SCENE_SESSIONING, GetPlayerGUID());
					sSceneCommHandler.SendPacketToSceneServer(packet, m_uSceneServerID);
				}
				else
				{
					sLog.outError("玩家当前不处于任何一场景,发往场景服的协议包被拦截(accName=%s,op=%u,size=%u)", m_strAccountName.c_str(), op_code, packet->size());
				}
			}
			else
			{
				sLog.outError("收到未成功验证的账号数据包accName=%s, opcode=%u, size=%u", m_strAccountName.c_str(), op_code, packet->size());
			}
			DeleteWorldPacket(packet);
		}
		else
		{
			sLog.outError("[GameSocket]accName=%s,agent=%s,Lua包转发注册异常op=%u,dst=%u", m_strAccountName.c_str(), m_strClientAgent.c_str(), op_code, it->second);
			DeleteWorldPacket(packet);
		}
	}
	else
	{
		sLog.outError("[GameSocket]accName=%s,agent=%s 收到异常的消息包op=%u", m_strAccountName.c_str(), m_strClientAgent.c_str(), op_code);
		DeleteWorldPacket(packet);
	}
	//if(m_disconnectcount > 0)
	//	_CheckRevcFrenquency(op_code);

	// sLog.outString("[GameSocket]收到来自账号["I64FMTD"]的消息包,op=%u,size=%u", GetPlayerGUID(), packet->GetOpcode(), packet->size());
	//if ( op_code < NUM_MSG_TYPES )
	//{
	//	sSceneCommHandler.m_client_recv_pack_count[op_code]++;
	//	if ( m_packetforbid[op_code] )					// 如果这个包被屏蔽，则直接删除此包
	//	{
	//		DeleteWorldPacket(Packet);
	//		return;
	//	}
	//	else if (InPacketBacklogState())								// 如果处于包积压状态，则试图放入积压队列中
	//	{
	//		// 放入积压队列，继续收包，判断一下queue的大小
	//		if (m_canPushInBacklogQueue[packet->GetOpcode()] > 0 && _pckBacklogQueue.GetElementCount() < 20)
	//		{
	//			_pckBacklogQueue.Push(packet);
	//			continue ;
	//		}
	//		else
	//		{
	//			DeleteWorldPacket(packet);
	//			continue;
	//		}
	//	}
	//}

	//switch(op_code)
	//{
	//case CMSG_PING:
	//	{
	//		_HandlePing(Packet);
	//		// 每5次ping(150秒)通知一次游戏服,让游戏服保持该会话PlayerEntity的活跃,而不被卸载
	//		if (m_pingCounter >= 5)
	//		{
	//			m_pingCounter = 0;
	//			if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//			{
	//				Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//				sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//			}
	//		}
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_AUTH_SESSION:
	//	{
	//		if ((m_AuthStatus == GSocket_auth_state_none) && (m_nAuthRequestID == 0) && !m_ShutdownEvent)
	//		{
	//			mClientStatus = eCSTATUS_SEND_AUTHREQUEST;
	//			mClientLog = "send auth request";
	//			_HandleAuthSession(Packet);
	//		}
	//		else
	//		{
	//			sLog.outError("重复收到账号 %u 的会话验证请求,status=%u,reqID=%u", m_PlayerGUID, uint32(m_AuthStatus), m_nAuthRequestID);
	//			DeleteWorldPacket(Packet);
	//		}
	//	}
	//	break;
	//case CMSG_ENTERGAME:
	//	{
	//		mClientStatus = eCSTATUS_SEND_ENTERGAME;
	//		mClientLog = "request enter game";
	//		if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//		{
	//			// 发送给游戏服
	//			Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//			sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//		}
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_MSG:
	//	{
	//		if (hadSessionAuthSucceeded())
	//			_HandlePlayerSpeek(Packet);
	//		DeleteWorldPacket(Packet);
	//	}
	//	break;
	//case CMSG_CHAR_CREATE:
	//	{
	//		std::string name;
	//		(*Packet) >> name;
	//	}
	//	break;
	//case CMSG_CHAR_RENAME:
	//	{
	//		std::string newname;
	//		(*Packet) >> newname;
	//		sLog.outString("receive rename request %s",newname.c_str());

	//		// 				if(sLang.containForbidWords(newname.c_str()))   //屏蔽字检查~~
	//		// 				{
	//		// 					WorldPacket packet(SMSG_CHAR_RENAME);
	//		// 					packet<<(uint8)2;
	//		// 					SendPacket(&packet);
	//		// 					sLog.outString("forbid words found %s",newname.c_str());
	//		// 				}
	//		// 				else
	//		// 				{
	//		if (hadSessionAuthSucceeded() && m_uSceneServerID > 0)
	//		{
	//			// 发送给游戏服
	//			Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//			sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//		}

	//		/*				}*/
	//	}
	//	break;
	//default:
	//	{
	//		if (hadSessionAuthSucceeded() && op_code < NUM_MSG_TYPES)
	//		{
	//			//这里根据玩家目前所在服务器和玩家状态，将根据m_transpacket里面的定义，分发数据包到不同服务器 贡文斌 9月4号
	//			// 进行消息包的分发
	//			switch (m_transpacket[op_code])
	//			{
	//			case PROC_GAME:
	//				{
	//					HookWorldPacket(Packet);
	//					if (m_uSceneServerID > 0)	// 玩家确实处于某条线内，发送数据
	//					{
	//						Packet->SetOpcode(g_local_server_conf.globalServerID * MAX_CLIENT_OPCODE + op_code);
	//						sSceneCommHandler.SendPacketToGameServer(Packet, m_PlayerGUID, CMSG_GATE_TO_GAME_SESSIONING, m_uSceneServerID);
	//					}
	//					else
	//						Log.Debug("数据包拦截","消息号:%d", op_code);
	//				}
	//				break;
	//			case PROC_GATEWAY:
	//				{
	//					HookWorldPacket(Packet);
	//					break;
	//				}
	//			}
	//		}
	//		DeleteWorldPacket(Packet);
	//		Packet = NULL;
	//	}
	//	break;
	//}
}

void GameSocket::SessionAuthedSuccessCallback(uint32 sessionIndex, string &platformAgent, string &accName, uint32 gmFlag, string existSessionToken)
{
	// 验证成功
	m_AuthStatus |= GSocket_auth_state_ret_succeed;

	// 生成登陆令牌(已经传入了存在的令牌数据，则不用生成新的令牌数据，这样可以保证玩家从头到尾只使用一个token数据)
	if (existSessionToken.length() > 0)
		m_strLoginToken = existSessionToken;
	else
		m_strLoginToken = GameSocket::GenerateGameSocketLoginToken(platformAgent, accName);

	pbc_wmessage *msg1004 = sPbcRegister.Make_pbc_wmessage("UserProto001004");
	pbc_wmessage_string(msg1004, "reloginKey", m_strLoginToken.c_str(), 0);
	// 通知回客户端
	WorldPacket packet(SMSG_001004, 128);
	sPbcRegister.EncodePbcDataToPacket(packet, msg1004);
	SendPacket(&packet);

	DEL_PBC_W(msg1004);

	m_SessionIndex = sessionIndex;
	m_strAccountName = accName;
	m_PlayerGUID = GameSocket::GeneratePlayerHashGUID(m_strClientAgent, accName);
	m_GMFlag = gmFlag;

	if (m_nAuthRequestID > 0)
	{
		// 如果之前有注册，则移除已经注册的令牌数据
		sSceneCommHandler.RemoveReloginAccountData(accName);
		sLog.outString("[会话验证成功]reqID=%u,账号%s(guid="I64FMTD"),gm=%u,token=%s", m_nAuthRequestID, m_strAccountName.c_str(), m_PlayerGUID, m_GMFlag, m_strLoginToken.c_str());
		m_nAuthRequestID = 0;
	}
	else
		sLog.outString("[会话重连成功]账号%s(guid="I64FMTD"),gm=%u,新token=%s", m_strAccountName.c_str(), m_PlayerGUID, m_GMFlag, m_strLoginToken.c_str());

	sSceneCommHandler.AddPendingGameList(this, m_PlayerGUID);
	// 注册最新令牌数据
	sSceneCommHandler.RegisterReloginAccountTokenData(m_strClientAgent, accName, m_PlayerGUID, m_strLoginToken, sessionIndex, gmFlag);
}

void GameSocket::SessionAuthedFailCallback(uint32 errCode)
{
	ASSERT(errCode != GameSock_auth_ret_success);

	mClientStatus = eCSTATUS_AUTHFAILED;
	mClientLog = "auth failed!";
	sLog.outError("[会话验证失败]accName=%s, errCode=%u", m_strAccountName.c_str(), uint32(errCode));
	m_AuthStatus |= GSocket_auth_state_ret_failed;

	// 通知回客户端
	WorldPacket packet(SMSG_001004, 4);
	packet.SetErrCode(errCode);
	SendPacket(&packet);
}

void GameSocket::SendChangeSceneResult(uint32 errCode, uint32 tarMapID /*= 0*/, float dstPosX /*= 0*/, float dstPosY /*= 0*/, float dstHeight /*= 0.0f*/)
{
	WorldPacket packet(SMSG_002006, 128);
	if (errCode != 0)
	{
		packet.SetErrCode(errCode);
	}
	else
	{
		pbc_wmessage *msg2006 = sPbcRegister.Make_pbc_wmessage("UserProto002006");
		if (msg2006 == NULL)
			return ;

		pbc_wmessage_integer(msg2006, "descSceneID", tarMapID, 0);
		pbc_wmessage_real(msg2006, "destPosX", dstPosX);
		pbc_wmessage_real(msg2006, "destPosY", dstPosY);
		pbc_wmessage_real(msg2006, "destPosHeight", dstHeight);

		sPbcRegister.EncodePbcDataToPacket(packet, msg2006);
		DEL_PBC_W(msg2006);
	}
	
	SendPacket(&packet);
}

bool GameSocket::InPacketBacklogState()
{
	bool bRet = false;

	m_backlogStateLock.Acquire();
	bRet = m_inPckBacklogState;
	m_backlogStateLock.Release();

	return bRet;
}

void GameSocket::TryToLeaveBackLogState()
{
	if (InPacketBacklogState())
		_LeaveBacklogState();
}

void GameSocket::TryToEnterBackLogState()
{
	if (!InPacketBacklogState())
		_EnterBacklogState();
}

void GameSocket::OnConnect()
{
	//建立连接的时候初始化收包时间
	//memset(m_packrecvtime,0,sizeof(uint32)*NUM_MSG_TYPES);
	
	m_checkcount = 0;
	mClientStatus = 0;
	m_beStuck = false;
	mClientIpAddr = GetIP();
	
	sLog.outString("[GameSocket] %p Connected(ip=%s)", this, mClientIpAddr.c_str());
}

extern TextLogger * Crash_Log;
void GameSocket::OnDisconnect()
{
	static const uint8 snGameSocketAuthFailedStatus = (GSocket_auth_state_sended_packet | GSocket_auth_state_ret_failed);
	
	sSceneCommHandler.OnIPDisconnect(GetIP());
	sLog.outString("[GameSocket] %p(%s) Disconnected(ip=%s),auth=%u,agent=%s,accName=%s,sindex=%u", this, szGetPlayerAccountName(), mClientIpAddr.c_str(),
					uint32(m_AuthStatus), m_strClientAgent.c_str(), m_strAccountName.c_str(), m_SessionIndex);

	if(m_nAuthRequestID != 0)
	{
		sLog.outError("未验证客户端断开！request:%u,accName=%s", m_nAuthRequestID, szGetPlayerAccountName());
		sLogonCommHandler.UnauthedSocketClose(m_nAuthRequestID);
		m_nAuthRequestID = 0;
	}

	// 在5分钟之内离开的话，要输出一下具体情况
	uint32 timeDiff = UNIXTIME - m_nSocketConnectedTime;
	if(timeDiff < 300)
	{
		// 发送了会话验证包
		if (hadSendedAuthPacket())
		{
			// 得到了会话验证结果
			if (hadGotSessionAuthResult())
			{
				Crash_Log->AddLineFormat("RECORD:玩家(agent=%s,accName=%s,guid="I64FMTD")离开！status=%d lastlog=%s IP=%s Time=%u秒 环境=%s,LineID=%u", m_strClientAgent.c_str(), m_strAccountName.c_str(), 
										m_PlayerGUID, mClientStatus, mClientLog.c_str(), GetIP(), timeDiff, mClientEnv.c_str(), m_uSceneServerID);
			}
			else
			{
				Crash_Log->AddLineFormat("RECORD:客户端(agent=%s,accName=%s)断开[%u秒],Auth=[已发会话验证包未等结果返回] status=%d IP=%s 环境=%s", m_strClientAgent.c_str(), m_strAccountName.c_str(), timeDiff, 
										mClientStatus, GetIP(), mClientEnv.c_str());
			}
		}
		else
		{
			// 没有发送会话验证包
			Crash_Log->AddLineFormat("RECORD:客户端离开！Auth=[未发送会话验证包]status=%d IP=%s Time=%u秒 环境=%s", mClientStatus, GetIP(), timeDiff, mClientEnv.c_str());
		}
	}

	if (hadGotSessionAuthResult())
	{
		if (hadSessionAuthSucceeded() && m_SessionIndex > 0)
		{
			if (m_uSceneServerID > 0)
			{
				// 如果场景服还存在,则通知场景服下线,否则直接通知中央服关闭会话
				if (sSceneCommHandler.GetServerSocket(m_uSceneServerID) != NULL)
					sSceneCommHandler.SendCloseSessionNoticeToScene(m_uSceneServerID, m_PlayerGUID, m_SessionIndex);
				else
					sSceneCommHandler.SendCloseServantNoticeToMaster(m_PlayerGUID, m_SessionIndex, true);
			}
			else
				sSceneCommHandler.RemoveDisconnectedSocketFromPendingGameList(m_PlayerGUID);
			// if (m_strLoginToken.length() > 0)
				// sSceneCommHandler.RegisterReloginAccountTokenData(m_strAccountName, m_PlayerGUID, m_strLoginToken, m_SessionIndex, m_GMFlag);
		}

		m_AuthStatus = snGameSocketAuthFailedStatus;
		m_uSceneServerID = 0;
	}
}

void GameSocket::_CheckRevcFrenquency(uint32 packopcode)
{
	//if(packopcode<NUM_MSG_TYPES && m_packetfrequency[packopcode]>0)
	//{
	//	if(UNIXMSTIME-m_packrecvtime[packopcode]<m_packetfrequency[packopcode])
	//	{
	//		m_checkcount++;
	//		if(m_checkcount>m_disconnectcount)
	//		{			
	//			sLog.outDebug("玩家%d发包过快%d,应该阻止",m_PlayerGUID,packopcode);
	//			//Disconnect();
	//		}
	//		if(m_packetfrequency[packopcode]==0xFFFFFFFF && m_packrecvtime[packopcode]>0)
	//		{
	//			//严格限制一次的包
	//			sLog.outError("玩家%d重复发包%d,直接短线",m_PlayerGUID,packopcode);
	//			Disconnect();
	//		}
	//	}
	//	m_packrecvtime[packopcode] = UNIXMSTIME;
	//}
}

void GameSocket::_EnterBacklogState()
{
	m_backlogStateLock.Acquire();
#ifdef _WIN64
	ASSERT(m_inPckBacklogState == false && "GameSocket进入包积压状态时标记不为false");
#else
	if (m_inPckBacklogState)
		sLog.outError("[积压态异常]GameSock(guid="I64FMTD") 尝试[进入]积压态时不为false,并残留有%u个包", m_PlayerGUID, _pckBacklogQueue.GetElementCount());
#endif

	if (_pckBacklogQueue.HasItems())
	{
		sLog.outError("[积压态异常]在(guid="I64FMTD")进入积压态时剩余%u个数据包未处理", m_PlayerGUID, _pckBacklogQueue.GetElementCount());
		_pckBacklogQueue.Clear();
	}
	sLog.outString("GameSocket (guid="I64FMTD") Enter BacklogState", m_PlayerGUID);
	m_inPckBacklogState = true;
	m_backlogStateLock.Release();
}

void GameSocket::_LeaveBacklogState()
{
	m_backlogStateLock.Acquire();
#ifdef _WIN64
	ASSERT(m_inPckBacklogState == true && m_uSceneServerID > 0 && "GameSocket进入包积压状态时标记不为false");
#else
	if (!m_inPckBacklogState)
		sLog.outError("[积压态异常] GameSocket(guid="I64FMTD") 尝试[离开]积压态时不为true", m_PlayerGUID);
#endif

	uint32 elementCount = _pckBacklogQueue.GetElementCount();
	if (_pckBacklogQueue.HasItems())
	{
		WorldPacket *pck = NULL;
		while (pck = _pckBacklogQueue.Pop())
		{
			// 进行消息包的分发
			/*switch (m_transpacket[pck->GetOpcode()])
			{
			case PROC_CENTRE:
			{
			HookWorldPacket(pck);
			pck->SetServerCodeAndId(CMSG_GATE_2_MASTER_SESSIONING, m_PlayerGUID);
			sSceneCommHandler.SendPacketToMasterServer(pck);
			}
			break;
			case PROC_GAME:
			{
			HookWorldPacket(pck);
			pck->SetOpcode(m_uPlatformServerID * MAX_CLIENT_OPCODE + pck->GetOpcode());
			pck->SetServerCodeAndId(CMSG_GATE_2_SCENE_SESSIONING, m_PlayerGUID);
			sSceneCommHandler.SendPacketToGameServer(pck, m_uSceneServerID);
			}
			break;
			case PROC_GATEWAY:
			{
			HookWorldPacket(pck);
			break;
			}
			}*/
			DeleteWorldPacket(pck);
			pck = NULL;
		}
	}

	sLog.outString("GameSocket (guid="I64FMTD") Leave BacklogState,Sended %u packets", m_PlayerGUID, elementCount);
	// 离开积压状态
	m_inPckBacklogState = false;
	m_backlogStateLock.Release();
}
