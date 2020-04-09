--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

--[[ 坐骑进阶相关功能 ]]

module("MasterScript.module.advanced.mount_packet_handler", package.seeall)
local _G = _G

local mountAdvancedConf = require("MasterScript.config.advanced.mount_advanced_conf")
local sShopSystem = require("MasterScript.module.shop.shop_system")

local __savePlayerMountData = nil   -- 保存玩家坐骑进阶数据
local __initPlayerMountData = nil   -- 初始化玩家的坐骑进阶数据
local __sendMountAdvancedLevelUpBonus = nil -- 发送坐骑进阶成功奖励

--[[ 获取当前人物坐骑进阶数据 ]]
local function HandleGetMountAdvancedData(guid, pb_msg)
	LuaLog.outDebug(string.format("HandleGetMountAdvancedData called,guid=%.0f", guid))
	-- body
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end

    if plr.mount_advanced_data == nil then
        plr.mount_advanced_data = __initPlayerMountData()
        -- 首次获取数据,保存到DB中
        __savePlayerMountData(plr)
    else
        --[[LuaLog.outDebug(string.format("%s,%s,%s,%s,%s", type(plr.mount_advanced_data.cur_level), type(plr.mount_advanced_data.cur_lucky_value), type(plr.mount_advanced_data.cur_illusion_level), 
                        type(plr.mount_advanced_data.hided_flag), type(plr.mount_advanced_data.tmp_ack_value)))]]
    end

    local mountData = plr.mount_advanced_data
    local msg7002 =
    {
        curAdvancedLevel = mountData.cur_level,
        curLuckyValue = mountData.cur_lucky_value,
        curIllusionLevel = mountData.cur_illusion_level,
        illusionHideFlag = mountData.hided_flag,
        curTempAckValue = mountData.tmp_ack_value
    }
    plr:SendPacket(Opcodes.SMSG_007002, "UserProto007002", msg7002)
end

--[[ 进行一次坐骑进阶 ]]
local function HandleDoMountAdvance(guid, pb_msg)
	LuaLog.outDebug(string.format("HandleDoMountAdvance called,guid=%.0f", guid))
	-- body
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil or plr:GetItemInterface() == nil then
        return 
    end
    local itemInterface = plr:GetItemInterface()
    if plr.mount_advanced_data == nil then
        plr.mount_advanced_data = __initPlayerMountData()
    end

    local mountData = plr.mount_advanced_data
    local curLevelConf = mountAdvancedConf.GetMountAdvancedConfigByLevel(mountData.cur_level)
    local nextLevelConf = mountAdvancedConf.GetMountAdvancedConfigByLevel(mountData.cur_level + 1)
    if curLevelConf == nil or nextLevelConf == nil then
        return 
    end

    -- 检测有无足够铜币
    if curLevelConf.need_coins ~= nil and curLevelConf.need_coins > 0 then
        if not plr:HasEnoughCoins(curLevelConf.need_coins) then
            return 
        end
    end

    -- 检测有无足够的进阶物品
    local needItemID = curLevelConf.need_items[1]
    local needItemCount = curLevelConf.need_items[2]
    local curItemCount = itemInterface:GetItemCount(needItemID, enumItemPackType.NORMAL)
    local deductItemCount = needItemCount
    local quickBuyItemCount = 0
    if curItemCount < needItemCount then
        -- 没有足够的物品扣除,但玩家可以选择使用快速购买
        if pb_msg.autoBuyItem then
            quickBuyItemCount = needItemCount - curItemCount
            if not sShopSystem.ItemQuickBuyCheck(plr, needItemID, quickBuyItemCount) then
                -- 快速购买检测失败
                return 
            else
                -- 可以进行快速购买,则只扣除当前拥有的物品数量
                deductItemCount = curItemCount
            end
        else
            return 
        end
    end

    -- 扣除进阶物品
    if deductItemCount > 0 then
        if itemInterface:RemoveItemFromNormalBackpackCheck(needItemID, deductItemCount) then
            itemInterface:RemoveItemFromNormalBackpack(needItemID, deductItemCount, true, true)
        else
            -- 无法成功扣除物品
            LuaLog.outError(string.format("[坐骑进阶]无法成功扣除玩家:%s(%.0f)的进阶物品(id=%u,%u个)", plr:GetName(), guid, needItemID, deductItemCount))
            return 
        end
    end
    -- 扣除进阶所需金钱
    if curLevelConf.need_coins ~= nil and curLevelConf.need_coins > 0 then
        plr:DeductCoins(curLevelConf.need_coins, true)
    end
    -- 进行物品自动快速购买
    if quickBuyItemCount > 0 then
        sShopSystem.ItemQuickBuy(plr, needItemID, quickBuyItemCount)
    end

    -- 开始进行进阶,进行幸运值的添加
    mountData.cur_lucky_value = mountData.cur_lucky_value + curLevelConf.lucky_val_per_time
    if mountData.cur_lucky_value >= nextLevelConf.level_up_need_lucky_values then
        -- 成功升级
        mountData.cur_lucky_value = 0
        mountData.cur_level = mountData.cur_level + 1
        mountData.tmp_ack_value = 0
        -- 发放奖励
        if curLevelConf.bonus_content ~= nil then
            __sendMountAdvancedLevelUpBonus(plr, curLevelConf.bonus_content)
        end
    else
        -- 未成功升级,添加临时攻击力
        mountData.tmp_ack_value = mountData.tmp_ack_value + curLevelConf.tmp_ack_per_time
    end

    -- 重新计算属性
    plr:CalcuProperties(true)

    -- 保存至DB
    __savePlayerMountData(plr)

    -- 返回给客户端
    local msg7004 =
    {
        curAdvancedLevel = mountData.cur_level,
        curLuckyValue = mountData.cur_lucky_value,
        curTempAckValue = mountData.tmp_ack_value
    }
    plr:SendPacket(Opcodes.SMSG_007004, "UserProto007004", msg7004)
