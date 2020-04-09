--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 与玩家技能部分相关的函数

module("MasterScript.module.player.lua_player", package.seeall)

--[[ 是否拥有某个等级的某种技能
]]
function LuaPlayer:HasSkill(skillID, level)
	return false
end


--[[ 是否已激活某技能的某个天赋
]]
function LuaPlayer:IsSkillGeniusActived(skillID, geniusIndex)
	return false
end


--endregion
