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
#include "auth/Sha1.h"
#include "LogonCommServer.h"
#include "AccountCache.h"
#include "InformationCore.h"

logonpacket_handler LogonCommServerSocket::ms_packetHandler[SERVER_MSG_OPCODE_COUNT];

void LogonCommServerSocket::InitLoginServerPacketHandler()
{
	for (int i = 0; i < SERVER_MSG_OPCODE_COUNT; ++i)
	{
		ms_packetHandler[i] = NULL;
	}

	ms_packetHandler[CMSG_GATE_2_LOGIN_AUTH_REQ]				= &LogonCommServerSocket::HandleAuthChallenge;
	ms_packetHandler[CMSG_GATE_2_LOGIN_PING]					= &LogonCommServerSocket::HandlePing;
	ms_packetHandler[CMSG_GATE_2_LOGIN_ACCOUNT_AUTH_REQ]		= &LogonCommServerSocket::HandleUserAccountAuthRequest;

}

LogonCommServerSocket::LogonCommServerSocket(SOCKET fd, const sockaddr_in * peer) : 
					TcpSocket(fd, 524288, 65536, true, peer, RecvPacket_head_server2server, SendPacket_head_server2server, 
					PacketCrypto_none, PacketCrypto_none)
{
	// do nothing
	last_ping = time(NULL);
	sInfoCore.AddServerSocket(this);
	removed = false;

	authenticated_flag = false;
	m_AssociateGatewayID = 0;
}

LogonCommServerSocket::~LogonCommServerSocket()
{

}

void LogonCommServerSocket::OnDisconnect()
{
	// if we're registered -> de-register
	if(!removed)
	{
		sInfoCore.RemoveGatewayConnectInfo(m_AssociateGatewayID);
		sInfoCore.RemoveServerSocket(this);
		removed = true;
	}
}

void LogonCommServerSocket::OnUserPacketRecved(WorldPacket *packet)
{
	HandlePacket(*packet);
	DeleteWorldPacket(packet);
}

void LogonCommServerSocket::HandlePacket(WorldPacket & recvData)
{
	uint32 opcode = recvData.GetOpcode();
	if(!authenticated_flag && opcode != CMSG_GATE_2_LOGIN_AUTH_REQ)
	{
		// invalid
		Disconnect();
		return;
	}
	
	if(opcode >= SERVER_MSG_OPCODE_COUNT || ms_packetHandler[opcode] == 0)
	{
		sLog.outError("Got unknwon packet from gateway:op=%u, len=%u", opcode, uint32(recvData.size()));
		return;
	}
	// sLog.outString("[网关]收到完整包 opcode = %u, size = %u", opcode, uint32(recvData.size()));
	(this->*(ms_packetHandler[opcode]))(recvData);
}

void LogonCommServerSocket::HandleUserAccountAuthRequest(WorldPacket & recvData)
{
	uint32 auth_req_id;
	string account_name, session_key, account_ip;
	recvData >> auth_req_id >> account_name >> session_key >> account_ip;
	
	sLog.outString("[网关会话验证]reqID=%u,账号=%s,key=%s,ip=%s,来自网关[%u]", auth_req_id, 
					account_name.c_str(), session_key.c_str(), account_ip.c_str(), m_AssociateGatewayID);

	if(sInfoCore.m_recvPhpQueue.HasItems())
		Sleep(100);

	uint32 error = GameSock_auth_ret_success;
	Account *acct = sAccountMgr.GetAccountInfo(account_name);
	if(acct == NULL)
	{
		//采用一次登录之后就不应该会进这个分支了
		sAccountMgr.LoadAccount(account_name);
		acct = sAccountMgr.GetAccountInfo(account_name);
	}
	string sessionkey = (acct != NULL) ? sInfoCore.GetSessionKey(acct->strAccountName) : "";
	uint64 key_valid_time = (acct != NULL) ? sInfoCore.GetSessionKeyExpireTime(acct->strAccountName) : 0;

	if ((acct == NULL) || (sessionkey != session_key))
	{
		error = GameSock_auth_ret_err_sessionkey;
	}
	else
	{
		if (sAccountMgr.IsIpBanned(account_ip.c_str()))
			error = GameSock_auth_ret_err_ip_ban;
		else if (sAccountMgr.IsAccountBanned(account_name))
			error = GameSock_auth_ret_err_account_ban;
	}

	// build response packet
	WorldPacket data(SMSG_LOGIN_2_GATE_ACCOUNT_AUTH_RET, 128);
	data.SetErrCode(error);
	data << auth_req_id;
	if(error == GameSock_auth_ret_success)
	{	
		uint32 session_index = sInfoCore.GenerateSessionIndex();
		data << session_index;
		data << acct->strAccountName;
		data << acct->nGMFlags;

		sLog.outString("[会话验证]ReqID=%u Success(spName=%s,name=%s,gmflag=%u)", auth_req_id, acct->strPlatformAgentName.c_str(), acct->strAccountName.c_str(), acct->nGMFlags);
	}
	else
	{
		sLog.outError("[会话验证]ReqID=%u Fail(errCode=%u)", auth_req_id, error);
	}
	SendPacket(&data);

	if (acct != NULL)
	{
		// 删除掉本次会话验证的sessionkey（方便下次设置）
		sInfoCore.DeleteSessionKey(acct->strAccountName);
	}
}

