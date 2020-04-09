--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("SceneScript.module.skill.skill_check_target_handler", package.seeall)
require("SceneScript.module.skill.skill_def")

-- 进行技能目标填充的脚本
local _G = _G

if _G.SkillCheckTargetHandles == nil then
	_G.SkillCheckTargetHandles = {}
end

local checkTargetHandles = _G.SkillCheckTargetHandles

-- 目标检测系列函数
-- index=1(单个敌人)
local function __handleTargetCheck_SingleEnemy(skillCastInfo, skillProto)
	-- 直接把技能目标列表返回
	return skillCastInfo.target_guid_list
end

-- index=2(以施法者为中心,前方扇形区域)
local function __handleTargetCheck_SectorFromCaster(skillCastInfo, skillProto)
	local casterObject = skillCastInfo.caster_object
	local castDirection = skillCastInfo.cast_direction
	local radius = skillProto.fill_target_params[1]
	local angle = skillProto.fill_target_params[2]
	local targetGuidList = skillCastInfo.target_guid_list
	-- LuaLog.outDebug(string.format("__handleTargetCheck_SectorFromCaster called,个数=%u,玩家朝向=%.2f,攻击范围%.2f,长度%.2f", #targetGuidList, castDirection, angle, radius))
	-- 过滤目标
	return SkillHelp.checkSectorTargetsValidtiy(casterObject:GetCppObject(), castDirection, angle, radius, targetGuidList)
end

-- index=3(以施法者为中心,半径为r的圆形)
local function __handleTargetCheck_RoundAroundCaster(skillCastInfo, skillProto)
	local guidList = skillCastInfo.target_guid_list
	local ret = {}
	local radius = skillProto.fill_target_params
	for i=1,#guidList do
		local dis = skillCastInfo.caster_object:CalcuDistance(guidList[i])
		if dis < radius then
			table.insert(ret, guidList[i])
		end
	end
	return ret
end

local function __handleTargetCheck_4(skillCastInfo, skillProto)
end

local function __handleTargetCheck_5(skillCastInfo, skillProto)
end

local function __handleTargetCheck_6(skillCastInfo, skillProto)
end

local function __handleTargetCheck_7(skillCastInfo, skillProto)
end

local function __handleTargetCheck_8(skillCastInfo, skillProto)
end

-- index=9(以目标坐标为中心,半径为r的圆形范围)
local function __handleTargetCheck_RoundAroundTargetPos(skillCastInfo, skillProto)
	local ret = {}
	local targetObj = nil
	local radius = skillProto.fill_target_params
	for i=1,#skillCastInfo.target_guid_list do
		targetObj = GetObjectData(skillCastInfo.target_guid_list[i])
		if targetObj ~= nil then
			local posX, posY = targetObj:GetCurPixelPos()
			local dis = gTool.CalcuDistance(posX, posY, skillCastInfo.target_posx, skillCastInfo.target_posy)
			if dis < radius then
				table.insert(ret, skillCastInfo.target_guid_list[i])
			end
		end
	end
	table.insert(ret, skillCastInfo.main_target_guid)
	return ret
end

function HandleSkillCheckTarget(index, ...)
	if checkTargetHandles[index] == nil then
		LuaLog.outError(string.format("无法找到type=%u的技能检测函数", index))
		return false
	end

	local ret = checkTargetHandles[index](...)
	return true, ret
end

table.insert(checkTargetHandles, __handleTargetCheck_SingleEnemy)
table.insert(checkTargetHandles, __handleTargetCheck_SectorFromCaster)
table.insert(checkTargetHandles, __handleTargetCheck_RoundAroundCaster)
table.insert(checkTargetHandles, __handleTargetCheck_4)
table.insert(checkTargetHandles, __handleTargetCheck_5)
table.insert(checkTargetHandles, __handleTargetCheck_6)
table.insert(checkTargetHandles, __handleTargetCheck_7)
table.insert(checkTargetHandles, __handleTargetCheck_8)
table.insert(checkTargetHandles, __handleTargetCheck_RoundAroundTargetPos)

--endregion
