--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 通用的gm命令分发器

module("gamecomm.gm_cmd_dispatcher", package.seeall)
local _G = _G

if _G.sGmCmdDispatcher == nil then
	_G.sGmCmdDispatcher = {}
end

local gmCmdDispatcher = _G.sGmCmdDispatcher

-- 注册处理gm命令的函数
gmCmdDispatcher.regGMCommand = function(cmd, func, transToScene)

	-- LuaLog.outDebug(string.format("gmCmdDispatcher.regGMCommand called, cmd=%s, type(cmd)=%s", cmd, type(cmd)))
	if type(cmd) ~= "string" then
		return 
	end

	if gmCmdDispatcher.functionList == nil then
		gmCmdDispatcher.functionList = {}
	end

	if gmCmdDispatcher.functionList[cmd] == nil then
		gmCmdDispatcher.functionList[cmd] = func
		LuaLog.outDebug(string.format("[GM]注册gm命令=%s", cmd))
	end

	if transToScene then
		if gmCmdDispatcher.transToSceneCmds == nil then
			gmCmdDispatcher.transToSceneCmds = {}
		end
		gmCmdDispatcher.transToSceneCmds[cmd] = true
	end
end

gmCmdDispatcher.needTransToScene = function(cmd)
	local ret = false
	if gmCmdDispatcher.transToSceneCmds ~= nil and gmCmdDispatcher.transToSceneCmds[cmd] ~= nil then
		ret = true
	end
	return ret
end

-- 是否拥有该gm命令处理函数
gmCmdDispatcher.haveGmCommand = function(cmd)
	local ret = false
	
	if gmCmdDispatcher.functionList ~= nil and gmCmdDispatcher.functionList[cmd] ~= nil then
		ret = true
	end
	
	-- LuaLog.outDebug(string.format("gmCmdDispatcher.haveGmCommand, cmd=%s,ret=%s", cmd, tostring(ret)))
	return ret
end

-- gm命令分发
gmCmdDispatcher.gmDispatcher = function(cmd, plr, params)
	-- LuaLog.outDebug(string.format("gmCmdDispatcher.gmDispatcher, cmd=%s", cmd))
	-- body
	if gmCmdDispatcher.functionList[cmd] ~= nil then
		gmCmdDispatcher.functionList[cmd](plr, params)
	end
end
--endregion
