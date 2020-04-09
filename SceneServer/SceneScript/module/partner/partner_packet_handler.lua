--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
--伙伴模块（放出，回收，替换，换装等）

module("SceneScript.module.partner.partner_packet_handler", package.seeall)
require("SceneScript.module.player.lua_player")

local _G = _G

local CREATE_PARTNER = 1		-- 创建伙伴
local REVOKE_PARTNER = 2		-- 回收伙伴

-- 设置伙伴出场操作
local function HandlePartnerAppearanceOp(guid, pb_msg)
	
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
    	return 
    end

    if pb_msg.opCode == CREATE_PARTNER then
    	plr:CreatePartner(pb_msg.partnerProtoID)
    elseif pb_msg.opCode == REVOKE_PARTNER then
    	plr:RemovePartner(pb_msg.partnerProtoID)
    end

end

-- 设置伙伴avatar操作
local function HandleSetPartnerAvatarOp(guid, pb_msg)
	
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
	if plr == nil then
		return 
	end

	local partner = plr:GetPartnerByGuid(pb_msg.partnerGuid)
	if partner == nil then
		return 
	end
	
   	-- 伙伴只有武器,套装,翅膀,坐骑四个属性可以进行avatar设置
    if pb_msg.propIndex < PartnerField.PARTNER_FIELD_WEAPON or pb_msg.propIndex > PartnerField.PARTNER_FIELD_MOUNT then
        return 
    end

    partner.cpp_partner:setUInt64Value(pb_msg.propIndex, pb_msg.propValue)
    LuaLog.outDebug(string.format("玩家:%s 的伙伴(proto=%u)设置avatar,index=%u,val=%u", plr.cpp_object:getPlayerUtf8Name(), partner.proto_id, pb_msg.propIndex, pb_msg.propValue))
end

table.insert(gInitializeFunctionTable, function()
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_004001, "UserProto004001", HandlePartnerAppearanceOp)             -- 请求伙伴创建/回收操作
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_004003, "UserProto004003", HandleSetPartnerAvatarOp)              -- 请求伙伴avatar的设置
end)

--endregion
