#include "stdafx.h"
#include "MasterCommServer.h"
#include "GateCommHandler.h"
#include "PlayerMgr.h"
#include "ObjectPoolMgr.h"

gatepacket_handler GatePacketHandler[SERVER_MSG_OPCODE_COUNT];

CMasterCommServer::CMasterCommServer(SOCKET fd, const sockaddr_in * peer) : TcpSocket(fd, 5485760, 5485760, true, peer, RecvPacket_head_server2server, SendPacket_head_server2server, 
																					  PacketCrypto_none, PacketCrypto_none)
{
	m_lastPingRecvUnixTime = time(NULL);
	removed = false;
	authenticated = 0;
	sInfoCore.AddServerSocket(this);
}

CMasterCommServer::~CMasterCommServer(void)
{
	WorldPacket *pck = NULL;
	while(pck = _recvQueue.Pop())
	{
		if (pck != NULL)
			DeleteWorldPacket(pck);
	}
	while(pck = _sendQueue.Pop())
	{
		if (pck != NULL)
			DeleteWorldPacket(pck);
	}
}

void CMasterCommServer::OnDisconnect()
{
	if (!removed)
	{
		sInfoCore.RemoveServerSocket(this);
		sLog.outError("网关服务器:%s断开%s", registername.c_str(),registeradderss.c_str());
	}
	sPlayerMgr.OnGateCommServerDisconnect(this);
}

void CMasterCommServer::OnUserPacketRecved(WorldPacket *packet)
{	
	sInfoCore.m_totalreceivedata += packet->size();
	sInfoCore.m_recvDataByteCountPer5min += packet->size();
	
	HandlePacket(packet);
}

// 用于发送的函数
void CMasterCommServer::SendPacket(WorldPacket* data, uint32 serverOpcode, uint64 guid)
{
	if(data == NULL)
		return ;

	if (!data->m_direct_new)
		data = NewWorldPacket(data);

	const uint32 packSize = data->size();
	const uint32 ret = SendingData((packSize > 0 ? (void*)data->contents() : NULL), packSize, data->GetOpcode(), serverOpcode, guid, data->GetErrorCode(), data->GetPacketIndex());

	switch (ret)
	{
	case Send_tcppck_err_none:
		sInfoCore.m_totalsenddata += packSize;
		sInfoCore.m_sendDataByteCountPer5min += packSize;
		++sInfoCore.m_totalSendPckCount;
		++sInfoCore.m_sendPacketCountPer5min;
		DeleteWorldPacket( data );
		break;
	case Send_tcppck_err_no_enough_buffer:
		{
			sLog.outError("[中央->网关%u]发包时缓冲区不足,数据开始积压(pckSize=%u,缓冲区可用:%u)", gateserver_id, packSize, GetWriteBufferSpace());
			if (data != NULL)
			{
				data->SetServerOpcodeAndUserGUID(serverOpcode, guid);
				_sendQueue.Push(data);
			}
		}
		break;
	default:
		sLog.outError("[中央->网关%u]发包时遇到其他错误(err=%u),serverOp=%u,op=%u,packLen=%u,userGuid="I64FMTD"", gateserver_id, ret, 
						serverOpcode, data->GetOpcode(), data->size(), data->GetUserGUID());
		DeleteWorldPacket(data);
		break;
	}
}

// 发送存储于缓冲区内的Packets
void CMasterCommServer::UpdateServerSendQueuePackets()
{
	if (!_sendQueue.HasItems())
		return ;

	WorldPacket *pServerPck = NULL;
	uint64 starttime = getMSTime64();
	uint32 startPckCount = _sendQueue.GetElementCount();

	while (pServerPck = _sendQueue.Pop())
	{
		const uint32 pckSize = pServerPck->size();
		const uint32 ret = SendingData((pckSize > 0 ? (void*)pServerPck->contents() : NULL), pckSize, pServerPck->GetOpcode(), pServerPck->GetServerOpCode(), 
										pServerPck->GetUserGUID(), pServerPck->GetErrorCode(), 0);

		if (ret == Send_tcppck_err_none)
		{
			// 发送成功，准备继续发送
			++sInfoCore.m_totalSendPckCount;
			++sInfoCore.m_sendPacketCountPer5min;
			DeleteWorldPacket( pServerPck );
		}
		else if (ret == Send_tcppck_err_no_enough_buffer)
		{
			// 马上投递一下发包缓冲,将包放回队列,准备下一次循环发送
			bool flag = true;
			if(GetWriteBufferSize() > 0)
			{
				LockWriteBuffer();
				Write((void*)&flag, 0);
				UnlockWriteBuffer();
			}
			_sendQueue.Push(pServerPck);
			sLog.outError("[中央->网关%u]发包时缓冲区不足,数据开始积压(pckSize=%u,缓冲区可用:%u),余下%u个包等下次发送", gateserver_id, pckSize, GetWriteBufferSpace(), _sendQueue.GetElementCount());
			break;
		}
		else
		{
			_sendQueue.Push(pServerPck);
			sLog.outError("[中央->网关%u]发包缓冲队列遇到其他错误(err=%u),将删除队列中%u个包", gateserver_id, ret, _sendQueue.GetElementCount());
			while (pServerPck = _sendQueue.Pop())
			{
				DeleteWorldPacket( pServerPck );
				pServerPck = NULL;
			}
			break;
		}
	}

	uint32 timeDiff = getMSTime64() - starttime;
	if (timeDiff > 2000)
	{
		sLog.outError("[中央->网关%u]发包队列耗时(%u ms)过长,本次共发送%u个包", gateserver_id, timeDiff, startPckCount - _sendQueue.GetElementCount());
	}
}

