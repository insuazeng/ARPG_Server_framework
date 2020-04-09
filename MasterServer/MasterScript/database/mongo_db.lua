--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.database.mongo_db", package.seeall)

require("mongo")
-- 本文件内用到的配置
local __tableNameConf = require("MasterScript.config.mongo_table_name_conf")

local _G = _G

-- 数据库实例
local __mongoInstance = nil

--[[ 初始化MongoDB
]]
function InitMongoDatabase(ip, port, username, password)
	-- body
	local addr = ip .. ":" .. tostring(port)
	local ret, err
    LuaLog.outDebug(string.format("LuaMongo 准备开始连接%s", addr))
    local option = 
    {
        auto_reconnect = true,
        rw_timeout = 0
    }
    
    __mongoInstance, err = mongo.Connection.New(option)
    if __mongoInstance == nil then
        LuaLog.outError(string.format("LuaMongo Connection.New Failed, err=%s", err))
        assert(false)
    end

    ret, err = __mongoInstance:connect(addr)
    if ret == nil or not ret then
        LuaLog.outError(string.format("LuaMongo Connect %s Failed, err=%s", addr, err))
        assert(false)
    end
    LuaLog.outDebug(string.format("LuaMongo 连接(addr=%s)建立完成, type mongoInstance=%s", addr, type(__mongoInstance)))
end

--[[ 检查连接存在
]]
function CheckConnection()
	-- body
end

--[[ 查询多条数据
]]
function Query(tableName, sql)
	-- body
	local t = __tableNameConf.GetTableName(tableName)
	local ret, err = __mongoInstance:query(t, sql)
	if ret == nil and err ~= nil then
		LuaLog.outError(string.format("MongoDB Query(table=%s,sql=%s) Failed, err=%s", tostring(t), tostring(sql), err))
	end
	
	return ret
end

--[[ 查询单条数据
]]
function FindOne(tableName, sql)
	local t = __tableNameConf.GetTableName(tableName)
	local data, err = __mongoInstance:find_one(t, sql)
	if data == nil and err ~= nil then
		LuaLog.outError(string.format("MongoDB Query(table=%s,sql=%s) Failed, err=%s", tostring(t), tostring(sql), err))
	end

	return data
end

--[[ 插入
]]
function Insert(tableName, sql)
	LuaLog.outDebug(string.format("MongoDB Insert Called,sql=%s", sql))
	-- body
	local t = __tableNameConf.GetTableName(tableName)
	local ret, err = __mongoInstance:insert(t, sql)
	if not ret and err ~= nil then
		LuaLog.outError(string.format("MongoDB Insert(table=%s,sql=%s) Failed, err=%s", tostring(t), tostring(sql), err))
	end
end

--[[ 更新
]]
function Update(tableName, querySql, updateSql, upsertFlag, multiFlag)
	-- body
	if type(updateSql) == "string" then	
		LuaLog.outDebug(string.format("MongoDB Update Called,sql=%s", updateSql))
	else
		LuaLog.outDebug(string.format("MongoDB Update Called,sql=%s", type(updateSql)))
	end
	
	local t = __tableNameConf.GetTableName(tableName)
	local upsert = false
	local multi = false
	
	if upsertFlag ~= nil then
		upsert = upsertFlag
	end
	
	if multiFlag ~= nil then
		multi = multiFlag
	end

	local ret, err = __mongoInstance:update(t, querySql, updateSql, upsert, multi)
	if not ret and err ~= nil then
		LuaLog.outError(string.format("MongoDB Update(table=%s,sql=%s) Failed, err=%s", tostring(t), tostring(sql), err))
	end
end

--[[ 删除
]]
function Delete(tableName, querySql, multi)
	LuaLog.outDebug(string.format("MongoDB Delete Called, sql=%s", querySql))
	-- body
	local t = __tableNameConf.GetTableName(tableName)
	
	local ret, err = __mongoInstance:remove(t, querySql, multi)

	if not ret and err ~= nil then
		LuaLog.outError(string.format("MongoDB Delete(table=%s,sql=%s) Failed, err=%s", tostring(t), tostring(querySql), err))
	end
end

--endregion
