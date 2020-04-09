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
#include "stdafx.h"

#include "GateCommHandler.h"
#include "SceneCommServer.h"

void SceneCommHandler::TimeoutSockets()
{
	uint32 t = time(NULL);
	set<CSceneCommServer*>::iterator itr, it2;
	CSceneCommServer *s = NULL;
	PlatformGatewaySocketMap::iterator pIt;

	serverSocketLock.Acquire();
	// 遍历平台网关socket
	for (pIt = m_platformGatewaySockets.begin(); pIt != m_platformGatewaySockets.end(); ++pIt)
	{
		for (itr = pIt->second.begin(); itr != pIt->second.end();)
		{
			it2 = itr;
			++itr;
			s = *it2;

			if(s->m_lastPingRecvUnixTime < t && ((t - s->m_lastPingRecvUnixTime) > 120))
			{
				sLog.outError("Closing Gateway Socket(platform=%u) due to ping timeout..", pIt->first);
				s->removed = true;
				pIt->second.erase(it2);
				s->Disconnect();
			}
		}
	}
	// 遍历未验证成功的网关socket
	if (!m_serverSockets.empty())
	{
		for(itr = m_serverSockets.begin(); itr != m_serverSockets.end();)
		{
			s = *itr;
			it2 = itr;
			++itr;

			if(s->m_lastPingRecvUnixTime < t && ((t - s->m_lastPingRecvUnixTime) > 120))
			{
				sLog.outError("Closing socket due to ping timeout.\n");
				s->removed = true;
				m_serverSockets.erase(it2);
				s->Disconnect();
			}
		}
	}
	serverSocketLock.Release();
}

//void	SceneCommHandler::SendServerPacket(WorldPacket* pack, uint32 ServerOpcode, uint32 accID/* = 0*/)
//{
//	serverSocketLock.Acquire();//这里为什么要注释掉呢？
//	set<CSceneCommServer*>::iterator ittemp;
//	for (set<CSceneCommServer*>::iterator it = m_serverSockets.begin(); it != m_serverSockets.end();)
//	{
//		ittemp = it;
//		++it;
//		if ((*ittemp) != NULL)
//		{
//			WorldPacket* pck = NewWorldPacket( pack );
//			(*ittemp)->SendPacket(pck, ServerOpcode, accID);
//		}
//	}
//	serverSocketLock.Release();
//}

void SceneCommHandler::UpdateServerSendPackets()
{
	PlatformGatewaySocketMap::iterator pIt;
	set<CSceneCommServer*>::iterator it;
	CSceneCommServer *sock = NULL;

	serverSocketLock.Acquire();//贡文斌 2010.2.5 有可能被网络线程的ondisconnect删除，也可能在update中删除
	/*set<CSceneCommServer*>::iterator ittemp;
	for (set<CSceneCommServer*>::iterator it = m_serverSockets.begin(); it != m_serverSockets.end();)
	{
		ittemp = it;
		++it;
		if ((*ittemp) != NULL)
		{
			(*ittemp)->UpdateServerSendQueuePackets();
			(*ittemp)->UpdateServerLowSendQueuePackets();
		}
	}*/
	for (pIt = m_platformGatewaySockets.begin(); pIt != m_platformGatewaySockets.end(); ++pIt)
	{
		for (it = pIt->second.begin(); it != pIt->second.end(); ++it)
		{
			sock = *it;
			if (sock != NULL)
			{
				sock->UpdateServerSendQueuePackets();
				sock->UpdateServerLowSendQueuePackets();
			}
		}
	}
	serverSocketLock.Release();
}

void SceneCommHandler::UpdateServerRecvPackets()
{
	serverSocketLock.Acquire();
	// 处理平台网关socket的包
	PlatformGatewaySocketMap::iterator pIt = m_platformGatewaySockets.begin();
	CSceneCommServer *sock = NULL;
	for (; pIt != m_platformGatewaySockets.end(); ++pIt)
	{
		set<CSceneCommServer*>::iterator it = pIt->second.begin();
		for (; it != pIt->second.end(); ++it)
		{
			sock = *it;
			if (sock != NULL)
			{
				sock->UpdateServerRecvQueuePackets();
			}
		}
	}

	if (!m_serverSockets.empty())
	{
		set<CSceneCommServer*>::iterator ittemp;
		for (set<CSceneCommServer*>::iterator it = m_serverSockets.begin(); it != m_serverSockets.end();)
		{
			ittemp = it;
			++it;
			if ((*ittemp) != NULL)
			{
				(*ittemp)->UpdateServerRecvQueuePackets();
			}
		}
	}
	
	serverSocketLock.Release();
}

void SceneCommHandler::AddPlatformGatewaySocket(uint32 platformID, CSceneCommServer *sock)
{
	serverSocketLock.Acquire();
	if (m_platformGatewaySockets.find(platformID) == m_platformGatewaySockets.end())
		m_platformGatewaySockets.insert(make_pair(platformID, set<CSceneCommServer*>()));
	m_platformGatewaySockets[platformID].insert(sock);
	serverSocketLock.Release(); 
}

void SceneCommHandler::RemovePlatformGatewaySocket(uint32 platformID, CSceneCommServer *sock)
{
	serverSocketLock.Acquire();
	if (m_platformGatewaySockets.find(platformID) != m_platformGatewaySockets.end())
		m_platformGatewaySockets[platformID].erase(sock);
	serverSocketLock.Release(); 
}

bool SceneCommHandler::TrySendPacket(uint32 platformID, WorldPacket * packet, uint32 serverOpcode /* = 0 */)
{
	CSceneCommServer *bestGateway = NULL;
	uint32 maxBufferSize = 0;

	serverSocketLock.Acquire();
	
	if (m_platformGatewaySockets.find(platformID) == m_platformGatewaySockets.end() || m_platformGatewaySockets[platformID].empty())
	{
		serverSocketLock.Release();
		return false;
	}

	set<CSceneCommServer*>::iterator it = m_platformGatewaySockets[platformID].begin();
	for (; it != m_platformGatewaySockets[platformID].end(); ++it)
	{
		if ((*it) == NULL)
			continue ;

		uint32 size = (*it)->GetWriteBuffer()->GetSpace();
		if (size > maxBufferSize)
		{
			maxBufferSize = size;
			bestGateway = (*it);
		}
	}
	serverSocketLock.Release();
	
	if ( bestGateway != NULL )
	{
		bestGateway->SendPacket( packet, serverOpcode );
		return true;
	}

	return false;
}
