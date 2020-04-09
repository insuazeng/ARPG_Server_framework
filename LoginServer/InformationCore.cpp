#include "stdafx.h"
#include "InformationCore.h"
#include "LogonCommServer.h"
#include "AccountCache.h"
#include "md5.h"

InformationCore::InformationCore()
{
	gateway_id_high = 0;
	session_index_high = 0;
	__printLoginSocketCntTimer = 120000;
}

InformationCore::~InformationCore()
{
	for (map<uint32, tagSPConfig*>::iterator it = m_SPConfig.begin(); it != m_SPConfig.end(); ++it)
	{
		delete it->second;
	}
	m_SPConfig.clear();
}

void InformationCore::AddGatewayConnectInfo(uint32 gate_id, tagGatewayConnectionData *info)
{
	gatewayInfoLock.Acquire();
	m_gatewayConnectionInfo.insert( make_pair( gate_id, info ) );
	gatewayInfoLock.Release();
}

tagGatewayConnectionData* InformationCore::GetGatewayConnectInfo(uint32 gate_id)
{
	tagGatewayConnectionData *ret = NULL;

	gatewayInfoLock.Acquire();
	map<uint32, tagGatewayConnectionData*>::iterator itr = m_gatewayConnectionInfo.find(gate_id);
	if(itr != m_gatewayConnectionInfo.end())
	{
		ret = itr->second;
	}
	gatewayInfoLock.Release();

	return ret;
}

void InformationCore::RemoveGatewayConnectInfo(uint32 gate_id)
{
	gatewayInfoLock.Acquire();
	map<uint32, tagGatewayConnectionData*>::iterator itr = m_gatewayConnectionInfo.find(gate_id);
	if(itr != m_gatewayConnectionInfo.end())
	{
		sLog.outString("Removing GatewayServer `%s` (%u) due to socket close.", itr->second->Address.c_str(), gate_id);
		counterLock.Acquire();
		m_PlayerCounter.erase(gate_id);
		counterLock.Release();
		delete itr->second;
		m_gatewayConnectionInfo.erase(itr);
	}
	gatewayInfoLock.Release();
}

uint64 InformationCore::GetSessionKeyExpireTime(string &accName)
{
	uint64 ret = 0;
	AccountSessionKeyMap::iterator itr = m_sessionkeys.find(accName);
	if(itr != m_sessionkeys.end())
	{
		ret = itr->second.key_valid_time;
	}
	return ret;
}

string InformationCore::GetSessionKey(string &accName)
{
	string bn;
	m_sessionKeyLock.Acquire();
	AccountSessionKeyMap::iterator itr = m_sessionkeys.find(accName);
	if(itr != m_sessionkeys.end())
	{
		bn = itr->second.session_key;
	}
	m_sessionKeyLock.Release();
	return bn;
}

void InformationCore::DeleteSessionKey(string &accName)
{
	m_sessionKeyLock.Acquire();
	AccountSessionKeyMap::iterator itr = m_sessionkeys.find(accName);
	if(itr != m_sessionkeys.end())
	{
		m_sessionkeys.erase(itr);
	}
	m_sessionKeyLock.Release();
}

void InformationCore::SetSessionKey(string &accName, string key, uint64 valid_expire_time)
{
	tagSessionKeyValidationData data;
	data.session_key = key;
	data.key_valid_time = valid_expire_time;

	m_sessionKeyLock.Acquire();
	m_sessionkeys.insert(make_pair(accName, data));
	m_sessionKeyLock.Release();
}

