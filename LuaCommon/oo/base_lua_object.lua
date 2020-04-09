--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("oo.base_lua_object", package.seeall)

local oo = require("oo.oo")

BaseLuaObject = oo.Class(nil, "BaseLuaObject")

-- 初始化
function BaseLuaObject:__init(guid)
    self.object_guid = guid
end

-- 获取guid
function BaseLuaObject:GetGUID(guid)
    return self.object_guid
end

--endregion
