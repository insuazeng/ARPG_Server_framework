#include "stdafx.h"
#include "PlayerMgr.h"
#include "ObjectPoolMgr.h"

CPlayerMgr::CPlayerMgr(void) : m_BlockChatInterval( 0 )
{
}

CPlayerMgr::~CPlayerMgr(void)
{
	Terminate();
}

void CPlayerMgr::Initialize()
{
	load_ForbidWord();
}

void CPlayerMgr::Terminate()
{
	for (ServantMap::iterator it = m_onlinePlayerList.begin(); it != m_onlinePlayerList.end(); ++it)
	{
		(*it).second->Term();
		delete (*it).second;
	}
	m_onlinePlayerList.clear();

	for ( BlockMap::iterator bitr = m_BlockPlayers.begin(); bitr != m_BlockPlayers.end(); ++bitr )
	{
		delete bitr->second;
	}
	m_BlockPlayers.clear();
}

void CPlayerMgr::OnGateCommServerDisconnect(CMasterCommServer *pServer)
{
	m_playersLock.AcquireReadLock();
	for (ServantMap::iterator it = m_onlinePlayerList.begin(); it != m_onlinePlayerList.end(); ++it)
	{
		if ((*it).second->m_pServerSocket == pServer)
			(*it).second->SetWaitLastData();
	}
	m_playersLock.ReleaseReadLock();
}

// 添加一个角色的服务
CPlayerServant * CPlayerMgr::CreatePlayerServant( ServantCreateInfo * info , uint32 sessionIndex, CMasterCommServer *pSocket )
{
	if ( info == NULL )
	{
		return NULL;
	}

	CPlayerServant * p = objPoolMgr.newPlayerServant(info, sessionIndex);
	if ( p != NULL )
	{
		p->SetCommSocketPtr( pSocket );
		// sLog.outString( "初始化Servant %s (sessionIndex %u)", info->m_accountname.c_str(), sessionIndex );
	}
	return p;
}

extern int g_serverTimeOffset;//时差

void CPlayerMgr::AddServant(CPlayerServant *pServant)
{
	m_playersLock.AcquireWriteLock();
	m_onlinePlayerList[pServant->m_RoleList.m_nPlayerGuid] = pServant;
	m_playersLock.ReleaseWriteLock();

	pServant->FillRoleListInfo();

	sLog.outString("[登陆]账号%s(源网关=%u)成功读取人物%u个,SessionIndex=%u", pServant->m_RoleList.m_strAccountName.c_str(), 
					pServant->GetServerSocket()->GetGateServerId(), pServant->m_RoleList.GetRolesNum(), pServant->m_nSessionIndex);

	pbc_wmessage *msg1006 = sPbcRegister.Make_pbc_wmessage("UserProto001006");
	pServant->WriteRoleListDataToBuffer(msg1006);

	WorldPacket packet(SMSG_001006, 8);
	sPbcRegister.EncodePbcDataToPacket(packet, msg1006);
	pServant->SendResultPacket(&packet);
	DEL_PBC_W(msg1006);

	pServant->m_nGameState = STATUS_LOGIN;
}

void CPlayerMgr::ReAddServantWithRealGuid(CPlayerServant *pServant, uint64 tempGuid, uint64 realGuid)
{
	// 从在线列表中移除临时的guid key而后再重新添加
	m_playersLock.AcquireWriteLock();
	m_onlinePlayerList.erase(tempGuid);
	ASSERT(m_onlinePlayerList.find(realGuid) == m_onlinePlayerList.end());
	m_onlinePlayerList.insert(make_pair(realGuid, pServant));
	m_playersLock.ReleaseWriteLock();
}

