#include "stdafx.h"
#include "config/ConfigEnv.h"
#include "SceneCommHandler.h"
#include "LogonCommHandler.h"
// #include "LanguageMgr.h"

extern bool m_ShutdownEvent;
extern GatewayServer g_local_server_conf;

CSceneCommHandler::CSceneCommHandler(void)
{
	g_totalSendBytesToClient=0;
	g_totalRecvBytesFromClient=0;

	g_transpacketnum=0;
	g_transpacketsize=0;

	g_totalSendDataToSceneServer=0;
	g_totalRecvDataFromSceneServer=0;

	g_totalSendDataToMasterServer=0;
	g_totalRecvDataFromMasterServer=0;

	m_nSessionIndexHigh = 0;

	string scene_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "GameServer", "r3m0t3b4d");
	char szScenePass[20] = { 0 };
	snprintf(szScenePass, 20, "%s", scene_pass.c_str());

	SHA_1 sha1;
	sha1.SHA1Reset(&m_sha1AuthKey);
	sha1.SHA1Input(&m_sha1AuthKey, (uint8*)szScenePass, 20);

	//memset(m_clientPacketCounter,0,NUM_MSG_TYPES*sizeof(uint32));
	memset(m_sceneServerPacketCounter, 0, sizeof(m_sceneServerPacketCounter));
	memset(m_masterServerPacketCounter, 0, sizeof(m_masterServerPacketCounter));
}

CSceneCommHandler::~CSceneCommHandler(void)
{
	for(std::vector<SceneServer*>::iterator i = m_SceneServerConfigs.begin(); i != m_SceneServerConfigs.end(); ++i)
	{
		delete (*i);
	}

	if (m_pMasterServerInfo)
	{
		delete m_pMasterServerInfo;
		m_pMasterServerInfo = NULL;
	}
	delete LanguageMgr::GetSingletonPtr();
}

CSceneCommClient* CSceneCommHandler::ConnectToSceneServer(string Address, uint32 Port, bool blockingConnect /* = true */, int32 timeOutSec /* = 1 */)
{
	CSceneCommClient *gameCommClinet = ConnectTCPSocket<CSceneCommClient>(Address.c_str(), Port, blockingConnect, timeOutSec);
	return gameCommClinet;
}

CMasterCommClient* CSceneCommHandler::ConnectToCentreServer(string Address, uint32 Port)
{
	CMasterCommClient* centreCommClient = ConnectTCPSocket<CMasterCommClient>(Address.c_str(), Port);
	return centreCommClient;
}

void CSceneCommHandler::Startup()
{
	CSceneCommClient::InitPacketHandlerTable();
	CMasterCommClient::InitPacketHandlerTable();

	// 读所有配置文件
	sLog.outColor(TNORMAL, "\n >> loading realms and scene / master server definitions... ");
	LoadRealmConfiguration();

	// 先连接中央服务器
	sLog.outColor(TNORMAL, "\n >> attempting to connect to master server... \n");
	if(m_pMasterServerInfo!=NULL)
		ConnectCentre(m_pMasterServerInfo);

	// 再尝试去连接所有游戏服务器
	sLog.outColor(TNORMAL, " >> attempting to connect to all scene servers... \n");
	for(std::vector<SceneServer*>::iterator itr = m_SceneServerConfigs.begin(); itr != m_SceneServerConfigs.end(); ++itr)
	{
		ConnectSceneServer(*itr);
	}
	sLog.outColor(TNORMAL, "\n");

#ifdef _WIN64
	uint32 recordcount = LanguageMgr::GetSingleton().LoadForbidWords("data/forbid.txt");
#else
	uint32 recordcount = LanguageMgr::GetSingleton().LoadForbidWords("data/forbid_linux.txt");
#endif
	sLog.outString(" %u forbidwords loaded!",recordcount);
}

void CSceneCommHandler::ConnectionDropped(uint32 ID)
{
	mapLock.Acquire();
	map<SceneServer*, CSceneCommClient*>::iterator itr = m_sceneServers.begin();
	for(; itr != m_sceneServers.end(); ++itr)
	{
		if(itr->first->remoteServerID == ID && itr->second != 0)
		{
			sLog.outColor(TNORMAL, " >> realm id %u connection was dropped unexpectedly. reconnecting next loop.",ID);
			sLog.outColor(TNORMAL, "\n");
			itr->second = 0;
			break;
		}
	}
	mapLock.Release();
	//add by 贡文斌 2种断开的情况都会调用这里，放这里比较合理。2010.2.25
	CloseAllGameSockets(ID);
	OnGameServerConnectionModified();
}

void CSceneCommHandler::OnGameServerConnectionModified()
{
	WorldPacket packet;
	FillValidateGameServerInfo(packet);
	
	mapLock.Acquire();
	
	packet.SetServerOpcodeAndUserGUID(CMSG_GATE_2_SCENE_SERVER_CONNECTION_STATUS, 0);
	SendPacketToSceneServer(&packet, 0xffffffff);

	packet.SetServerOpcodeAndUserGUID(CMSG_GATE_2_MASTER_SERVER_CONNECTION_STATUS, 0);
	SendPacketToMasterServer(&packet);
	
	mapLock.Release();
}

