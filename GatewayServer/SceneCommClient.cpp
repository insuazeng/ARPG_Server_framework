#include "stdafx.h"
#include "SceneCommClient.h"
#include "SceneCommHandler.h"

//每次update处理的busypacket
#define MAX_BUSYPACK_PERUPDATE 2

extern GatewayServer g_local_server_conf;

scenepacket_handler SceneServerPacketHandlers[SERVER_MSG_OPCODE_COUNT];

CSceneCommClient::CSceneCommClient(SOCKET fd,const sockaddr_in * peer) : TcpSocket(fd, 5485760, 5485760, true, peer, RecvPacket_head_server2server, 
																				   SendPacket_head_server2server, PacketCrypto_none, PacketCrypto_none)
{
	int arg2 = 1024*1024;
	setsockopt(m_fd, SOL_SOCKET, SO_SNDBUF, (char*)&arg2, sizeof(int));
	setsockopt(m_fd, SOL_SOCKET, SO_RCVBUF, (char*)&arg2, sizeof(int));

	m_lastPingUnixtime = m_lastPongUnixtime = time(NULL);

	_id=0;
	latency_ms = 0;
	authenticated = 0;
	m_registered = false;
}

CSceneCommClient::~CSceneCommClient(void)
{
	// 将包缓冲队列中的剩余包全部删除(如果有的话)
	sLog.outString("~CSceneCommClient %p", this);

	_sendQueueLock.Acquire();
	WorldPacket *pServerPck = NULL;
	while(pServerPck = _sendQueue.front())
	{
		if (pServerPck != NULL)
		{
			delete pServerPck;
			pServerPck = NULL;
			_sendQueue.pop_front();
		}
	}
	_sendQueueLock.Release();

	while(pServerPck = _recvQueue.front())
	{
		if (pServerPck != NULL)
		{
			//delete pServerPck;
			DeleteWorldPacket(pServerPck);
			_recvQueue.pop_front();
		}
	}
	while(pServerPck = _busyQueue.front())
	{
		if (pServerPck != NULL)
		{
			//delete pServerPck;
			DeleteWorldPacket(pServerPck);
			_busyQueue.pop_front();
		}
	}
}

void CSceneCommClient::OnUserPacketRecved(WorldPacket *packet)
{
	uint32 serverOp = packet->GetServerOpCode();
	if (serverOp != 0)
	{
		sSceneCommHandler.m_sceneServerPacketCounter[serverOp]++;
	}
	else
	{
		uint32 op = packet->GetOpcode();

#ifdef _WIN64
		ASSERT(op < SERVER_MSG_OPCODE_COUNT && "网关接收到来自场景服op与serverOp都等于0的数据包");
#endif
		
		sSceneCommHandler.m_sceneServerPacketCounter[op]++;
		packet->SetServerOpcodeAndUserGUID(op, packet->GetUserGUID());
	}

	sSceneCommHandler.g_totalRecvDataFromSceneServer += packet->size();

	serverOp = packet->GetServerOpCode();
	if (serverOp == SMSG_SCENE_2_GATE_AUTH_RESULT || serverOp == SMSG_SCENE_2_GATE_PONG)
	{
		HandleSceneServerPacket(packet);
		DeleteWorldPacket(packet);
		packet = NULL;
	}
	//else if(serverOp == GAMESMSG_SENDGLOBALMSG)
	//{
	//	// 广播消息需要放入缓存队列中慢慢处理
	//	_busyQueue.Push(pWorldPck);
	//}
	else
	{
		_recvQueue.Push(packet);
	}
}

void CSceneCommClient::SendPacket(WorldPacket *data)
{
	//除了GAMERCMSG_SESSIONING包需要指定servercode之外，其他的包直接使用worldpacket里面的opcode
	if(!data)
	{
		return;
	}
	uint32 serverOp = data->GetServerOpCode();
	if(serverOp != CMSG_GATE_2_SCENE_SESSIONING)
	{
		// 因为传送给游戏服的消息包，serveropcode跟opcode是混用的，因此在此处将opcode进行统一。方便中央服发消息的处理 @add by haoye 2012.8.22
		uint32 opCode = data->GetOpcode();
		if (opCode == 0 && serverOp != 0)
		{
			data->SetOpcode(serverOp);
			opCode = serverOp;
		}
		ASSERT(opCode < SERVER_MSG_OPCODE_COUNT);
		sSceneCommHandler.m_sceneServerPacketCounter[opCode]++;
	}
	else
		sSceneCommHandler.m_sceneServerPacketCounter[CMSG_GATE_2_SCENE_SESSIONING]++;

	const uint32 pckSize = data->size();
	const uint32 ret = SendingData((data->size() ? (void*)data->contents() : NULL), pckSize, data->GetOpcode(), serverOp, data->GetUserGUID(), data->GetErrorCode(), data->GetPacketIndex());
	switch (ret)
	{
	case Send_tcppck_err_none:
		sSceneCommHandler.g_totalSendDataToSceneServer += pckSize;
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[网关->场景%u]包发送缓冲(可用%u,pckSize=%u)不够,准备使用缓冲队列", _id, GetWriteBufferSpace(), pckSize);
			WorldPacket *pck = NewWorldPacket(data);
			_sendQueueLock.Acquire();
			_sendQueue.Push(pck);
			_sendQueueLock.Release();
		}
		break;
	default:
		{
			sLog.outError("[网关->场景%u]发包时遇到其他错误(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", _id, ret, 
				data->GetServerOpCode(), data->GetOpcode(), pckSize, data->GetUserGUID());
		}
		break;
	}
}

