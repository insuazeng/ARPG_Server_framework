#include "stdafx.h"
#include "StatisticsSystem.h"

StatisticsSystem::StatisticsSystem()
{
	m_bNewRecord = false;
	memset(&m_tmLastOnlineUpdate,0,sizeof(m_tmLastOnlineUpdate));
}

StatisticsSystem::~StatisticsSystem()
{
}

void StatisticsSystem::Update(time_t diff)
{
	tm* tmLocalCurrentTime;
	time_t tCurrentTime = UNIXTIME;

	tmLocalCurrentTime = localtime(&tCurrentTime);
	if( (tmLocalCurrentTime->tm_min % 5 == 0) && (tmLocalCurrentTime->tm_min != m_tmLastOnlineUpdate.tm_min) )
	{
		memcpy(&m_tmLastOnlineUpdate,tmLocalCurrentTime,sizeof(tm));
        
		tagSqlExecutedCounter countData;

		LogDatabase.FillSqlExecutedCountInfo(countData);
		
		uint32 sendBytePerMin = sGateCommHandler.m_sendDataByteCountPer5min / 5;
		uint32 recvBytePerMin = sGateCommHandler.m_recvDataByteCountPer5min / 5;
		uint32 sendPacketCount = sGateCommHandler.m_sendPacketCountPer5min;

		LogDatabase.Execute("Insert Into game_sqlquery_count Values(0,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,0,0,NOW());", sGateCommHandler.m_sceneServerID, sPark.GetSessionCount(), countData.uSelect, 
							countData.uUpdate, countData.uDelete, countData.uInsert, countData.uReplace, countData.uOthers, sendBytePerMin, recvBytePerMin, sendPacketCount);
        
		LogDatabase.ClearSqlExecutedCountInfo();
		
		sGateCommHandler.m_sendDataByteCountPer5min = 0;
		sGateCommHandler.m_recvDataByteCountPer5min = 0;
		sGateCommHandler.m_sendPacketCountPer5min = 0;
	}

	return;
}

void StatisticsSystem::Init()
{
}

void StatisticsSystem::OnShutDownServer()
{
	SaveAll();
}
void StatisticsSystem::DailyEvent()
{
	SaveAll();
}

void StatisticsSystem::SaveAll()
{

}

void StatisticsSystem::WeeklyEvent()
{
}

void StatisticsSystem::OnInstanceCreated( Instance *in )
{
}

void StatisticsSystem::OnInstancePassed( Instance *in )
{
	if ( in == NULL )
		return;
}

void StatisticsSystem::OnPlayerLeaveUndercity( Instance *in, Player * plr )
{
	if ( in == NULL || plr == NULL )
	{
		return;
	}
}