void CSceneCommHandler::FillValidateGameServerInfo(WorldPacket &pck)
{
	uint32 counter = 0;
	pck << counter;
	mapLock.Acquire();
	map<SceneServer*, CSceneCommClient*>::iterator it = m_sceneServers.begin();
	for (; it != m_sceneServers.end(); ++it)
	{
		if (it->second != NULL)
		{
			pck << it->first->remoteServerID;
			++counter;
		}
	}
	mapLock.Release();
	if (counter > 0)
		pck.put(0, (uint8*)&counter, sizeof(uint32));
}

void CSceneCommHandler::CentreConnectionDropped()
{
	sLog.outColor(TNORMAL, " >> CenterServer connection lose  . reconnecting next loop.");
	sLog.outColor(TNORMAL, "\n");

	mapLock.Acquire();
	m_pMasterServer = NULL;
	m_pMasterServerInfo->Registered = false;
	mapLock.Release();

	//断开当前网关上的所有链接。中央服那边在收到网关断开事件时，应该把和该网关相关的玩家都置成等待下线数据状态。如果游戏服和中央服之间仍然有网关链接，则可以顺利保存
	CloseAllGameSockets(0);
}

void CSceneCommHandler::AdditionAck(uint32 ID, uint32 ServID)
{
	map<SceneServer*, CSceneCommClient*>::iterator itr = m_sceneServers.begin();
	for(; itr != m_sceneServers.end(); ++itr)
	{
		if(itr->first->remoteServerID == ID)
		{
			itr->first->registeredIDOnRemoteServer = ServID;
			itr->first->Registered = true;
			break;
		}
	}
}

void CSceneCommHandler::AdditionAckFromCentre(uint32 id)
{
	m_pMasterServerInfo->remoteServerID = id;
	m_pMasterServerInfo->Registered = true;
}

void CSceneCommHandler::UpdateSockets()
{
	mapLock.Acquire();

	map<SceneServer*, CSceneCommClient*>::iterator itr = m_sceneServers.begin();
	CSceneCommClient *cs = NULL;
	
	uint32 t = time(NULL);

	for(; itr != m_sceneServers.end(); ++itr)
	{
		cs = itr->second;
		if(cs != 0)
		{
			if(cs->m_lastPongUnixtime < t && ((t - cs->m_lastPongUnixtime) > 60))
			{
				// no pong for 60 seconds -> remove the socket
				sLog.outError(" ERROR:场景服务器 %u ping超时%d秒，%d链接丢失.", itr->first->remoteServerID, (t - cs->m_lastPongUnixtime), t);

				if(t-cs->m_lastPingUnixtime > 40)
				{
					sLog.outError("是网关没有ping，不断开.\n");
					cs->SendPing();
					cs->m_lastPongUnixtime = cs->m_lastPingUnixtime;//重置
					continue;
				}
				// 删除全部客户端套接字(Disconnect掉)
				//CloseAllGameSockets();
				cs->Disconnect();
				//cs->_id = 0;
				itr->second = 0;
				continue;
			}

			if( (t - cs->m_lastPingUnixtime) > SERVER_PING_INTERVAL )
			{
				// send a ping packet.
				cs->SendPing();
				sLog.outDebug("ping scene server");
			}
		}
		else
		{
			// check retry time
			if(itr->first->IsOpen && (t >= itr->first->RetryTime)&&!m_ShutdownEvent)
			{
				ConnectSceneServer(itr->first);
			}
		}
	}

	if (m_pMasterServer != NULL)
	{
		if ( (t - m_pMasterServer->m_lastPingUnixTime) > SERVER_PING_INTERVAL )
		{
			// send a ping packet.
			m_pMasterServer->SendPing();
			sLog.outDebug("ping master server");
		}

		if (m_pMasterServer != NULL && m_pMasterServer->m_lastPongUnixTime < t && ((t - m_pMasterServer->m_lastPongUnixTime) > 60))
		{
			// no pong for 60 seconds -> remove the socket
			sLog.outError(" ERROR:中央服务器 %u ping超时%d秒，链接丢失.", m_pMasterServer->m_nThisGateId, (t - m_pMasterServer->m_lastPongUnixTime));
			m_pMasterServer->Disconnect();
			m_pMasterServer = NULL;
		}
	}
	else
	{
		if(t >= m_pMasterServerInfo->RetryTime&&!m_ShutdownEvent)
		{
			ConnectCentre(m_pMasterServerInfo);
		}
	}

	mapLock.Release();
}

void CSceneCommHandler::CloseAllServerSockets()
{
	mapLock.Acquire();

	map<SceneServer*, CSceneCommClient*>::iterator itr = m_sceneServers.begin();
	CSceneCommClient * cs;
	uint32 t = time(NULL);
	for(; itr != m_sceneServers.end(); ++itr)
	{
		cs = itr->second;
		if(cs != 0)
		{
			sLog.outString(" 主动关闭与场景服的链接.\n");
			cs->Disconnect();
			itr->second = 0;
		}
	}

	if (m_pMasterServer != NULL)
	{
		sLog.outString(" 主动关闭与中央服的链接.\n");
		m_pMasterServer->Disconnect();
		m_pMasterServer = NULL;
	}

	mapLock.Release();
}

