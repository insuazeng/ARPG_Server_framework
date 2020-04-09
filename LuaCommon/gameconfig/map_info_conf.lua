--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

--地图定义读取

module("gameconfig.map_info_conf", package.seeall)

local proto = require("Config/LuaConfigs/map/map_info")

local _G = _G

if _G.gMapInfo == nil then
	_G.gMapInfo = {}
end

local mapInfo = _G.gMapInfo

local function __insertProtoTable(mainTable, subTable)
	for k,v in pairs(subTable) do
		-- LuaLog.outDebug(string.format("key=%s,%u, val=%s", type(k), tonumber(k), type(v)))
		mainTable[tonumber(k)] = v
	end
end

-- 读取
local function __loadMapInfoConfig()
	__insertProtoTable(mapInfo, proto)
end

-- reload
local function __reloadMapInfoConfig()
	mapInfo = {}
	proto = gTool.require_ex("Config/LuaConfigs/map/map_info")
	__insertProtoTable(mapInfo, proto)
end

function GetMapInfo(mapID)
	return mapInfo[mapID]
end


table.insert(gInitializeFunctionTable, __loadMapInfoConfig)

sGameReloader.regConfigReloader("map_info", __reloadMapInfoConfig)
--endregion

