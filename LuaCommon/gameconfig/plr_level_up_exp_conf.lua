--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gameconfig.plr_level_up_exp_conf", package.seeall)

local levelUpNeedExps = require("Config/LuaConfigs/zs_level_up_prop")

local _G = _G

if _G.gPlayerLevelUpExps == nil then
	_G.gPlayerLevelUpExps = {}
end

local expArray = _G.gPlayerLevelUpExps

local function __insertExpTable(mainTable, subTable)
    local maxLevel = 0

	for k,v in pairs(subTable) do
        local lv = tonumber(k)
		-- LuaLog.outDebug(string.format("type k=%s, type v=%s", type(k), type(v)))
        mainTable[lv] = tonumber(v.need_exp)

        -- 最大等级要在此处获取
        if lv > maxLevel then
            maxLevel = lv
        end
	end
    
    _G.MAX_PLAYER_LEVEL = maxLevel
    LuaLog.outString(string.format("[最大等级]当前游戏最大等级被设置为:%u", maxLevel))
end

-- 初始化读取
local function __loadLevelUpExpConfig()
    -- LuaLog.outDebug(string.format("__loadLevelUpExpConfig called"))
	__insertExpTable(expArray, levelUpNeedExps)
end

-- reload
local function __reloadLevelUpExpConfig()
	-- LuaLog.outDebug(string.format("__reloadLevelUpExpConfig called"))
    expArray = {}
   	levelUpNeedExps = gTool.require_ex("Config/LuaConfigs/zs_level_up_prop")
    __insertExpTable(expArray, levelUpNeedExps)
end

function GetLevelUpNeedExp(nextLevel)
    if expArray[nextLevel] == nil then
        return nil
    end

	return expArray[nextLevel]
end

table.insert(gInitializeFunctionTable, __loadLevelUpExpConfig)
sGameReloader.regConfigReloader("level_up_exp", __reloadLevelUpExpConfig)

--endregion
