--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.player.character_packet_handler", package.seeall)
local _G = _G

local __sendRoleCreateErrorResult = nil

-- 请求创建角色
local function HandleRoleCreateReq(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleRoleCreateReq call,guid=%.0f", guid))
	local servant = CPlayerMgr.findPlayerServant(guid)
    if servant == nil then
        return
    end

    if pb_msg.roleName == nil or pb_msg.career == nil then
    	return
    end 

    -- 一个账号最多可创建一个角色
    if servant:getRoleCount() > 1 then
    	__sendRoleCreateErrorResult(ErrorCodes.Err_00100802, servant)
    	return 
    end

    -- 判断角色名长度
    if string.len(pb_msg.roleName) < 2 or string.len(pb_msg.roleName) > 16 then
        __sendRoleCreateErrorResult(ErrorCodes.Err_00100804, servant)
        return
    end
    
    -- 判断是否已经存在PlayerInfo
    if sPlayerMgr.GetPlayerInfoByName(pb_msg.roleName) ~= nil then
        __sendRoleCreateErrorResult(ErrorCodes.Err_00100803, servant)
        return 
    end

    -- 判断职业是否合法
    if pb_msg.career == PLAYER_CAREER.NONE or pb_msg.career >= PLAYER_CAREER.COUNT then
        __sendRoleCreateErrorResult(ErrorCodes.Err_00100805, servant)
        return
    end

    -- 特殊字符的检测
    LuaLog.outString("等待解决")

    -- 创建角色
    local info = sPlayerMgr.CreateNewPlayer(servant:getAgentName(), servant:getAccountName(), pb_msg.roleName, pb_msg.career)
    if info == nil then
        LuaLog.outError(string.format("创建角色失败,账号名=%s", servant:getAccountName()))
        return
    else
        LuaLog.outString(string.format("[创角成功]账号:%s(agent=%s)创角成功,guid=%.0f,name=%s", servant:getAccountName(), servant:getAgentName(),
                                        info:GetGUID(), info.role_name))
    end

    -- 创角成功,通知客户端
    local msg1008 =
    {
        roleGuid = info:GetGUID(),
        roleName = info.role_name,
        level = 1,
        career = info.career
    }
    
    local pck = WorldPacket.newPacket(Opcodes.SMSG_001008)
    local ret = gLibPBC.encode("UserProto001008", msg1008, function(data, len)
        -- body
        return pck:appendProtobufData(data, len)
    end)
    servant:sendResultPacket(pck)
    WorldPacket.delPacket(pck)

end

-- 请求进入游戏
local function HandleEnterGameReq(guid, pb_msg)
	LuaLog.outDebug(string.format("HandleEnterGameReq called,guid=%q,chooseRoleGuid=%.0f", tostring(guid), pb_msg.chooseRoleGuid))
	local plrInfo = sPlayerMgr.CanChoosePlayerEnterGame(guid, pb_msg.chooseRoleGuid)

	if plrInfo == nil then
		return 
	end
	
	-- 选择角色,进入游戏
	sPlayerMgr.ChoosePlayerEnterGame(guid, plrInfo)
end

__sendRoleCreateErrorResult = function(errCode, servant)
	-- body
    local pck = WorldPacket.newPacket(Opcodes.SMSG_001008)
    pck:luaSetErrorCode(errCode)
    servant:sendResultPacket(pck)
    WorldPacket.delPacket(pck)
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001007, "UserProto001007", HandleRoleCreateReq)      -- 请求创建角色
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001009, "UserProto001009", HandleEnterGameReq)       -- 请求进行游戏登陆
end)

--endregion