void LogonCommServerSocket::HandlePing(WorldPacket & recvData)
{
	uint32 count;
	recvData >> count;

	tagGatewayConnectionData *info = sInfoCore.GetGatewayConnectInfo(m_AssociateGatewayID);
	if (info != NULL)
	{
		sInfoCore.counterLock.Acquire();
		sInfoCore.m_PlayerCounter[m_AssociateGatewayID].plrCounter = count;
		sInfoCore.counterLock.Release();
	}

	//本地数据包队列处理时间过长，PONG包返回是否直接放到网络线程接收的地方？
	uint32 curTime = time(NULL);
	int32 diff = curTime - last_ping - SERVER_PING_INTERVAL;
	if(diff >= 2)
		sLog.outString("LoginServer Pong delay %d sec", diff);

	last_ping = curTime;
	// sLog.outDebug("LoginServer Pong back to Gateway [%u] at %u", m_AssociateGatewayID, last_ping);

	WorldPacket data(SMSG_LOGIN_2_GATE_PONG, 4);
	SendPacket(&data);
}

void LogonCommServerSocket::SendPacket(WorldPacket *data)
{
	const uint32 pckSize = data->size();
	const uint32 ret = SendingData((pckSize > 0 ? (void*)data->contents() : NULL), pckSize, data->GetOpcode(), data->GetServerOpCode(), data->GetUserGUID(), data->GetErrorCode(), data->GetPacketIndex());
	if (ret != Send_tcppck_err_none)
		sLog.outError("[登陆->网关%u]发包时遇到其他错误(err=%u),op=%u,packSize=%u", m_AssociateGatewayID, ret, data->GetOpcode(), data->size());
}

void LogonCommServerSocket::HandleReloadAccounts(WorldPacket & recvData)
{
	sAccountMgr.ReloadAccounts(true);
}

void LogonCommServerSocket::HandleAuthChallenge(WorldPacket & recvData)
{
	unsigned char key[20];
	uint32 ret = Server_auth_ret_success;
	recvData.read(key, 20);
	
	// check if we have the correct password
	SHA_1 sha1;
	int sha1Result = sha1.SHA1Result(&sInfoCore.m_gatewayAuthSha1Data, key);
	if(sha1Result != shaSuccess)
	{
		ret = Server_auth_ret_err_key;
		sLog.outString("ERROR:网关的Key不匹配，拒绝连接,sha1Result=%d", sha1Result);
	}
	
	if (ret == Server_auth_ret_success)
		sLog.outString("Gateway Authentication request from %s, result=OK.",(char*)inet_ntoa(m_peer.sin_addr));
	else
		sLog.outError("Gateway Authentication request from %s, result=FAIL.",(char*)inet_ntoa(m_peer.sin_addr));

	printf("Key: ");
	for(int i = 0; i < 20; ++i)
		printf("%.2X", key[i]);
	printf("\n");

	/* packets are encrypted from now on */
	// 暂时不对包进行加密
	/*SetRecvPackCryptoType(PacketCrypto_recv_all, key, 20);
	SetSendPackCryptoType(PacketCrypto_send_all, key, 20);*/

	/* send the response packet */
	WorldPacket packet(SMSG_LOGIN_2_GATE_AUTH_RESULT, 32);
	packet << ret;
	
	/* set our general var */
	authenticated_flag = (ret == Server_auth_ret_success) ? true : false;
	if(authenticated_flag)
	{	
		tagGatewayConnectionData *data = new tagGatewayConnectionData;
		recvData >> data->Name >> data->Address >> data->Port;

		// Address是"ip:端口"的形式,在这里吧ip取出来,方便给客户端使用
		size_t pos = data->Address.find(":");
		ASSERT(pos != string::npos && "登陆服获取到的网关登陆地址并非有效的");
		data->IP = data->Address.substr(0, pos);

		uint32 my_id = sInfoCore.GenerateGatewayInfoID();
		sLog.outString("Registering gateway `%s` under ID %u.", data->Address.c_str(), my_id);

		// Add to the main realm list
		sInfoCore.AddGatewayConnectInfo(my_id, data);
		m_AssociateGatewayID = my_id;

		tagGatewayPlayerCountData plrData;
		plrData.ipAddr = data->IP;
		plrData.plrCounter = 0;
		plrData.port = data->Port;
		sInfoCore.counterLock.Acquire();
		sInfoCore.m_PlayerCounter.insert(make_pair(my_id, plrData));
		sInfoCore.counterLock.Release();

		// 返回包
		packet << my_id;				// Realm ID
		packet << data->Address;
		SendPacket(&packet);
	}
	else
	{
		SendPacket(&packet);
	}
}

