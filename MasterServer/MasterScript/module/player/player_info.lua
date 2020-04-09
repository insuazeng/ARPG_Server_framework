--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.player.player_info", package.seeall)

local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject

PlayerInfo = oo.Class(superClass, "PlayerInfo")


function PlayerInfo:__init(guid, nickName)
	superClass.__init(self, guid)

	-- 赋值info当中其他数据
	self.acc_name = ""					-- 账号名
	self.agent_name = "" 				-- 代理名
	self.role_name = nickName 			-- 昵称

	self.career = 0 					-- 职业
	self.create_time = 0 				-- 创角时间
	self.cur_level = 1					-- 等级
	self.fight_power = 0				-- 战斗力

	self.last_online_time = 0			-- 最近一次的在线时间(如离线为上次下线时间,如在线为本次登陆时间)
	self.today_online_count = 0			-- 今日在线时长
	self.first_enter_scene_time = 0		-- 初次进入场景时间
	self.last_join_guild_time = 0		-- 最近一次进入公会的时间

	self.ban_expire_time = 0			-- 角色封禁到期时间
	self.vip_level = 0					-- vip等级

	self.entity_obj = nil				-- 实体对象(登陆时赋值,下线后置空)
end

function PlayerInfo:ToString()

end

function PlayerInfo:OnPlayerLogin(plr)
	self.entity_obj = plr
	-- 设置本次在线时间
	self.last_online_time = Park.getCurUnixTime()
end

function PlayerInfo:OnPlayerLogout()
	self.entity_obj = nil
	-- 设置上次在线时间
	self.last_online_time = Park.getCurUnixTime()
end

function PlayerInfo:IsPlayeOnline()
	if self.entity_obj ~= nil then
		return true
	end
	return false
end

--endregion