end

--[[ 进行坐骑幻化 ]]
local function HandleMakeMountIllusion(guid, pb_msg)
	LuaLog.outDebug(string.format("HandleMakeMountIllusion called,guid=%.0f", guid))
    
    if pb_msg.wantIllusionLevel == nil or pb_msg.wantIllusionLevel <= 0 then 
        return 
    end

	-- body
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end

    if plr.mount_advanced_data == nil then
        plr.mount_advanced_data = __initPlayerMountData()
    end
    
    local mountData = plr.mount_advanced_data
    if pb_msg.wantIllusionLevel == mountData.cur_illusion_level or pb_msg.wantIllusionLevel > mountData.cur_level then
        return 
    end

    mountData.cur_illusion_level = pb_msg.wantIllusionLevel
    -- 保存至DB
    __savePlayerMountData(plr)
    -- 同步至场景服
    -- ..
    -- 通知回客户端
    local msg7006 =
    {
        curIllusionLevel = mountData.cur_illusion_level
    }
    plr:SendPacket(Opcodes.SMSG_007006, "UserProto007006", msg7006)

end

-- [[ 设置坐骑形象隐藏 ]]
local function HandleSetMountHideFlag(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleSetMountHideFlag called,guid=%.0f", guid))

    if pb_msg.hideFlag == nil then return end

    -- body
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end
    
    if plr.mount_advanced_data == nil then
        plr.mount_advanced_data = __initPlayerMountData()
    end

    local mountData = plr.mount_advanced_data
    local updateSucceed = false
    if pb_msg.hideFlag then
        if not mountData.hided_flag then
            mountData.hided_flag = true
            -- 设置形象到场景服
            updateSucceed = true
        end
    else
        if mountData.hided_flag then
            mountData.hided_flag = false
            -- 设置形象到场景服
            updateSucceed = true
        end
    end

    if updateSucceed then
        -- 返回至客户端
        local msg7008 = 
        {
            hideResult = mountData.hided_flag
        }
        plr:SendPacket(Opcodes.SMSG_007008, "UserProto007008", msg7008)
        -- 修改后的值保存进DB
        __savePlayerMountData(plr)
    end
end

__savePlayerMountData = function(plrData)
    local guid = plrData:GetGUID()
    local mountData = plrData.mount_advanced_data
    local querySql = string.format("{player_guid:%.0f}", guid)
    local hideFlag = 0
    if mountData.hided_flag then
        hideFlag = 1
    end
    -- LuaLog.outDebug(string.format("%s,%s,%s,%s,%s", type(mountData.cur_level), type(mountData.cur_lucky_value), type(mountData.cur_illusion_level), type(hideFlag), type(mountData.tmp_ack_value)))
    local updateSql = string.format("{$set:{mount_advanced_data:{cur_level:%u,cur_lucky_value:%u,cur_illusion_level:%u,hided_flag:%u,tmp_ack_value:%u}}}", mountData.cur_level, 
                                    mountData.cur_lucky_value, mountData.cur_illusion_level, hideFlag, mountData.tmp_ack_value)
    sMongoDB.Update("character_datas", querySql, updateSql, true)
end

__initPlayerMountData = function()
    local initData =
    {
        cur_level           = 1,
        cur_lucky_value     = 0,
        cur_illusion_level  = 1,
        hided_flag          = false,
        tmp_ack_value       = 0
    }
    return initData
end

__sendMountAdvancedLevelUpBonus = function(plrObj, bonusContent)
    
    if plrObj:GetItemInterface() == nil or bonusContent == nil then
        return 
    end

    local itemInterface = plrObj:GetItemInterface()
    local itemID = 0
    local itemCount = 0
    -- 如果背包能容下该些物品,则直接进行物品发放
    if itemInterface:CanCreateItemListToBackpack(bonusContent) then
        for i=1,#bonusContent do
            itemID = bonusContent[i][1]
            itemCount = bonusContent[i][2]
            if itemInterface:CreateItemToBackpackCheck(itemID, itemCount) then
                itemInterface:CreateItemToBackpack(itemID, itemCount, true)
            else
                LuaLog.outError(string.format("无法为玩家:%s(guid=%.0f)创建坐骑进阶奖励(id=%u,数量%u)", plr:GetName(), plr:GetGUID(), itemID, itemCount))
            end
        end
    else
        -- 否则通过邮件进行奖励发放
    end

end

-- 协议注册
table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_007001, "UserProto007001", HandleGetMountAdvancedData)       -- 请求获取坐骑进阶数据
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_007003, "UserProto007003", HandleDoMountAdvance)             -- 请求进行坐骑进阶
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_007005, "UserProto007005", HandleMakeMountIllusion)          -- 请求设置坐骑幻化形象
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_007007, "UserProto007007", HandleSetMountHideFlag)           -- 请求设置屏蔽幻化形象
end)

--endregion
