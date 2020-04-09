#include "stdafx.h"
#include "Park.h"

int Park::getServerGlobalID(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	lua_pushinteger(l, sPark.GetServerID());

	LuaReturn(l, top, 1);
}

int Park::getSelfSceneServerID(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushinteger(l, sGateCommHandler.GetSelfSceneServerID());

	LuaReturn(l, top, 1);
}

int Park::getCurUnixTime(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	lua_pushinteger(l, UNIXTIME);

	LuaReturn(l, top, 1);
}

int Park::getSessionByGuid(lua_State *l)
{
	int32 top = lua_gettop(l);
	int ret = 0;
	uint64 guid = (uint64)lua_tointeger(l, 1);
	
	GameSession *session = sPark.FindSession(guid);
	if (session != NULL)
	{
		Lunar<GameSession>::push(l, session, false);
		ret = 1;
	}

	LuaReturn(l, top, ret);
}

int Park::setPlayerVisibleUpdateMaskBits(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 bitCount = lua_tointeger(l, 1);
	if (bitCount == 0 || !lua_istable(l, 2))
	{
		sLog.outError("setPlayerVisibleUpdateMaskBits failed, bitCount=%u or param 2 error", bitCount);
		LuaReturn(l, top, 0);
	}

	Player::m_visibleUpdateMask.Clear();
	Player::m_visibleUpdateMask.SetCount(bitCount);
	
	stringstream ss;

	uint32 cnt = lua_objlen(l, 2);
	lua_pushnil(l);
	for (int i = 0; i < cnt; ++i)
	{
		lua_next(l, 2);
		uint32 idx = lua_tointeger(l, -1);
		lua_pop(l, 1);
		if (idx < bitCount)
		{
			ss << idx << ",";
			Player::m_visibleUpdateMask.SetBit(idx);
		}
		else
		{
			sLog.outError("setPlayerVisibleUpdateMaskBits failed, bitIndex(%u) error,bitCount=%u", idx, bitCount);
		}
	}
	lua_pop(l, 1);
	
	sLog.outDebug("[系统设置]Player类UpdateMask标记(%u个)有:%s", bitCount, ss.str().c_str());
	LuaReturn(l, top, 0);
}

int Park::setMonsterVisibleUpdateMaskBits(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 bitCount = lua_tointeger(l, 1);
	if (bitCount == 0 || !lua_istable(l, 2))
	{
		sLog.outError("setMonsterVisibleUpdateMaskBits failed, bitCount=%u or param 2 error", bitCount);
		LuaReturn(l, top, 0);
	}

	Monster::m_visibleUpdateMask.Clear();
	Monster::m_visibleUpdateMask.SetCount(bitCount);

	uint32 cnt = lua_objlen(l, 2);
	lua_pushnil(l);
	for (int i = 0; i < cnt; ++i)
	{
		lua_next(l, 2);
		uint32 idx = lua_tointeger(l, -1);
		lua_pop(l, 1);
		if (idx < bitCount)
		{
			Monster::m_visibleUpdateMask.SetBit(idx);
		}
		else
		{
			sLog.outError("setMonsterVisibleUpdateMaskBits failed, bitIndex(%u) error,bitCount=%u", idx, bitCount);
		}
	}
	lua_pop(l, 1);

	LuaReturn(l, top, 0);
}

int Park::setPartnerVisibleUpdateMaskBits(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 bitCount = lua_tointeger(l, 1);
	if (bitCount == 0 || !lua_istable(l, 2))
	{
		sLog.outError("setPartnerVisibleUpdateMaskBits failed, bitCount=%u or param 2 error", bitCount);
		LuaReturn(l, top, 0);
	}

	Partner::m_visibleUpdateMask.Clear();
	Partner::m_visibleUpdateMask.SetCount(bitCount);

	uint32 cnt = lua_objlen(l, 2);
	lua_pushnil(l);
	for (int i = 0; i < cnt; ++i)
	{
		lua_next(l, 2);
		uint32 idx = lua_tointeger(l, -1);
		lua_pop(l, 1);
		if (idx < bitCount)
		{
			Partner::m_visibleUpdateMask.SetBit(idx);
		}
		else
		{
			sLog.outError("setPartnerVisibleUpdateMaskBits failed, bitIndex(%u) error,bitCount=%u", idx, bitCount);
		}
	}
	lua_pop(l, 1);

	LuaReturn(l, top, 0);
}

int Park::trySendPacket(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint32 srcPlatfromID = lua_tointeger(l, 1);
	WorldPacket *pck = (WorldPacket *)lua_touserdata(l, 2);
	uint32 serverOp = lua_tointeger(l, 3);
	sGateCommHandler.TrySendPacket(srcPlatfromID, pck, serverOp);

	LuaReturn(l, top, 0);
}


LUA_IMPL(Park, Park)

LUA_METD_START(Park)
LUA_METD_END

LUA_FUNC_START(Park)
L_FUNCTION(Park, getServerGlobalID)
L_FUNCTION(Park, getSelfSceneServerID)
L_FUNCTION(Park, getCurUnixTime)
L_FUNCTION(Park, getSessionByGuid)
L_FUNCTION(Park, setPlayerVisibleUpdateMaskBits)
L_FUNCTION(Park, setMonsterVisibleUpdateMaskBits)
L_FUNCTION(Park, setPartnerVisibleUpdateMaskBits)
LUA_FUNC_END

