#pragma once

#include "DirectSocket.h"
#include "auth/Sha1.h"

struct tagGatewayConnectionData
{
	string Name;			// 网关名
	string Address;			// "ip地址:监听端口"
	string IP;				// ip地址
	uint32 Port;			// 监听端口号
};

// 单网关对应的玩家人数
struct tagGatewayPlayerCountData
{
	uint32 plrCounter;		// 玩家数量
	string ipAddr;			// 该网关的ip地址
	uint32 port;			// 端口号
};

struct tagSessionKeyValidationData
{
	tagSessionKeyValidationData()
	{
		key_valid_time = 0;
	}
	string session_key;
	uint64 key_valid_time;
};
typedef HM_NAMESPACE::hash_map<string, tagSessionKeyValidationData> AccountSessionKeyMap;

class LogonCommServerSocket;
struct tagSPConfig;

class InformationCore : public Singleton<InformationCore>
{
public:
	InformationCore();
	~InformationCore();

private:
	Mutex										m_sessionKeyLock;
	AccountSessionKeyMap						m_sessionkeys;
	map<uint32, tagGatewayConnectionData*>		m_gatewayConnectionInfo;
	set<LogonCommServerSocket*>					m_serverSockets;
	Mutex serverSocketLock;
	Mutex gatewayInfoLock;
	uint32 gateway_id_high;
	uint32 session_index_high;			// 会话索引（在登陆服全局处理唯一的会话索引）

	Mutex clientLock;
	set<DirectSocket*> m_ClientSockets;

public:
	Mutex counterLock;
	map<uint32, tagGatewayPlayerCountData> m_PlayerCounter;			// map<为网关注册的本地id, 连接人数+ip+端口>

	void FillBestGatewayInfo(string &addr, uint32 &port);
	inline Mutex & getServerSocketLock() { return serverSocketLock; }
	inline Mutex & getRealmLock() { return gatewayInfoLock; }
	void InsertClientSocket(DirectSocket * newsocket);
	void EraseClientSocket(DirectSocket * oldsocket);

	SHA1Context m_gatewayAuthSha1Data;	// gateway用来连接验证的sha1数据
	string  PHP_SECURITY;
	inline uint32 GenerateGatewayInfoID()
	{
		return ++gateway_id_high;
	}
	inline uint32 GenerateSessionIndex()
	{
		uint32 uRet = 0;
		gatewayInfoLock.Acquire();
		uRet = ++session_index_high;
		gatewayInfoLock.Release();
		return uRet;
	}
	
	// Sessionkey Management
	uint64			GetSessionKeyExpireTime(string &accName);
	string			GetSessionKey(string &accName);
	void			DeleteSessionKey(string &accName);
	void			SetSessionKey(string &accName, string key, uint64 valid_expire_time);

	void						AddGatewayConnectInfo(uint32 gate_id, tagGatewayConnectionData *info);
	tagGatewayConnectionData*	GetGatewayConnectInfo(uint32 gate_id);
	void						RemoveGatewayConnectInfo(uint32 gate_id);

	inline void   AddServerSocket(LogonCommServerSocket * sock) { serverSocketLock.Acquire(); m_serverSockets.insert( sock ); serverSocketLock.Release(); }
	inline void   RemoveServerSocket(LogonCommServerSocket * sock) { serverSocketLock.Acquire(); m_serverSockets.erase( sock ); serverSocketLock.Release(); }

	void			TimeoutSockets(uint32 diff);
	void			UpdateRegisterPacket();

	FastQueue<WorldPacket*, Mutex> m_recvPhpQueue;
	void	HandlePhpSessionKeyRegister(WorldPacket& packet);

public:
	bool LoadStartParam(int argc, char* argv[]);
	void SetStartParm(const char* key,std::string val);
	const char* GetStartParm(const char* key);
	int GetStartParmInt(const char* key);

	void LoadSpConfig();
	uint32 GetSpConfigID(string &spName);
	tagSPConfig* GetSpConfig(uint32 spID);

private:
	std::map<std::string, std::string> m_startParms;
	std::map<uint32, tagSPConfig*> m_SPConfig;			// 渠道运营商配置
	std::map<std::string, uint32> m_SPConfigNameKey;	// 以sp名为key的信息配置

	uint32 __printLoginSocketCntTimer;
};

#define sInfoCore InformationCore::GetSingleton()
