#pragma once

#include "RC4Engine.h"
#include "ServerCommonDef.h"

enum ENUMMSGPRIORITY
{
	HIGH_PRIORITY = 1,
	LOW_PRIORITY
};

enum PLAYER_ENTER_GAME_RET 
{
	P_ENTER_RET_SUCCESS,					// 成功创建会话
	P_ENTER_RET_CREATE_SESSION_FAILED,		// 创建会话对象失败
	P_ENTER_RET_LOADDB_FAILED,				// 读取数据失败
	P_ENTER_RET_DELAY_ADD,					// 延迟加入
};

#define GATESOCKET_SENDBUF_SIZE 5485760
#define GATESOCKET_RECVBUF_SIZE 5485760

class CSceneCommServer : public TcpSocket
{
public:
	CSceneCommServer(SOCKET fd,const sockaddr_in * peer);
	~CSceneCommServer(void);

public:
	// Func
	virtual void OnConnect();
	virtual void OnDisconnect();
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	void SendSessioningResultPacket(WorldPacket* pck, uint64 userGuid, ENUMMSGPRIORITY msgPriority = HIGH_PRIORITY);// 本函数专门用来发送会话消息
	//void DeletePlayerSocket(uint32 nPlayerAccountID);

	void HandlePacket(WorldPacket *recvData);	//数据包初步处理
	void HandlePing(WorldPacket &recvData);			
	
	void HandleGameServerConnectionStatus(WorldPacket &recvData);		// 场景服连接状态
	void HandleGameSessionPacket(WorldPacket* pRecvData);				// 处理游戏进行中的消息

	void HandleGatewayAuthReq(WorldPacket &recvData);				// 被请求验证一个网关服务器
	void HandleRequestEnterGame(WorldPacket &recvData);				// 被请求进入游戏
	void HandleCloseSession(WorldPacket &recvData);					// gateserver要求移除对话
	// void HandleObjectDataSyncNotice(WorldPacket &recvData);			// 处理其他服需要同步对象数据的请求
	void HandleServerCMD( WorldPacket & packet );					// 处理服务器命令
	// void HandleMasterServerPlayerMsgEvent(WorldPacket &recvData);	// 玩家在中央服操作同步到场景
	// void HandleMasterServerSystemMsgEvent(WorldPacket &recvData);	// 中央服到场景服的服务端消息
	// void HandleSceneServerSystemMsgEventFromOtherScene(WorldPacket &recvData);	// 场景服到其他场景服的服务端消息

	/************************************************/
	static void InitPacketHandlerTable();//初始化数据包处理表
	/************************************************/

	//void HandleGlobalVar(WorldPacket &recvPacket);	//全局变量相关，当前处理节日NPC领取礼品

	// 用于发送的函数
	void FASTCALL SendPacket(WorldPacket* data, uint32 nServerOPCode = 0 , uint64 userGuid = 0 );	// 发送数据到发送缓冲区

	// 发送存储于缓冲区内的Packets
	void UpdateServerSendQueuePackets();
	// 发送低优先级的发包队列
	void UpdateServerLowSendQueuePackets();
	// 处理储存在接收缓冲队列中的Packets
	void UpdateServerRecvQueuePackets();
	// 服务端间包处理函数
	void ServerPacketProc(WorldPacket *recvPacket, uint32 *packetOpCounter);

public:
	// Variable
	uint32 m_lastPingRecvUnixTime;
	bool removed;
	uint32 associate_gateway_id;
	uint32 src_platform_id;			// 源平台id
	uint32 authenticated;
	uint64 _lastSendLowLevelPacketMsTime;
	
	uint32 registerPort;
	std::string registername;
	std::string registeradderss;

public:
	FastQueue<WorldPacket*, Mutex> _recvQueue;					// 收包队列
	FastQueue<WorldPacket*, Mutex> _lowrecvQueue;				// 低优先处理队列
	FastQueue<WorldPacket*, Mutex>	_highPrioritySendQueue;		// 高优先级发送队列
	FastQueue<WorldPacket*, Mutex>	_lowPrioritySendQueue;		// 低优先级发送队列

private:
	Mutex sendMutex;
};

typedef void (CSceneCommServer::*gamepacket_handler)(WorldPacket &);
