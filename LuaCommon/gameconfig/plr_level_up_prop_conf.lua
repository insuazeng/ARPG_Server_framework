--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gameconfig.plr_level_up_prop_conf", package.seeall)

local zsLevelUpProps = require("Config/LuaConfigs/zs_level_up_prop")
local fsLevelUpProps = require("Config/LuaConfigs/fs_level_up_prop")

if _G.gPlayerLevelUpProps == nil then
	_G.gPlayerLevelUpProps = {}
	_G.gPlayerLevelUpProps[PLAYER_CAREER.ZS] = {}
	_G.gPlayerLevelUpProps[PLAYER_CAREER.FS] = {}
end

local levelUpProp = _G.gPlayerLevelUpProps

local function __insertPropTable(mainTable, subTable)

	for k,v in pairs(subTable) do
        local lv = tonumber(k)
        local prop = 
        {
        	v.base_hp,
        	v.base_ack,
            v.base_def,
        	v.base_hit,
        	v.base_dodge,
        	v.base_cri,
        	v.base_tough,
            v.base_cri_dmg,
            v.base_cri_dmg_percentage
    	}
        prop.hit_klv = v.hit_klv
        prop.dodge_klv = v.dodge_klv
        prop.cri_klv = v.cri_klv
        prop.cri_reduce_klv = v.cri_reduce_klv
        prop.hp_pack_info = v.hp_pack_info
		-- LuaLog.outDebug(string.format("type k=%s, type v=%s", type(k), type(v)))
        mainTable[lv] = prop
	end
end

-- 初始化读取
local function __loadLevelUpPropConfig()
    -- LuaLog.outDebug(string.format("__loadLevelUpExpConfig called"))
	__insertPropTable(levelUpProp[PLAYER_CAREER.ZS], zsLevelUpProps)
	__insertPropTable(levelUpProp[PLAYER_CAREER.FS], fsLevelUpProps)
end

-- reload
local function __reloadLevelUpPropConfig()
	-- LuaLog.outDebug(string.format("__reloadLevelUpExpConfig called"))
    levelUpProp[PLAYER_CAREER.ZS] = {}
    levelUpProp[PLAYER_CAREER.FS] = {}
   	zsLevelUpProps = gTool.require_ex("Config/LuaConfigs/zs_level_up_prop")
   	fsLevelUpProps = gTool.require_ex("Config/LuaConfigs/fs_level_up_prop")

    __insertPropTable(levelUpProp[PLAYER_CAREER.ZS], zsLevelUpProps)
	__insertPropTable(levelUpProp[PLAYER_CAREER.FS], fsLevelUpProps)
end

function GetLevelProp(career, level)
	if career == PLAYER_CAREER.NONE or career >= PLAYER_CAREER.COUNT then
		return nil
	end

	return levelUpProp[career][level]
end

--[[ 获取参与战斗计算的一些属性(只跟等级配置相关) ]]
function GetHitKLV(level)
    if level > MAX_PLAYER_LEVEL then
        level = MAX_PLAYER_LEVEL
    end
    return levelUpProp[PLAYER_CAREER.ZS][level].hit_klv
end

function GetDodgeKLV(level)
    if level > MAX_PLAYER_LEVEL then
        level = MAX_PLAYER_LEVEL
    end
    return levelUpProp[PLAYER_CAREER.ZS][level].dodge_klv
end

function GetCriKLV(level)
    if level > MAX_PLAYER_LEVEL then
        level = MAX_PLAYER_LEVEL
    end
    return levelUpProp[PLAYER_CAREER.ZS][level].cri_klv
end

function GetCriReduceKLV(level)
    if level > MAX_PLAYER_LEVEL then
        level = MAX_PLAYER_LEVEL
    end
    return levelUpProp[PLAYER_CAREER.ZS][level].cri_reduce_klv
end
--[[ 以上 ]]

--[[ 获取血包的数据配置 ]]
function GetHpPoolValueStoreLimit(level)
    if level > MAX_PLAYER_LEVEL then
        level = MAX_PLAYER_LEVEL
    end

    return levelUpProp[PLAYER_CAREER.ZS][level].hp_pack_info[1]
end
--[[ 以上 ]]

table.insert(gInitializeFunctionTable, __loadLevelUpPropConfig)
sGameReloader.regConfigReloader("level_up_prop", __reloadLevelUpPropConfig)

--endregion
