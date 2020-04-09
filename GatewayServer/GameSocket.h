#pragma once

#include "auth/hohoCrypt.h"
#include "auth/FSM.h"

class WorldPacket;

enum CLIENTSTATUS
{
	eCSTATUS_AUTHFAILED = -2,
	eCSTATUS_ATTACK = -1,
	eCSTATUS_CONNECTED = 0,
	eCSTATUS_SEND_AUTHREQUEST = 1,
	eCSTATUS_SEND_ENTERGAME = 2,
	eCSTATUS_ENTERGAME_SUCESS = 3,
	eCSTATUS_SEND_CHARINFO = 4,
	eCSTATUS_SEND_CREATECHAR = 5,
};

enum GameSocketAuthStatus
{
	GSocket_auth_state_none				= 0x00,			// 
	GSocket_auth_state_sended_packet	= 0x01,			// 已经发送了验证包
	GSocket_auth_state_ret_succeed		= 0x02,			// 得到了会话验证结果(验证成功)
	GSocket_auth_state_ret_failed		= 0x04,			// 得到了会话验证结果(验证失败)
};

class GameSocket : public TcpSocket
{
public:
	GameSocket(SOCKET fd, const sockaddr_in * peer);
	~GameSocket(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }

	// 对包进行压缩后发出
	bool _OutPacketCompress(uint32 opcode, uint32 packSize, uint32 errCode, const void* data);
	
	void SendPacket(WorldPacket * data);
	void OnConnect();
	void OnDisconnect();

	// 更新待发送的实际的数据包
	void UpdateQueuedPackets();
	// 会话验证成功
	void SessionAuthedSuccessCallback(uint32 sessionIndex, string &platformAgent, string &accName, uint32 gmFlag, string existSessionToken);
	// 会话验证失败
	void SessionAuthedFailCallback(uint32 errCode);
	// 通知切换场景结果
	void SendChangeSceneResult(uint32 errCode, uint32 tarMapID = 0, float dstPosX = 0.0f, float dstPosY = 0.0f, float dstHeight = 0.0f);

	// 更新待发送的实际的数据包
	bool InPacketBacklogState();
	void TryToLeaveBackLogState();
	void TryToEnterBackLogState();
	
	inline uint64 GetPlayerGUID() { return m_PlayerGUID; }
	inline const char* szGetPlayerAccountName() { return m_strAccountName.c_str(); }
	inline uint32 GetPlayerLine() { return m_uSceneServerID; }
	inline string& GetPlayerIpAddr() { return mClientIpAddr; } 
	inline bool hadSendedAuthPacket() { return (m_AuthStatus & GSocket_auth_state_sended_packet) > 0 ? true : false; }
	inline bool hadSessionAuthSucceeded() { return (m_AuthStatus & GSocket_auth_state_ret_succeed) > 0 ? true : false; }
	inline bool hadGotSessionAuthResult() 
	{ 
		bool bRet = false;
		if ((m_AuthStatus & GSocket_auth_state_ret_failed) || (m_AuthStatus & GSocket_auth_state_ret_succeed))
			bRet = true;
		return bRet;
	}

public:
	static void InitTransPacket(void);
	static string GenerateGameSocketLoginToken(const string &agent, const string &accName);

	/* 在玩家真正（选角色）进入游戏之前,是不存在玩家guid的
	*  在这里，生成一个临时的guid作为键值，供gateway-master进行通信
	*  等到选角成功进入游戏后，会使用角色guid（全局唯一）来作为正式的key
	*/
	static uint64 GeneratePlayerHashGUID(const string& agent, const string& accName);

public:
	uint64 m_lastPingRecvdMStime;		// 上次接收来自客户端ping的时间
	uint32 m_SessionIndex;				// 和游戏服务器同步的对话索引
	string m_strClientAgent;			// 客户端来自的渠道名
	
	uint32 m_uSceneServerID;			// 所对应游戏逻辑服务器(在几线)
	uint32 m_nChatBlockedExpireTime;	// 禁言到期时间

	// 验证相关
	uint8  m_AuthStatus;				// socket登陆的验证状态
	uint32 m_nAuthRequestID;			// 在本网关进程进行递增，标志唯一的登陆验证请求
	string m_strAuthSessionKey;			// 会话key，由phpSocket生成的。（用于初次登陆验证）
	string m_strLoginToken;				// 登陆令牌（用于断线重登陆）

	std::string m_strAccountName;		// 玩家账号名
	uint64 m_PlayerGUID;				// 本GameSocket对应的角色guid(当玩家正式选角进入游戏后,该id为正式guid,否则为网关通过hash算法生成的一个guid)
	uint32 m_GMFlag;					// gm值
	uint64 m_ReloginNextMsTime;			// 允许重登陆的时间

	//uint64 m_packrecvtime[NUM_MSG_TYPES];	// 上次收包事件
	uint32 m_checkcount;					// 收包过快次数，超过一定数目则断开

	// static uint32 m_packetfrequency[NUM_MSG_TYPES];	// 客户端收包频率控制
	// static uint32 m_disconnectcount;					// 断开次数上限
	// static uint8 m_transpacket[NUM_MSG_TYPES];		// 客户端包转发配置
	// static uint8 m_packetforbid[NUM_MSG_TYPES];		// 此数据包是否屏蔽，用于临时屏蔽一些功能模块

	uint32 m_nSocketConnectedTime;		// socket连入的时间,用于统计玩家流失
	int32 mClientStatus;				// 客户端状态
	
private:
	bool	m_beStuck;				// 测试网络阻塞
	bool	m_allowConnect;

	bool	m_inPckBacklogState;	// 处于包积压状态
	Mutex	m_backlogStateLock;		// 积压状态改变的锁

	FastQueue<WorldPacket*, Mutex> _pckBacklogQueue;	// 包积压缓冲

#ifdef USE_FSM
	CFSM	_sendFSM;
	CFSM	_receiveFSM;
#endif

	FastQueue<WorldPacket*, Mutex> _queue;//包发送队列

	std::string mClientVersion;			// 客户端版本号
	std::string mClientLog;				// 客户端实时日志
	std::string mClientEnv;				// 客户端环境
	std::string mClientIpAddr;			// 客户端id地址
	
private:
	void _HandleUserAuthSession(WorldPacket* recvPacket);	// 处理验证会话
	void _HandlePing(WorldPacket* recvPacket);				// 处理客户端ping
	void _HandlePlayerSpeek(WorldPacket *recvPacket);

	void _HookRecvedWorldPacket(WorldPacket *recvPacket);	// 监听已经接受到的数据包
	bool _HookSendWorldPacket(WorldPacket* recvPacket);     // 监听准备发送出去的数据包
	void _CheckRevcFrenquency(uint32 packopcode);		//检查收包频率

	void _EnterBacklogState();
	void _LeaveBacklogState();
};