void CPlayerMgr::InvalidServant(uint64 guid, uint32 sessionIndex, CMasterCommServer *pFromServer, bool ingoreIndex /*= false*/, bool ingoreServerSocket /*= false*/,bool direct /*= true*/)
{
	m_playersLock.AcquireWriteLock();

	ServantMap::iterator it = m_onlinePlayerList.find(guid);
	if (it != m_onlinePlayerList.end() && (ingoreIndex || sessionIndex == it->second->m_nSessionIndex))
	{
		CPlayerServant *pServant = it->second;
		if (guid != pServant->GetCurPlayerGUID())
		{
			sLog.outError("需要设置下线的Servant对象,Guid值不匹配(传参="I64FMTD",当前="I64FMTD")", guid, pServant->GetCurPlayerGUID());
			m_playersLock.ReleaseWriteLock();
			return ;
		}
		if (!ingoreServerSocket && pFromServer != NULL && pServant->GetServerSocket() != pFromServer)
		{
			if (pServant->GetServerSocket() == NULL)
				sLog.outError("Servant guid="I64FMTD",index=%u Can't be invalid as valus is NULL with srcServerSocket %p", guid, pServant->m_nSessionIndex, pFromServer);
			m_playersLock.ReleaseWriteLock();
			return ;
		}

		if(direct)
		{		
			//这里有两种，一种是收到游戏服保存并退出的数据包后设置，一种是网关直接通知不在游戏服的玩家退出
			sLog.outDebug("[下线]玩家guid="I64FMTD",sessionIndex=%u,源网关套接字被设置为NULL,准备直接退出登出,不等待", guid, pServant->m_nSessionIndex);
			pServant->SetCommSocketPtr(NULL);
		}
		else
		{
			pServant->SetWaitLastData();
			sLog.outDebug("guid= "I64FMTD" need wait playsavedata from gateserver %u, sessionindex = %u", guid, pServant->GetServerSocket(), pServant->m_nSessionIndex);
		}

		// 设置离线标记
		// GameDatabase.Execute("UPDATE character_dynamic_info set online_flag=0 where player_guid="I64FMTD";", plrGuid);
		BSONObjBuilder b;
		b.appendIntOrLL("player_guid", guid);
		MongoGameDatabase.update(sDBConfLoader.GetMongoTableName("character_datas"), mongo::Query(b.obj()), BSON("$set" << BSON("online_flag" << 0)), false);
	}
	else
		sLog.outDebug("servant "I64FMTD" sessionindex %u not in m_onlinePlayerList", guid, sessionIndex);

	m_playersLock.ReleaseWriteLock();

	// 让等待队列中的玩家下线
	for (std::list<CPlayerServant*>::iterator it = _queueServants.begin(); it != _queueServants.end(); ++it)
	{
		CPlayerServant *pServant = *it;
		if(!ingoreIndex && sessionIndex != pServant->m_nSessionIndex)
			continue;

		if (!ingoreServerSocket && pFromServer != NULL && pServant->GetServerSocket() != pFromServer)
		{
			if (pServant->GetServerSocket() == NULL)
				sLog.outError("Servant %u Can't be invalid as valus is NULL with srcServerSocket %p", pServant->m_nSessionIndex, pFromServer);
		}
		
		sLog.outString("servant "I64FMTD" close Servant whiled in pending queue, sessionindex = %d", guid, pServant->m_nSessionIndex);
		pServant->SetCommSocketPtr(NULL);
	}
}

void CPlayerMgr::SessioningProc(WorldPacket *recvData, uint64 guid)
{
	m_playersLock.AcquireWriteLock();
	ServantMap::iterator it = m_onlinePlayerList.find(guid);
	if (it != m_onlinePlayerList.end() && it->second != NULL)
		(*it).second->QueuePacket(recvData);
	else
	{
		sLog.outError("收到不存在的玩家(guid="I64FMTD")的数据包,op=%u", guid, recvData->GetOpcode());
		DeleteWorldPacket(recvData);
	}
	m_playersLock.ReleaseWriteLock();
}

void CPlayerMgr::SaveAllOnlineServants()
{
	m_playersLock.AcquireWriteLock();
	sLog.outString("Save %u Servants At SaveAllOnlineServants", (uint32)m_onlinePlayerList.size());
	for (ServantMap::iterator it = m_onlinePlayerList.begin(); it != m_onlinePlayerList.end(); ++it)
	{
		(*it).second->LogoutServant(true);
	}
	m_playersLock.ReleaseWriteLock();
}

void CPlayerMgr::UpdatePlayerServants(float diff)
{
	// 处理每个玩家到来的消息包
	ServantMap::iterator it = m_onlinePlayerList.begin(), it2;
	CPlayerServant *servant = NULL;

	for (; it != m_onlinePlayerList.end();)
	{
		servant = (*it).second;
		it2 = it;
		++it;
		if (servant == NULL)
		{
			m_onlinePlayerList.erase(it2);
			continue;
		}
		int ret = servant->Update(diff);
		if (-1 == ret)		// 该servant失效了，要在列表中删除它！
		{
			sLog.outString("[下线成功]中央服务器删除会话 agentName=%s,accName=%s,sessionindex=%u", servant->strGetAgentName().c_str(), servant->szGetAccoundName(), servant->m_nSessionIndex);
			objPoolMgr.gcPlayerServant(servant);
			m_onlinePlayerList.erase(it2);
		}
	}
	UpdateQueueServants(diff);
}

