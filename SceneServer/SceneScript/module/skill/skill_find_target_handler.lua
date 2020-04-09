--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("SceneScript.module.skill.skill_find_target_handler", package.seeall)
require("SceneScript.module.skill.skill_def")

-- 进行技能目标查找并填充的脚本
local _G = _G

if _G.SkillFindTargetHandles == nil then
	_G.SkillFindTargetHandles = {}
end

local findTargetHandles = _G.SkillFindTargetHandles

-- 目标查找系列函数
-- index=1(单个敌人)
local function __handleFindTarget_SingleEnemy(skillCastInfo, skillProto)
	local ret = {}
	if skillCastInfo.main_target_guid ~= 0 then
		table.insert(ret, main_target_guid)
	end
	return ret
end

-- index=2(以施法者为中心,前方扇形区域)
local function __handleFindTarget_SectorFromCaster(skillCastInfo, skillProto)
	local obj = skillCastInfo.caster_object:GetCppObject()
	local guidList = SkillHelp.fillObjectGuidsAroundObject(obj, skillProto.cast_range)
	local ret = {}
	
	if guidList ~= nil then
		for i=1,#guidList do
			if guidList[i] ~= 0 and guidList[i] ~= skillCastInfo.main_target_guid then
				table.insert(ret, guidList[i])
			end
		end
	end

	if skillCastInfo.main_target_guid ~= 0 then
		table.insert(ret, skillCastInfo.main_target_guid)
	end
	return ret
end

-- index=3(以施法者为中心,半径为r的圆形)
local function __handleFindTarget_RoundAroundCaster(skillCastInfo, skillProto)
	local obj = skillCastInfo.caster_object:GetCppObject()
	LuaLog.outDebug(string.format("__handleFindTarget_RoundAroundCaster called, type=%s", type(obj)))
	local guidList = SkillHelp.fillObjectGuidsAroundObject(obj, skillProto.cast_range)
	local ret = {}
	
	if guidList ~= nil then
		for i=1,#guidList do
			if guidList[i] ~= 0 and guidList[i] ~= skillCastInfo.main_target_guid then
				table.insert(ret, guidList[i])
			end
		end
	end

	if skillCastInfo.main_target_guid ~= 0 then
		table.insert(ret, skillCastInfo.main_target_guid)
	end
	return ret
end

local function __handleFindTarget_4(skillCastInfo, skillProto)
end

local function __handleFindTarget_5(skillCastInfo, skillProto)
end

local function __handleFindTarget_6(skillCastInfo, skillProto)
end

local function __handleFindTarget_7(skillCastInfo, skillProto)
end

local function __handleFindTarget_8(skillCastInfo, skillProto)
end

-- index=9(以目标坐标为中心,半径为r的圆形范围)
local function __handleFindTarget_RoundAroundTargetPos(skillCastInfo, skillProto)

end

function HandleSkillFindTarget(index, ...)
	if findTargetHandles[index] == nil then
		LuaLog.outError(string.format("无法找到type=%u的技能目标查找函数", index))
		return nil
	end

	return findTargetHandles[index](...)
end

table.insert(findTargetHandles, __handleFindTarget_SingleEnemy)
table.insert(findTargetHandles, __handleFindTarget_SectorFromCaster)
table.insert(findTargetHandles, __handleFindTarget_RoundAroundCaster)
table.insert(findTargetHandles, __handleFindTarget_4)
table.insert(findTargetHandles, __handleFindTarget_5)
table.insert(findTargetHandles, __handleFindTarget_6)
table.insert(findTargetHandles, __handleFindTarget_7)
table.insert(findTargetHandles, __handleFindTarget_8)
table.insert(findTargetHandles, __handleFindTarget_RoundAroundTargetPos)

--endregion
