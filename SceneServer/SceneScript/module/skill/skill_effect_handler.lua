--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 进行技能效果的脚本
module("SceneScript.module.skill.skill_effect_handler", package.seeall)
require("SceneScript.module.skill.skill_def")

-- 进行技能目标填充的脚本
local _G = _G

if _G.SkillEffectHandles == nil then
	_G.SkillEffectHandles = {}
end

local effectHandles = _G.SkillEffectHandles

-- 元素攻击加成百分比字段
local ELEMENT_ACK_PERCENTAGE_FIELDS = 
{
	enumFightProp.ELE_GOLD_ACK_PERCENTAGE,
	enumFightProp.ELE_WOOD_ACK_PERCENTAGE,
	enumFightProp.ELE_WATER_ACK_PERCENTAGE,
	enumFightProp.ELE_FIRE_ACK_PERCENTAGE,
	enumFightProp.ELE_EARTH_ACK_PERCENTAGE
}

-- 元素抗性固定值
local ELEMENT_RESIST_FIXED_FIELDS =
{
	enumFightProp.ELE_GOLD_RESIST_FIXED,
	enumFightProp.ELE_WOOD_RESIST_FIXED,
	enumFightProp.ELE_WATER_RESIST_FIXED,
	enumFightProp.ELE_FIRE_RESIST_FIXED,
	enumFightProp.ELE_EARTH_RESIST_FIXED
}

-- 元素抗性百分比
local ELEMENT_RESIST_PERCENTAGE_FIELDS =
{
	enumFightProp.ELE_GOLD_RESIST_PERCENTAGE,
	enumFightProp.ELE_WOOD_RESIST_PERCENTAGE,
	enumFightProp.ELE_WATER_RESIST_PERCENTAGE,
	enumFightProp.ELE_FIRE_RESIST_PERCENTAGE,
	enumFightProp.ELE_EARTH_RESIST_PERCENTAGE
}

local __calcuNormalAttackDamage = nil   -- 计算普通攻击伤害
local __calcuSkillAttackDamage = nil    -- 计算技能攻击伤害

-- 造成百分比伤害
local function __HandleSkillEffect1(skillProto, hitResult, casterObject, targetObject)
	local ackFactor = skillProto.effect_params / 100.0

	local hurtValue = 0
    if skillProto.type == enumSkillType.Normal then
        hurtValue = __calcuNormalAttackDamage(casterObject, targetObject, hitResult, ackFactor)
    else
        hurtValue = __calcuSkillAttackDamage(casterObject, targetObject, hitResult, ackFactor)
    end
	return true, hurtValue
end

-- 造成百分比伤害,同时附带固定伤害
local function __HandleSkillEffect2(skillProto, hitResult, casterObject, targetObject)
	local ackFactor = skillProto.effect_params[1] / 100
    local extraDmg = skillProto.effect_params[2]

    local hurtValue = 0
    if skillProto.type == enumSkillType.Normal then
        hurtValue = __calcuNormalAttackDamage(casterObject, targetObject, hitResult, ackFactor, extraDmg)
    else
        hurtValue = __calcuSkillAttackDamage(casterObject, targetObject, hitResult, ackFactor, extraDmg)
    end

	-- LuaLog.outDebug(string.format("技能造成固定伤害 %u 点", param))
	return true, hurtValue
end

-- 造成百分比伤害,同时附带固定伤害,同时附带元素属性伤害
local function __HandleSkillEffect3(skillProto, hitResult, casterObject, targetObject)
	
end