void CPlayerMgr::UpdateQueueServants( float diff )
{
	CPlayerServant * servant = NULL;
	CPlayerServant * pExist = NULL;

	std::vector<CPlayerServant*> vecReAddServants;
	for(std::list< CPlayerServant *>::iterator itr = _queueServants.begin();itr!=_queueServants.end();++itr)
	{
		servant = *itr;
		if(servant->GetServerSocket()==NULL || !servant->GetServerSocket()->IsConnected())
		{
			sLog.outError( "Error: user guid= "I64FMTD" disconnected before add to game", servant->GetCurPlayerGUID() );
			continue;
		}
		if ( getMSTime64() - servant->m_nReloginStartTime < 1000 ) // 此代码可以去掉，现在保留用来测试下面的重复登录问题
		{
			vecReAddServants.push_back(servant);
			continue;
		}
		pExist = FindServant( servant->strGetAgentName(), servant->strGetAccountName() );
		if ( pExist != NULL )               // 延迟处理后再次登录导致
		{
			if ( pExist->IsBeDeleted() || pExist->IsWaitLastData())    // 其他操作尚未处理完
			{
				vecReAddServants.push_back(servant);
				continue;
			}
			sLog.outError( "Error: 非法servant guid="I64FMTD" 被添加！", servant->GetCurPlayerGUID() );
			servant->DisConnect();
			objPoolMgr.gcPlayerServant(servant);
		}
		else
		{
			AddServant( servant );
			sLog.outDebug( "玩家%s servant被延迟加入成功！", servant->szGetAccoundName() );
		}
	}
	_queueServants.clear();

	if(!vecReAddServants.empty())
		sLog.outDebug( "还有%u个servant等待中！", uint32(vecReAddServants.size()) );

	for (vector<CPlayerServant*>::iterator it = vecReAddServants.begin(); it != vecReAddServants.end(); ++it)
	{
		AddPendingServant(*it);
	}
}

CPlayerServant * CPlayerMgr::FindServant( uint64 guid )
{
	ServantMap::iterator itr;
	CPlayerServant * servant = NULL;
	m_playersLock.AcquireWriteLock();
	itr = m_onlinePlayerList.find( guid );
	if ( itr != m_onlinePlayerList.end() )
	{
		servant = itr->second;
	}
	m_playersLock.ReleaseWriteLock();
	return servant;
}

CPlayerServant * CPlayerMgr::FindServant( const string &agent, const string &account )
{
	ServantMap::iterator itr = m_onlinePlayerList.begin();
	CPlayerServant *ret = NULL, *servant = NULL;
	m_playersLock.AcquireWriteLock();
	for (; itr != m_onlinePlayerList.end(); ++itr)
	{
		servant = itr->second;
		if (servant == NULL)
			continue ;
		if (servant->strGetAccountName() == account && servant->strGetAgentName() == agent)
		{
			ret = servant;
			break;
		}
	}
	m_playersLock.ReleaseWriteLock();
	return ret;
}

uint32 CPlayerMgr::GetBlockPlayer( uint32 accoutid )
{
	BlockMap::iterator itr = m_BlockPlayers.find( accoutid );
	if ( itr == m_BlockPlayers.end() )
	{
		return CHAT_STATE_FREE;
	}
	if ( itr->second->block_state == CHAT_SQL_BLOCKING )
	{
		return CHAT_STATE_BLOCKED;
	}
	return CHAT_STATE_FREE;
}

