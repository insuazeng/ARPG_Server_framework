#include "stdafx.h"
#include "SceneScript.h"
#include "SceneLuaTimer.h"
#include "3rd/pbc/pbc-lua.h"
#include "3rd/luacjson/lua_cjson.h"
#include "3rd/luabson/lua_bson.h"
#include "script/lua_export/LuaLog.h"
#include "script/lua_export/LuaTimer.h"
#include "script/lua_export/SystemUtil.h"
#include "SkillHelp.h"
#include "SceneMapMgr.h"

const char* SceneScript::ms_szSceneScriptInitializeFunc = "SceneServerInitialize";
const char* SceneScript::ms_szSceneScriptTerminateFunc = "SceneServerTerminate";

SceneScript::SceneScript(void)
{

}

SceneScript::~SceneScript(void)
{

}

bool SceneScript::registLocalLibs()
{
	static luaL_Reg luax_exts[] = 
	{
		{ "cjson", luaopen_cjson },
		{ NULL, NULL }
	};

	luaopen_bson(m_pLua);
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
	LuaTimer::Init(SceneLuaTimer::GetSingletonPtr());
	Lunar<LuaTimer>::Register(m_pLua);
	Lunar<SystemUtil>::Register(m_pLua);
	Lunar<Park>::Register(m_pLua);
	Lunar<GameSession>::Register(m_pLua);
	Lunar<Player>::Register(m_pLua);
	Lunar<Monster>::Register(m_pLua);
	Lunar<Partner>::Register(m_pLua);
	Lunar<SkillHelp>::Register(m_pLua);
	Lunar<SceneMapMgr>::Register(m_pLua);

	return true;
}

bool SceneScript::callInitialize()
{
	int top = lua_gettop(m_pLua);

	bool bRet = true;
	if (m_bInitialize && isFunctionExists(SceneScript::ms_szSceneScriptInitializeFunc))
	{
		ScriptParamArray args;
		bRet = Call(SceneScript::ms_szSceneScriptInitializeFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}
	return bRet;
}

bool SceneScript::callTerminate()
{
	bool bRet = true;
	if (!m_bInitialize)
		return bRet;

	if (isFunctionExists(SceneScript::ms_szSceneScriptTerminateFunc))
	{
		ScriptParamArray args;
		bRet = Call(SceneScript::ms_szSceneScriptTerminateFunc, &args, &args);
	}
	else
	{
		bRet = false;
	}

	return bRet;
}
