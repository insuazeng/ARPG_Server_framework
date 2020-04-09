#include "stdafx.h"
#include "Monster.h"

Player* Monster::FindAttackablePlayerTarget(uint32 alertRange)
{
	if ( !IsInWorld() )
		return NULL;

	Player *aimPlayer = NULL;
	vector<Player*> vecPlayer;

	if ( m_inRangePlayers.empty() )
	{
		for ( PlayerStorageMap::iterator itr = m_mapMgr->m_PlayerStorage.begin(); 
			itr != m_mapMgr->m_PlayerStorage.end(); ++itr )
		{
			aimPlayer = itr->second;
			if ( aimPlayer == NULL || !aimPlayer->isAlive() || !CanSee(aimPlayer) )
				continue;

			if(CalcDistanceWithModelSize(aimPlayer) < alertRange)
			{
				vecPlayer.push_back(aimPlayer);
				if (vecPlayer.size() > 10)		// 最多10个目标
					break ;
			}
		}
	}
	else
	{
		for (hash_set<Player*>::iterator it = GetInRangePlayerSetBegin(); it != GetInRangePlayerSetEnd(); ++it)
		{
			aimPlayer = (*it);
			if ( aimPlayer == NULL || !aimPlayer->isAlive() || !CanSee(aimPlayer) )
				continue;

			if(CalcDistanceWithModelSize(aimPlayer) < alertRange)
			{
				vecPlayer.push_back(aimPlayer);
				if (vecPlayer.size() > 10)		// 最多10个目标
					break ;
			}
		}
	}

	Player *pRet = NULL;
	if (!vecPlayer.empty())
		pRet = vecPlayer[RandomUInt(vecPlayer.size())];

	return pRet;
}