void CSceneCommHandler::UpdateGameSockets()
{
	//static const char *szLoginType[] = { "正常登陆", "延迟登陆" };

	GameSocket *pPlayerSocket = NULL;
	uint64 curTime = getMSTime64();
	GameSocketMap::iterator it = online_players.begin();
	GameSocketMap::iterator itcur;
	
	while (it != online_players.end())
	{
		// 超时便通知游戏服务器删除该会话
		itcur = it;
		it++;
		pPlayerSocket = itcur->second;

		if (pPlayerSocket != NULL)
		{
			int32 timeDiff = curTime - pPlayerSocket->m_lastPingRecvdMStime;
			if (timeDiff > 30000)
			{
				// 超时30秒客户端得不到回应，则将该客户端删除，并通知服务器删除该客户端的会话对象
				sLog.outString("[GameSocket]ping超时(%u毫秒,curTime="I64FMTD",lastRecv="I64FMTD")即将断开,name=%s(guid="I64FMTD"),authStatus=%u", timeDiff, curTime, pPlayerSocket->m_lastPingRecvdMStime,
								pPlayerSocket->m_strAccountName.c_str(), pPlayerSocket->GetPlayerGUID(), (uint32)pPlayerSocket->m_AuthStatus);
				pPlayerSocket->Disconnect();
			}
			
			//else if((theTime-pPlayerSocket->m_GameStartTime)>240&&pPlayerSocket->mClientStatus<2)//四分钟不操作断线
			//{
			//	sLog.outString("Client login timeout,disconnect,accountid＝%d",itcur->first);
			//	pPlayerSocket->Disconnect();
			//}

			if(pPlayerSocket->hadSessionAuthSucceeded())
			{
				pPlayerSocket->UpdateQueuedPackets();
			}
			if(!pPlayerSocket->hadSessionAuthSucceeded())
			{				
				__RemoveOnlineGameSocket(itcur->first);
			}
		}
	}
}

void CSceneCommHandler::UpdatePendingGameSockets()
{
	uint64 curMsTime = getMSTime64();
	// 请求登陆的账号列表
	pendingGameLock.Acquire();
	if (!m_pendingGameSockets.empty())
	{
		uint16 login_counter = 0;
		GameSocket *sock = NULL;
		GameSocketMap::iterator it = m_pendingGameSockets.begin(), it2;

		for (; it != m_pendingGameSockets.end(); )
		{
			it2 = it++;
			uint64 guid = it2->first;
			sock = it2->second;
			if(__CheckGameSocketOnlineAndDisconnect(sock->m_strClientAgent, sock->m_strAccountName))		// 网关依然保持该Socket的连接，等断开再进行连接
				continue;
			// 如果未到允许的重登陆时间,也再等一下
			if (sock->m_ReloginNextMsTime > 0 && curMsTime < sock->m_ReloginNextMsTime)
				continue;

			// 如果sock还未得到登陆验证返回，也暂时不进入游戏
			if(!sock->hadGotSessionAuthResult())
				continue;

			// 添加至在线玩家列表
			__AddGameSocketToOnlineList(guid, sock);

			// 从等待连入游戏的列表中移除
			m_pendingGameSockets.erase(it2);

			// 请求至中央服登陆(获取角色列表)
			WorldPacket packet(CMSG_GATE_2_MASTER_ROLELIST_REQ, 128);
			packet << sock->GetPlayerGUID();
			packet << sock->m_strClientAgent;
			packet << sock->m_strAccountName;
			packet << sock->m_SessionIndex;						//应该发送能唯一标示这次登陆的数字
			packet << sock->m_GMFlag;
			SendPacketToMasterServer(&packet);

			cLog.Success("Login", "对话登陆加入成功(代理=%s,账号=%s)向中央服请求角色列表", sock->m_strClientAgent.c_str(), sock->m_strAccountName.c_str());
		}
	}
	pendingGameLock.Release();
}

void CSceneCommHandler::UpdateClientSendQueuePackets()
{
	mapLock.Acquire();
	for (map<SceneServer*, CSceneCommClient*>::iterator it = m_sceneServers.begin(); it != m_sceneServers.end(); ++it)
	{
		if ((it->second) != NULL)
		{
			(*(it->second)).UpdateServerSendQueuePackets();
		}
	}
	mapLock.Release();
}

void CSceneCommHandler::UpdateClientRecvQueuePackets()
{
	uint64 startTime = getMSTime64();
	map<SceneServer*, CSceneCommClient*>::iterator iterGame = m_sceneServers.begin();
	for ( ; iterGame != m_sceneServers.end(); ++iterGame)
	{
		if ((iterGame->second) != NULL)
		{
			(*(iterGame->second)).UpdateServerRecvQueuePackets();
		}
	}
	uint64 startTime2 = getMSTime64();
	if (startTime2 - startTime > 2000)
		sLog.outError("网关 与游戏服 Socket update时间过长 %u ms", startTime2 - startTime);
	
	// update从中央服收到的packet
	if (m_pMasterServer != NULL)
		m_pMasterServer->UpdateServerRecvQueuePackets();

	uint32 diff = getMSTime64() - startTime2;
	if (diff > 2000)
		sLog.outError("网关 与中央服 Socket update时间过长 %u ms", diff);
}

