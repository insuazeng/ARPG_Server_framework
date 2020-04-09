#ifndef OBJECTPOOLMGR_H
#define OBJECTPOOLMGR_H

#include "ObjectPool.h"

class ObjectPoolMgr : public Singleton< ObjectPoolMgr >
{
public:
    ObjectPoolMgr();
    ~ObjectPoolMgr();

    void InitBuffer();

	// 玩家会话对象
	CPlayerServant* newPlayerServant(ServantCreateInfo * info, uint32 sessionIndex);
	void CreatePlayerServantBuffer(uint32 servantNum);
	void gcPlayerServant(CPlayerServant *servant);
	inline const uint32 GetServantPoolSize() { return m_playerServantPool.getPoolSize() / 1024; }
	inline const uint32 GetServantPoolAllocedSize() { return m_playerServantPool.getAllocedObjSize() / 1024; }

	// 玩家信息数据
	/*PlayerInfo* newPlayerInfo();
	void CreatePlayerInfoBuffer(uint32 playerNum);
	void gcPlayerInfo(PlayerInfo *plrInfo);
	inline const uint32 GetPlayerInfoPoolSize() { return m_playerInfoPool.getPoolSize() / 1024; }
	inline const uint32 GetPlayerInfoAllocedSize() { return m_playerInfoPool.getAllocedObjSize() / 1024; }*/

    void PrintObjPoolsUseStatus();

private:
	CObjectPool<CPlayerServant> m_playerServantPool;
	// CObjectPool<PlayerInfo> m_playerInfoPool;
};

#define objPoolMgr ObjectPoolMgr::GetSingleton()

#endif  // OBJECTPOOLMGR_H
