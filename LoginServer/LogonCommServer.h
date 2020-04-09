/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#ifndef __LOGON_COMM_SERVER_H
#define __LOGON_COMM_SERVER_H

#include "network/Network.h"
#include "RC4Engine.h"
#include "ServerCommonDef.h"

class LogonCommServerSocket;
typedef void (LogonCommServerSocket::*logonpacket_handler)(WorldPacket&);

//网关用来来接，验证的TCP
class LogonCommServerSocket : public TcpSocket
{
public:
	bool authenticated_flag;			// 是否已经经过验证
	uint32 last_ping;
	bool removed;
	uint32 m_AssociateGatewayID;		// 关联的网关id

public:
	LogonCommServerSocket(SOCKET fd,const sockaddr_in * peer);
	~LogonCommServerSocket();

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	
public:
	void OnDisconnect();
	void SendPacket(WorldPacket * data);
	void HandlePacket(WorldPacket & recvData);

	void HandlePing(WorldPacket & recvData);
	void HandleUserAccountAuthRequest(WorldPacket & recvData);
	void HandleReloadAccounts(WorldPacket & recvData);
	void HandleAuthChallenge(WorldPacket & recvData);

public:
	static logonpacket_handler ms_packetHandler[SERVER_MSG_OPCODE_COUNT];
	static void InitLoginServerPacketHandler();
};

#endif
