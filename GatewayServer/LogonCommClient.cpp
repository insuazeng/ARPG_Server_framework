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

#include "stdafx.h"
#include "LogonCommClient.h"
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"

extern GatewayServer g_local_server_conf;

logonpacket_handler LogonCommClient::ms_packetHandler[SERVER_MSG_OPCODE_COUNT];

void LogonCommClient::InitPacketHandler()
{
	for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
	{
		ms_packetHandler[i] = NULL;
	}

	ms_packetHandler[SMSG_LOGIN_2_GATE_AUTH_RESULT]						= &LogonCommClient::HandleAuthResponse;
	ms_packetHandler[SMSG_LOGIN_2_GATE_PONG]							= &LogonCommClient::HandlePong;
	ms_packetHandler[SMSG_LOGIN_2_GATE_ACCOUNT_AUTH_RET]				= &LogonCommClient::HandleAccountAuthResult;
}

LogonCommClient::LogonCommClient(SOCKET fd,const sockaddr_in * peer) : TcpSocket(fd, 524288, 65536, true, peer, RecvPacket_head_server2server, SendPacket_head_server2server, 
																							 PacketCrypto_none, PacketCrypto_none)
{
	// do nothing
	last_ping_unixtime = last_pong_unixtime = time(NULL);
	latency = 0;
	authenticated_ret = false;
}

void LogonCommClient::OnUserPacketRecved(WorldPacket *packet)
{
	HandlePacket(*packet);
	DeleteWorldPacket(packet);
}

void LogonCommClient::HandlePacket(WorldPacket & recvData)
{
	uint32 opcode = recvData.GetOpcode();
	if(opcode >= SERVER_MSG_OPCODE_COUNT || ms_packetHandler[opcode] == NULL)
	{
		sLog.outError("Got unknwon packet from login: opcode=%u, size=%u", opcode, uint32(recvData.size()));
		return;
	}

	(this->*(ms_packetHandler[opcode]))(recvData);
}

void LogonCommClient::HandleAccountAuthResult(WorldPacket & recvData)
{
	cLog.Notice("测试", "接收到来自登陆服务器的验证回馈..\n");
	uint32 request_id;
	recvData >> request_id;

	Mutex &m = sLogonCommHandler.GetPendingLock();
	m.Acquire();

	// find the socket with this request
	GameSocket * sock = sLogonCommHandler.GetSocketByRequest(request_id);
	if(sock == 0 || sock->hadGotSessionAuthResult())	   // Expired/Client disconnected
	{
		m.Release();
		sLog.outError("session request expired:socket %d ", sock);
		return;
	}
	// extract sessionkey / account information (done by WS)
	sLogonCommHandler.RemoveUnauthedSocket(request_id);		// 删除验证包
	m.Release();

	ASSERT(sock->m_nAuthRequestID == request_id);
	
	string accName;
	uint32 sessiongIndex = 0, gmFlag = 0;
	if (recvData.GetErrorCode() == GameSock_auth_ret_success)
	{
		recvData >> sessiongIndex >> accName >> gmFlag;
		sessiongIndex = sSceneCommHandler.GenerateSessionIndex();				// 这次采用网关生成的会话索引（登陆服返回的不再使用）
		sock->SessionAuthedSuccessCallback(sessiongIndex, sock->m_strClientAgent, accName, gmFlag, "");
	}
	else
		sock->SessionAuthedFailCallback(recvData.GetErrorCode());
}

void LogonCommClient::HandlePong(WorldPacket & recvData)
{
	if(latency)
	{
		sLog.outDebug(">>loginserver latency: %ums", getMSTime64() - ping_ms_time);
	}
	else
	{
		sLog.outDebug(">>loginserver无延迟..");
	}
	latency = getMSTime64() - ping_ms_time;
	
	if(latency>1000)
		sLog.outError("登录服务器ping时间太长%d", latency);
	
	last_pong_unixtime = time(NULL);
}

void LogonCommClient::SendPing()
{
	WorldPacket data(CMSG_GATE_2_LOGIN_PING, 4);
	data << sSceneCommHandler.GetOnlinePlayersCount();
	SendPacket(&data);
	
	ping_ms_time = getMSTime64();
	
	// uint32 curTime = time(NULL);
	// sLog.outDetail("ping login server diff %u seconds..\n", curTime - last_ping_unixtime);
	sLog.outDebug("ping login server");
	last_ping_unixtime = time(NULL);
}

void LogonCommClient::SendPacket(WorldPacket *data)
{
	const uint32 pckSize = data->size();
	const uint32 ret = SendingData((pckSize > 0 ? (void*)data->contents() : NULL), data->size(), data->GetOpcode(), data->GetServerOpCode(), data->GetUserGUID(), 0, data->GetPacketIndex());
	if (ret != Send_tcppck_err_none)
		sLog.outError("[网关->登陆]发包时遇到其他错误(err=%u),op=%u,packSize=%u", ret, data->GetOpcode(), data->size());
}

void LogonCommClient::OnDisconnect()
{
	sLog.outString("登录服务器断开OnDisconnect");
	sLogonCommHandler.ConnectionDropped();	
}

LogonCommClient::~LogonCommClient()
{

}

void LogonCommClient::SendChallenge()
{
	uint8 *key = (uint8*)(sLogonCommHandler.m_sha1AuthKey.Intermediate_Hash);

	printf("Key:");
	sLog.outColor(TGREEN, " ");
	for(int i = 0; i < 20; ++i)
		printf("%.2X ", key[i]);
	sLog.outColor(TNORMAL, "\n");

	// 暂时不对包进行加密
	/*SetRecvPackCryptoType(PacketCrypto_recv_all, key, 20);
	SetSendPackCryptoType(PacketCrypto_send_all, key, 20);*/

	WorldPacket data(CMSG_GATE_2_LOGIN_AUTH_REQ, 128);
	data.append(key, 20);
	data << g_local_server_conf.localServerName << g_local_server_conf.localServerAddress 
		<< g_local_server_conf.localServerListenPort << g_local_server_conf.globalServerID;

	SendPacket(&data);
}

void LogonCommClient::HandleAuthResponse(WorldPacket & recvData)
{
	uint32 ret;
	recvData >> ret;
	sLog.outColor(TYELLOW, "%u", ret);

	if(ret != Server_auth_ret_success)
	{
		authenticated_ret = false;
	}
	else
	{
		authenticated_ret = true;
		uint32 register_id;
		string register_name;
		recvData >> register_id >> register_name;

		sLog.outColor(TNORMAL, "\n        >> realm `%s` registered under id ", register_name.c_str());
		sLog.outColor(TGREEN, "%u", register_id);

		sLogonCommHandler.AdditionAck(register_id);
	}
}

