--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.config.mongo_table_name_conf", package.seeall)

local nameConfig = require("Config/LuaConfigs/mongodb_table_name")

local _G = _G

if _G.gMongoTableName == nil then
	_G.gMongoTableName = {}
end

local tableNames = _G.gMongoTableName

local function __insertNameTable(dbName, mainTable, subTable)
	for k,v in pairs(subTable) do
		-- LuaLog.outDebug(string.format("key=%s,%u, val=%s", type(k), tonumber(k), type(v)))
		local key = tostring(k)
		local val = dbName .. "." .. v.table_name
		mainTable[key] = val
		LuaLog.outDebug(string.format("key=%s,tableName=%s", key, val))
	end
end

-- 读取
local function __loadMongoTableNameConfig()
	local dbName = Park.getMongoDBName()
	__insertNameTable(dbName, tableNames, nameConfig)
end

-- reload
local function __reloadMongoTableNameConfig()
	tableNames = {}
	local dbName = Park.getMongoDBName()
	nameConfig = gTool.require_ex("Config/LuaConfigs/mongodb_table_name")
	__insertNameTable(dbName, tableNames, nameConfig)
end

function GetTableName(key)
	local ret = nil
	if gMongoTableName[key] == nil then
		local dbName = Park.getMongoDBName()
		ret = dbName .. "." .. key .. "_tmp"
	else
		ret = gMongoTableName[key]
	end
	return ret
end


table.insert(gInitializeFunctionTable, __loadMongoTableNameConfig)

--endregion
