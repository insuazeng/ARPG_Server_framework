--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.gmcommand.gm_packet_handler", package.seeall)
local sItemEffectMaker = require("MasterScript.module.item.item_effect_handler")
local _G = _G

local itemProtoConf = require("gameconfig.item_proto_conf")

local function HandleGmCommandMessage(guid, pb_msg)
	LuaLog.outDebug(string.format("HandleGmCommandMessage called, guid=%.0f, cmd=%s", guid, pb_msg.commandLine))
	if pb_msg.commandLine == nil or string.len(pb_msg.commandLine) == 0 then
		return 
	end

	-- 查找玩家对象
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then
        return 
    end

    -- 先读取命令名
	local cmd = nil
	local paramsLine = ""
	-- 查找第一个空格,空格前为gm的命令名
	local pos = string.find(pb_msg.commandLine, " ")
	if pos ~= nil then
		cmd = string.sub(pb_msg.commandLine, 1, pos-1)
		-- 参数列表是空格后的字符串内容
		paramsLine = string.sub(pb_msg.commandLine, pos+1, -1)
	else
		-- 不存在空格,则整行就只有命令名
		cmd = pb_msg.commandLine
	end
	
	LuaLog.outDebug(string.format("GM命令解析完成,cmd=%s,param=%s", cmd, paramsLine))

	local transToScene = false
	if sGmCmdDispatcher.haveGmCommand(cmd) then	

	    -- 判断player的gm权限
	    -- ..
	    -- ..

		-- 参数解析
		local paramArray = {}
		if string.len(paramsLine) > 0 then
			for val in string.gmatch(paramsLine, "%d+") do
				table.insert(paramArray, tonumber(val))
			end	
		end
		-- 执行gm命令
		sGmCmdDispatcher.gmDispatcher(cmd, plr, paramArray)

		if sGmCmdDispatcher.needTransToScene(cmd) then
			transToScene = true
		end
	else
		transToScene = true
	end

	if transToScene then
		-- 可能场景服会处理该gm命令,交给场景服处理
		local curSceneID = plr:GetCurSceneServerID()
		if curSceneID ~= nil and curSceneID > 0 then
			local pck = WorldPacket.newPacket(0)
			pck:writeUInt32(curSceneID)              -- 发送至对应的场景服
	    	pck:writeUInt64(plr:GetGUID()) -- 属主guid
	    	pck:writeString(cmd)
	    	pck:writeString(paramsLine)
    	    plr:SendServerPacket(pck, ServerOpcodes.MASTER_2_SCENE_GM_COMMAND_LINE)
		end
	end
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_001015, "UserProto001015", HandleGmCommandMessage)      -- 请求执行gm命令
end)

-- 用gm命令获取物品
local function __gmGetItem(plr, params)
	LuaLog.outDebug(string.format("__gmGetItem called, params1=%s", type(params[1])))
	if params == nil or params[1] == nil then
		return 
	end

	local itemID = params[1]
	local proto = itemProtoConf.GetItemProto(itemID)
	if proto == nil then
		plr:SendPopupNotice("输入的道具id不合法")
		return
	end
	
	local itemCount = proto.max_count
	if params[2] ~= nil then
		itemCount = params[2]
	end

	local itemInterface = plr:GetItemInterface()
	if itemInterface ~= nil and itemInterface:CreateItemToBackpackCheck(itemID, itemCount) then
		itemInterface:CreateItemToBackpack(itemID, itemCount, true)
		local str = string.format("成功创建物品:%s(%u)个", proto.name, itemCount)
		plr:SendPopupNotice(str)
	else
		plr:SendPopupNotice("创建物品参数检测失败,无法创建")
	end

end