void CSceneCommHandler::ConnectSceneServer(SceneServer *server)
{
	if(server->IsOpen)
		sLog.outString("	>> connecting to SceneServer %d:`%s` on `%s:%u`...", server->remoteServerID, server->remoteServerName.c_str(), server->remoteServerAddress.c_str(), server->remoteServerPort);
	else
	{
		m_sceneServers[server] = 0;
		return ;
	}
	
	server->RetryTime = time(NULL) + 3;	// 随失败次数延长重连间隔
	server->Registered = false;
	bool blockingConnect = true;
	
	CSceneCommClient * conn = ConnectToSceneServer(server->remoteServerAddress, server->remoteServerPort, blockingConnect);
	m_sceneServers[server] = conn;
	if(conn == 0)
	{
		if(server->IsOpen)
		{
			sLog.outError(" fail!\n	   server connection failed. I will try again later.");
		}
		return;
	}

	sLog.outString("ok!");
	sLog.outString("        >> authenticating...");

	uint64 tt = getMSTime64() + 300;
	
	conn->SendChallenge(server->remoteServerID);
	sLog.outString("        >> result:");
	while(!conn->authenticated)
	{
		if(getMSTime64() >= tt)
		{
			sLog.outString(" timeout..");
			break;
		}
		Sleep(30);
	}

	if(conn->authenticated != 1)
	{
		sLog.outString(" failure..");
		m_sceneServers[server] = 0;
		conn->Disconnect();
		return;
	}
	else
	{
		sLog.outString("ok!");
	}

	// Send the initial ping
	conn->SendPing();
	sLog.outString("        >> registering realms... ");
	conn->_id = server->remoteServerID;
	sLog.outString("        >> ping test: %u ms", conn->latency_ms);

	// 连入某个场景服，需要通知给全部
	OnGameServerConnectionModified();
}

void CSceneCommHandler::ConnectCentre(MasterServer *server)
{
	sLog.outString("	>> connecting to MasterServer:`%s` on `%s:%u`...", server->remoteServerName.c_str(), server->remoteServerAddress.c_str(), server->remoteServerPort);
	server->RetryTime = time(NULL) + 3;
	server->Registered = false;

	m_pMasterServer = ConnectToCentreServer(server->remoteServerAddress, server->remoteServerPort);
	if(m_pMasterServer == 0)
	{
		sLog.outColor(TRED, " fail!\n	   server connection failed. I will try again later.");
		sLog.outColor(TNORMAL, "\n");
		return;
	}
	sLog.outColor(TGREEN, " ok!\n");
	sLog.outColor(TNORMAL, "        >> authenticating...\n");
	sLog.outColor(TNORMAL, "        >> ");
	
	uint64 tt = getMSTime64() + 300;
	m_pMasterServer->SendChallenge( server->remoteServerID );
	sLog.outColor(TNORMAL, "        >> result:");
	
	while(m_pMasterServer!=NULL&&!m_pMasterServer->authenticated)
	{
		if(getMSTime64() >= tt)
		{
			sLog.outColor(TYELLOW, " timeout.\n");
			break;
		}
		sLog.outColor(TNORMAL, " .");
		Sleep(30);
	}

	if(m_pMasterServer==NULL || m_pMasterServer->authenticated != 1)
	{
		sLog.outColor(TRED, " failure.\n");
		if(m_pMasterServer!=NULL)
			m_pMasterServer->Disconnect();
		m_pMasterServer =NULL;
		return;
	}
	else
		sLog.outColor(TGREEN, " ok!\n");

	// Send the initial ping
	m_pMasterServer->SendPing();

	sLog.outColor(TNORMAL, "        >> registering realms... ");

	m_pMasterServer->m_nThisGateId = server->remoteServerID;

	sLog.outColor(TNORMAL, "\n        >> ping test: ");
	sLog.outColor(TYELLOW, "%u ms", m_pMasterServer->latency_ms);
	sLog.outColor(TNORMAL, "\n");

	OnGameServerConnectionModified();
}

