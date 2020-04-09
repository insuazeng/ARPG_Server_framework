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
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"
#include "config/ConfigEnv.h"
#include "auth/Sha1.h"

extern bool m_ShutdownEvent;

LogonCommHandler::LogonCommHandler()
{
	next_request = 1;

	m_pLoginServerSocket = NULL;
	m_LoginServerData.Registered = false;
	m_LoginServerData.RetryTime = 0;
	
	string logon_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "LoginServer", "change_me_logon");
	char szLogonPass[20] = { 0 };
	snprintf(szLogonPass, 20, "%s", logon_pass.c_str());
	SHA_1 sha1;
	sha1.SHA1Reset(&m_sha1AuthKey);
	sha1.SHA1Input(&m_sha1AuthKey, (uint8*)szLogonPass, 20);
}

LogonCommHandler::~LogonCommHandler()
{

}

void LogonCommHandler::Startup()
{
	// 初始化登陆服包处理函数列表
	LogonCommClient::InitPacketHandler();

	// Try to connect to all logons.
	sLog.outColor(TNORMAL, "\n >> loading realms and logon server definitions... ");
	
	// 读取要连接的登录服务器配置，并加入servers中，目前只有一个
	LoadRealmConfiguration();

	sLog.outColor(TNORMAL, " >> attempting to connect to all logon servers... \n");
	ConnectLoginServer(&m_LoginServerData);

	sLog.outColor(TNORMAL, "\n");
}

void LogonCommHandler::ConnectLoginServer(LoginServer * server)
{
	sLog.outString(">> connecting to LoginServer:`%s` on `%s:%u`...", server->remoteServerName.c_str(), server->remoteServerAddress.c_str(), server->remoteServerPort);
	server->RetryTime = time(NULL) + 5;
	server->Registered = false;
	LogonCommClient *conn = ConnectTCPSocket<LogonCommClient>(server->remoteServerAddress.c_str(), server->remoteServerPort, false, 2);
	if(conn == 0)
	{
		sLog.outColor(TRED, " fail!\n	   server connection failed. I will try again later.");
		sLog.outColor(TNORMAL, "\n");
		return;
	}
	sLog.outColor(TGREEN, " ok!\n");
	sLog.outColor(TNORMAL, "        >> authenticating...\n");
	sLog.outColor(TNORMAL, "        >> ");

	uint64 tt = getMSTime64()+500;
	conn->SendChallenge();
	sLog.outColor(TNORMAL, "        >> result:");
	while(!conn->authenticated_ret)
	{
		if(getMSTime64() >= tt)
		{
			sLog.outColor(TYELLOW, " timeout.\n");
			break;
		}
		sLog.outColor(TNORMAL, " .");
		Sleep(20);
	}

	if(!conn->authenticated_ret)
	{
		sLog.outColor(TRED, " failure.\n");
		conn->Disconnect();
		return;
	}
	else
		sLog.outColor(TGREEN, " ok!\n");

	m_pLoginServerSocket = conn;
	m_pLoginServerSocket->SendPing();

	sLog.outColor(TNORMAL, "\n        >> ping test: ");
	sLog.outColor(TYELLOW, "%ums", m_pLoginServerSocket->latency);
	sLog.outColor(TNORMAL, "\n");
}

void LogonCommHandler::AdditionAck(uint32 ServID)
{
	m_LoginServerData.registeredIDOnRemoteServer = ServID;
	m_LoginServerData.Registered = true;
}

void LogonCommHandler::UpdateSockets()
{
	mapLock.Acquire();
	LogonCommClient *cs = m_pLoginServerSocket;
	uint32 t = time(NULL);

	// 连接在的话，检查ping，如果不在，试图重连
	if(cs != NULL)
	{
		if(cs->last_pong_unixtime < t)
		{
			int32 time_diff = t - cs->last_pong_unixtime;
			if (time_diff > 60)
			{
				// no pong for 60 seconds -> remove the socket
				sLog.outError(" 由于连接超时 断开登录服务器 id %u connection dropped due to pong timeout.", m_LoginServerData.remoteServerID);
				cs->Disconnect();
				m_pLoginServerSocket = NULL;
				cs = NULL;
			}
		}

		// 发送ping
		if (cs != NULL && (t - cs->last_ping_unixtime) > SERVER_PING_INTERVAL)
		{
			// send ping
			cs->SendPing();
		}
	}
	else
	{
		// check retry time
		if(t >= m_LoginServerData.RetryTime && !m_ShutdownEvent)
		{
			ConnectLoginServer(&m_LoginServerData);
		}
	}

	mapLock.Release();
}

void LogonCommHandler::ConnectionDropped()
{
	sLog.outColor(TNORMAL, " >> login server connection was dropped unexpectedly. reconnecting next loop.\n");

	mapLock.Acquire();
	m_pLoginServerSocket = NULL;
	mapLock.Release();
}

uint32 LogonCommHandler::GenerateRequestID()
{
	uint32 uRet = ILLEGAL_DATA_32;
	
	pendingLock.Acquire();
	uRet = next_request++;
	pendingLock.Release();

	return uRet;
}

bool LogonCommHandler::ClientConnected(string &accountName, GameSocket *clientSocket)
{
	// Send request packet to login server.

	if (m_pLoginServerSocket == NULL)
		return false;

	uint32 req_id = clientSocket->m_nAuthRequestID;
	pendingLock.Acquire();
	m_pendingAuthSockets[req_id] = clientSocket;

	clientSocket->m_AuthStatus |= GSocket_auth_state_sended_packet;

	WorldPacket data(CMSG_GATE_2_LOGIN_ACCOUNT_AUTH_REQ, 100);
	data << req_id;
	data << accountName;
	data << clientSocket->m_strAuthSessionKey;
	data << clientSocket->GetIP();
	m_pLoginServerSocket->SendPacket(&data);

	pendingLock.Release();
	sLog.outString("[会话验证请求]reqID=%u,accName=%s,sessionKey=%s传递给登陆服", req_id, accountName.c_str(), clientSocket->m_strAuthSessionKey.c_str());

	return true;
}

void LogonCommHandler::UnauthedSocketClose(uint32 id)
{
	pendingLock.Acquire();
	m_pendingAuthSockets.erase(id);
	pendingLock.Release();
}

void LogonCommHandler::RemoveUnauthedSocket(uint32 id)
{
	pendingLock.Acquire();
	m_pendingAuthSockets.erase(id);
	pendingLock.Release();
}

void LogonCommHandler::LoadRealmConfiguration()
{
	//读取要连接的登录服务器socket信息
	m_LoginServerData.remoteServerID = 0;
	m_LoginServerData.remoteServerName = Config.GlobalConfig.GetStringDefault("LoginServer", "Name", "UnkLogon");
	m_LoginServerData.remoteServerAddress = Config.GlobalConfig.GetStringDefault("LoginServer", "Host", "127.0.0.1");
	m_LoginServerData.remoteServerPort = Config.GlobalConfig.GetIntDefault("LoginServer", "ServerPort", 8293);
	sLog.outColor(TYELLOW, "1 logon servers, ");
}
