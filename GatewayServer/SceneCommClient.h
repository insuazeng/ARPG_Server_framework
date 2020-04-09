#pragma once

#include "ServerCommonDef.h"

class CSceneCommClient : public TcpSocket
{
public:
	CSceneCommClient(SOCKET fd,const sockaddr_in * peer);
	~CSceneCommClient(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	virtual void OnDisconnect();					// 当连接被关闭时

	void SendPing();								// 向游戏服务器发送ping的请求
	void SendChallenge(uint32 serverid);			// 初次连接时，向游戏服务器发送验证的请求
	
	static void InitPacketHandlerTable();			// 初始化数据包处理表

	// 消息处理函数
	void HandleSceneServerPacket(WorldPacket *pPacket);				// 处理数据，调用下面Handle开头的方法
	
	void HandleAuthResponse(WorldPacket *pPacket);					// 得到验证结果
	void HandlePong(WorldPacket *pPacket);							// 得到ping的结果
	void HandleGameSessionPacketResult(WorldPacket *pPacket);		// 得到游戏服务器返回的信息（即将信息转发给对应的客户端套接字）
	void HandleEnterGameResult(WorldPacket *pPacket);
	void HandleChangeSceneServerNotice(WorldPacket *pPacket);		// 游戏服通知某玩家需要切换场景

	void HandleSavePlayerData(WorldPacket *pPacket);				// 处理游戏服发过来的保存玩家信息请求，主要是处理下线保存
	void HandleSendGlobalMsg(WorldPacket *pPacket);					// gameserver要求发送全局消息
	void HandleTransferPacket(WorldPacket *pPacket);				// 处理游戏服务器转发的包

	// 用于发送的函数
	void SendPacket(WorldPacket *data);								// 发送数据到发送缓冲区
	
	// 将处于发送缓冲队列的Packets发出
	void UpdateServerSendQueuePackets();
	// 处理处于接收缓冲队列的Packets
	void UpdateServerRecvQueuePackets();
	
public:
	uint32 m_lastPingUnixtime;						
	uint32 m_lastPongUnixtime;

	uint64 ping_ms_time;						
	uint32 latency_ms;						// 延迟
	uint32 _id;								// 本端口连接对应sceneserver的id，用来识别不同的scenerserver

	uint32 authenticated;					// 是否通过验证
	bool m_registered;

public:
	FastQueue<WorldPacket*, DummyLock>	_sendQueue;		// 包发送队列
	Mutex								_sendQueueLock;	// 互斥体

	FastQueue<WorldPacket*, Mutex>		_recvQueue;
	FastQueue<WorldPacket*, Mutex>		_busyQueue;//处理时间需要很长的数据包

};

typedef void (CSceneCommClient::*scenepacket_handler)(WorldPacket *);