void CPlayerMgr::LoadBlockPlayers( bool first /* = false */ )
{
	/*QueryResult * result = LogDatabase.Query( "SELECT * FROM game_forbid_chat WHERE forbid_state < %u", ( first ? 2 : 1 ) );

	Field * fields = NULL;
	BlockChatRecord * block = NULL;

	if ( result != NULL )
	{
		do
		{
			fields = result->Fetch();
			uint32 account_id = fields[1].GetUInt32();
			if ( m_BlockPlayers.find( account_id ) == m_BlockPlayers.end() )
			{
				block = new BlockChatRecord;
				if ( block == NULL )
				{
					sLog.outError( "Fail to allocate memory for BlockChatRecord" );
					continue;
				}
				m_BlockPlayers[account_id] = block;
			}
			else
			{
				block = m_BlockPlayers[account_id];
			}
			block->id = fields[0].GetUInt32();
			block->accountid = account_id;
			block->block_state = fields[2].GetUInt32();
			block->start_time = fields[5].GetUInt32();
			block->end_time = fields[6].GetUInt32();
		} while ( result->NextRow() );
		delete result;
	}

	result = LogDatabase.Query( "Select * from game_forbid_chat where forbid_state = 3" );
	if ( result != NULL )
	{
		do 
		{
			fields = result->Fetch();
			uint32 record_id = fields[0].GetUInt32();
			BlockMap::iterator itr = m_BlockPlayers.find( fields[1].GetUInt32() );
			if ( itr != m_BlockPlayers.end() )
			{
				if ( itr->second->block_state == CHAT_SQL_BLOCKING )
				{
					WorldPacket packet( CENTER_TO_GATE_BLOCKCHAT, 8 );
					packet << itr->second->accountid;
					packet << (uint32)CHAT_STATE_FREE;
					sInfoCore.SendMsgToGateServers( &packet, CENTER_TO_GATE_BLOCKCHAT );
				}
				delete itr->second;
				m_BlockPlayers.erase( itr );
			}

			LogDatabase.Execute( "Update game_forbid_chat Set forbid_state = 2 where id = %u", record_id );
		} while ( result->NextRow() );
		delete result;
	}*/
}

void CPlayerMgr::UpdateBlockChat( time_t diff )
{
	/*if ( m_BlockChatInterval > UNIXTIME )
	{
		return;
	}

	m_BlockChatInterval = UNIXTIME + 5;
	LoadBlockPlayers( false );
	BlockMap::iterator itr = m_BlockPlayers.begin();
	BlockMap::iterator itr2;
	while( itr != m_BlockPlayers.end() )
	{
		itr2 = itr++;
		BlockChatRecord * record = itr2->second;

		if ( record->block_state != CHAT_SQL_NEW && record->end_time < UNIXTIME )
		{
			WorldPacket packet( CENTER_TO_GATE_BLOCKCHAT, 8 );
			packet << record->accountid;
			packet << (uint32)CHAT_STATE_FREE;
			sInfoCore.SendMsgToGateServers( &packet, CENTER_TO_GATE_BLOCKCHAT );
			m_BlockPlayers.erase( itr2 );
			LogDatabase.Execute( "Update game_forbid_chat Set forbid_state = 2 where id = %u", record->id );
			delete record;
			continue;
		}

		if ( record->block_state == CHAT_SQL_NEW && record->start_time <= UNIXTIME )
		{
			if ( record->end_time < UNIXTIME )
			{
				LogDatabase.Execute( "Update game_forbid_chat set forbid_state = 2 where id=%u", record->id );
				m_BlockPlayers.erase( itr2 );
				delete record;
				continue;
			}

			LogDatabase.Execute( "Update game_forbid_chat Set forbid_state = 1 Where id = %u", record->id );

			WorldPacket packet( CENTER_TO_GATE_BLOCKCHAT, 8 );
			packet << record->accountid;
			packet << (uint32)CHAT_STATE_BLOCKED;
			sInfoCore.SendMsgToGateServers( &packet, CENTER_TO_GATE_BLOCKCHAT );
			record->block_state = CHAT_SQL_BLOCKING;
		}
	}*/
}

void CPlayerMgr::load_ForbidWord()
{
	/*QueryResult *result = NULL;
	result = WorldDatabase.Query("select context,level from forbid_words");

	if ( NULL != result )
	{
		if( !m_forbidWord.empty() || !m_forbidWordlevel1.empty() )
		{
			m_forbidWord.clear();
			m_forbidWordlevel1.clear();
		}

		Field * fields = NULL;
		fields = result->Fetch();
		do 
		{
			if( fields[1].GetUInt32() == FORBID_LEVELLOW )
			{
				m_forbidWord.push_back(fields[0].GetString());
			}
			else
			{
				m_forbidWordlevel1.push_back(fields[0].GetString());
			}

		} while (result->NextRow());
	}

	if( NULL != result )
	{
		delete result;
		result = NULL;
	}*/
}