void CSceneCommClient::UpdateServerSendQueuePackets()
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
				sLog.outError("[网关->场景%u]队列发包时遇到其他错误(err=%u),准备删除队列中所有数据包(%u个)", _id, ret, _sendQueue.GetElementCount());
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

void CSceneCommClient::OnDisconnect()
{
	if(_id != 0)
	{
		sLog.outError("场景服(id=%u)连接中断.", _id);
		sSceneCommHandler.ConnectionDropped(_id);
	}
}

void CSceneCommClient::SendPing()
{
	ping_ms_time = getMSTime64();

	WorldPacket data(CMSG_GATE_2_SCENE_PING, 4);
	SendPacket(&data);

	m_lastPingUnixtime = time(NULL);
}

void CSceneCommClient::SendChallenge(uint32 serverid)
{
	uint8* key = (uint8*)(sSceneCommHandler.m_sha1AuthKey.Intermediate_Hash);

	sLog.outColor(TGREEN, " ");
	for(int i = 0; i < 20; ++i)
	{
		printf("%.2X ", key[i]);
	}
	sLog.outColor(TNORMAL, "\n");

	WorldPacket data(CMSG_GATE_2_SCENE_AUTH_REQ, 20);
	data.append(key, 20);
	data << serverid;			// 发送给游戏服务器他的ID？很怪异。一台游戏服务器可以连接多台网关，但是他们的id要统一。
	
	data << g_local_server_conf.localServerName 
		<< g_local_server_conf.localServerAddress 
		<< g_local_server_conf.localServerListenPort 
		<< g_local_server_conf.globalServerID;

	sLog.outString("auth target SceneServerID=%d,packSize=%d", serverid, data.size());
	SendPacket(&data);
}

void CSceneCommClient::UpdateServerRecvQueuePackets()
{
	uint64 starttime = getMSTime64();

	/*bool alreadybusy=false;
	if(getMSTime()-starttime>1000)
	{
	sLog.outError("ERROR:游戏%d线分包时间太长",_id);
	alreadybusy = true;
	}*/
	//此函数时间必须控制在约1秒的时间内结束
	WorldPacket *pRecvPck = NULL;
	uint32 busycount = 0;
	while (pRecvPck = _recvQueue.front())
	{	
		// 如果逻辑包太多的话，那么吧其中的移动包移除
		//if((busycount++>10000||alreadybusy)&&pRecvPck->GetServerOpCode()==GAMERSMSG_SESSIONING_RESULT&&pRecvPck->GetOpcode()==SMSG_OBJECT_MOVE)//更新的包太多程序的其他类update会轮不到 贡文斌 7.9
		//{
		//	DeleteWorldPacket(pRecvPck);
		//	pRecvPck = NULL;
		//	_recvQueue.pop_front();
		//	if(busycount>10000)
		//	{
		//		if(getMSTime()-starttime<1000)
		//		{
		//			sLog.outString("不到1秒，还能处理更多%d线数据包",_id);
		//			busycount=5000;
		//		}
		//		else
		//			break;
		//	}
		//}
		//else
		//{
			HandleSceneServerPacket(pRecvPck);
			DeleteWorldPacket(pRecvPck);
			pRecvPck = NULL;
			_recvQueue.pop_front();
		//}
	}
	if(busycount>0)
		m_lastPongUnixtime = time(NULL);

	busycount=0;
	while (pRecvPck = _busyQueue.front())
	{
		if (pRecvPck != NULL)
		{
			HandleSceneServerPacket(pRecvPck);
			DeleteWorldPacket(pRecvPck);
			//delete pRecvPck;
			pRecvPck = NULL;
			_busyQueue.pop_front();
		}
		busycount++;
		if(busycount>MAX_BUSYPACK_PERUPDATE)
			break;
	}
	
}

void CSceneCommClient::HandleSceneServerPacket(WorldPacket *pPacket)
{
	uint32 serverOp = pPacket->GetServerOpCode();
	if(serverOp >= SERVER_MSG_OPCODE_COUNT)
	{
		sLog.outError("网关收到来自场景服%u的错误包,op=%u,serverOp=%u,pckSize=%u,serverOp异常", _id, pPacket->GetOpcode(), serverOp, pPacket->size());
	}
	else if (SceneServerPacketHandlers[serverOp] == NULL)
	{
		sLog.outError("网关收到来自场景服%u的错误包,op=%u,serverOp=%u,pckSize=%u,找不到对应Handler", _id, pPacket->GetOpcode(), serverOp, pPacket->size());
	}
	else
	{
		(this->*(SceneServerPacketHandlers[serverOp]))(pPacket);
	}
}

