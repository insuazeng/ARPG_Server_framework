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

//
// GameSession.cpp
//
#include "stdafx.h"
#include "ObjectMgr.h"
#include "ServerCommonDef.h"

GameSession::GameSession()
{

}

GameSession::~GameSession()
{

}

void GameSession::InitSession(string &accName, uint32 srcPlatform, string &agentName, CSceneCommServer *sock, uint32 index, uint64 plrGuid, uint32 gmflag /*= 0*/, bool internalPlayer /*= false*/)
{
	_accountName = accName;
	_accountAgentName = agentName;
	_srcPlatformID = srcPlatform;
	_guid = plrGuid;

	_logoutMsTime = 0;
	_loggingOut = false;
	instanceId = 0;
	m_SessionIndex = index;
	m_NextSceneServerID = 0;
	m_pServerSocket = sock;
	bDeleted = false;
	m_GMFlags = gmflag;
	m_internal_player_flag = internalPlayer;
	m_player = NULL;
}

void GameSession::TermSession()
{
	_accountName = "";
	_accountAgentName = "";
	_guid = 0;
	if (m_player != NULL && m_player->GetGUID() > 0)
	{
		sLog.outError("在GameSession析构时有残存的Player对象(guid="I64FMTD")", m_player->GetGUID());
	}
	m_player = NULL;
	WorldPacket* packet;
	while((packet = _recvQueue.Pop()))
		DeleteWorldPacket(packet);
}

void GameSession::SendPacket(WorldPacket* packet, ENUMMSGPRIORITY msgPriority/* = HIGH_PRIORITY*/)
{	
	deleteMutex.Acquire();
	if(m_pServerSocket && m_pServerSocket->IsConnected())
		m_pServerSocket->SendSessioningResultPacket(packet, GetUserGUID(), msgPriority);
	deleteMutex.Release();
}

void GameSession::SendServerPacket(WorldPacket* packet)
{	
	// 此处之所以要设置serverOpCode是因为部分主动调用的会话结果包，
	// 如果不设置serverOpCode会在后续被设置为opcode导致网关处理出错 @haoye 13.7.11
	uint32 serverOpCode = packet->GetServerOpCode() != 0 ? packet->GetServerOpCode() : 0;
	uint64 guid = serverOpCode > 0 ? packet->GetUserGUID() : 0;
	deleteMutex.Acquire();
	if(m_pServerSocket&&m_pServerSocket->IsConnected())
		m_pServerSocket->SendPacket(packet, serverOpCode, guid);
	deleteMutex.Release();
}

int GameSession::Update(uint32 InstanceID)
{
	WorldPacket* packet;
	OpcodeHandler * Handler;

	if(InstanceID != instanceId)
	{
		// We're being updated by the wrong thread.
		// "Remove us!" - 2
		return 2;
	}

	if(!m_pServerSocket)
	{
		bDeleted = true;
		m_NextSceneServerID = 0;
		LogoutPlayer(true);//正常退出
		return 1;
	}

	while ((packet = _recvQueue.Pop()))
	{
		const uint32 op = packet->GetOpcode();
		switch (op)
		{
		case CMSG_002003:
			HandlePlayerMove(*packet);
			break;
		case CMSG_002005:
			HandlePlayerSceneTransfer(*packet);
			break;
		default:
			{
				// 交给lua处理
				ScriptParamArray params;
				params << op;
				params << GetUserGUID();
				if (!packet->empty())
				{
					size_t sz = packet->size();
					params.AppendUserData((void*)packet->contents(), sz);
					params << sz;
				}
				LuaScript.Call("HandleWorldPacket", &params, &params);
			}
			break;
		}
		DeleteWorldPacket( packet );

		if(InstanceID != instanceId)
		{
			// If we hit this -> means a packet has changed our map.
			return 2;
		}
	}

	if(InstanceID != instanceId)
	{
		// If we hit this -> means a packet has changed our map.
		return 2;
	}

	if( _logoutMsTime > 0 && (getMSTime64() >= _logoutMsTime) && instanceId == InstanceID)
	{
		// 玩家切换场景服导致的角色登出本场景服
		bDeleted = true;
		LogoutPlayer(true);
		return 1;
	}

	return 0;
}

//
void GameSession::LogoutPlayer(bool SaveToCentre)
{
	if(_loggingOut)
		return;
	_loggingOut = true;

	if (m_player != NULL && m_player->GetGUID() > 0)
	{
		ScriptParamArray params;
		params << m_player->GetGUID();
		params << SaveToCentre;
		LuaScript.Call("LogoutPlayer", &params, NULL);

		m_player = NULL;
	}
	SetLogoutTimer(0);
}

void GameSession::SendCreateSuccessPacket()
{
	Player *plr = GetPlayer();

	WorldPacket packet(SMSG_SCENE_2_GATE_ENTER_GAME_RET, 128);
	packet << GetPlayer()->GetGUID();
	packet << uint32(P_ENTER_RET_SUCCESS);
	packet << sGateCommHandler.m_sceneServerID;
	packet << uint32(UNIXTIME);
	packet << plr->strGetUtf8NameString();
	packet << plr->GetCareer();
	packet << plr->GetLevel();
	packet << (uint32)plr->GetUInt64Value(UNIT_FIELD_MAX_HP);
	packet << (uint32)plr->GetUInt64Value(UNIT_FIELD_CUR_HP);
	packet << plr->GetMapId();
	plr->WriteCurPos(packet);
	packet << sSceneMapMgr.GetPositionHeight(plr->GetMapId(), plr->GetPositionX(), plr->GetPositionY());
	SendServerPacket(&packet);
}