void InformationCore::TimeoutSockets(uint32 diff)
{
	// burlex: this is vulnerable to race conditions, adding a mutex to it.
	serverSocketLock.Acquire();

	uint32 t = time(NULL);
	// check the ping time
	set<LogonCommServerSocket*>::iterator itr, it2;
	LogonCommServerSocket * s;
	for(itr = m_serverSockets.begin(); itr != m_serverSockets.end();)
	{
		s = *itr;
		it2 = itr;
		++itr;

		if(s->last_ping < t && ((t - s->last_ping) > 60))
		{
			// ping timeout
			sLog.outError("Closing gateway socket(id=%u) due to ping timeout:%d", s->m_AssociateGatewayID, t - s->last_ping);
			s->removed = true;
			RemoveGatewayConnectInfo(s->m_AssociateGatewayID);
			m_serverSockets.erase(it2);

			s->Disconnect();
		}
	}

	serverSocketLock.Release();

	uint32 loginSocketCount = 0;
	clientLock.Acquire();
	set<DirectSocket*>::iterator itrClient1, itrClient2;
	DirectSocket * sDirect = NULL;
	for (itrClient1=m_ClientSockets.begin(); itrClient1!=m_ClientSockets.end();)
	{
		sDirect = *itrClient1;
		itrClient2 = itrClient1;
		++itrClient1;
		if (sDirect->last_recv < t && (t-sDirect->last_recv) > 30)	// 移除较长时间没动作的客户端连接
		{
			int diff = t - sDirect->last_recv;
			sLog.outError("Closing direct socket(id=%d) due to ping timeout:%d", sDirect->GetFd(), diff);
			sDirect->Disconnect();
		}
	}
	loginSocketCount = m_ClientSockets.size();
	clientLock.Release();

	// 每个2分钟打印一下当前在线的登陆socket
	if (__printLoginSocketCntTimer >= diff)
	{
		__printLoginSocketCntTimer -= diff;
	}
	else
	{
		sLog.outString("当前处于连接状态LoginSocketCount=%u", loginSocketCount);
		__printLoginSocketCntTimer = 120000;
	}
}

void InformationCore::UpdateRegisterPacket()
{
	WorldPacket* pRecvPck = NULL;
	while (pRecvPck = m_recvPhpQueue.Pop())
	{
		if (pRecvPck != NULL)
		{
			HandlePhpSessionKeyRegister(*pRecvPck);
			DeleteWorldPacket( pRecvPck );
		}
	}
}

void InformationCore::HandlePhpSessionKeyRegister(WorldPacket& packet)
{
	// 接收到php传来的信息，先验证phpkey的合法性，再计算并设置SessionKey
	/*string sp_name, acc_name, php_key, version, time_val, rand_val;
	packet >> sp_name >> acc_name >> php_key >> version >> time_val >> rand_val;
	uint32 spID = GetSpConfigID(sp_name);
	if (spID == ILLEGAL_DATA)
	{
		sLog.outError("[PHP_Socket] 收到非法SP(name=%s)发送的PHP_Key(accName=%s,key=%s)", sp_name.c_str(), acc_name.c_str(), php_key.c_str());
		return ;
	}

	// phpAuthKey = md5 ( TICKET_SUBFIX . $user_name . $cur_time. $rand_val ););
	MD5 md5;
	std::string md5string = sInfoCore.PHP_SECURITY;
	md5string += acc_name;
	md5string += time_val;
	md5string += rand_val;
	md5.update(md5string);
	string md5_result = md5.toString();
	if (md5_result != php_key)
	{
		sLog.outError("[PHP_Socket] 接收到的PHP_Key非法(spName=%s,accName=%s recvMD5=%s,calcuMD5=%s)", sp_name.c_str(),
						acc_name.c_str(), php_key.c_str(), md5_result.c_str());
		return ;
	}

	// 检测是否存在账户，如果不存在的话，需要新建账户
	Account *acc_info = sAccountMgr.GetAccount(acc_name);
	if (acc_info == NULL)
	{
		// 新建一个账户
		acc_info = sAccountMgr.AddNewAccount(spID, 0, acc_name, "fromplatform", 0);
	}
	if (acc_info == NULL)
	{
		sLog.outError("[PHP_Socket] 获取账号(accName=%s)信息失败", acc_name.c_str());
		return ;
	}

	md5.reset();
	md5string = php_key + rand_val;
	md5.update(md5string);
	md5string = acc_name + md5.toString();
	md5.reset();
	md5.update(md5string);
	md5_result = md5.toString();

	sLog.outString("[PHP_Socket]设置账号 %s(from=%s) ClientKey=%s", acc_name.c_str(), sp_name.c_str(), md5_result.c_str());
	SetSessionKey(acc_info->strAccountName, md5_result, (time(NULL) + 3));					// 3秒内登陆有效
	*/
}