// 处理储存在接收缓冲队列中的Packets
void CMasterCommServer::UpdateServerRecvQueuePackets()
{
	uint64 start_time = getMSTime64();
	uint32 packet_sum = 0;
	uint16 packet_op_counter[SERVER_MSG_OPCODE_COUNT] = { 0 };
	WorldPacket *pRecvPck = NULL;

	while (pRecvPck = _recvQueue.Pop())
	{
		if (pRecvPck == NULL)
			continue;

		// 处理接收到的这个包！
		if (pRecvPck->GetServerOpCode() == CMSG_GATE_2_MASTER_SESSIONING)	// 如果serveropcode等于CENTRECMSG_PLAYERGAME_REQUEST，说明让servant去处理，包的删除在servant的update中进行
		{
			HandlePlayerGameRequest(*pRecvPck);
			++packet_op_counter[CMSG_GATE_2_MASTER_SESSIONING];
		}
		else
		{
			uint32 pckOpcode = pRecvPck->GetOpcode();
			if(pckOpcode >= SERVER_MSG_OPCODE_COUNT)
			{
				sLog.outError("中央服收到来自网关%u的错误包,op=%u,serverOp=%u,Opcode异常", gateserver_id, pckOpcode, pRecvPck->GetServerOpCode());
				DeleteWorldPacket( pRecvPck );
			}
			else if (GatePacketHandler[pckOpcode] == NULL)
			{
				// 交给lua处理
				ScriptParamArray params;
				params << pckOpcode;
				if (!pRecvPck->empty())
				{
					size_t sz = pRecvPck->size();
					params.AppendUserData((void*)pRecvPck->contents(), sz);
					params << sz;
				}
				LuaScript.Call("HandleServerPacket", &params, NULL);

				DeleteWorldPacket( pRecvPck );
			}
			else
			{
				++packet_op_counter[pckOpcode];
				(this->*(GatePacketHandler[pckOpcode]))(*pRecvPck);	// 其他包处理完可直接删除
				DeleteWorldPacket(pRecvPck);
			}
		}
		++packet_sum;
	}

	uint32 time_diff = getMSTime64() - start_time;
	if (time_diff > 1000)
	{
		sLog.outError("中央服 处理网关[%s]数据时间([%u]毫秒,[%u]个包)较长", registeradderss.c_str(), time_diff, packet_sum);
		for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
		{
			if (packet_op_counter[i] > 0)
				sLog.outString("处理opcode = %u 包有 %u 个", i, uint32(packet_op_counter[i]));
		}
	}
}

void CMasterCommServer::HandlePacket(WorldPacket *recvData)
{
	/*if(recvData->GetOpcode() == CMSG_GATE_2_MASTER_PING)
	{
		uint32 curTime = time(NULL);
		int32 diff = curTime - m_lastPingRecvUnixTime - SERVER_PING_INTERVAL;
		if (diff >= 2)
			sLog.outColor(TYELLOW, "GatewayServer: %s Ping delay %d sec\n", registername.c_str(), diff);
		m_lastPingRecvUnixTime = time(NULL);
	}*/
	_recvQueue.Push(recvData);
}

void CMasterCommServer::DeletePlayerSocket(uint64 userGuid, uint32 sessionindex, bool direct)
{
	WorldPacket packet(SMSG_MASTER_2_GATE_CLOST_SESSION_NOTICE, 4);
	packet << userGuid;
	packet << sessionindex;
	packet << direct;
	SendPacket(&packet, SMSG_MASTER_2_GATE_CLOST_SESSION_NOTICE, 0);
}
