--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.gmcommand.gm_packet_handler", package.seeall)
local _G = _G

--[[ gm命令(由中央服传递过来)
]]
local function HandleGMCommandFromMaster(msg, msg_len)
	LuaLog.outDebug(string.format("HandleGMCommandFromMaster called, type msg=%s, msgLen=%u", type(msg), msg_len))

	local var = msg:readUInt32()				-- 场景服id
	local plrGuid = msg:readUInt64()
	local cmd = msg:readString()
	local params = msg:readString()
	
	if sGmCmdDispatcher.haveGmCommand(cmd) then
		local plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
		if plr == nil then
			return 
		end
		-- 参数解析
		local paramArray = {}
		if string.len(params) > 0 then
			for val in string.gmatch(params, "%d+") do
				table.insert(paramArray, val)
			end	
		end
		-- 执行gm命令
		sGmCmdDispatcher.gmDispatcher(cmd, plr, paramArray)
	end
	
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.MASTER_2_SCENE_GM_COMMAND_LINE, HandleGMCommandFromMaster)
end)

-- gm命令具体处理函数
local function __gmReload(plr, params)
	LuaLog.outString(string.format("玩家(guid=%.0f)准备进行数据Reload", plr:GetGUID()))
	ReloadGameConfig()
end

table.insert(gInitializeFunctionTable, function()
	LuaLog.outDebug(string.format("准备注册场景服gm命令"))
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sGmCmdDispatcher.regGMCommand("reload", __gmReload)			-- 数据reload
end)

--endregion
