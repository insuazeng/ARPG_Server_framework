#ifndef OBJECTPOOLMGR_H
#define OBJECTPOOLMGR_H

#include "ObjectPool.h"

class ObjectPoolMgr : public Singleton< ObjectPoolMgr >
{
public:
    ObjectPoolMgr();
    ~ObjectPoolMgr();

    void InitBuffer();

    // Session≥ÿ
    void	CreateGameSessionBuffer(uint32 sessionNum);
    GameSession* newSession();
    void	deleteGameSession(GameSession *pSession);
    inline uint32 GetGameSessionPoolSize() { return m_gameSessionPool.getPoolSize() / 1024; }
    inline int GetGameSessionPoolAllocedSize() { return m_gameSessionPool.getAllocedObjSize() / 1024; }

	// π÷ŒÔ≥ÿ
	Monster* newMonster(uint64 guid);
	void CreateMonstersBuffer(uint32 crNums);
	void DeleteMonsterToBuffer(Monster *m);
	inline uint32 GetCreaturePoolSize() { return m_monsterPool.getPoolSize() / 1024; }
	inline int GeTCreaturePoolAllocedSize() { return m_monsterPool.getAllocedObjSize() / 1024; }

	void PrintObjPoolsUseStatus();

private:
    CObjectPool<GameSession> m_gameSessionPool;
	CObjectPool<Monster> m_monsterPool;
};

#define objPoolMgr ObjectPoolMgr::GetSingleton()

#endif  // OBJECTPOOLMGR_H
