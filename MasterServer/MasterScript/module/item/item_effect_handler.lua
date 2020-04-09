--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.item.item_effect_handler", package.seeall)
local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")

-- 进行技能目标填充的脚本
local _G = _G

if _G.ItemEffectHandles == nil then
	_G.ItemEffectHandles = {}
end

local itemEffectHandles = _G.ItemEffectHandles

-- 使用血包
local function __handleUseHpPackage(plr, itemProto, useCount)
	-- LuaLog.outDebug(string.format("type usedetail=%s", type(itemProto.use_detail)))
	local addValuePerItem = itemProto.use_detail[1].val
	if addValuePerItem == nil or addValuePerItem <= 0 then
		return false, 0
	end

	-- 试图使用血包道具
	if plr.hp_pool_data == nil then
		plr.hp_pool_data = 
		{
			cur_store_value = 0,
			last_used_time = 0
		}
	end

	local poolData = plr.hp_pool_data

	-- 血池存储上限
	local storeLimit = levelUpPropConf.GetHpPoolValueStoreLimit(plr:GetLevel())
	if poolData.cur_store_value >= storeLimit then
		plr:SendPopupNotice("你的血量储存池已经达到上限")
		return true, 0
	end

	-- 检查能最多用几个血包道具
	local remainValue = storeLimit - poolData.cur_store_value
	local cnt = math.floor(remainValue / addValuePerItem)
	if (remainValue % addValuePerItem) > 0 then
		cnt = cnt + 1
	end

	local totalAddValue = 0
	if cnt > useCount then
		cnt = useCount
		totalAddValue = addValuePerItem * cnt
		poolData.cur_store_value = poolData.cur_store_value + totalAddValue
	else
		totalAddValue = remainValue
		poolData.cur_store_value = storeLimit
	end

	-- 存储至DB
	local querySql = string.format("{player_guid:%.0f}", plr:GetGUID())
	local updateSql = string.format("{$set:{hp_pool_data:{cur_store_value:%.0f,last_used_time:%u}}}", poolData.cur_store_value, poolData.last_used_time)
	-- local updateSql = string.format("{$set:{hp_pool_data.$.cur_store_value:%.0f,hp_pool_data.$.last_used_time:%u}}", poolData.cur_store_value, poolData.last_used_time)
	-- local updateSql = string.format("{$set:{hp_pool_store_value:%.0f}}", poolData.cur_store_value)
	sMongoDB.Update("character_datas", querySql, updateSql, true)

	local notice = string.format("使用成功,血池增加量%u,当前总量=%u", totalAddValue, poolData.cur_store_value)
	plr:SendPopupNotice(notice)
	return true, cnt
end

function HandleUseItemEffect(plr, itemProto, useCount)
	if itemProto.main_class == enumItemMainClass.COMSUME then
		if itemProto.sub_class == enumComsumeItemSubClass.HP_PACKAGE then
			return __handleUseHpPackage(plr, itemProto, useCount)
		end
	end
	return false, 0
end

--[[table.insert(checkTargetHandles, __handleTargetCheck_SingleEnemy)
table.insert(checkTargetHandles, __handleTargetCheck_SectorFromCaster)
table.insert(checkTargetHandles, __handleTargetCheck_RoundAroundCaster)
table.insert(checkTargetHandles, __handleTargetCheck_4)
table.insert(checkTargetHandles, __handleTargetCheck_5)
table.insert(checkTargetHandles, __handleTargetCheck_6)
table.insert(checkTargetHandles, __handleTargetCheck_7)
table.insert(checkTargetHandles, __handleTargetCheck_8)
table.insert(checkTargetHandles, __handleTargetCheck_RoundAroundTargetPos)]]

--endregion
