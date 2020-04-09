#include "stdafx.h"

#include "ServerCommonDef.h"
#include "DirectSocket.h"
#include "AccountCache.h"
#include "InformationCore.h"
#include "md5.h"

#ifdef EPOLL_TEST
#define WORLDSOCKET_SENDBUF_SIZE 102400
#define WORLDSOCKET_RECVBUF_SIZE 102400
#else
#ifndef _WIN64
#define WORLDSOCKET_SENDBUF_SIZE 1024
#define WORLDSOCKET_RECVBUF_SIZE 1024
#else
#define WORLDSOCKET_SENDBUF_SIZE 512
#define WORLDSOCKET_RECVBUF_SIZE 512
#endif

#endif

DirectSocket::DirectSocket(SOCKET fd, const sockaddr_in *peer) : TcpSocket(fd, WORLDSOCKET_RECVBUF_SIZE, WORLDSOCKET_SENDBUF_SIZE, true, peer, 
																		RecvPacket_head_user2server, SendPacket_head_server2user, PacketCrypto_none, PacketCrypto_none)
{
	last_recv = 0;
}

DirectSocket::~DirectSocket(void)
{
	sLog.outString("~DirectSocket %p", this);
}

void DirectSocket::OnUserPacketRecved(WorldPacket *packet)
{
	// sLog.outString("[D_Socket]读取一个完整包(size=%u, opcode=%u, ip=%s)", packet->size(), packet->GetOpcode(), mClientIpAddr.c_str());
	sLog.outString("[D_Socket]读取一个完整包(size=%u, opcode=%u, ip=%s m_bufferPool=%d)", packet->size(), packet->GetOpcode(), mClientIpAddr.c_str(), packet->m_bufferPool);
	switch(packet->GetOpcode())
	{
	case CMSG_001001:				// 请求账号登陆
		last_recv = time(NULL);
		HandleLogin(*packet);
		DeleteWorldPacket(packet);
		last_recv = time(NULL);
		break;
	//case LCMSG_AUTO_REGISTER:	// 请求自动注册账号
	//	last_recv = time(NULL);
	//	HandleAccountAutoRegister(*packet);
	//	DeleteWorldPacket(packet);
	//	break;
#ifdef EPOLL_TEST
	case CMSG_001101:
		{
			// epoll环境下的测试,接受到的数据直接拷贝双份再返回
			WorldPacket pck(SMSG_001102, packet->size() * 2);
			pck.append((uint8*)packet->contents(), packet->size());
			pck.append((uint8*)packet->contents(), packet->size());
			SendPacket(&pck);
			DeleteWorldPacket(packet);
			last_recv = time(NULL);
		}
		break;
#endif
	default:
		sLog.outError("[D_Socket]收到不明类型数据包,size=%u,opcode=%u,ip=%s", uint32(packet->size()), uint32(packet->GetOpcode()), GetIP());
		DeleteWorldPacket(packet);
		break;
	}
}

void DirectSocket::OnConnect()
{
	sInfoCore.InsertClientSocket(this);
	mClientIpAddr = GetIP();
	last_recv = UNIXTIME;
	sLog.outString("[D_Socket: %p] OnConnect (ip=%s)", this, mClientIpAddr.c_str());
	// Sleep(1000);
}

void DirectSocket::OnDisconnect()
{
	sInfoCore.EraseClientSocket(this);
	sLog.outString("[D_Socket: %p] OnDisconnect (ip=%s)", this, mClientIpAddr.c_str());
}

void DirectSocket::SendPacket(WorldPacket * packet)
{
	if(!IsConnected())
	{
		return;
	}
	if (m_deleted)
	{
		return;
	}
	if((packet->size() + m_sendPackHeaderSize) >= m_writeBuffer->GetSpace())
	{
		sLog.outError("DirectSocket发生缓冲区满");
		return;	// no enough space in the send buffer
	}
	
	tagStoCPacketHead headData;
	headData.opcode = packet->GetOpcode();
	headData.packSize = packet->size();
	headData.errorCode = packet->GetErrorCode();
	
	bool rv = false;
	
	LockWriteBuffer();
	rv = m_writeBuffer->Write((const uint8*)&headData, m_sendPackHeaderSize);
	if (rv && packet->size() > 0)
	{
		rv = m_writeBuffer->Write(packet->contents(), packet->size());
	}
	if (rv)
	{
		Write(&rv, 0);
	}
	
	UnlockWriteBuffer();
	
	return;
}
