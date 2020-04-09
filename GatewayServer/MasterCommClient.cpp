#include "stdafx.h"
#include "MasterCommClient.h"
#include "SceneCommHandler.h"

//---HandleServerCMD所需要的变量和头文件
extern bool m_ShutdownEvent;
extern uint32 m_ShutdownTimer;
//---HandleServer

masterpacket_handler MasterServerPacketHandler[SERVER_MSG_OPCODE_COUNT];
extern GatewayServer g_local_server_conf;

CMasterCommClient::CMasterCommClient(SOCKET fd, const sockaddr_in *peer) : TcpSocket(fd, 5485760, 5485760, true, peer, RecvPacket_head_server2server, 
																					 SendPacket_head_server2server, PacketCrypto_none, PacketCrypto_none)
{
	m_lastPingUnixTime = m_lastPongUnixTime = time(NULL);
	ping_ms_time = 0;
	authenticated = 0;
	latency_ms = 0;
	m_nThisGateId = 0;
}

CMasterCommClient::~CMasterCommClient(void)
{
	sLog.outString("~CMasterCommClient %p", this);

	_sendQueueLock.Acquire();
	WorldPacket *pServerPck = NULL;
	while(pServerPck = _sendQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
	_sendQueueLock.Release();

	while(pServerPck = _recvQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
	while(pServerPck = _busyQueue.Pop())
	{
		if (pServerPck != NULL)
			DeleteWorldPacket(pServerPck);
	}
}

void CMasterCommClient::OnUserPacketRecved(WorldPacket *packet)
{
	uint32 serverOp = packet->GetServerOpCode();
	if (serverOp != 0)
	{
		if(serverOp < SERVER_MSG_OPCODE_COUNT)
			++sSceneCommHandler.m_masterServerPacketCounter[serverOp];
	}
	else
	{
		uint32 op = packet->GetOpcode();

#ifdef _WIN64
		ASSERT(op < SERVER_MSG_OPCODE_COUNT);
#endif // _WIN64

		packet->SetServerOpcodeAndUserGUID(op, packet->GetUserGUID());
		++sSceneCommHandler.m_masterServerPacketCounter[op];
	}
	
	sSceneCommHandler.g_totalRecvDataFromMasterServer += packet->size();
	serverOp = packet->GetServerOpCode();
	// 这里进入具体每个包的处理
	if (serverOp == SMSG_MASTER_2_GATE_AUTH_RESULT || serverOp == SMSG_MASTER_2_GATE_PONG )
	{
		HandleCentrePakcets(packet);
		DeleteWorldPacket(packet);
		packet = NULL;
	}
	else
	{
		_recvQueue.Push(packet);
	}
}

void CMasterCommClient::OnDisconnect()
{
	if(m_nThisGateId != 0)
	{
		sLog.outString("ERROR:网关 与 中央 服务器连接中断OnDisconnect.\n");
		sSceneCommHandler.CentreConnectionDropped();		// handler去管理
		m_nThisGateId = 0;
	}
}

void CMasterCommClient::SendPacket(WorldPacket *data)
{	
	//除了GAMERCMSG_SESSIONING包需要指定servercode之外，其他的包直接使用worldpacket里面的opcode
	if(data == NULL)
		return;

	uint32 serverOp = data->GetServerOpCode();
	if(serverOp != CMSG_GATE_2_MASTER_SESSIONING)
	{
		uint32 opCode = data->GetOpcode();
		if (opCode == 0 && serverOp != 0)
		{
			data->SetOpcode(serverOp);
			opCode = serverOp;
		}
		ASSERT(opCode < SERVER_MSG_OPCODE_COUNT);
		sSceneCommHandler.m_masterServerPacketCounter[opCode]++;
	}
	else
		sSceneCommHandler.m_masterServerPacketCounter[CMSG_GATE_2_MASTER_SESSIONING]++;
	
	sSceneCommHandler.g_totalSendDataToMasterServer += data->size();
	
	const uint32 pckSize = data->size();
	const uint32 ret = SendingData((data->size() ? (void*)data->contents() : NULL), pckSize, data->GetOpcode(), serverOp, data->GetUserGUID(), data->GetErrorCode(), data->GetPacketIndex());
	switch (ret)
	{
	case Send_tcppck_err_none:
		sSceneCommHandler.g_totalSendDataToSceneServer += pckSize;
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[网关->中央]包发送缓冲(可用%u,pckSize=%u)不够,准备使用积压队列", GetWriteBufferSpace(), pckSize);
			WorldPacket *pck = NewWorldPacket(data);
			_sendQueueLock.Acquire();
			_sendQueue.Push(pck);
			_sendQueueLock.Release();
		}
		break;
	default:
		{
			sLog.outError("[网关->中央]发包时遇到其他错误(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", ret, 
				data->GetServerOpCode(), data->GetOpcode(), pckSize, data->GetUserGUID());
		}
		break;
	}
}

void CMasterCommClient::UpdateServerSendQueuePackets()
{
	if (!_sendQueue.HasItems())
		return ;

	_sendQueueLock.Acquire();
	WorldPacket *pck = NULL;
	while (pck = _sendQueue.front())
	{
		const uint32 pckSize = pck->size();
		const uint32 ret = SendingData((pckSize ? (void*)pck->contents() : NULL), pckSize, pck->GetOpcode(), pck->GetServerOpCode(), pck->GetUserGUID(), pck->GetErrorCode(), pck->GetUserGUID());
		switch (ret)
		{
		case Send_tcppck_err_none:
			sSceneCommHandler.g_totalSendDataToSceneServer += pckSize;
			DeleteWorldPacket(pck);
			_sendQueue.pop_front();
			break;
		case Send_tcppck_err_no_enough_buffer:
			{
				// 包依然积压,退出函数,等下次循环
				_sendQueueLock.Release();
				return ;
			}
			break;
		default:
			{
				// 队列发包有严重错误,把队列中的包都删除掉
				sLog.outError("[网关->中央]队列发包时遇到其他错误(err=%u),准备删除队列中所有数据包(%u个)", ret, _sendQueue.GetElementCount());
				while(pck = _sendQueue.Pop())
				{
					if (pck != NULL)
						DeleteWorldPacket(pck);
				}
				_sendQueueLock.Release();
				return ;
			}
			break;
		}
	}
	_sendQueueLock.Release();
}

void CMasterCommClient::UpdateServerRecvQueuePackets()
{
	//此函数时间必须控制在约1秒的时间内结束
	WorldPacket *pRecvPck = NULL;

	uint32 busycount = 0;
	while (pRecvPck = _recvQueue.Pop())
	{
		if (pRecvPck != NULL)
		{
			HandleCentrePakcets(pRecvPck);		// 处理来自中央服务器的包
			DeleteWorldPacket(pRecvPck);
			pRecvPck = NULL;
		}
		if(busycount++>1000)		//更新的包太多程序的其他类update会轮不到 贡文斌 7.9
			break;
	}
	if(busycount>0)
		m_lastPongUnixTime = time(NULL);
	busycount = 0;
	while (pRecvPck = _busyQueue.Pop())
	{
		if (pRecvPck != NULL)
		{
			HandleCentrePakcets(pRecvPck);		// 处理来自中央服务器的包
			DeleteWorldPacket(pRecvPck);
			pRecvPck = NULL;
		}
		busycount++;
		if(busycount>2)
			break;
	}
}

void CMasterCommClient::SendPing()
{
	ping_ms_time = getMSTime64();

	WorldPacket pck(CMSG_GATE_2_MASTER_PING, 4);
	SendPacket(&pck);
	
	m_lastPingUnixTime = time(NULL);
}

void CMasterCommClient::SendChallenge(uint32 serverId)
{
	uint8 * key = (uint8*)sSceneCommHandler.m_sha1AuthKey.Intermediate_Hash;

	printf("Key:");
	sLog.outColor(TGREEN, " ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X ", key[i]);
	}
	sLog.outColor(TNORMAL, "\n");

	WorldPacket data(CMSG_GATE_2_MASTER_AUTH_REQ, 20);
	data.append(key, 20);
	data << g_local_server_conf.localServerName 
		<< g_local_server_conf.localServerAddress 
		<< g_local_server_conf.localServerListenPort 
		<< g_local_server_conf.globalServerID;

	SendPacket(&data);
}

void CMasterCommClient::HandleCentrePakcets(WorldPacket *packet)
{
	uint32 serverOp = packet->GetServerOpCode();
	if(serverOp >= SERVER_MSG_OPCODE_COUNT)
	{
		sLog.outError("网关收到来自中央服的错误包,op=%u,serverOp=%u,pckSize=%u,serverOp异常", packet->GetOpcode(), serverOp, packet->size());
	}
	else if (MasterServerPacketHandler[serverOp] == NULL)
	{
		sLog.outError("网关收到来自中央服的错误包,op=%u,serverOp=%u,pckSize=%u,找不到对应Handler", packet->GetOpcode(), serverOp, packet->size());
	}
	else
	{
		(this->*(MasterServerPacketHandler[packet->GetServerOpCode()]))(packet);
	}
}

