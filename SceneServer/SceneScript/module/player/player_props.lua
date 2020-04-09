--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.player.lua_player", package.seeall)

local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")

-- 计算玩家属性
function LuaPlayer:CalcuProperties(needNotice)

    local fProp = self.fight_props

    -- 1、获取中央服战斗属性
    for i=1,enumFightProp.COUNT do
        fProp[i] = self.master_fight_props[i]
    end
    
    -- 2、叠加buff、debuff属性

    self:SetEntityData(UnitField.UNIT_FIELD_MAX_HP, fProp[enumFightProp.BASE_HP])
    if self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] > self.entity_datas[UnitField.UNIT_FIELD_MAX_HP] then
        self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, self.entity_datas[UnitField.UNIT_FIELD_MAX_HP])
    end

    -- 计算战斗力
    local startTime = SystemUtil.getMsTime64()
    self:CalcuFightPower(needNotice)
    local timeDiff = SystemUtil.getMsTime64() - startTime
    LuaLog.outDebug(string.format("战力计算耗时%u ms, 战力=%u", timeDiff, self.fight_power))
    
    -- 直接通知到客户端
    if needNotice then

    end
end

--[[ 计算角色战斗力
]]
function LuaPlayer:CalcuFightPower(needNotice)
    -- body
    local fProp = self.fight_props

    self.fight_power = fProp[enumFightProp.BASE_HP] / 2 + fProp[enumFightProp.BASE_ACK] * 10 + fProp[enumFightProp.BASE_DEF] * 10 + 
                        fProp[enumFightProp.BASE_CRI] * 10 + fProp[enumFightProp.BASE_CRI_DMG] * 10 + fProp[enumFightProp.BASE_CRI_DMG_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.BASE_CRI_DMG_REDUCE] * 10 + fProp[enumFightProp.BASE_CRI_DMG_REDUCE_PERCENTAGE] * 15 + fProp[enumFightProp.BASE_TOUGH] * 10 + 
                        fProp[enumFightProp.BASE_HIT] * 10 + fProp[enumFightProp.BASE_DODGE] * 10 + fProp[enumFightProp.AD_ACK_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.AD_ACK_FIXED] * 10 + fProp[enumFightProp.AD_DMG_PERCENTAGE] * 15 + fProp[enumFightProp.AD_DMG_FIXED] * 10 + 
                        fProp[enumFightProp.AD_DEF_PERCENTAGE] * 15 + fProp[enumFightProp.AD_DEF_FIXED] * 10 + fProp[enumFightProp.AD_IGNORE_DEF_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.AD_IGNORE_DEF_FIXED] * 10 + fProp[enumFightProp.AD_DMG_REDUCE_PERCENTAGE] * 15 + fProp[enumFightProp.AD_DMG_REDUCE_FIXED] * 10 + 
                        fProp[enumFightProp.AD_HIT_PERCENTAGE] * 15 + fProp[enumFightProp.AD_DODGE_PERCENTAGE] * 15 + fProp[enumFightProp.AD_CRI_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.AD_TOUGH_PRECENTAGE] * 15 + fProp[enumFightProp.SP_ZHICAN_FIXED] * 20 + fProp[enumFightProp.SP_BINGHUAN_FIXED] * 20 + 
                        fProp[enumFightProp.SP_SLOWDOWN_PERCENTAGE] * 20 + fProp[enumFightProp.SP_FAINT_FIXED] * 20 + fProp[enumFightProp.SP_WEAK_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.SP_WEAK_FIXED] * 20 + fProp[enumFightProp.SP_POJIA_PERCENTAGE] * 15 + fProp[enumFightProp.SP_POJIA_FIXED] * 20 + 
                        fProp[enumFightProp.SP_SLIENT_FIXED] * 20 + fProp[enumFightProp.SP_JINSHEN_FIXED] * 20 + fProp[enumFightProp.SP_BINGPO_FIXED] * 20 + 
                        fProp[enumFightProp.SP_JIANDING_FIXED] * 20 + fProp[enumFightProp.SP_JIANTI_FIXED] * 20 + fProp[enumFightProp.SP_WUJIN_FIXED] * 20 + 
                        fProp[enumFightProp.SP_DMG_BOSS_PERCENTAGE] * 30 + fProp[enumFightProp.SP_DMG_PVP_PERCENTAGE] * 30 + fProp[enumFightProp.SP_DMG_PVP_REDUCE_PERCENTAGE] * 30 + 
                        fProp[enumFightProp.ELE_GOLD_ACK_FIXED] * 10 + fProp[enumFightProp.ELE_GOLD_ACK_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_WOOD_ACK_FIXED] * 10 + 
                        fProp[enumFightProp.ELE_WOOD_ACK_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_WATER_ACK_FIXED] * 10 + fProp[enumFightProp.ELE_WATER_ACK_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.ELE_FIRE_ACK_FIXED] * 10 + fProp[enumFightProp.ELE_FIRE_ACK_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_EARTH_ACK_FIXED] * 10 + 
                        fProp[enumFightProp.ELE_EARTH_ACK_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_GOLD_RESIST_FIXED] * 10 + fProp[enumFightProp.ELE_GOLD_RESIST_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.ELE_WOOD_RESIST_FIXED] * 10 + fProp[enumFightProp.ELE_WOOD_RESIST_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_WATER_RESIST_FIXED] * 10 + 
                        fProp[enumFightProp.ELE_WATER_RESIST_PERCENTAGE] * 15 + fProp[enumFightProp.ELE_FIRE_RESIST_FIXED] * 10 + fProp[enumFightProp.ELE_FIRE_RESIST_PERCENTAGE] * 15 + 
                        fProp[enumFightProp.ELE_EARTH_RESIST_FIXED] * 10 + fProp[enumFightProp.ELE_EARTH_RESIST_PERCENTAGE] * 15
    -- 通知到客户端
    if needNotice then
        local msg1028 = 
        {
            fightPower = self.fight_power
        }
        self:SendPacket(Opcodes.SMSG_001028, "UserProto001028", msg1028)
    end
end

--[[ 填充角色面板基本数据
]]
function LuaPlayer:FillCharPanelInfo()
    local fProp = self.fight_props
    local msg = 
    {
        plrGuid = self:GetGUID(),
        guildGuid = 0,
        coupleGuid = 0,
        coins = self.cur_coins,
        bindCashs = self.cur_bind_cash,
        cashs = self.cur_cash,
        curExp = self.entity_datas[PlayerField.PLAYER_FIELD_CUR_EXP],
        ack = fProp[enumFightProp.BASE_ACK],
        def = fProp[enumFightProp.BASE_DEF],
        critical = fProp[enumFightProp.BASE_CRI],
        tough = fProp[enumFightProp.BASE_TOUGH],
        hit = fProp[enumFightProp.BASE_HIT],
        dodge = fProp[enumFightProp.BASE_DODGE],
        fightPower = self.fight_power,
        vipLevel = 0,
        bestPrivilegeID = 0
    }

    return msg
end

--[[ 填充角色战斗详细属性
]]
function LuaPlayer:FillFightPropDetail()
	local fProp = self.fight_props
    local msg = 
    {
        propValues = {}
    }
	for i=1,enumFightProp.COUNT do
		table.insert(msg.propValues, fProp[i])
	end
    return msg
end



--endregion
