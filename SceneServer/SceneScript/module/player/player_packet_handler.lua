--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 玩家自身功能相关数据包处理
module("SceneScript.module.player.player_packet_handler", package.seeall)

local _G = _G

--[[ 玩家请求复活
]]
local function HandlePlayerRelive(guid, pb_msg)
    -- 判断玩家是否死亡
    -- 是否在场景中
    -- 根据复活方式进行复活
    LuaLog.outDebug(string.format("玩家%q请求复活", tostring(guid)))

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil or plr:isDead() == false then
    	return 
    end

    local curHp = 0
    if pb_msg.reliveType == 1 then      -- 普通复活
    	curHp = plr:OnNormalRelive()
    end

    local msg3008 = 
    {
    	relivePlayerGuid = guid,
		reliveHp = curHp,
	}
	LuaLog.outDebug(string.format("玩家%q复活成功,当前hp=%u", tostring(guid), curHp))

    plr:SendPopupNotice(string.format("复活成功"))
    
    plr:SendPacketToSet(Opcodes.SMSG_003008, "UserProto003008", msg3008, true)
end

--[[ 请求设置avatar值
]]
local function HandleSetAvatarValue(guid, pb_msg)
    
    -- LuaLog.outDebug("HandleSetAvatarValue called")

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return
    end

    local newValue = pb_msg.avatarValue
    if pb_msg.avatarValue == nil then
        newValue = 0
    end

    -- 当前只有武器,套装,翅膀,坐骑四个属性可以进行avatar设置
    if pb_msg.avatarIndex < PlayerField.PLAYER_FIELD_WEAPON or pb_msg.avatarIndex > PlayerField.PLAYER_FIELD_MOUNT then
        return 
    end

    plr.cpp_object:setUInt64Value(pb_msg.avatarIndex, newValue)
    -- LuaLog.outDebug(string.format("玩家 %s 设置avatar,index=%u,val=%u", plr.cpp_object:getPlayerUtf8Name(), pb_msg.avatarIndex, newValue))

end

--[[ 请求获取角色数据
]]
local function HandleGetCharInfo(guid, pb_msg)
    -- body
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return
    end
    local msg1020 = plr:FillCharPanelInfo()
    LuaLog.outDebug(string.format("HandleGetCharInfo called, guid=%.0f, type msg=%s", guid, type(msg1020)))
    plr:SendPacket(Opcodes.SMSG_001020, "UserProto001020", msg1020)
end

--[[ 请求获取角色详细战斗属性
]]
local function HandleGetCharFightPropDetail(guid, pb_msg)
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return
    end
    LuaLog.outDebug(string.format("HandleGetCharFightPropDetail called, guid=%.0f", guid))
    local msg1022 = plr:FillFightPropDetail()
    plr:SendPacket(Opcodes.SMSG_001022, "UserProto001022", msg1022)
end

--[[ 加载场景完成
]]
local function HandleSceneLoadedFinished(guid, pb_msg)
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then return end

    local session = Park.getSessionByGuid(guid)
    if session ~= nil then
        local ret = session:onSceneLoaded()
        if ret then
            -- 第一次进入游戏界面,发送面板数据
            local msg1020 = plr:FillCharPanelInfo()
            plr:SendPacket(Opcodes.SMSG_001020, "UserProto001020", msg1020)
        else

        end
    else
        
    end
end

table.insert(gInitializeFunctionTable, function()
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001017, "UserProto001017", HandleSetAvatarValue)     -- 请求设置avatar
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001019, "UserProto001019", HandleGetCharInfo)        -- 请求获取角色面板数据
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001021, "UserProto001021", HandleGetCharFightPropDetail)  -- 请求获取战斗详细属性数据
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_002001, "UserProto002001", HandleSceneLoadedFinished)  -- 通知场景加载完成
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_003007, "UserProto003007", HandlePlayerRelive)       -- 请求复活
end)
--endregion
