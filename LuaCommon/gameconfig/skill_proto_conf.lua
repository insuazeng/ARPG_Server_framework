--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

--技能定义读取

module("gameconfig.skill_proto_conf", package.seeall)

local proto = require("Config/LuaConfigs/skill_system/skill_proto")

local _G = _G

if _G.gSkillProto == nil then
	_G.gSkillProto = {}
end

local skillProtos = _G.gSkillProto

local function __insertProtoTable(mainTable, subTable)
	local skillCount = 0
	local skillProtoCount = 0
	for k,v in pairs(subTable) do
		local skillID = tonumber(k)
		-- LuaLog.outDebug(string.format("key=%s,%u, val=%s", type(k), tonumber(k), type(v)))
		if v.level ~= #v.effect_params then
			LuaLog.outError(string.format("技能读取出错,技能效果个数与等级不匹配,skillID=%u,level=%u,数组大小=%u", skillID, v.level, #v.effect_params))
		end
		skillCount = skillCount + 1
		for i=1,v.level do
			local fullID = skillID * 100 + i
			local proto = 
			{
				skill_id = fullID,
				level = i,
				type = v.type,
				name = v.name,
				next_skill_id = v.next_skill_id,
				need_career = v.need_career,
				genius_ids = v.genius_ids,
				cast_range = v.cast_range,
				no_target_cast = v.no_target_cast,
				max_target = v.max_target,
				cd_time = v.cd_time,
				fill_target_type = v.fill_target_type,
				fill_target_params = v.fill_target_params,
				effect_type = v.effect_type,
				effect_params = v.effect_params[i],
				buffs = v.buffs,
				debuffs = v.debuffs,
				action_play_time = v.action_play_time,
				dmg_effect_info = v.dmg_effect_info,
				dmg_effect_count = 0,
				cast_start_event = v.cast_start_event,
				rechoose_target_before_dmg = false
			}
			if v.dmg_effect_info ~= nil then
				proto.dmg_effect_count = #v.dmg_effect_info
			end
			if v.rechoose_target_before_dmg ~= nil and v.rechoose_target_before_dmg > 0 then
				proto.rechoose_target_before_dmg = true
			end
			mainTable[fullID] = proto
		end
	end
	LuaLog.outString(string.format("共读取%u条技能配置", skillCount))
end

-- 读取
local function __loadSkillProtoConfig()
	__insertProtoTable(skillProtos, proto)
end

-- reload
local function __reloadSkillProtoConfig()
	skillProtos = {}

	proto = gTool.require_ex("Config/LuaConfigs/skill_system/skill_proto")
	__insertProtoTable(skillProtos, proto)
end

function GetSkillProto(protoID)
	return skillProtos[protoID]
end


table.insert(gInitializeFunctionTable, __loadSkillProtoConfig)

sGameReloader.regConfigReloader("skill_proto", __reloadSkillProtoConfig)
--endregion
