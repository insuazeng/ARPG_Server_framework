#include "stdafx.h"
#include "PlayerServant.h"

int CPlayerServant::getAccountName(lua_State *l)
{
	int32 top = lua_gettop(l);
	lua_pushstring(l, m_RoleList.m_strAccountName.c_str());

	LuaReturn(l, top, 1);
}

int CPlayerServant::getAgentName(lua_State *l)
{
	int32 top = lua_gettop(l);
	lua_pushstring(l, m_RoleList.m_strAgentName.c_str());

	LuaReturn(l, top, 1);
}

int CPlayerServant::getRoleCount(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint32 cnt = m_RoleList.GetRolesNum();
	lua_pushinteger(l, (lua_Integer)cnt);

	LuaReturn(l, top, 1);
}

int CPlayerServant::getSessionIndex(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushinteger(l, (lua_Integer)m_nSessionIndex);

	LuaReturn(l, top, 1);
}

int CPlayerServant::getCurSceneServerID(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushinteger(l, (lua_Integer)m_nCurSceneServerID);

	LuaReturn(l, top, 1);
}

int CPlayerServant::addNewRole(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint64 guid = (uint64)lua_tointeger(l, 1);
	string roleName = lua_tostring(l, 2);
	uint32 career = lua_tointeger(l, 3);

	tagSingleRoleInfo newRole(guid, roleName.c_str(), 1, career, 0);
	m_RoleList.AddSingleRoleInfo(newRole);

	LuaReturn(l, top, 0);
}

int CPlayerServant::setRoleListInfo(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	if (lua_istable(l, 1))
	{
		int32 roleCount = lua_objlen(l, 1);

		for (int i = 1; i <= roleCount; ++i)
		{
			tagSingleRoleInfo info;
			lua_rawgeti(l, 1, i);
			if (!lua_istable(l, -1))
			{
				lua_pop(l, 1);
				continue;
			}

			int32 fieldCount = lua_objlen(l, -1);

			lua_rawgeti(l, -1, 1);
			info.m_nPlayerID = (uint64)lua_tointeger(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 2);
			info.m_strPlayerName = lua_tostring(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 3);
			info.m_nLevel = lua_tointeger(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 4);
			info.m_nCareer = lua_tointeger(l, -1);
			lua_pop(l, 1);
			lua_rawgeti(l, -1, 5);
			info.m_nBanExpiredTime = lua_tointeger(l, -1);
			lua_pop(l, 1);

			// 添加到链表
			m_RoleList.AddSingleRoleInfo(info);
			lua_pop(l, 1);
		}
	}

	LuaReturn(l, top, 0);
}

int CPlayerServant::onEnterSceneNotice(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint32 sceneServerID = lua_tointeger(l, 1);
	uint32 enterTime = lua_tointeger(l, 2);

	m_bHasPlayer = true;
	m_nGameState = STATUS_GAMEING;

	uint32 curSceneServerID = GetCurSceneServerID();
	if (curSceneServerID != 0)
		sLog.outError("玩家 guid="I64FMTD" 进入场景服时,原场景服索引不为0(%u)", GetCurPlayerGUID(), curSceneServerID);

	SetSceneServerID(sceneServerID, enterTime, true);
	
	LuaReturn(l, top, 0);
}

int CPlayerServant::sendServerPacket(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 0);

	SendServerPacket(pck);

	LuaReturn(l, top, 0);
}

int CPlayerServant::sendResultPacket(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 0);

	SendResultPacket(pck);

	LuaReturn(l, top, 0);
}


LUA_IMPL(CPlayerServant, CPlayerServant)

LUA_METD_START(CPlayerServant)
L_METHOD(CPlayerServant, getAccountName)
L_METHOD(CPlayerServant, getAgentName)
L_METHOD(CPlayerServant, getRoleCount)
L_METHOD(CPlayerServant, getSessionIndex)
L_METHOD(CPlayerServant, getCurSceneServerID)
L_METHOD(CPlayerServant, addNewRole)
L_METHOD(CPlayerServant, setRoleListInfo)
L_METHOD(CPlayerServant, onEnterSceneNotice)
L_METHOD(CPlayerServant, sendServerPacket)
L_METHOD(CPlayerServant, sendResultPacket)
LUA_METD_END

LUA_FUNC_START(CPlayerServant)
LUA_FUNC_END

