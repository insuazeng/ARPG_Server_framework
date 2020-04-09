--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.item.item_packet_handler", package.seeall)
local sItemEffectMaker = require("MasterScript.module.item.item_effect_handler")
local _G = _G

--[[ 获取背包中物品数据
]]
local function HandleGetPackItemList(guid, pb_msg)
    -- LuaLog.outDebug(string.format("HandleGetPackItemList call,packIndex=%u", pb_msg.packIndex))
	-- body
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil or plr:GetItemInterface() == nil then
        return 
    end

    local packIndex = pb_msg.packIndex
    if packIndex == nil or packIndex <= 0 or packIndex > enumItemPackType.COUNT then
    	return 
    end
    -- 发送背包内物品列表
    plr:GetItemInterface():SendPackItemDatasToClient(packIndex)
end

--[[ 物品使用
]]
local function HandleUseItem(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleUseItem call,guid=%.0f,useCnt=%u", pb_msg.itemGuid, pb_msg.useCount))
	-- body
    if pb_msg.itemGuid == nil or pb_msg.useCount == nil then
        return 
    end

    if pb_msg.useCount <= 0 then return end

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then return end

    local itemInterface = plr:GetItemInterface()
    if itemInterface == nil then return end

    local itemObj = itemInterface:GetItemByGuid(pb_msg.itemGuid, enumItemPackType.NORMAL)
    if itemObj == nil or not itemObj:CanBeUse() then return end

    local proto = itemObj:GetProto()
    if proto == nil or pb_msg.useCount > proto.max_use_count then return end
    -- 等级不符合要求无法使用
    if proto.require_level > 1 and plr:GetLevel() < proto.require_level then return end
    -- 未配置使用参数的,也无法使用
    --[[if proto.use_detail == nil then
        LuaLog.outError(string.format("物品(proto=%u)未配置使用参数,无法使用", proto.item_id))
        return 
    end]]

    local curCount = itemObj:GetItemAmount()
    if curCount < pb_msg.useCount then
        return 
    end
    local useRet, realUsedCount = sItemEffectMaker.HandleUseItemEffect(plr, proto, pb_msg.useCount)
    
    if not useRet then
        -- LuaLog.outError(string.format(""))
        plr:SendPopupNotice("该物品使用失败,可能是无法使用或未找到处理函数")
        return 
    end

    if realUsedCount > 0 then
        local newCount = curCount - realUsedCount
        if newCount <= 0 then
            -- 物品被使用完毕
            itemInterface:RemoveItemFromSpecifyPack(itemObj, itemObj:GetPackIndex(), true, true)
            plr:SendRemoveItemNotice(pb_msg.itemGuid)
        else
            -- 有些物品被设定为使用后绑定,则进行绑定
            if proto.bind_type == enumItemBindType.USE_TO_BIND then
                itemObj:SetBindFlag(true)
            end
            itemObj:SetItemAmount(newCount)
            itemObj:SaveToDB()
            plr:SendGetItemData(itemObj)
        end
    end

    local msg6006 = 
    {
        itemGuid = pb_msg.itemGuid,
        useCount = realUsedCount
    }
    plr:SendPacket(Opcodes.SMSG_006006, "UserProto006006", msg6006)

end

--[[ 物品熔炼
]]
local function HandleSmeltItem(guid, pb_msg)
	-- body
end

--[[ 物品出售
]]
local function HandleSellItem(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleSellItem call,guid=%.0f,sellcnt=%u", pb_msg.itemGuid, pb_msg.sellCount))
	-- body
    if pb_msg.itemGuid == nil or pb_msg.sellCount == nil then
        return 
    end

    if pb_msg.sellCount <= 0 then return end

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then return end

    local itemInterface = plr:GetItemInterface()
    if itemInterface == nil then return end

    local itemObj = itemInterface:GetItemByGuid(pb_msg.itemGuid, enumItemPackType.NORMAL)
    if itemObj == nil then return end

    local proto = itemObj:GetProto()
    -- 无法出售
    if proto == nil or proto.sell_flag == nil or proto.sell_flag == 0 then return end

    local singlePrice = proto.sell_price
    if singlePrice == nil or singlePrice == 0 then
        singlePrice = 1
    end

    local curCount = itemObj:GetItemAmount()
    if curCount < pb_msg.sellCount then
        return 
    end

    local newCount = curCount - pb_msg.sellCount
    local totalPrice = pb_msg.sellCount * singlePrice

    if newCount <= 0 then
        -- 物品被出售完毕
        itemInterface:RemoveItemFromSpecifyPack(itemObj, itemObj:GetPackIndex(), true, true)
        plr:SendRemoveItemNotice(pb_msg.itemGuid)
    else
        itemObj:SetItemAmount(newCount)
        itemObj:SaveToDB()
        plr:SendGetItemData(itemObj)
    end

    -- 添加金钱
    plr:AddCoins(totalPrice, true)

    local msg6008 =
    {
        itemGuid = pb_msg.itemGuid,
        sellCount = pb_msg.sellCount,
        incomeCoins = totalPrice
    }
    plr:SendPacket(Opcodes.SMSG_006008, "UserProto006008", msg6008)
end

--[[ 背包之间交换道具
]]
local function HandleSwapItem(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleSwapItem call,guid=%.0f,dstPack=%u", pb_msg.srcItemGuid, pb_msg.destPackIndex))
	-- body
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil or plr:GetItemInterface() == nil then
        return 
    end

    local packIndex = pb_msg.destPackIndex
    if packIndex == nil or packIndex <= 0 or packIndex > enumItemPackType.COUNT then
        return 
    end

    plr:GetItemInterface():SwapItem(pb_msg.srcItemGuid, packIndex)
end

--[[ 背包整理
]]
local function HandleMergePacketItems(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleMergePacketItems call,guid=%.0f,dstPack=%u", guid, pb_msg.packIndex))
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil or plr:GetItemInterface() == nil then
        return 
    end

    local packIndex = pb_msg.packIndex
    if packIndex == enumItemPackType.NORMAL or packIndex == enumItemPackType.BANK then
        plr:GetItemInterface():MergePacketItems(packIndex)
    end
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_006001, "UserProto006001", HandleGetPackItemList)       -- 请求获取背包物品列表
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_006003, "UserProto006003", HandleSwapItem)              -- 请求进行物品交换
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_006005, "UserProto006005", HandleUseItem)               -- 请求使用物品
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_006007, "UserProto006007", HandleSellItem)              -- 请求出售物品
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_006017, "UserProto006017", HandleMergePacketItems)      -- 请求整理背包物品
    -- sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001007, "UserProto001007", HandleRoleCreateReq)      -- 请求创建角色
end)

--endregion
