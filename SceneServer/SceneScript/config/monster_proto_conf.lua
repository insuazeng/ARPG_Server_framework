--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.config.monster_proto_conf", package.seeall)

local proto1001 = require("Config/LuaConfigs/monster_proto/1001")
local proto1002 = require("Config/LuaConfigs/monster_proto/1002")
local proto1003 = require("Config/LuaConfigs/monster_proto/1003")

local _G = _G

if _G.gMonsterProtos == nil then
	_G.gMonsterProtos = {}
end

local monsterProtos = _G.gMonsterProtos

local function __insertProtoTable(mainTable, subTable)
	for k,v in pairs(subTable) do
		local protoID = tonumber(k)
		local config = 
		{
			monster_name = v.monster_name,
			model_size = v.model_size,
			alert_range = v.alert_range,
			leave_combat_range = v.leave_combat_range,
			relive_interval = v.relive_interval,
			attack_interval = v.attack_interval,
			move_speed = v.move_speed,
			level = v.level,
			exp = v.exp,
			hp = v.hp,
			fight_props = {}
		}

		local fProps = config.fight_props
		for i=1, enumFightProp.COUNT do
			fProps[i] = 0
		end

		fProps[enumFightProp.BASE_ACK] = v.ack
		fProps[enumFightProp.BASE_DEF] = v.def
		fProps[enumFightProp.BASE_CRI] = v.cri	
		fProps[enumFightProp.BASE_CRI_DMG] = v.cri_dmg
		fProps[enumFightProp.BASE_CRI_DMG_PERCENTAGE] = v.cri_dmg_percentage
		fProps[enumFightProp.BASE_CRI_DMG_REDUCE] = v.cri_dmg_reduce
		fProps[enumFightProp.BASE_CRI_DMG_REDUCE_PERCENTAGE] = v.cri_dmg_reduce_percentage
		fProps[enumFightProp.BASE_TOUGH] = v.tough
		fProps[enumFightProp.BASE_HIT] = v.hit
		fProps[enumFightProp.BASE_DODGE] = v.dodge

		fProps[enumFightProp.AD_ACK_PERCENTAGE] = v.ad_ack_percentage
		fProps[enumFightProp.AD_ACK_FIXED] = v.ad_ack_fixed
		fProps[enumFightProp.AD_DMG_PERCENTAGE] = v.ad_dmg_percentage
		fProps[enumFightProp.AD_DMG_FIXED] = v.ad_dmg_fixed
		fProps[enumFightProp.AD_DEF_PERCENTAGE] = v.ad_def_percentage
		fProps[enumFightProp.AD_DEF_FIXED] = v.ad_def_fixed
		fProps[enumFightProp.AD_IGNORE_DEF_PERCENTAGE] = v.ad_ignore_def_percentage
		fProps[enumFightProp.AD_IGNORE_DEF_FIXED] = v.ad_ignore_def_fixed
		fProps[enumFightProp.AD_DMG_REDUCE_PERCENTAGE] = v.ad_dmg_reduce_percentage
		fProps[enumFightProp.AD_DMG_REDUCE_FIXED] = v.ad_dmg_reduce_fixed
		fProps[enumFightProp.AD_HIT_PERCENTAGE] = v.ad_hit_percentage
		fProps[enumFightProp.AD_DODGE_PERCENTAGE] = v.ad_dodge_percentage
		fProps[enumFightProp.AD_CRI_PERCENTAGE] = v.ad_cri_percentage
		fProps[enumFightProp.AD_TOUGH_PRECENTAGE] = v.ad_tough_percentage

		fProps[enumFightProp.SP_ZHICAN_FIXED] = v.sp_zhican_fixed
		fProps[enumFightProp.SP_BINGHUAN_FIXED] = v.sp_binghuan_fixed
		fProps[enumFightProp.SP_SLOWDOWN_PERCENTAGE] = v.sp_slowdown_percentage
		fProps[enumFightProp.SP_FAINT_FIXED] = v.sp_faint_fixed
		fProps[enumFightProp.SP_FAINT_TIME] = v.sp_faint_time
		fProps[enumFightProp.SP_WEAK_PERCENTAGE] = v.sp_weak_percentage
		fProps[enumFightProp.SP_WEAK_FIXED] = v.sp_weak_fixed
		fProps[enumFightProp.SP_POJIA_PERCENTAGE] = v.sp_pojia_percentage
		fProps[enumFightProp.SP_POJIA_FIXED] = v.sp_pojia_fixed
		fProps[enumFightProp.SP_SLIENT_FIXED] = v.sp_slient_fixed
		fProps[enumFightProp.SP_SLIENT_TIME] = v.sp_slient_time
		fProps[enumFightProp.SP_JINSHEN_FIXED] = v.sp_jinshen_fixed
		fProps[enumFightProp.SP_BINGPO_FIXED] = v.sp_bingpo_fixed
		fProps[enumFightProp.SP_JIANDING_FIXED] = v.sp_jianding_fixed
		fProps[enumFightProp.SP_JIANTI_FIXED] = v.sp_jianti_fixed
		fProps[enumFightProp.SP_WUJIN_FIXED] = v.sp_wujin_fixed

		fProps[enumFightProp.ELE_GOLD_ACK_FIXED] = v.ele_gold_ack_fixed
		fProps[enumFightProp.ELE_GOLD_ACK_PERCENTAGE] = v.ele_gold_ack_percentage
		fProps[enumFightProp.ELE_WOOD_ACK_FIXED] = v.ele_wood_ack_fixed
		fProps[enumFightProp.ELE_WOOD_ACK_PERCENTAGE] = v.ele_wood_ack_percentage
		fProps[enumFightProp.ELE_WATER_ACK_FIXED] = v.ele_water_ack_fixed
		fProps[enumFightProp.ELE_WATER_ACK_PERCENTAGE] = v.ele_water_ack_percentage
		fProps[enumFightProp.ELE_FIRE_ACK_FIXED] = v.ele_fire_ack_fixed
		fProps[enumFightProp.ELE_FIRE_ACK_PERCENTAGE] = v.ele_fire_ack_percentage
		fProps[enumFightProp.ELE_EARTH_ACK_FIXED] = v.ele_earth_ack_fixed
		fProps[enumFightProp.ELE_EARTH_ACK_PERCENTAGE] = v.ele_earth_ack_percentage

		fProps[enumFightProp.ELE_GOLD_RESIST_FIXED] = v.ele_gold_resist_fixed
		fProps[enumFightProp.ELE_GOLD_RESIST_PERCENTAGE] = v.ele_gold_resist_percentage
		fProps[enumFightProp.ELE_WOOD_RESIST_FIXED] = v.ele_wood_resist_fixed
		fProps[enumFightProp.ELE_WOOD_RESIST_PERCENTAGE] = v.ele_wood_resist_percentage
		fProps[enumFightProp.ELE_WATER_RESIST_FIXED] = v.ele_water_resist_fixed
		fProps[enumFightProp.ELE_WATER_RESIST_PERCENTAGE] = v.ele_water_resist_percentage
		fProps[enumFightProp.ELE_FIRE_RESIST_FIXED] = v.ele_fire_resist_fixed
		fProps[enumFightProp.ELE_FIRE_RESIST_PERCENTAGE] = v.ele_fire_resist_percentage
		fProps[enumFightProp.ELE_EARTH_RESIST_FIXED] = v.ele_earth_resist_fixed
		fProps[enumFightProp.ELE_EARTH_RESIST_PERCENTAGE] = v.ele_earth_resist_percentage

		for i=1, enumFightProp.COUNT do
			if fProps[i] == nil then
				LuaLog.outError(string.format("MonsterID=%u, 战斗属性索引 %u 值为nil", protoID, i))
			end
		end

		mainTable[protoID] = config
	end
