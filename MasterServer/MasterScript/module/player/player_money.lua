--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 与玩家金钱部分功能相关的函数

module("MasterScript.module.player.lua_player", package.seeall)

local __saveMoneyDataToDB = nil 	-- 保存金钱数据
local __syncMoneyDataToScene = nil	-- 同步金钱数据到场景服

--[[ 拥有足够多的铜币
]]
function LuaPlayer:HasEnoughCoins(needCoins)
	if needCoins == nil or needCoins <= 0 then
		return false
	end

	local ret = false
	if self.cur_coins >= needCoins then
		ret = true
	end
	
	return ret
end

--[[ 添加铜币
]]
function LuaPlayer:AddCoins(addValue, notice, moneyGetType)
	if addValue == nil or addValue <= 0 then
		return
	end
	-- body
	self.cur_coins = self.cur_coins + addValue

	-- 检查一下有没超过铜币最大上限
	if self.cur_coins > 99999999999 then
		self.cur_coins = 99999999999
	end

	-- 保存到DB
	__saveMoneyDataToDB(self)

	-- 写入流水
	if moneyGetType ~= nil then

	end

	if notice then
		self:SendMoney()
	end

	__syncMoneyDataToScene(self)
end

--[[ 扣除铜币
]]
function LuaPlayer:DeductCoins(deductValue, notice, moneyDeductType)
	if deductValue == nil or deductValue <= 0 then
		return 
	end 

	if self.cur_coins >= deductValue then
		self.cur_coins = self.cur_coins - deductValue
	else
		self.cur_coins = 0
	end

	-- 保存到DB
	__saveMoneyDataToDB(self)

	-- 写入流水
	if moneyDeductType ~= nil then
	end

	if notice then
		self:SendMoney()
	end

	__syncMoneyDataToScene(self)
end

--[[ 拥有足够的金钱(绑元或非绑元) ]]
function LuaPlayer:HasEnoughCash(needCash, onlyUnbindCash)
	local totalCash = self.cur_cash + self.cur_bind_cash
	if onlyUnbindCash then
		totalCash = self.cur_cash
	end

	local ret = false
	if totalCash >= needCash then
		ret = true
	end
	return ret
end

--[[ 扣除金钱 ]]
function LuaPlayer:DeductCash(deductValue, onlyUnbindCash, notice, moneyDeductType)
	if deductValue == nil or deductValue <= 0 then
		return 
	end

	if not onlyUnbindCash then
		-- 先扣除绑定元宝
		if self.cur_bind_cash >= deductValue then
			self.cur_bind_cash = self.cur_bind_cash - deductValue
			deductValue = 0
		else
			deductValue = deductValue - self.cur_bind_cash
			self.cur_bind_cash = 0
		end
	end

	if deductValue > 0 then
		-- 扣除非绑元宝
		if self.cur_cash >= deductValue then
			self.cur_cash = self.cur_cash - deductValue
		else
			deductValue = deductValue - self.cur_cash
			self.cur_cash = 0
			-- 不够扣除
			-- LuaLog.outError(string.format(""))
		end
	end

	-- 保存数据
	__saveMoneyDataToDB(self)

	-- 写入流水
	if moneyDeductType ~= nil then

	end

	-- 发送通知
	if notice then
		self:SendMoney()
	end

	__syncMoneyDataToScene(self)
end

--[[ 添加绑定元宝 ]]
function LuaPlayer:AddBindCash(addValue, notice, moneyGetType)
	if addValue == nil or addValue <= 0 then
		return
	end

	self.cur_bind_cash = self.cur_bind_cash + addValue
	-- 检查一下有没超过绑元最大上限
	if self.cur_bind_cash > 99999999999 then
		self.cur_bind_cash = 99999999999
	end

	-- 保存到DB
	__saveMoneyDataToDB(self)

	-- 写入流水
	if moneyGetType ~= nil then

	end

	if notice then
		self:SendMoney()
	end

	__syncMoneyDataToScene(self)
end

--[[ 添加非绑定元宝 ]]
function LuaPlayer:AddUnBindCash(addValue, notice, moneyGetType)
	if addValue == nil or addValue <= 0 then
		return
	end
	self.cur_cash = self.cur_cash + addValue

	-- 检查一下有没超过非绑元最大上限
	if self.cur_cash > 99999999999 then
		self.cur_cash = 99999999999
	end

	-- 保存到DB
	__saveMoneyDataToDB(self)

	-- 写入流水
	if moneyGetType ~= nil then

	end

	if notice then
		self:SendMoney()
	end

	__syncMoneyDataToScene(self)
end

--[[ 发送金钱更新信息
]]
function LuaPlayer:SendMoney()
	local msg1024 = 
	{
		coins = self.cur_coins,
		bindCashs = self.cur_bind_cash,
		cashs = self.cur_cash 
	}
	self:SendPacket(Opcodes.SMSG_001024, "UserProto001024", msg1024)
end

--[[ 保存金钱数据到数据库 ]]
__saveMoneyDataToDB = function(plr)
	local querySql = string.format("{player_guid:%.0f}", plr:GetGUID())
	local updateSql = string.format("{$set:{cur_coins:%.0f,cur_bind_cash:%u,cur_cash:%u}}", plr.cur_coins, plr.cur_bind_cash, plr.cur_cash)
	sMongoDB.Update("character_datas", querySql, updateSql, true)
end

--[[ 同步金钱数据到场景服 ]]
__syncMoneyDataToScene = function(plr)
	-- 将等级同步至场景服
    local t =
    {
        curCoins = plr.cur_coins,
        curBindCash = plr.cur_bind_cash,
        curCash = plr.cur_cash
    }
    plr:SyncObjectDataToScene(enumSyncObject.PLAYER_DATA_MTS, enumObjDataSyncOp.UPDATE, plr:GetGUID(), t)
end

--endregion
