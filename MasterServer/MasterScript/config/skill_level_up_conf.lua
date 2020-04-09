--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.config.skill_level_up_conf", package.seeall)

local proto = require("Config/LuaConfigs/skill_system/skill_level_up")

local _G = _G

if _G.gSkillLevelUp == nil then
	_G.gSkillLevelUp = {}
end

function GetSkillLvUpConfig(skillID, level)
	return nil
end

--endregion
