--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.aura.lua_aura", package.seeall)

local auraProtoConf = require("SceneScript.config.aura_proto_conf")

local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject

LuaAura = oo.Class(superClass, "LuaAura")

local __onBuffTimeOut = nil
local __onDebuffTimeOut = nil

function LuaAura:__init(casterGuid, targetGuid, proto)
	
	-- aura对象不存在guid
	superClass.__init(self, 0)

	self.cast_time = SystemUtil.getMsTime64()
	self.caster_guid = casterGuid
	self.owner_guid = targetGuid
	self.aura_proto = proto
	
end

function LuaAura:Term()
	-- aura析构,取消定时器
	if self.aura_timer_id ~= nil then
		gTimer.unRegCallback(self.aura_timer_id)
		self.aura_timer_id = nil
	end
end

function LuaAura:WriteDataToPacket(msg)

end

-- 让buff生效
function LuaAura:DoBuffEffect()

end

-- 让debuff生效
function LuaAura:DoDebuffEffect()

end

-- 手动移除（即让buff先失效，再移除）
function LuaAura:RemoveByMaunal()

end

__onBuffTimeOut = function(callbackID, curRepeatCount, auraObject)
	-- body

end

__onDebuffTimeOut = function(callbackID, curRepeatCount, auraObject)
	-- body

end



--endregion

