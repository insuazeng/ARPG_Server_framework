--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

--物品基础定义配置
module("gameconfig.item_proto_conf", package.seeall)

local proto = require("Config/LuaConfigs/item/item_proto")

local _G = _G

if _G.gItemProtos == nil then
	_G.gItemProtos = {}
end

local itemProtos = _G.gItemProtos

local function __insertProtoTable(mainTable, subTable)
	for k,v in pairs(subTable) do
		-- LuaLog.outDebug(string.format("key=%s,%u, val=%s", type(k), tonumber(k), type(v)))
		mainTable[tonumber(k)] = v
	end
end

-- 读取
local function __loadItemProtoConfig()
	__insertProtoTable(itemProtos, proto)
end

-- reload
local function __reloadItemProtoConfig()
	itemProtos = {}
	proto = gTool.require_ex("Config/LuaConfigs/item/item_proto")
	__insertProtoTable(itemProtos, proto)
end

function GetItemProto(itemID)
	return itemProtos[itemID]
end


table.insert(gInitializeFunctionTable, __loadItemProtoConfig)

sGameReloader.regConfigReloader("item_proto", __reloadItemProtoConfig)

--endregion