end

-- 读取
local function __loadMonsterProtoConfig()

	__insertProtoTable(monsterProtos, proto1001)
	__insertProtoTable(monsterProtos, proto1002)
    __insertProtoTable(monsterProtos, proto1003)

	--[[for k,v in pairs(monsterProtos) do
		LuaLog.outDebug(string.format("type k=%s, type v=%s", type(k), type(v)))
	end

	local proto = GetMonsterProto(10010001)
	if proto ~= nil then
		print("found!")
	end]]
end

-- reload
local function __reloadMonsterProtoConfig()
	monsterProtos = {}

	proto1001 = gTool.require_ex("Config/LuaConfigs/monster_proto/1001")
	proto1002 = gTool.require_ex("Config/LuaConfigs/monster_proto/1002")
    proto1003 = gTool.require_ex("Config/LuaConfigs/monster_proto/1003")

    __insertProtoTable(monsterProtos, proto1001)
    __insertProtoTable(monsterProtos, proto1002)
    __insertProtoTable(monsterProtos, proto1003)
end

function GetMonsterProto(protoID)
	return monsterProtos[protoID]
end


table.insert(gInitializeFunctionTable, __loadMonsterProtoConfig)

sGameReloader.regConfigReloader("monster_proto", __reloadMonsterProtoConfig)
--endregion
