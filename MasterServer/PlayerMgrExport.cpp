#include "stdafx.h"
#include "PlayerMgr.h"

int CPlayerMgr::findPlayerServant(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint64 guid = lua_tointeger(l, 1);
	CPlayerServant *pServant = sPlayerMgr.FindServant(guid);
	int32 ret = 0;
	if (pServant != NULL)
	{
		Lunar<CPlayerServant>::push(l, pServant, false);
		ret = 1;
	}

	LuaReturn(l, top, ret);
}

int CPlayerMgr::updatePlayerServantRealGuid(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint64 tempGuid = lua_tointeger(l, 1);
	uint64 realGuid = lua_tointeger(l, 2);
	CPlayerServant *pServant = sPlayerMgr.FindServant(tempGuid);
	ASSERT(pServant != NULL);

	// 设置真实的玩家guid
	pServant->ResetCurPlayerGUID(realGuid);
	// 重新加入列表
	sPlayerMgr.ReAddServantWithRealGuid(pServant, tempGuid, realGuid);

	LuaReturn(l, top, 0);
}

int CPlayerMgr::invalidServant(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint64 guid = lua_tointeger(l, 1);
	uint32 sessionIndex = lua_tointeger(l, 2);
	bool ignoreSessionIndex = lua_toboolean(l, 3);
	bool ignoreServerSocket = lua_toboolean(l, 4);
	bool directLogout = lua_toboolean(l, 5);

	sPlayerMgr.InvalidServant(guid, sessionIndex, NULL, ignoreSessionIndex, ignoreServerSocket, directLogout);
	LuaReturn(l, top, 0);
}

LUA_IMPL(CPlayerMgr, CPlayerMgr)

LUA_METD_START(CPlayerMgr)
LUA_METD_END

LUA_FUNC_START(CPlayerMgr)
L_FUNCTION(CPlayerMgr, findPlayerServant)
L_FUNCTION(CPlayerMgr, updatePlayerServantRealGuid)
L_FUNCTION(CPlayerMgr, invalidServant)
LUA_FUNC_END