void CPlayerMgr::Save_ChatLog( const char* pcName, const uint32 uGuid, const std::string & pcText ,bool toalllog,uint32 chanel,uint32 group)
{
	/*std::string context = LogDatabase.EscapeString(pcText);
	if(toalllog)
	{
		LogDatabase.Execute("INSERT INTO game_chatlog_all VALUES(0,%u,%u,'%s',%u,'%s',NOW());",chanel,group,pcName,uGuid,context.c_str());
	}
	else
	{
		LogDatabase.Execute("INSERT INTO game_chatlog VALUES(0,'%s',%u,'%s',NOW());",pcName,uGuid,context.c_str());
	}*/
}

void CPlayerMgr::DailyClearForbidLog()
{
	//LogDatabase.Execute("TRUNCATE TABLE game_chatlog");
}

uint32 CPlayerMgr::AnalysisWords( const char* pcName, const uint32 uGuid, const string & strText )
{
	const std::string * pStr;

#ifdef NEED_CONVERT_LOWER
	std::string content;
	content.resize( strText.length(), '\0' );
	transform( strText.begin(), strText.end(), strText.begin(), tolower );
	pStr = &content;
#else
	pStr = &strText;
#endif

	for ( std::vector<string>::iterator iter = m_forbidWordlevel1.begin(); iter != m_forbidWordlevel1.end(); ++iter )
	{
		if ( std::string::npos != pStr->find( *iter ) )
		{
			Save_ChatLog( pcName, uGuid, strText,false,0,0);
			return FORBID_LEVELHIGH;
		}
	}

	for ( std::vector<string>::iterator iter = m_forbidWord.begin(); iter != m_forbidWord.end(); ++iter )
	{
		if ( std::string::npos != pStr->find( *iter ) )
		{
			Save_ChatLog( pcName, uGuid, strText,false,0,0);
			return FORBID_LEVELLOW;
		}
	}
	return FORBID_LEVELLOW;
}

void CPlayerMgr::ReLoadFrbidWords()
{
	load_ForbidWord();
}

void CPlayerMgr::FillSpecifiedConditonAccountIDs(vector<uint64> &vecGUIDs, bool onlineFlag, uint32 minLevel /*= 0*/, uint32 maxLevel /*= ILLEGAL_DATA*/)
{
	// 根据某种条件填入玩家的游戏账号，方面后面批量发送数据库
	/*CPlayerServant *plr = NULL;
	PlayerInfoMap::const_iterator iter = m_playersInfo.begin();
	for(; iter != m_playersInfo.end(); iter++)
	{
		PlayerInfo *plrInfo = iter->second;
		if(plrInfo == NULL || plrInfo->m_loggedInPlayer == NULL)
			continue;

		plr = plrInfo->m_loggedInPlayer;
		if (onlineFlag && !plr->IsInGameStatus(STATUS_GAMEING))
			continue ;

		if (plrInfo->level < minLevel || plrInfo->level > maxLevel)
			continue ;

		vecGUIDs.push_back(plrInfo->guid);
	}*/
}

void CPlayerMgr::SendPacketToOnlinePlayers(WorldPacket &packet)
{
	// 仅仅针对在线并已经登陆到游戏内的玩家发送
	vector<uint64> onlineGuids;
	FillSpecifiedConditonAccountIDs(onlineGuids, true);
	if (!onlineGuids.empty())
		SendPacketToPartialPlayers(packet, onlineGuids);
}

void CPlayerMgr::SendPacketToOnlinePlayersWithSpecifiedLevel(WorldPacket &packet, uint32 minLevel /*= 0*/, uint32 maxLevel /*= ILLEGAL_DATA*/)
{
	vector<uint64> onlineGuids;
	FillSpecifiedConditonAccountIDs(onlineGuids, true, minLevel, maxLevel);
	if (!onlineGuids.empty())
		SendPacketToPartialPlayers(packet, onlineGuids);
}

void CPlayerMgr::SendPacketToPartialPlayers(WorldPacket &packet, vector<uint64> &vecUserGUIDs)
{
	if (vecUserGUIDs.empty())
		return ;

	WorldPacket globalPacket(packet.GetOpcode(), 128);
	uint32 maxSize = vecUserGUIDs.size();
	globalPacket << maxSize;
	for (int i = 0; i < maxSize; ++i)
	{
		globalPacket << vecUserGUIDs[i];
	}
	globalPacket.append(packet.contents(), packet.size());
	// sInfoCore.SendMsgToGateServers( &globalPacket );
}


