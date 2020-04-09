--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 技能系统相关协议处理

module("MasterScript.module.skill.skill_packet_handler", package.seeall)

require("MasterScript.module.player.lua_player")
require("MasterScript.module.player.player_skill")

local sSkillProtoConf = require("gameconfig.skill_proto_conf")
local sSkillGeniusConf = require("MasterScript.config.skill_genius_conf")
local sSkillLevelUpConf = require("MasterScript.config.skill_level_up_conf")

local _G = _G

local SKILL_GENIUS_INDEX_MIN = 1
local SKILL_GENIUS_INDEX_MAX = 4

-- 获取玩家技能数据
local function HandleGetPlayerSkillData(guid, pb_msg)
	
end

-- 进行主动技能升级
local function HandleActiveSkillLevelUp(guid, pb_msg)
    
    local levelUpConf = sSkillLevelUpConf.GetSkillLvUpConfig(pb_msg.skillID, pb_msg.skillLevel)
	if levelUpConf == nil then
        return 
    end

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end

    -- 判断等级
    if plr:GetLevel() < levelUpConf.need_role_level then 
        return 
    end

    -- 判断铜币
    if levelUpConf.need_coins ~= nil and levelUpConf.need_coins > 0 then
        if not plr:HasEnoughCoins(levelUpConf.need_coins) then
            return
        end
    end
    
    -- 扣除铜币,提升技能等级
    
    
end

-- 进行主动技能天赋激活
local function HandleActiveSkillGenius(guid, pb_msg)
    
    local skillProto = sSkillProtoConf.GetSkillProto(pb_msg.skillID)
    if skillProto == nil or skillProto.genius_ids == nil then
        return
    end
    
    if pb_msg.geniusIndex == nil or pb_msg.geniusIndex < SKILL_GENIUS_INDEX_MIN or pb_msg.geniusIndex > SKILL_GENIUS_INDEX_MAX then
        return
    end

    if skillProto.genius_ids[pb_msg.geniusIndex] == nil then
        return
    end
    
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end

    -- 玩家是否拥有该技能
    if not plr:HasSkill(pb_msg.skillID) then
        return
    end

    -- 已激活也不再处理
    if plr:IsSKillGeniusActived(pb_msg.skillID, pb_msg.geniusIndex) then
        return 
    end
	
    -- 进行技能天赋激活
    
end

-- 被动技能强化(点穴)
local function HandlePassiveSkillStrength(guid, pb_msg)

end


table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_003009, "UserProto003009", HandleGetPlayerSkillData)       -- 请求获取技能数据
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_003011, "UserProto003011", HandleActiveSkillLevelUp)       -- 请求进行技能升级
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_003013, "UserProto003013", HandleActiveSkillGenius)        -- 请求激活技能天赋
end)


--endregion
