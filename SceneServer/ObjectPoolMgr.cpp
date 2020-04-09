#include "stdafx.h"
#include "ObjectMgr.h"
#include "ObjectPoolMgr.h"

ObjectPoolMgr::ObjectPoolMgr()
{

}

ObjectPoolMgr::~ObjectPoolMgr()
{

}

void ObjectPoolMgr::InitBuffer()
{
#ifdef _WIN64
	CreateGameSessionBuffer( 20 );
	CreateMonstersBuffer( 100 );
#else
	CreateGameSessionBuffer( sPark.GetPlayerLimit() );
	CreateMonstersBuffer( 500 );
#endif
}

void ObjectPoolMgr::PrintObjPoolsUseStatus()
{
    sLog.outString("GameSession ³Ø Size = %u KB£¬µ±Ç°·ÖÅäSize = %d KB", GetGameSessionPoolSize(), GetGameSessionPoolAllocedSize());
}

void ObjectPoolMgr::CreateGameSessionBuffer(uint32 sessionNum)
{
    m_gameSessionPool.InitPool(sessionNum, sessionNum * 1.3f);
}

GameSession* ObjectPoolMgr::newSession()
{
    GameSession *pSession = m_gameSessionPool.newObject();
    return pSession;
}

void ObjectPoolMgr::deleteGameSession(GameSession *pSession)
{
    pSession->TermSession();
    m_gameSessionPool.deleteObject(pSession);
}

Monster* ObjectPoolMgr::newMonster(uint64 guid)
{
	Monster *m = new Monster();
	m->Initialize(guid);
	return m;
}

void ObjectPoolMgr::CreateMonstersBuffer(uint32 crNums)
{
	m_monsterPool.InitPool(crNums, crNums * 1.2f);
}

void ObjectPoolMgr::DeleteMonsterToBuffer(Monster *m)
{
	SAFE_DELETE(m);
}