--[[ 计算普攻伤害
]]
__calcuNormalAttackDamage = function(casterObject, targetObject, hitResult, ackFactor, extraDmg, elementIndex, elementDmg)

    -- 伤害加成默认为0.0
    local dmgAddtion = 0
    local casterProp = casterObject.fight_props
    local targetProp = targetObject.fight_props
    -- 尝试获取boss伤害系数或pvp伤害系数加成
    if casterObject:isPlayer() then
    	if targetObject:isPlayer() then
    		dmgAddtion = casterProp[enumFightProp.SP_DMG_PVP_PERCENTAGE] / 10000 - targetProp[enumFightProp.SP_DMG_PVP_REDUCE_PERCENTAGE] / 10000
		elseif targetObject:isBossMonster() then
			dmgAddtion = casterProp[enumFightProp.SP_DMG_BOSS_PERCENTAGE] / 10000
		end
    end

    local val = ((casterProp[enumFightProp.BASE_ACK] + casterProp[enumFightProp.AD_ACK_FIXED] - casterProp[enumFightProp.SP_WEAK_FIXED]) *
                (ackFactor + dmgAddtion + (casterProp[enumFightProp.AD_ACK_PERCENTAGE] / 10000 - casterProp[enumFightProp.SP_WEAK_PERCENTAGE] / 10000)) - 
                (targetProp[enumFightProp.BASE_DEF] + targetProp[enumFightProp.AD_DEF_FIXED] - casterProp[enumFightProp.SP_POJIA_FIXED]) *
                (1.0 + (targetProp[enumFightProp.AD_DEF_PERCENTAGE] / 10000 - casterProp[enumFightProp.SP_POJIA_PERCENTAGE] / 10000)) +
                casterProp[enumFightProp.AD_IGNORE_DEF_FIXED] * (1.0 + casterProp[enumFightProp.AD_IGNORE_DEF_PERCENTAGE] / 10000)) *
                (1.0 + (casterProp[enumFightProp.AD_DMG_PERCENTAGE] / 10000 - targetProp[enumFightProp.AD_DMG_REDUCE_PERCENTAGE] / 10000)) +
                (casterProp[enumFightProp.AD_DMG_FIXED] - targetProp[enumFightProp.AD_DMG_REDUCE_FIXED])

    -- 普攻本身有额外增加伤害
    if extraDmg ~= nil then
    	val = val + extraDmg
    end
    
    -- 如果有属性伤害,则加上属性伤害值(伤害值由技能本身提供(技能附带了伤害值才意味着存在该种属性伤害),防御加成值、百分比，属性加成百分比由角色本身自带)
    if elementIndex ~= nil and elementDmg ~= nil then
    	local ackPercentField = ELEMENT_ACK_PERCENTAGE_FIELDS[elementIndex]
    	local resistFixeField = ELEMENT_RESIST_FIXED_FIELDS[elementIndex]
    	local resistPercentField = ELEMENT_RESIST_PERCENTAGE_FIELDS[elementIndex]
    	if ackPercentField ~= nil then
    		val = val + (elementDmg * (1.0 + casterProp[ackPercentField] / 10000) - 
    			targetProp[resistFixeField] * (1.0 + targetProp[resistPercentField] / 10000))
    	end
    end

    -- 如果是暴击,则需要加上暴击伤害
    if hitResult == enumSkillHitResult.CRITICAL then
    	val = val + casterProp[enumFightProp.BASE_CRI_DMG] * (1.0 + casterProp[enumFightProp.BASE_CRI_DMG_PERCENTAGE] / 10000) - 
                targetProp[enumFightProp.BASE_CRI_DMG_REDUCE] * (1.0 + targetProp[enumFightProp.BASE_CRI_DMG_REDUCE_PERCENTAGE] / 10000)
    end

    -- 最小伤害值(人物攻击值与当前计算结果值,哪个大取哪个)
    local ackValue = casterProp[enumFightProp.BASE_ACK] * (1.0 + casterProp[enumFightProp.AD_ACK_PERCENTAGE] / 10000)
    if ackValue > val then
        val = ackValue
    end

    return val
end

