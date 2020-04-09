#pragma once

//客户端用来登录验证
class DirectSocket : public TcpSocket
{
public:
	DirectSocket(SOCKET fd,const sockaddr_in * peer);
	~DirectSocket(void);

public:
	virtual void OnUserPacketRecved(WorldPacket *packet);
	virtual void OnUserPacketRecvingTimeTooLong(int64 usedTime, uint32 loopCount) { }
	
public:
	void OnConnect();
	void OnDisconnect();
	void SendPacket(WorldPacket * packet);
	
	uint32 last_recv;

private:
	// 请求登陆
	void HandleLogin(WorldPacket &packet);
	// 请求自动注册账号
	void HandleAccountAutoRegister(WorldPacket &packet);

	// ip地址
	string mClientIpAddr;
};