void CSceneCommHandler::LoadRealmConfiguration()
{
	int32 nSceneServerCount = GetStartParmInt("scount");
	ASSERT(nSceneServerCount > 0 && "待连接的场景服数量必须大于1!");

	for (int i = 0; i < nSceneServerCount; ++i)
	{
		char szBlock[64] = { 0 };
		snprintf(szBlock,sizeof(szBlock),"SceneServer%d",i+1);
		//游戏服务器配置
		SceneServer * ls = new SceneServer;
		ls->remoteServerID = Config.GlobalConfig.GetIntDefault(szBlock, "GameID", 1);
		ls->remoteServerName = Config.GlobalConfig.GetStringDefault(szBlock, "Name", "默认gameserver");
		ls->remoteServerAddress = Config.GlobalConfig.GetStringDefault(szBlock, "Host", "127.0.0.1");
		ls->remoteServerPort = Config.GlobalConfig.GetIntDefault(szBlock, "ListenPort", 0);
		
		ls->IsOpen = true;
		if (false)
		{
			// 这里可能会有默认的开关，用来表示该网关是否连接这个场景服
			ls->IsOpen = false;
		}

		if(ls->remoteServerPort != 0)
			m_SceneServerConfigs.push_back(ls);
	}
	sLog.outColor(TYELLOW, "%d scene servers, ", nSceneServerCount);

	// 读取跨服服务器配置
	int32 openCrossServer = Config.GlobalConfig.GetIntDefault("CrossServer", "Open", 0);
	if (openCrossServer > 0)
	{
		SceneServer *ls = new SceneServer;
		ls->remoteServerID = Config.GlobalConfig.GetIntDefault("CrossServer", "GameID", 10);
		ls->remoteServerName = Config.GlobalConfig.GetStringDefault("CrossServer", "Name", "默认跨服");
		ls->remoteServerAddress = Config.GlobalConfig.GetStringDefault("CrossServer", "Host", "127.0.0.1");
		ls->remoteServerPort = Config.GlobalConfig.GetIntDefault("CrossServer", "ListenPort", 0);

		ls->IsOpen = true;
		if(ls->remoteServerPort != 0)
			m_SceneServerConfigs.push_back(ls);
	}
	sLog.outColor(TYELLOW, "1 cross servers ..\n");

	// 到登陆服务器注册用的服务器信息,一个网关就一个可连接服务器
	g_local_server_conf.localServerName = Config.GlobalConfig.GetStringDefault("GateServer", "Name", "默认网关服务器");
	g_local_server_conf.localServerAddress = Config.GlobalConfig.GetStringDefault("GateServer", "Host", "192.168.0.88");
	g_local_server_conf.localServerListenPort = Config.GlobalConfig.GetIntDefault("GateServer", "ListenPort", 0);
	g_local_server_conf.remoteServerID = Config.GlobalConfig.GetIntDefault("Global", "ServerID", 0);
	g_local_server_conf.globalServerID = Config.GlobalConfig.GetIntDefault("Global", "ServerID", 0);

	sLog.outColor(TBLUE, "current gateway Name=%s Address=%s Port=%u.\n", g_local_server_conf.localServerName.c_str(), g_local_server_conf.localServerAddress.c_str(), 
					g_local_server_conf.localServerListenPort);

	// 读取中央服务器的相关配置
	m_pMasterServerInfo = new MasterServer;
	m_pMasterServerInfo->remoteServerID = 0;
	m_pMasterServerInfo->remoteServerName = Config.GlobalConfig.GetStringDefault("MasterServer", "Name", "默认centreServer");
	m_pMasterServerInfo->remoteServerAddress = Config.GlobalConfig.GetStringDefault("MasterServer", "Host", "127.0.0.1");
	m_pMasterServerInfo->remoteServerPort = Config.GlobalConfig.GetIntDefault("MasterServer", "ListenPort", 0);
}

void CSceneCommHandler::RegisterReloginAccountTokenData(string agent_name, string &acc_name, uint64 guid, string &token_data, uint32 index, uint32 gm_flag)
{
	tagGameSocketCacheData data;
	data.plr_guid = guid;
	data.agent_name = agent_name;

	// 这里注册一个新的SessionIndex，其实是为了断线重连时做新的index用
	data.session_index = GenerateSessionIndex();

	data.gm_flag = gm_flag;
	data.relogin_valid_expire_time = UNIXTIME + TIME_HOUR * 12;		// 暂定12小时的重登陆有效期
	data.acc_name = acc_name;
	data.token_data = token_data;

	sLog.outString("[会话令牌注册]Acc=%s,Token=%s,到期时间=%s", acc_name.c_str(), token_data.c_str(), ConvertTimeStampToMYSQL(data.relogin_valid_expire_time).c_str());

	reloginGameSocketLock.Acquire();
	m_gamesock_login_token.insert(make_pair(acc_name, data));
	reloginGameSocketLock.Release();
}

void CSceneCommHandler::RemoveReloginAccountData(string &acc_name)
{
	reloginGameSocketLock.Acquire();
	if (m_gamesock_login_token.find(acc_name) != m_gamesock_login_token.end())
	{
		m_gamesock_login_token.erase(acc_name);
		sLog.outString("[会话令牌移除] accName=%s", acc_name.c_str());
	}
	reloginGameSocketLock.Release();
}

int32 CSceneCommHandler::CheckAccountReloginValidation(string &acc_name, string &recv_token)
{
	tagGameSocketCacheData *data = GetCachedGameSocketData(acc_name);
	// 不存在该玩家的登陆key
	if (data == NULL)			
		return -1;

	// 登陆key不匹配
	int32 ret = data->token_data.compare(recv_token);
	if (ret != 0)
	{
		RemoveReloginAccountData(acc_name);
		return -2;
	}
	// 登陆key超时失效
	if (UNIXTIME >= data->relogin_valid_expire_time)
	{
		RemoveReloginAccountData(acc_name);
		return -3;
	}
	return 0;
}

tagGameSocketCacheData* CSceneCommHandler::GetCachedGameSocketData(string &acc_name)
{
	tagGameSocketCacheData *data = NULL;
	reloginGameSocketLock.Acquire();
	HM_NAMESPACE::hash_map<string, tagGameSocketCacheData>::iterator it = m_gamesock_login_token.find(acc_name);
	if (it != m_gamesock_login_token.end())
		data = &(it->second);
	reloginGameSocketLock.Release();
	return data;
}

