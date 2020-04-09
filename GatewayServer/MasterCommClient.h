#pragma once

#include <RC4Engine.h>

class CMasterCommClient : public TcpSocket
{
public:
	CMasterCommClient(SOCKET fd, const sockaddr_in *peer);
	~CMasterCommClient(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	virtual void OnDisconnect();					// 当连接被关闭时

	// 用于发送的函数
	void FASTCALL SendPacket(WorldPacket *data);	// 发送数据到发送缓冲区

	// 将处于发送缓冲队列的Packets发出
	void UpdateServerSendQueuePackets();
	// 处理处于接收缓冲队列的Packets
	void UpdateServerRecvQueuePackets();

	void SendPing();
	void SendChallenge(uint32 serverId);
	static void InitPacketHandlerTable();		// 初始化数据包处理列表

	//作为处理后面所有handle的代理函数
	void HandleCentrePakcets(WorldPacket *packet);

	void HandlePong(WorldPacket *packet);
	void HandleAuthResponse(WorldPacket *packet);
	void HandleCloseGameSocket(WorldPacket *packet);				// 关闭一个gamesocket
	void HandleChooseGameRoleResponse(WorldPacket *packet);			// 选角进入游戏的返回
	void HandlePlayerGameResponse(WorldPacket *packet);
	void HandleChangeSceneServerMasterNotice(WorldPacket *packet);	// 中央服通知网关进行目标场景切换
	void HandleEnterInstanceNotice(WorldPacket *packet);			// 请求进入副本或系统实例
	void HandleExitInstanceNotice(WorldPacket *packet);				// 退出副本或系统实例
	void HandleSendGlobalMsg(WorldPacket *packet);
	void HandleSendPartialGlobalMsg(WorldPacket *packet);			// 发送全局消息给部分的socket
	void HandleTranferPacket( WorldPacket * packet );				// 直接转发给对应游戏服务器
	void HandleBlockChat( WorldPacket * packet );					// 禁言处理 by eeeo@2011.4.27
	void HandleServerCMD( WorldPacket * packet );					// 处理服务器命令

public:
	uint32 m_lastPingUnixTime;
	uint32 m_lastPongUnixTime;
	uint64 ping_ms_time;
	uint32 latency_ms;			// 延迟
	uint32 authenticated;
	uint32 m_nThisGateId;	// 本网关服务于中央服务器处注册得到的id

	//已经不用发送队列的，而是直接给GameCommHandler处理 by tangquan
	FastQueue<WorldPacket*, DummyLock> _sendQueue;		// 包发送队列
	Mutex							_sendQueueLock;		// 互斥体
	FastQueue<WorldPacket*, Mutex> _recvQueue;
	FastQueue<WorldPacket*, Mutex> _busyQueue;//处理时间需要很长的数据包（已经不用了by tangquan）
};

typedef void (CMasterCommClient::*masterpacket_handler)(WorldPacket *);
