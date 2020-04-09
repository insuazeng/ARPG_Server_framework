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
	CreatePlayerServantBuffer( 20 );
	// CreatePlayerInfoBuffer( 20 );

#else
	CreatePlayerServantBuffer( 1000 );
	// CreatePlayerInfoBuffer( 3000 );

#endif
}

void ObjectPoolMgr::PrintObjPoolsUseStatus()
{
	sLog.outString("PlayerServant 池 Size = %u KB，当前分配Size = %d KB", GetServantPoolSize(), GetServantPoolAllocedSize());
	// sLog.outString("PlayerInfo 池 Size = %u KB，当前分配Size = %d KB", GetPlayerInfoPoolSize(), GetPlayerInfoAllocedSize());
}

CPlayerServant* ObjectPoolMgr::newPlayerServant(ServantCreateInfo * info, uint32 sessionIndex)
{
	CPlayerServant *servant = m_playerServantPool.newObject();
	servant->Init(info, sessionIndex);
	return servant;
}

void ObjectPoolMgr::CreatePlayerServantBuffer(uint32 servantNum)
{
	m_playerServantPool.InitPool(servantNum, servantNum * 1.2f);
}

void ObjectPoolMgr::gcPlayerServant(CPlayerServant *servant)
{
	servant->Term();
	m_playerServantPool.deleteObject(servant);
}

//PlayerInfo* ObjectPoolMgr::newPlayerInfo()
//{
//	PlayerInfo *info = m_playerInfoPool.newObject();
//	return info;
//}
//
//void ObjectPoolMgr::CreatePlayerInfoBuffer(uint32 playerNum)
//{
//	m_playerInfoPool.InitPool(playerNum, playerNum * 1.2f);
//}
//
//void ObjectPoolMgr::gcPlayerInfo(PlayerInfo *plrInfo)
//{
//	plrInfo->Term();
//	m_playerInfoPool.deleteObject(plrInfo);
//}