void CSceneCommHandler::AddPendingGameList(GameSocket *playerSocket, uint64 tempGuid)
{
	if (playerSocket == NULL)
		return ;

	// 如果该socket在线,则进行断开,并且不让其立即请求至中央服登陆,而是延迟1000毫秒
	bool existSocket = __CheckGameSocketOnlineAndDisconnect(playerSocket->m_strClientAgent, playerSocket->m_strAccountName);
	if (existSocket)
		playerSocket->m_ReloginNextMsTime = getMSTime64() + 1000;

	// 把socket放入等待游戏的列表
	pendingGameLock.Acquire();
	if (m_pendingGameSockets.find(tempGuid) != m_pendingGameSockets.end())
	{
		ASSERT(m_pendingGameSockets[tempGuid]->m_uSceneServerID == 0);
		m_pendingGameSockets[tempGuid]->Disconnect();
		m_pendingGameSockets.erase(tempGuid);
	}
	m_pendingGameSockets.insert(make_pair(tempGuid, playerSocket));
	pendingGameLock.Release();
}

void CSceneCommHandler::RemoveDisconnectedSocketFromPendingGameList(uint64 guid)
{
	// 把socket放入等待游戏的列表
	pendingGameLock.Acquire();
	if (m_pendingGameSockets.find(guid) != m_pendingGameSockets.end())
	{
		sLog.outString("Socket断开,移除等待连入游戏的会话(guid="I64FMTD")", guid);
		m_pendingGameSockets.erase(guid);
	}
	pendingGameLock.Release();
}

void CSceneCommHandler::SendCloseSessionNoticeToScene(uint32 scene_server_id, uint64 guid, uint32 session_index)
{
	// 请求中央服与游戏服一共对账号进行关闭
	WorldPacket packet(CMSG_GATE_2_SCENE_CLOSE_SESSION_REQ, 16);
	packet << g_local_server_conf.globalServerID;
	packet << guid << session_index;
	SendPacketToSceneServer(&packet, scene_server_id);
	sLog.outString("[会话关闭]通知[场景服%u]关闭会话guid="I64FMTD",sessionIndex=%u", scene_server_id, guid, session_index);
}

void CSceneCommHandler::SendCloseServantNoticeToMaster(uint64 guid, uint32 session_index, bool direct)
{
	WorldPacket packet(CMSG_GATE_2_MASTER_CLOSE_SESSION_REQ, 16);
	packet << guid << session_index << direct;
	SendPacketToMasterServer(&packet);
	sLog.outString("[会话关闭]通知[中央服]关闭会话guid="I64FMTD",sessionIndex=%u,direct=%d", guid, session_index, direct);
}

void CSceneCommHandler::UpdateGameSocektGuid(uint64 tempGuid, uint64 realGuid, GameSocket *sock)
{
	onlineLock.Acquire();
	online_players.erase(tempGuid);
	sock->m_PlayerGUID = realGuid;
	online_players.insert(make_pair(realGuid, sock));
	onlineLock.Release();
}

void CSceneCommHandler::AddNewPlayerToSceneServer( GameSocket * playerSocket, ByteBuffer *enterData, uint32 targetSceneServerID )
{
	WorldPacket packet( CMSG_GATE_2_SCENE_ENTER_GAME_REQ, 128 );
	packet << playerSocket->m_PlayerGUID;
	packet << playerSocket->m_SessionIndex;		// 应该发送能唯一标示这次登陆的数字
	packet << playerSocket->m_GMFlag;

	// 附加进入游戏的数据
	uint32 curReadPos = enterData->rpos();
    packet.append(enterData->contents(curReadPos), enterData->size() - curReadPos);

	for (map<SceneServer*, CSceneCommClient*>::iterator it = m_sceneServers.begin(); it != m_sceneServers.end(); ++it)
	{
		CSceneCommClient* pServerSocket = it->second;
		if (it->first->remoteServerID == targetSceneServerID && pServerSocket != NULL)
		{
			pServerSocket->SendPacket(&packet);
			playerSocket->m_uSceneServerID = targetSceneServerID;
		}
	}

	if (playerSocket->m_uSceneServerID == 0)		// 没找到合适的游戏服务器
		sLog.outError("玩家 guid="I64FMTD" 准备登入的[%u]号场景服不存在！", playerSocket->GetPlayerGUID(), targetSceneServerID);
}

CSceneCommClient* CSceneCommHandler::GetServerSocket(uint32 serverid)
{
	CSceneCommClient* serversocket = NULL;
	mapLock.Acquire();
	map<SceneServer*, CSceneCommClient*>::iterator itr = m_sceneServers.begin();
	for(; itr != m_sceneServers.end(); ++itr)
	{
		if(itr->first->remoteServerID == serverid)
		{
			serversocket =  itr->second;
			break;
		}
	}
	mapLock.Release();
	return serversocket;
}

void CSceneCommHandler::SetGameServerOLPlayerNums(uint32 serverId, uint32 playerNum)
{
	std::vector<SceneServer*>::iterator it = m_SceneServerConfigs.begin();
	for (; it != m_SceneServerConfigs.end(); ++it)
	{
		if ((*it)->remoteServerID == serverId)
		{
			(*it)->remoteServerOnlineCount = playerNum;
			return ;
		}
	}
}

void CSceneCommHandler::WriteGameServerOLNumsToPacket(WorldPacket &pack)
{
	uint32 opencount = 0;
	pack << m_SceneServerConfigs.size();
	for (std::vector<SceneServer*>::iterator it = m_SceneServerConfigs.begin(); it != m_SceneServerConfigs.end(); ++it)
	{
		if((*it)->IsOpen)
		{
			pack << (*it)->remoteServerID;
			pack << (*it)->remoteServerOnlineCount;
			opencount++;
		}
	}
	pack.put(0,(const uint8*)&opencount,sizeof(uint32));
}

