/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

#pragma once

#include "CommonTypes.h"
#include "Singleton.h"
#include "md5.h"
#include "threading/Mutex.h"
#include "auth/Sha1.h"

class CSceneCommServer;
class WorldPacket;
struct tagMailInfo;

typedef map<uint32, set<CSceneCommServer*> > PlatformGatewaySocketMap;

class SceneCommHandler : public Singleton <SceneCommHandler>
{
	set<CSceneCommServer*> m_serverSockets;
	PlatformGatewaySocketMap m_platformGatewaySockets;		// 平台网关套接字map<平台id,网关套接字集合>
	Mutex serverSocketLock;
	uint32 gateserverhigh;

public:
	uint32 m_totalsenddata;				
	uint32 m_totalreceivedata;			
	uint32 m_totalSendPckCount;			// 发包总数

	uint32 m_sendDataByteCountPer5min;	// 5分钟的发送数据量
	uint32 m_recvDataByteCountPer5min;	// 5分钟的接收数据量
	uint32 m_sendPacketCountPer5min;	// 5分钟的发包数量

	uint32 m_TimeOutSendCount;			// 时间到而发包的数量
	uint32 m_SizeOutSendCount;			// 数据量到而发包的数量
	SHA1Context m_gatewayAuthSha1Data;	// gateway用来连接验证的sha1数据
	
	uint32 m_sceneServerID;				// 这台游戏服务器的id编号（应该是lineId）
	Mutex serverIDLock;

	inline Mutex & getServerSocketLock()
	{ 
		return serverSocketLock; 
	}

	SceneCommHandler()
	{ 
		gateserverhigh = 0;
		m_totalsenddata=0;
		m_totalreceivedata=0;
		m_totalSendPckCount = 0;

		m_sendDataByteCountPer5min = 0;
		m_recvDataByteCountPer5min = 0;
		m_sendPacketCountPer5min = 0;

		m_SizeOutSendCount = 0;
		m_TimeOutSendCount = 0;
		m_sceneServerID = 0;
	}

	inline void SetSelfSceneServerID(uint32 serverIndex)
	{
		m_sceneServerID = serverIndex;
	}
	inline const uint32 GetSelfSceneServerID()
	{
		return m_sceneServerID;
	}

	inline uint32 GenerateRealmID()
	{
		serverIDLock.Acquire();
		uint32 ret = ++gateserverhigh;
		serverIDLock.Release();
		return ret;
	}

	inline void   AddServerSocket(CSceneCommServer * sock)
	{ 
		serverSocketLock.Acquire();
		m_serverSockets.insert( sock ); 
		serverSocketLock.Release(); 
	}
	inline void   RemoveServerSocket(CSceneCommServer * sock)
	{ 
		serverSocketLock.Acquire();
		m_serverSockets.erase( sock );
		serverSocketLock.Release(); 
	}
	
	void AddPlatformGatewaySocket(uint32 platformID, CSceneCommServer *sock);
	void RemovePlatformGatewaySocket(uint32 platformID, CSceneCommServer *sock);

	bool TrySendPacket(uint32 platformID, WorldPacket * packet, uint32 serverOpcode = 0);

	void	TimeoutSockets();
	// 发送游戏服务器端的积压包
	void	UpdateServerSendPackets();
	// 处理服务器接收的包
	void	UpdateServerRecvPackets();

	// void	SendServerPacket(WorldPacket* pack, uint32 ServerOpcode, uint32 accID = 0);
};

#define sGateCommHandler SceneCommHandler::GetSingleton()

