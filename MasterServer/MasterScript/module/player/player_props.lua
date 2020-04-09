--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.player.lua_player", package.seeall)
local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")
local mountAdvancedConf = require("MasterScript.config.advanced.mount_advanced_conf")

--[[ 在中央服计算角色战斗属性 ]]
function LuaPlayer:CalcuProperties(syncToScene)
	local plrInfo = self.plr_info
	local tmpProps = {}
	for i=1,enumFightProp.COUNT do
		tmpProps[i] = 0
	end

	-- 1、通过配置得到玩家该等级的战斗属性
	local levelProps = levelUpPropConf.GetLevelProp(plrInfo.career, plrInfo.cur_level)
	if levelProps ~= nil then
		tmpProps[enumFightProp.BASE_HP] = levelProps[1]
    	tmpProps[enumFightProp.BASE_ACK] = levelProps[2]
    	tmpProps[enumFightProp.BASE_DEF] = levelProps[3]
    	tmpProps[enumFightProp.BASE_HIT] = levelProps[4]
    	tmpProps[enumFightProp.BASE_DODGE] = levelProps[5]
    	tmpProps[enumFightProp.BASE_CRI] = levelProps[6]
    	tmpProps[enumFightProp.BASE_TOUGH] = levelProps[7]
    	tmpProps[enumFightProp.BASE_CRI_DMG] = levelProps[8]
    	tmpProps[enumFightProp.BASE_CRI_DMG_PERCENTAGE] = levelProps[9]
	end

	-- 2、叠加装备属性
	
	-- 3、叠加各进阶系统的战斗属性
	-- [[3.1坐骑进阶系统]]
	if self.mount_advanced_data ~= nil and self.mount_advanced_data.cur_level > 0 then
		local mountAdConfig = mountAdvancedConf.GetMountAdvancedConfigByLevel(self.mount_advanced_data.cur_level)
		if mountAdConfig ~= nil then
			for i=1, enumFightProp.COUNT do
				if mountAdConfig.fight_props[i] ~= nil and mountAdConfig.fight_props[i] > 0 then
					tmpProps[i] = tmpProps[i] + mountAdConfig.fight_props[i]
				end
			end
		end
		-- 临时攻击力加成
		tmpProps[enumFightProp.BASE_ACK] = tmpProps[enumFightProp.BASE_ACK] + self.mount_advanced_data.tmp_ack_value
	end

	-- 
	local modifiedValues = {}
	for i=1,enumFightProp.COUNT do
		if self.fight_props[i] ~= tmpProps[i] then
			self.fight_props[i] = tmpProps[i]
			-- 需要同步才设置
			if syncToScene then
				modifiedValues[i] = tmpProps[i]				
			end
		end
	end

	if syncToScene then
		-- 把数值有改变的属性同步至场景服
		self:SyncObjectDataToScene(enumSyncObject.FIGHT_PROPS_MTS, enumObjDataSyncOp.UPDATE, self:GetGUID(), modifiedValues)
	end
end

--[[ 发送获取经验的通知 ]]
function LuaPlayer:SendExpUpdateNotice(addExp)
	local msg1026 = 
	{
		curExp = self.cur_exp,
		expAddValue = addExp
	}

	self:SendPacket(Opcodes.SMSG_001026, "UserProto001026", msg1026)
end

--[[ 发送升级通知 ]]
function LuaPlayer:SendLevelUpNotice(oldLevel, newLevel)
	-- body
	local msg1030 = 
	{
		oldLevel = oldLevel,
		newLevel = newLevel
	}
	self:SendPacket(Opcodes.SMSG_001030, "UserProto001030", msg1030)
end

--endregion
