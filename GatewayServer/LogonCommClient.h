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

#ifndef __LOGON_COMM_CLIENT_H
#define __LOGON_COMM_CLIENT_H

#include "ServerCommonDef.h"

class LogonCommClient;
typedef void (LogonCommClient::*logonpacket_handler)(WorldPacket&);

//本连接主要处理网关和登录服务器之间启动和保持连接的处理，没有玩家数据的沟通
class LogonCommClient : public TcpSocket
{
public:
	LogonCommClient(SOCKET fd, const sockaddr_in * peer);
	~LogonCommClient();
	
public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	
	void SendPacket(WorldPacket *data);		// 发送数据到发送缓冲区
	void OnDisconnect();					// 当连接被关闭时

	void SendPing();						// 向登陆服务器发送ping的请求
	void SendChallenge();					// 初次连接时，向登陆服务器发送验证的请求

	void HandlePacket(WorldPacket & recvData);	// 处理数据，调用下面Handle开头的方法

	void HandleAuthResponse(WorldPacket & recvData);		// 得到验证结果
	void HandlePong(WorldPacket & recvData);				// 得到ping的结果
	void HandleAccountAuthResult(WorldPacket & recvData);	// 得到玩家请求验证后的结果

	uint32 last_ping_unixtime;						
	uint32 last_pong_unixtime;

	uint64 ping_ms_time;						
	uint32 latency;							// 延迟

	bool authenticated_ret;					// 是否通过验证

public:
	static logonpacket_handler ms_packetHandler[SERVER_MSG_OPCODE_COUNT];
	static void InitPacketHandler();

};

#endif