void CSceneCommHandler::AddCompressedPacketFrenquency(uint16 opCode, uint32 length, uint32 compressed_length)
{
	m_compressPackLock.Acquire();
	if (m_compressPacketFrenc.find(opCode) == m_compressPacketFrenc.end())
		m_compressPacketFrenc.insert(make_pair(opCode, map<uint32, uint32>()));

	// 根据压缩率来进行分段
	uint32 compressedRate = float(compressed_length) / length * 100 / 5;
	m_compressPacketFrenc[opCode][compressedRate]++;
	m_compressPackLock.Release();
}

void CSceneCommHandler::PrintCompressedPacketCount()
{
	for (map<uint16, map<uint32, uint32> >::iterator it = m_compressPacketFrenc.begin(); it != m_compressPacketFrenc.end(); ++it)
	{
		sLog.outString("数据包[opcode=%u]压缩频率使用情况", (uint32)it->first);
		for (map<uint32, uint32>::iterator it2 = it->second.begin(); it2 != it->second.end(); ++it2)
		{
			sLog.outString("压缩率在%u-%u间的包数量 =%u个", it2->first, it2->first + 5, it2->second);
		}
	}
}

void CSceneCommHandler::__RemoveOnlineGameSocket(uint64 guid)
{
	onlineLock.Acquire();
	GameSocketMap::iterator itr = online_players.find(guid);
	if (itr != online_players.end())
	{
		GameSocket *sock = itr->second;
		if (sock->hadSessionAuthSucceeded())				// 已经被重新验证过
		{
			cLog.Error("警告：","socket 已经被更新,不必移除 guid="I64FMTD"\n", guid);
		}
		else
		{
			sLog.outString("网关服移除GameSocket,账号 %s(guid="I64FMTD",会话索引=%u)", sock->szGetPlayerAccountName(), guid, sock->m_SessionIndex);
			sock->Disconnect();
			online_players.erase(itr);
		}
	}
	onlineLock.Release();
}

void CSceneCommHandler::__AddGameSocketToOnlineList(uint64 guid, GameSocket *sock)
{
	onlineLock.Acquire();
#ifdef _WIN64
	ASSERT(online_players.find(guid) == online_players.end() && "添加socket对象至在线列表时已有对象存在");
#endif // _WIN64
	online_players.insert(make_pair(guid, sock));
	onlineLock.Release();
}

bool CSceneCommHandler::__CheckGameSocketOnlineAndDisconnect(const string &agent, const string &accName)
{
	bool has = false;
	GameSocket *sock = NULL;
	onlineLock.Acquire();
	GameSocketMap::iterator itr = online_players.begin();
	for (; itr != online_players.end(); ++itr)
	{
		sock = itr->second;
		if (sock != NULL && sock->m_strAccountName == accName && sock->m_strClientAgent == agent)
		{
			sock->Disconnect();
			has = true;
			break;
		}
	}
	onlineLock.Release();
	return has;
}

void CSceneCommHandler::SendPacketToSceneServer(WorldPacket *data, uint32 uServerID)
{
	if (m_sceneServers.empty())
	{
		sLog.outError("找不到对应的游戏服务器进行发送..opcode = %u", data->GetServerOpCode());
		return ;
	}

	map<SceneServer*, CSceneCommClient*>::iterator it;
	//-1则所有游戏服务器转发
	if ( uServerID == (uint32)-1 )
	{
		CSceneCommClient * server = NULL;
		for ( it = m_sceneServers.begin(); it !=m_sceneServers.end(); ++it )
		{
			server = it->second;
			if ( server != NULL)
			{
				server->SendPacket( data );
			}
		}
	}
	else
	{
		//modify by tangquan 2011/11/11
		for(it = m_sceneServers.begin();it != m_sceneServers.end(); it++)
		{
			if(it->first->remoteServerID == uServerID)
				break;
		}

		if(it != m_sceneServers.end())
		{
			CSceneCommClient* togameserver = it->second;
			if(togameserver != NULL)
			{
				togameserver->SendPacket( data );
			}
			//else
			//	sLog.outError("GameServer已经断开");
		}
		else
		{
			sLog.outError("找不到对应的游戏服务器进行发送..opcode = %u", data->GetServerOpCode());
		}
	}
}

void CSceneCommHandler::SendPacketToMasterServer(WorldPacket *data)
{
	if (m_pMasterServer == NULL)
	{
		sLog.outError("找不到中央服务器进行发送！");
		return ;
	}
	m_pMasterServer->SendPacket(data);
}

void CSceneCommHandler::SendGlobaleMsg(WorldPacket *packet)
{
	GameSocket* sock=NULL;
	onlineLock.Acquire();
	GameSocketMap::iterator itr = online_players.begin();
	for(;itr != online_players.end(); ++itr)
	{
		itr->second->SendPacket(packet);
	}
	onlineLock.Release();
}