--[[ 计算技能攻击伤害
]]
__calcuSkillAttackDamage = function(casterObject, targetObject, hitResult, ackFactor, extraDmg, elementIndex, elementDmg)

    -- 伤害加成默认为0.0
    local dmgAddtion = 0
    local casterProp = casterObject.fight_props
    local targetProp = targetObject.fight_props
    -- 尝试获取boss伤害系数或pvp伤害系数加成
    if casterObject:isPlayer() then
        if targetObject:isPlayer() then
            dmgAddtion = casterProp[enumFightProp.SP_DMG_PVP_PERCENTAGE] / 10000 - targetProp[enumFightProp.SP_DMG_PVP_REDUCE_PERCENTAGE] / 10000
        elseif targetObject:isBossMonster() then
            dmgAddtion = casterProp[enumFightProp.SP_DMG_BOSS_PERCENTAGE] / 10000
        end
    end

    local val = ((casterProp[enumFightProp.BASE_ACK] + casterProp[enumFightProp.AD_ACK_FIXED] - casterProp[enumFightProp.SP_WEAK_FIXED]) *
                (ackFactor + dmgAddtion + (casterProp[enumFightProp.AD_ACK_PERCENTAGE] / 10000 - casterProp[enumFightProp.SP_WEAK_PERCENTAGE] / 10000)) - 
                (targetProp[enumFightProp.BASE_DEF] + targetProp[enumFightProp.AD_DEF_FIXED] - casterProp[enumFightProp.SP_POJIA_FIXED]) *
                (1.0 + (targetProp[enumFightProp.AD_DEF_PERCENTAGE] / 10000 - casterProp[enumFightProp.SP_POJIA_PERCENTAGE] / 10000)) +
                casterProp[enumFightProp.AD_IGNORE_DEF_FIXED] * (1.0 + casterProp[enumFightProp.AD_IGNORE_DEF_PERCENTAGE] / 10000)) *
                (1.0 + (casterProp[enumFightProp.AD_DMG_PERCENTAGE] / 10000 - targetProp[enumFightProp.AD_DMG_REDUCE_PERCENTAGE] / 10000)) +
                (casterProp[enumFightProp.AD_DMG_FIXED] - targetProp[enumFightProp.AD_DMG_REDUCE_FIXED])
    
    -- 技能本身有额外增加伤害
    if extraDmg ~= nil then
    	val = val + extraDmg
    end
    
    -- 如果有属性伤害,则加上属性伤害值(伤害值由技能本身提供(技能附带了伤害值才意味着存在该种属性伤害),防御加成值、百分比，属性加成百分比由角色本身自带)
    if elementIndex ~= nil and elementDmg ~= nil then
    	local ackPercentField = ELEMENT_ACK_PERCENTAGE_FIELDS[elementIndex]
    	local resistFixeField = ELEMENT_RESIST_FIXED_FIELDS[elementIndex]
    	local resistPercentField = ELEMENT_RESIST_PERCENTAGE_FIELDS[elementIndex]
    	if ackPercentField ~= nil then
    		val = val + (elementDmg * (1.0 + casterProp[ackPercentField] / 10000) - 
    			targetProp[resistFixeField] * (1.0 + targetProp[resistPercentField] / 10000))
    	end
    end

    -- 如果是暴击,则需要加上暴击伤害
    if hitResult == enumSkillHitResult.CRITICAL then
    	val = val + casterProp[enumFightProp.BASE_CRI_DMG] * (1.0 + casterProp[enumFightProp.BASE_CRI_DMG_PERCENTAGE] / 10000) - 
                targetProp[enumFightProp.BASE_CRI_DMG_REDUCE] * (1.0 + targetProp[enumFightProp.BASE_CRI_DMG_REDUCE_PERCENTAGE] / 10000)
    end

    -- 最小伤害值(人物攻击值与当前计算结果值,哪个大取哪个)
    local ackValue = casterProp[enumFightProp.BASE_ACK] * (1.0 + casterProp[enumFightProp.AD_ACK_PERCENTAGE] / 10000)
    if ackValue > val then
        val = ackValue
    end

    return val
end

function HandleSkillEffect(index, ...)
	if effectHandles[index] == nil then
		LuaLog.outError(string.format("无法找到type=%u的技能效果生效函数", index))
		return false
	end

	return effectHandles[index](...)
end

table.insert(effectHandles, __HandleSkillEffect1)
table.insert(effectHandles, __HandleSkillEffect2)
table.insert(effectHandles, __HandleSkillEffect3)


--endregion
