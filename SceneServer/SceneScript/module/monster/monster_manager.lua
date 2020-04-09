--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.monster.monster_manager", package.seeall)
local MonsterModule = require("SceneScript.module.monster.lua_monster")

local _G = _G

if _G.sMonsterMgr == nil then
    _G.sMonsterMgr = {}
end

local sMonsterMgr = _G.sMonsterMgr

-- 获取lua中的player对象
function GetMonsterByGuid(guid)
    return sMonsterMgr[guid]
end

-- 移除lua中的player对象
function RemoveMonsterByGuid(guid)
	local monster = GetMonsterByGuid(guid)
	if monster ~= nil then
		monster:Term()
		sMonsterMgr[guid] = nil
	end
end

-- 创建一个player对象
function CreateMonster(guid, protoID, attackMode, moveType)
    local monster = MonsterModule.LuaMonster(guid, protoID, attackMode, moveType)
    sMonsterMgr[guid] = monster
end

--endregion
