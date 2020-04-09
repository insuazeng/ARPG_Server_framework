--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.partner.lua_partner", package.seeall)
local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject

LuaPartner = oo.Class(superClass, "LuaPartner")

function LuaPartner:__init(guid, proto, luaPlayer, cppPartner)
	
	superClass.__init(self, guid)
	self.owner_player = luaPlayer
	self.cpp_partner = cppPartner
	self.proto_id = proto

	LuaLog.outDebug(string.format("玩家:%q 伙伴(proto=%u,guid=%q) 被创建成功", tostring(luaPlayer:GetGUID()), proto, tostring(self:GetGUID())))
end

function LuaPartner:Term()
	self.owner_player = nil
	self.cpp_partner = cppPartner
	self.proto_id = 0
	
end


--endregion
