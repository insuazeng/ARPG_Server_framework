#include "stdafx.h"
#include "MasterScript.h"
#include "MasterLuaTimer.h"
#include "3rd/pbc/pbc-lua.h"
#include "3rd/luacjson/lua_cjson.h"
#include "3rd/luamongo/lua_mongo.h"
#include "3rd/luabson/lua_bson.h"
#include "script/lua_export/LuaLog.h"
#include "script/lua_export/LuaTimer.h"
#include "script/lua_export/SystemUtil.h"

const char* MasterScript::ms_szMasterScriptInitializeFunc = "MasterServerInitialize";
const char* MasterScript::ms_szMasterScriptTerminateFunc = "MasterServerTerminate";

MasterScript::MasterScript(void)
{

}


MasterScript::~MasterScript(void)
{

}

bool MasterScript::registLocalLibs()
{
	static luaL_Reg luax_exts[] = 
	{
		{ "cjson", luaopen_cjson },
		// { "mongo", luaopen_mongo },

		{ NULL, NULL }
	};

	luaopen_bson(m_pLua);
	luaopen_mongo(m_pLua);
	luaopen_protobuf_c(m_pLua);

	luaL_Reg* lib = luax_exts;
	lua_getglobal(m_pLua, "package");
	lua_getfield(m_pLua, -1, "preload");
	for (; lib->func; lib++)
	{
		lua_pushcfunction(m_pLua, lib->func);
		lua_setfield(m_pLua, -2, lib->name);
	}
	lua_pop(m_pLua, 2);

	// 注册一些脚本内要用到的接口类
	Lunar<LuaLog>::Register(m_pLua);
	Lunar<WorldPacket>::Register(m_pLua);
	LuaTimer::Init(MasterLuaTimer::GetSingletonPtr());
	Lunar<LuaTimer>::Register(m_pLua);
	Lunar<SystemUtil>::Register(m_pLua);

	Lunar<SceneMapMgr>::Register(m_pLua);
	Lunar<Park>::Register(m_pLua);
	Lunar<CPlayerMgr>::Register(m_pLua);
	Lunar<CPlayerServant>::Register(m_pLua);

	return true;
}

bool MasterScript::callInitialize()
{
	bool bRet = true;
	if (m_bInitialize && isFunctionExists(MasterScript::ms_szMasterScriptInitializeFunc))
	{
		ScriptParamArray args;
		bRet = Call(MasterScript::ms_szMasterScriptInitializeFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

bool MasterScript::callTerminate()
{
	bool bRet = true;
	if (!m_bInitialize)
		return bRet;

	if (isFunctionExists(MasterScript::ms_szMasterScriptTerminateFunc))
	{
		ScriptParamArray args;
		bRet = Call(MasterScript::ms_szMasterScriptTerminateFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}

	return bRet;
}
