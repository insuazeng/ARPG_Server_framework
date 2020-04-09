#pragma once

#include "auth/Sha1.h"
#include "ServerCommonDef.h"

class CMasterCommServer;
class WorldPacket;

class CCentreCommHandler : public Singleton<CCentreCommHandler>
{
public:
	CCentreCommHandler(void);
	~CCentreCommHandler(void);

public:
	inline Mutex& GetServerLocks()
	{
		return m_serverSocketLocks;
	}

public:
	uint32 GenerateGateServerID();
	void AddServerSocket(CMasterCommServer *pServerSocket);
	void RemoveServerSocket(CMasterCommServer *pServerSocket);
	void TimeoutSockets();												// 检测超时
	bool TrySendPacket(WorldPacket *packet, uint32 serverOpCode);		// 尝试发送一个数据包
	void SendMsgToGateServers(WorldPacket *pack, uint32 serverOpcode);	// 将消息传至所有网关服务器

	void UpdateServerSendPackets();
	void UpdateServerRecvPackets();

	uint32 m_totalsenddata;			// 总发送数据量
	uint32 m_totalreceivedata;		// 总接收数据量
	uint32 m_totalSendPckCount;		// 总发包数

	uint32 m_sendDataByteCountPer5min;	// 5分钟的发送数据量
	uint32 m_recvDataByteCountPer5min;	// 5分钟的接收数据量
	uint32 m_sendPacketCountPer5min;	// 5分钟的发包数量

	SHA1Context m_gatewayAuthSha1Data;	// gateway用来连接验证的sha1数据
	std::string m_phpLoginKey;			// 平台登陆key

private:
	set<CMasterCommServer*> m_serverSockets;		// 与多网关连接的中央服务器套接字
	Mutex m_serverSocketLocks;						// 
	uint32 m_nGateServerIdHigh;						// 编号
	Mutex m_serverIdLock;
};

#define sInfoCore CCentreCommHandler::GetSingleton()
