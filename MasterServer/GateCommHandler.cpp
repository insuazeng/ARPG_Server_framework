#include "stdafx.h"
#include "GateCommHandler.h"
#include "MasterCommServer.h"
#include "PlayerMgr.h"

CCentreCommHandler::CCentreCommHandler(void)
{
	m_nGateServerIdHigh = 0;
	m_totalreceivedata = 0;
	m_totalsenddata = 0;
	m_totalSendPckCount = 0;

	m_sendDataByteCountPer5min = 0;
	m_recvDataByteCountPer5min = 0;
	m_sendPacketCountPer5min = 0;
}

CCentreCommHandler::~CCentreCommHandler(void)
{
}

uint32 CCentreCommHandler::GenerateGateServerID()
{
	m_serverIdLock.Acquire();
	uint32 ret = ++m_nGateServerIdHigh;
	m_serverIdLock.Release();
	return ret;
}

void CCentreCommHandler::AddServerSocket(CMasterCommServer *pServerSocket)
{
	m_serverSocketLocks.Acquire();
	m_serverSockets.insert(pServerSocket);
	m_serverSocketLocks.Release();
}

void CCentreCommHandler::RemoveServerSocket(CMasterCommServer *pServerSocket)
{
	m_serverSocketLocks.Acquire();
	m_serverSockets.erase(pServerSocket);
	m_serverSocketLocks.Release();
}

void CCentreCommHandler::TimeoutSockets()
{
	uint32 curTime = time(NULL);
	set<CMasterCommServer*>::iterator it, it2;
	CMasterCommServer *pSocket;
	m_serverSocketLocks.Acquire();
	for (it = m_serverSockets.begin(); it != m_serverSockets.end();)
	{
		pSocket = *it;
		it2 = it;
		++it;
		if (pSocket->m_lastPingRecvUnixTime < curTime && ((curTime - pSocket->m_lastPingRecvUnixTime) > 60))
		{
			sLog.outError("Closing GateServer: %d Socket due to ping timeout\n", pSocket->GetGateServerId());
			pSocket->removed = true;
			m_serverSockets.erase(it2);
			pSocket->Disconnect();
		}
	}
	m_serverSocketLocks.Release();
}

bool CCentreCommHandler::TrySendPacket(WorldPacket *packet, uint32 serverOpCode)
{
	// 从连入的网关socket内挑选一个写缓冲区最多的网关，将包发出去
	CMasterCommServer *pSocket = NULL;
	uint32 maxBufferSize = 0;
	uint32 curSize = 0;
	bool bRet = false;
	m_serverSocketLocks.Acquire();

	for (set<CMasterCommServer*>::iterator it = m_serverSockets.begin(); it != m_serverSockets.end(); ++it)
	{
		if ((*it) == NULL)
			continue ;
		curSize = (*it)->GetWriteBuffer()->GetSpace();
		if (curSize > maxBufferSize)
		{
			pSocket = (*it);
			maxBufferSize = curSize;
		}
	}

	if (pSocket != NULL)
	{
		WorldPacket *pck = NewWorldPacket(packet);
		pSocket->SendPacket(pck, serverOpCode, 0);
		bRet = true;
	}
	m_serverSocketLocks.Release();
	return bRet;
}

void CCentreCommHandler::SendMsgToGateServers(WorldPacket *pack, uint32 serverOpcode)
{
	set<CMasterCommServer*>::iterator it, it2;
	m_serverSocketLocks.Acquire();
	for (it = m_serverSockets.begin(); it != m_serverSockets.end();)
	{
		it2 = it;
		++it;
		if ((*it2) != NULL)
		{
			WorldPacket *pck = NewWorldPacket(pack);
			pck->SetOpcode(serverOpcode);
			(*it2)->SendPacket(pck, serverOpcode, 0);
		}
	}
	m_serverSocketLocks.Release();
}

void CCentreCommHandler::UpdateServerSendPackets()
{
	set<CMasterCommServer*>::iterator it, it2;
	m_serverSocketLocks.Acquire();
	for (it = m_serverSockets.begin(); it != m_serverSockets.end();)
	{
		it2 = it;
		++it;
		if ((*it2) != NULL)
		{
			(*it2)->UpdateServerSendQueuePackets();
		}
	}
	m_serverSocketLocks.Release();
}

void CCentreCommHandler::UpdateServerRecvPackets()
{
	m_serverSocketLocks.Acquire();
	for (set<CMasterCommServer*>::iterator it = m_serverSockets.begin(); it != m_serverSockets.end(); ++it)
	{
		if ((*it) != NULL)
			(*it)->UpdateServerRecvQueuePackets();
	}
	m_serverSocketLocks.Release();
}