void CSceneCommHandler::SendGlobalMsgToPartialPlayer(WorldPacket &rawPacket)
{
	uint32 accCounter, accID;
	rawPacket >> accCounter;
	set<uint32> setAccountIDs;
	for (int i = 0; i < accCounter; ++i)
	{
		rawPacket >> accID;
		setAccountIDs.insert(accID);
	}
	uint32 remainDataLength = rawPacket.size() - rawPacket.rpos();
	WorldPacket packet(rawPacket.GetOpcode(), remainDataLength);
	packet.append(rawPacket.contents(rawPacket.rpos()), remainDataLength);

	onlineLock.Acquire();
	GameSocketMap::iterator itr = online_players.begin();
	for(; itr != online_players.end(); ++itr)
	{
		if (setAccountIDs.find(itr->first) != setAccountIDs.end())
			itr->second->SendPacket(&packet);
	}
	onlineLock.Release();
}

void CSceneCommHandler::CloseAllGameSockets(uint32 serverid)		// id为0表示全部关闭
{
	sLog.outString("[场景服%u线断开] 导致所有玩家链接被关闭", serverid);
	onlineLock.Acquire();
	GameSocket* psocket = NULL;
	for (GameSocketMap::iterator it = online_players.begin(); it != online_players.end(); ++it)
	{
		psocket = it->second;
		if((serverid==0 || psocket->m_uSceneServerID==serverid))
			psocket->Disconnect();			// 所在服务器断开
	}
	onlineLock.Release();
}

void CSceneCommHandler::OutPutPacketLog()
{
	/*sLog.outString("玩家端口总共发送数据 %dKB，接收数据%dKB",g_totalSendBytesToClient>>10,g_totalRecvBytesFromClient>>10);
	for(int i=0;i<NUM_MSG_TYPES;++i)
	{
	if(m_clientPacketCounter[i]>10)
	sLog.outString("%d{%s} = %d 个;",i,LookupName(i, g_worldOpcodeNames),m_clientPacketCounter[i]);
	}*/

	sLog.outString("中心服务器端口总共发送数据 %dKB，接收数据%dKB",g_totalSendDataToMasterServer>>10,g_totalRecvDataFromMasterServer>>10);
	for(int i=0;i<SERVER_MSG_OPCODE_COUNT;++i)
	{
		if(m_masterServerPacketCounter[i]>10)
		sLog.outString("%d = %d 个;",i,m_masterServerPacketCounter[i]);
	}
	
	sLog.outString("游戏服务器端口总共发送数据 %dKB，接收数据%dKB",g_totalSendDataToSceneServer>>10,g_totalRecvDataFromSceneServer>>10);
	for(int i=0;i<SERVER_MSG_OPCODE_COUNT;++i)
	{
		if(m_sceneServerPacketCounter[i]>10)
		sLog.outString("%d = %d 个;",i,m_sceneServerPacketCounter[i]);
	}
	
	sLog.outString("游戏在线人数：");
	std::vector<SceneServer*>::iterator it = m_SceneServerConfigs.begin();
	for (; it != m_SceneServerConfigs.end(); ++it)
	{
		sLog.outString("%u号 场景服在线人数 %u",(*it)->remoteServerID, (*it)->remoteServerOnlineCount);
	}
}

bool CSceneCommHandler::OnIPConnect(const char* ip)
{	
	uint32 ipaddr = (uint32)inet_addr(ip);
	uint32 samecount = 0;
	std::map<uint32,uint32>::iterator iter;
	connectionLock.Acquire();
	iter = m_ipcount.find(ipaddr);
	if(iter==m_ipcount.end())
	{
		m_ipcount[ipaddr] = 1;
		samecount = 1;
	}
	else
	{
		iter->second++;
		samecount = iter->second;
	}
	connectionLock.Release();
	if(samecount>m_nIPPlayersLimit)
	{
		sLog.outString("IP:%s的玩家连接过多%d,拒绝",ip,samecount);
		return false;
	}
	return true;
}
void CSceneCommHandler::OnIPDisconnect(const char* ip)
{
	uint32 ipaddr = (uint32)inet_addr(ip);
	std::map<uint32,uint32>::iterator iter;
	connectionLock.Acquire();
	iter = m_ipcount.find(ipaddr);
	if(iter!=m_ipcount.end())
	{
		iter->second--;
		if(iter->second==0)
			m_ipcount.erase(iter);
	}
	connectionLock.Release();
}
bool CSceneCommHandler::LoadStartParam(int argc, char* argv[])
{
	//参数必须配对
	if(argc%2!=1)
	{
		cLog.Error("启动参数","启动参数格式不正确，未进行读取！");
		return false;
	}
	for(uint32 i=1;i<argc;i+=2)
	{
		if(argv[i][0]='-')
		{
			std::string parmname = argv[i]+1;
			std::string parmval = argv[i+1];
			m_startParms[parmname]=parmval;
		}
	}
	return true;
}
void CSceneCommHandler::SetStartParm(const char* key,std::string val)
{
	m_startParms[key]=val;
}
const char* CSceneCommHandler::GetStartParm(const char* key)
{
	if (m_startParms.find(key) != m_startParms.end())
		return m_startParms[key].c_str();
	else
		return "";
}
int CSceneCommHandler::GetStartParmInt(const char* key)
{
	if (m_startParms.find(key) != m_startParms.end())
	{
		int val = atoi(m_startParms[key].c_str());
		return val;
	}
	else
		return 0;
}