void InformationCore::FillBestGatewayInfo(string &addr, uint32 &port)
{
	counterLock.Acquire();
	if (m_PlayerCounter.empty())
	{
		counterLock.Release();
		return ;
	}

	uint32 maxcount = ILLEGAL_DATA_32;

	map<uint32, tagGatewayPlayerCountData>::iterator itr = m_PlayerCounter.begin();
	for (; itr != m_PlayerCounter.end(); ++itr)
	{
		if (itr->second.plrCounter < maxcount || addr == "")
		{
			maxcount = itr->second.plrCounter;
			addr = itr->second.ipAddr;
			port = itr->second.port;
		}
	}
	counterLock.Release();
}

void InformationCore::InsertClientSocket(DirectSocket * newsocket)
{
	clientLock.Acquire();
	m_ClientSockets.insert(newsocket);
	clientLock.Release();
}

void InformationCore::EraseClientSocket(DirectSocket * oldsocket)
{
	clientLock.Acquire();
	m_ClientSockets.erase(oldsocket);
	clientLock.Release();
}

bool InformationCore::LoadStartParam(int argc, char* argv[])
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

void InformationCore::SetStartParm(const char* key,std::string val)
{
	m_startParms[key]=val;
}

const char* InformationCore::GetStartParm(const char* key)
{
	if (m_startParms.find(key) != m_startParms.end())
		return m_startParms[key].c_str();
	else
		return "";
}

int InformationCore::GetStartParmInt(const char* key)
{
	if (m_startParms.find(key) != m_startParms.end())
	{
		int val = atoi(m_startParms[key].c_str());
		return val;
	}
	else
		return 0;
}

void InformationCore::LoadSpConfig()
{
	//for (map<uint32, tagSPConfig*>::iterator it = m_SPConfig.begin(); it != m_SPConfig.end(); ++it)
	//{
	//	delete it->second;
	//}
	//m_SPConfig.clear();
	//m_SPConfigNameKey.clear();

	//QueryResult *ret = Database_World->Query("SELECT * from sp_config where open_state=1 order by sp_id asc;");
	//if (ret == NULL)
	//	return ;

	//Field *fields = NULL;
	//do 
	//{
	//	fields = ret->Fetch();
	//	tagSPConfig *conf = new tagSPConfig();
	//	conf->sp_id = fields[0].GetUInt32();
	//	conf->sp_name = fields[1].GetString();

	//	m_SPConfig.insert(make_pair(conf->sp_id, conf));
	//	m_SPConfigNameKey.insert(make_pair(conf->sp_name, conf->sp_id));

	//} while (ret->NextRow());
	//delete ret;

	//sLog.outString("[渠道配置]共读取 %u 个正在运营状态的渠道信息", uint32(m_SPConfig.size()));
}

uint32 InformationCore::GetSpConfigID(string &spName)
{
	uint32 uRet = ILLEGAL_DATA_32;
	if (m_SPConfigNameKey.find(spName) != m_SPConfigNameKey.end())
		uRet = m_SPConfigNameKey[spName];
	
	return uRet;
}

tagSPConfig* InformationCore::GetSpConfig(uint32 spID)
{
	if (m_SPConfig.find(spID) != m_SPConfig.end())
		return m_SPConfig[spID];

	return NULL;
}