-- 用gm命令使用物品
local function __gmUseItem(plr, params)
	LuaLog.outDebug(string.format("__gmUseItem called, params1=%s", type(params[1])))
	if params == nil or params[1] == nil then
		return 
	end

	local itemID = params[1]
	local proto = itemProtoConf.GetItemProto(itemID)
	if proto == nil then
		plr:SendPopupNotice("输入的道具id不合法")
		return
	end

	local itemInterface = plr:GetItemInterface()
    if itemInterface == nil then
    LuaLog.outDebug(string.format("__gmUseItem called, itemInterfac== nil"))
        return
    end

    local playerItemCount = itemInterface:GetItemCount(itemID, enumItemPackType.NORMAL)
    if params[2] ~= nil then
        if playerItemCount < params[2]  then
            itemCount = playerItemCount
        else
            itemCount = params[2]
        end
    end

    local itemObj = itemInterface:GetItemByItemID(itemID, enumItemPackType.NORMAL)
    if itemObj == nil or not itemObj:CanBeUse() then 
    LuaLog.outDebug(string.format("__gmUseItem called, itemObj-CanBeUse()"))
    return end

    local proto = itemObj:GetProto()
    if proto == nil or itemCount > proto.max_use_count then 
     LuaLog.outDebug(string.format("__gmUseItem called, itemCount > proto.max_use_count"))
    return end
    if proto.require_level > 1 and plr:GetLevel() < proto.require_level then 
    LuaLog.outDebug(string.format("__gmUseItem called, plr:GetLevel() < proto.require_level"))
    return end

    local curCount = itemObj:GetItemAmount()
    if curCount < itemCount then
    LuaLog.outDebug(string.format("__gmUseItem called, plr:GetLevel() < curCount < itemCount"))
        return 
    end
    local useRet, realUsedCount = sItemEffectMaker.HandleUseItemEffect(plr, proto, itemCount)
    
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
            plr:SendRemoveItemNotice(itemObj:GetGUID())
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
        itemGuid = itemObj:GetGUID(),
        useCount = realUsedCount
    }
    plr:SendPacket(Opcodes.SMSG_006006, "UserProto006006", msg6006)
	
   LuaLog.outDebug(string.format("__gmUseItem called, success"))
end

-- gm命令清除角色身上物品
local function __gmClearItemPack(plr, params)
	-- body
	local itemInterface = plr:GetItemInterface()
	if itemInterface == nil then
		return 
	end 
	
	if params[1] == nil then
		itemInterface:ClearAllPackItems()
		plr:SendPopupNotice("角色背包所有物品清理成功")
	elseif params[1] == enumItemPackType.NORMAL or params[1] == enumItemPackType.BANK then
		itemInterface:ClearAllPackItems(params[1])
		plr:SendPopupNotice("角色指定背包物品清理成功")
	else
		plr:SendPopupNotice("指定背包索引有误,无法删除")
	end

end

-- 重读配置
local function __gmReload(plr, params)
	LuaLog.outString(string.format("玩家:%s(guid=%.0f)准备进行数据Reload", plr:GetName(), plr:GetGUID()))
	ReloadGameConfig()
end

-- gm命令添加游戏内货币
local function __gmGetMoney(plr, params)
	if params[1] == nil or params[2] == nil or params[2] <= 0 then
		plr:SendPopupNotice("指定参数有误")
		return 
	end
	if params[2] > 10000000 then
		params[2] = 10000000
	end

	if params[1] == enumMoneyType.COIN then
		plr:AddCoins(params[2], true)
		plr:SendPopupNotice(string.format("成功增加铜币:%u", params[2]))
	elseif params[1] == enumMoneyType.BIND_CASH then
		plr:AddBindCash(params[2], true)
		plr:SendPopupNotice(string.format("成功增加绑定元宝:%u", params[2]))
	elseif params[1] == enumMoneyType.CASH then
		plr:AddUnBindCash(params[2], true)
		plr:SendPopupNotice(string.format("成功增加元宝:%u", params[2]))
	else

	end

end

table.insert(gInitializeFunctionTable, function()
	LuaLog.outDebug(string.format("准备注册中央服gm命令"))
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sGmCmdDispatcher.regGMCommand("get", __gmGetItem)					-- 获取物品
    sGmCmdDispatcher.regGMCommand("clean_pkg", __gmClearItemPack)		-- 清理背包
    sGmCmdDispatcher.regGMCommand("reload", __gmReload, true)			-- 数据reload
    sGmCmdDispatcher.regGMCommand("money", __gmGetMoney)				-- 添加游戏货币
    sGmCmdDispatcher.regGMCommand("use", __gmUseItem)					-- 使用物品
end)

--endregion
