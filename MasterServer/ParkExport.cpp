#include "stdafx.h"
#include "Park.h"

int Park::getMongoDBName(lua_State *l)
{
	int32 top = lua_gettop(l);
	string name = sDBConfLoader.GetMongoDBName();
	lua_pushstring(l, name.c_str());

	LuaReturn(l, top, 1);
}

int Park::getServerGlobalID(lua_State *l)
{
	int32 top = lua_gettop(l);
	lua_pushinteger(l, (lua_Integer)sPark.GetServerFullID());

	LuaReturn(l, top, 1);
}

int Park::getCurUnixTime(lua_State *l)
{
	int32 top = lua_gettop(l);
	lua_pushinteger(l, (lua_Integer)UNIXTIME);

	LuaReturn(l, top, 1);
}

int Park::getSceneServerIDOnLogin(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	uint32 mapID = lua_tointeger(l, 1);
	uint32 ret = sPark.GetSceneIDOnPlayerLogin(mapID);
	lua_pushinteger(l, (lua_Integer)ret);

	LuaReturn(l, top, 1);
}

LUA_IMPL(Park, Park)

LUA_METD_START(Park)
LUA_METD_END

LUA_FUNC_START(Park)
L_FUNCTION(Park, getMongoDBName)
L_FUNCTION(Park, getServerGlobalID)
L_FUNCTION(Park, getCurUnixTime)
L_FUNCTION(Park, getSceneServerIDOnLogin)
LUA_FUNC_END

