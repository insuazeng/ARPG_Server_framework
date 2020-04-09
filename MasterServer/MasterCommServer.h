#pragma once

#include "auth/Sha1.h"
#include "ServerCommonDef.h"

class CMasterCommServer : public TcpSocket
{
	friend class CCentreCommHandler;
public:
	CMasterCommServer(SOCKET fd, const sockaddr_in * peer);
	virtual ~CMasterCommServer(void);

public:
	inline uint32 GetGateServerId()
	{
		return gateserver_id;
	}

public:
	void OnDisconnect();
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	// 用于发送的函数
	void SendPacket(WorldPacket* data, uint32 serverOpcode, uint64 guid);	// 发送数据到发送缓冲区
	
	// 发送存储于缓冲区内的Packets
	void UpdateServerSendQueuePackets();
	// 处理储存在接收缓冲队列中的Packets
	void UpdateServerRecvQueuePackets();

	static void InitPacketHandlerTable();		// 初始化数据包处理表
	void HandlePacket(WorldPacket *recvData);
	void HandlePing(WorldPacket &recvData);		// 
	void HandleSceneServerConnectionStatus(WorldPacket &recvData);
	void HandleAuthChallenge(WorldPacket &recvData);
	void HandleRequestRoleList(WorldPacket &recvData);
	// void HandleEnterSceneNotice(WorldPacket &recvData);
	void HandleChangeSceneServerNotice(WorldPacket &recvData);
	void HandlePlayerGameRequest(WorldPacket &recvData);
	void HandleCloseServantRequest(WorldPacket &recvData);
	void HandleSceneServerPlayerMsgEvent(WorldPacket &recvData);
	void HandleSceneServerSystemMsgEvent(WorldPacket &recvData);

	void DeletePlayerSocket(uint64 userGuid, uint32 sessionindex, bool direct);
	
public:
	FastQueue<WorldPacket*, Mutex> _recvQueue;		// 收包队列
	FastQueue<WorldPacket*, Mutex> _sendQueue;		// 发包队列 

private:
	// 保存一些具体信息
	uint32 m_lastPingRecvUnixTime;
	bool removed;
	
	uint32 gateserver_id;
	uint32 authenticated;

	std::string registername;
	std::string registeradderss;

};

typedef void (CMasterCommServer::*gatepacket_handler)(WorldPacket &);