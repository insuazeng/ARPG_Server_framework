--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.item.item_interface", package.seeall)

local oo = require("oo.oo")
ItemInterface = oo.Class(nil, "ItemInterface")

local BaseItemModule = require("MasterScript.module.item.base_item")
local EquipItemModule = require("MasterScript.module.item.equipment_item")

local itemProtoConf = require("gameconfig.item_proto_conf")

function ItemInterface:__init(owner)
	-- body
	self.owner = owner
	
	self.pack_items = {}

	for i=1,enumItemPackType.COUNT do
		self.pack_items[i] = {}
	end

	LuaLog.outString(string.format("玩家:%s(guid=%.0f)的Lua背包对象被 建立", owner:GetName(), owner:GetGUID()))
end

function ItemInterface:Term()
	local itemCount = 0
	for i=1,enumItemPackType.COUNT do
		for k,v in pairs(self.pack_items[i]) do
			v:Term()
			itemCount = itemCount + 1
		end
		self.pack_items[i] = {}
	end
	
	LuaLog.outString(string.format("玩家:%s(guid=%.0f)的Lua背包(物品数=%u)对象被 析构", self.owner:GetName(), self.owner:GetGUID(), itemCount))
	
	self.owner = nil
	self.pack_items = {}
end

--[[ 申请一个物品对象
]]
function ItemInterface:__newItemObject(itemProto, itemGuid)
	-- body
	local obj = nil
	-- 创建BaseItem或EquipmentItem对象
	if itemProto.main_class == enumItemMainClass.EQUIPMENT then
		obj = EquipItemModule.EquipmentItem(itemGuid, itemProto.item_id, self.owner:GetGUID())
	else
		obj = BaseItemModule.BaseItem(itemGuid, itemProto.item_id, self.owner:GetGUID())
	end

	return obj
end

--[[ 创建一个物品对象(不在外部调用)
]]
function ItemInterface:__CreateItem(proto)
	-- body
	local itemGuid = sPlayerMgr.GenerateItemMaxGuid()
	local itemObj = self:__newItemObject(proto, itemGuid)
	if itemObj == nil then
		return nil
	end

	-- 根据物品类型,对物品的数据进行初始化

	return itemObj
end

--[[ 读取背包数据
]]
function ItemInterface:LoadItemsFromDB()
	
	-- LuaLog.outDebug(string.format("ItemInterface:LoadItemsFromDB called, plrGuid=%.0f", self.owner:GetGUID()))
	-- 从db中读取该玩家物品
	local startTime = SystemUtil.getMsTime64()
	local plrGuid = self.owner:GetGUID()
	local sql = string.format("{owner_guid:%.0f}", plrGuid)
	local itemDatas = sMongoDB.Query("player_items", sql)
	if itemDatas == nil then
		return
	end
	local itemCounts = { 0, 0, 0 }

	local proto = nil
	for ret in itemDatas:results() do
		if ret.item_guid == nil or ret.item_guid == 0 or ret.proto_id == nil then
			LuaLog.outError(string.format("玩家(guid=%.0f)物品guid(%q)或protoID(%q)不合法,无法读取物品", plrGuid, tostring(ret.item_guid), tostring(ret.proto_id)))
			return 
		end

		proto = itemProtoConf.GetItemProto(ret.proto_id)
		if proto ~= nil then
			local item = self:__newItemObject(proto, ret.item_guid)
			if item ~= nil then
				item:LoadFromDB(ret)
				self:AddItemToSpecifyPack(item, ret.pack_index)
				itemCounts[ret.pack_index] = itemCounts[ret.pack_index] + 1
			else
				LuaLog.outError(string.format("__newItemObject失败,无法为玩家(guid=%.0f)创建物品(proto=%u)", plrGuid, ret.proto_id))
			end
		else
			LuaLog.outError(string.format("物品原型无法找到,无法为玩家(guid=%.0f)创建物品(proto=%u)", plrGuid, ret.proto_id))
		end
	end
	local timeDiff = SystemUtil.getMsTime64() - startTime
	LuaLog.outString(string.format("玩家%s(guid=%.0f)上线读取背包物品共%u个,耗时%u毫秒", self.owner:GetName(), self.owner:GetGUID(), (itemCounts[1] + itemCounts[2] + itemCounts[3]), timeDiff))
end

--[[ 保存DB
]]
function ItemInterface:SaveItemsToDB()
	LuaLog.outDebug(string.format("ItemInterface:SaveItemsToDB called, plrGuid=%.0f", self.owner:GetGUID()))

	for i=1,enumItemPackType.COUNT do
		for k,v in pairs(self.pack_items[i]) do
			if v ~= nil then
				v:SaveToDB()
			end
		end
	end
end

--[[ 根据物品guid获取对应物品
]]
function ItemInterface:GetItemByGuid(itemGuid, packIndex)
	-- body
	local item = nil
	if packIndex ~= nil and packIndex > 0 and packIndex <= enumItemPackType.COUNT then
		local packItems = self.pack_items[packIndex]
		for k,v in pairs(packItems) do
			if v ~= nil and v:GetGUID() == itemGuid then
				item = v
				break
			end
		end
	else
		for i=1,enumItemPackType.COUNT do
			for k,v in pairs(self.pack_items[i]) do
				if v ~= nil and v:GetGUID() == itemGuid then
					item = v
					break
				end
			end
		end
	end 

	return item
end

--[[ 获取指定类型物品的总数量 ]]
function ItemInterface:GetItemByItemID(itemID, packIndex)
	if packIndex ~= enumItemPackType.NORMAL and packIndex ~= enumItemPackType.BANK then
		return 0
	end
	local item = nil
	if packIndex ~= nil and packIndex > 0 and packIndex <= enumItemPackType.COUNT then
		local packItems = self.pack_items[packIndex]
		for k,v in pairs(packItems) do
			if v ~= nil and v.proto_id == itemID then
				item = v
				break
			end
		end
	else
		for i=1,enumItemPackType.COUNT do
			for k,v in pairs(self.pack_items[i]) do
				if v ~= nil and v.proto_id == itemID then
					item = v
					break
				end
			end
		end
	end 

	return item
end

--[[ 获取指定类型物品的总数量 ]]
function ItemInterface:GetItemCount(itemID, packIndex)
	if packIndex ~= enumItemPackType.NORMAL and packIndex ~= enumItemPackType.BANK then
		return 0
	end
	local cnt = 0

	local packItems = self.pack_items[packIndex]
	for k,v in pairs(packItems) do
		if v.proto_id == itemID then
			cnt = cnt + v:GetItemAmount()
		end
	end
	return cnt
end

--[[ 获取能堆叠该物品的物品(可能会获取一份,渐次将物品存放)
]]
function ItemInterface:GetItemObjectCanContainItem(itemID, itemCount, packIndex)

	local proto = itemProtoConf.GetItemProto(itemID)
	if proto == nil then
		return nil, itemCount
	end

	-- 如果该物品堆叠数量只有1,直接返回创建新物品即可
	if proto.max_count <= 1 then
		return {}, itemCount
	end

	--寻找同样为绑定状态的道具
	local bindFlag = false
	if proto.bind_type == enumItemBindType.DEFAULT_BIND then
		bindFlag = true
	end

	-- 只在普通背包与仓库背包中找,其他背包不支持
	if packIndex ~= enumItemPackType.NORMAL and packIndex ~= enumItemPackType.BANK then
		return nil, itemCount
	end

	local ret = { }
	local packItems = self.pack_items[packIndex]
	for k,v in pairs(packItems) do
		if v.proto_id == itemID and v:IsBindItem() == bindFlag and v:GetItemAmount() < proto.max_count then
			local containCount = proto.max_count - v:GetItemAmount()
			if containCount >= itemCount then
				containCount = itemCount
				ret[v] = containCount
				itemCount = 0
				break
			else
				ret[v] = containCount
				itemCount = itemCount - containCount
			end
		end
	end

	-- 返回可容纳该物品的已有物品对象列表,和剩余个数
	return ret, itemCount
end

--[[ 获取能移除指定数量的物品
]]
function ItemInterface:GetItemObjectCanRemoveAmount(itemID, removeAmount, bindFirst)
	local ret = {}
	-- 只在普通背包内移除
	local packItems = self.pack_items[enumItemPackType.NORMAL]
	local unBindItemList = {}

	for k,v in pairs(packItems) do
		if v.proto_id == itemID then
			if bindFirst then
				-- 优先移除绑定道具,非绑定道具会先放入一个列表中,等到绑定物品不够扣除时,才进行扣除
				if v:IsBindItem() then
					local curCount = v:GetItemAmount()
					if curCount >= removeAmount then
						ret[v] = removeAmount
						removeAmount = 0
						break
					else
						removeAmount = removeAmount - curCount
						ret[v] = curCount
					end	
				else
					table.insert(unBindItemList, v)
				end
			else
				local curCount = v:GetItemAmount()
				if curCount >= removeAmount then
					ret[v] = removeAmount
					removeAmount = 0
					break
				else
					removeAmount = removeAmount - curCount
					ret[v] = curCount
				end
			end
		end 
	end

	-- 如果剩余数量大于0,并且非绑定道具背包中不存在道具,则认为物品不够
	if removeAmount > 0 and #unBindItemList > 0 then
		for i=1,#unBindItemList do
			local v = unBindItemList[i]
			local curCount = v:GetItemAmount()
			if curCount >= removeAmount then
				ret[v] = removeAmount
				removeAmount = 0
				break
			else
				removeAmount = removeAmount - curCount
				ret[v] = curCount
			end
		end
	end

	-- 扣除非绑物品后数量依然大于0,说明的确是不够物品移除了
	if removeAmount > 0 then
		ret = nil
	end

	return ret
end

--[[ 创建物品列表至背包的检查
参数itemList:数组[ [物品a],[物品b],[物品c] ],元素中[1]存放itemID,[2]存放item数量
]]
function ItemInterface:CanCreateItemListToBackpack(itemList)
	if itemList == nil then return end
	local proto = nil 
	local itemID = 0
	local itemCount = 0
	local needEmptySlot = 0

	for i=1,#itemList do
		itemID = itemList[i][1]
		itemCount = itemList[i][2]
		proto = itemProtoConf.GetItemProto(itemID)
		if proto == nil then
			return false
		end
		local containItems, remainCount = self:GetItemObjectCanContainItem(itemID, itemCount, enumItemPackType.NORMAL)
		if remainCount > 0 then
			local needSlot = math.floor(remainCount / proto.max_count)
			if (remainCount % proto.max_count) > 0 then
				needSlot = needSlot + 1
			end
			needEmptySlot = needEmptySlot + needSlot
		end
	end

	if needEmptySlot > 0 then
		if (table.length(self.pack_items[enumItemPackType.NORMAL]) + needEmptySlot) > enumItemPackGridLimit.NORMAL then
			return false
		end
	end
	return true
end

--[[ 创建物品至背包的检查
]]
function ItemInterface:CreateItemToBackpackCheck(itemID, amount)
	local proto = itemProtoConf.GetItemProto(itemID)
	if proto == nil then
		return false
	end

	-- body
	local containItems, remainCount = self:GetItemObjectCanContainItem(itemID, amount, enumItemPackType.NORMAL)
	if containItems == nil then
		return false
	end

	-- 此处可能会创建超过1个格子数的物品(即单格物品最大堆叠数为99,但一次性创建999个)
	if remainCount > 0 then
		local gridCount = math.floor(remainCount / proto.max_count)
		if (remainCount % proto.max_count) > 0 then
			gridCount = gridCount + 1
		end
		if table.length(self.pack_items[enumItemPackType.NORMAL]) + gridCount > enumItemPackGridLimit.NORMAL then
			return false
		end
	end
	
	return true
end

--[[ 创建物品至背包
]]
function ItemInterface:CreateItemToBackpack(itemID, amount, needNotice, itemGetType)
	local proto = itemProtoConf.GetItemProto(itemID)
	
	if proto == nil or amount == nil or amount <= 0 then
		return 
	end

	-- 查找能否与当前已有同类物品进行堆叠
	local containItems, remainCount = self:GetItemObjectCanContainItem(itemID, amount, enumItemPackType.NORMAL)
	-- 能找容纳这些数量的物品
	if containItems ~= nil then
		for k,v in pairs(containItems) do
			local oldAmount = k:GetItemAmount()
			local newAmount = oldAmount + v
			k:SetItemAmount(newAmount)
			LuaLog.outString(string.format("[物品系统]为玩家:%s(guid=%.0f)创建物品(proto=%u,数量=%u),已有物品(guid=%.0f)能容纳部分(%u->%u)", self.owner:GetName(), self.owner:GetGUID(),
										itemID, amount, k:GetGUID(), oldAmount, newAmount))
			k:SaveToDB()
			-- 通知至客户端
			if needNotice then
				self.owner:SendGetItemData(k)
			end
		end
	end

	-- 背包中已有物品已经可以容纳新增数量,则不进行新物品的创建了
	if remainCount == nil or remainCount <= 0 then
		return 
	end
	-- 需要创建的物品个数(因为存在超量创建,即单格物品最大99个,但一次性可能会创建999个)
	local loopCount = math.floor(remainCount / proto.max_count)
	if remainCount % proto.max_count > 0 then
		loopCount = loopCount + 1
	end

	for i=1,loopCount do
		if remainCount <= 0 then
			break
		end

		local itemNum = remainCount
		if remainCount > proto.max_count then
			itemNum = proto.max_count
		end
		remainCount = remainCount - itemNum

		-- 创建新物品容纳剩余数量
		local itemObj = self:__CreateItem(proto)
		if itemObj == nil then
			LuaLog.outError(string.format("[物品系统]为玩家:%s(guid=%.0f)创建物品(proto=%u,num=%u)失败,余下数量=%u", self.owner:GetName(), self.owner:GetGUID(),
											itemID, itemNum, remainCount))
			break
		end

		-- 设置数量和绑定标记
		itemObj:SetItemAmount(itemNum)
		if proto.bind_type == enumItemBindType.DEFAULT_BIND then
			itemObj:SetBindFlag(true)
		end

		-- 设置物品背包索引,并添加到背包中
		self:AddItemToSpecifyPack(itemObj, enumItemPackType.NORMAL)

		-- 通知到客户端
		if needNotice then
			self.owner:SendGetItemData(itemObj)
		end

		-- 保存到数据库中
		itemObj:SaveToDB()

		LuaLog.outString(string.format("[物品系统]成功为玩家:%s(guid=%.0f)创建一个新物品(guid=%.0f,proto=%u,数量=%u)", self.owner:GetName(), self.owner:GetGUID(),
										itemObj:GetGUID(), itemID, itemNum))

		-- 保存至物品流水
		if itemGetType ~= nil then
			
		end
	end
end

--[[ 创建新手物品
]]
function ItemInterface:CreateRookieItem()

end

--[[ 能否将物品交换至指定背包中
]]
function ItemInterface:CanSwapItemToSpecifyPack(item, dstPackIndex)
	-- body
	if dstPackIndex == nil or dstPackIndex <= 0 or dstPackIndex > enumItemPackType.COUNT then
		return -1
	end

	-- 如果物品已经处于该背包中,则不允许交换
	if item:GetPackIndex() == dstPackIndex then
		return -2
	end
	
	local itemProto = item:GetProto()
	if itemProto == nil then
		return -3
	end

	-- 装备背包的话,只有装备类道具能放入
	if dstPackIndex == enumItemPackType.EQUIPMENT then
		if itemProto.main_class ~= enumItemMainClass.EQUIPMENT then
			return -4
		end
	end

	local packItems = self.pack_items[dstPackIndex]
	
	local existItem = nil
	if dstPackIndex == enumItemPackType.EQUIPMENT then
		-- 如果是装备背包,玩家想安装必须替换那个已安装的下来
		for k,v in pairs(packItems) do
			local proto = v:GetProto()
			if proto ~= nil and proto.sub_class == itemProto.sub_class then
				existItem = v
				break
			end
		end
	else
		local limitCount = enumItemPackGridLimit.NORMAL
		if dstPackIndex == enumItemPackType.BANK then
			limitCount = enumItemPackGridLimit.BANK
		end
		-- 如果是普通背包或仓库背包,必须有空位或容得下该物品数量才能安装
		if itemProto.max_count <= 1 then
			if table.length(self.pack_items[dstPackIndex]) >= limitCount then
				return -5
			end
		else
			local containItems, remainCount = self:GetItemObjectCanContainItem(itemProto.item_id, item:GetItemAmount(), dstPackIndex)
			if containItems == nil or remainCount == nil then
				return -6
			else
				if remainCount > 0 and table.length(self.pack_items[dstPackIndex]) >= limitCount then
					return -7
				end
			end
		end
	end

	return 0, existItem
end

--[[ 添加物品至指定背包中
]]
function ItemInterface:AddItemToSpecifyPack(item, packIndex)
	-- body
	if packIndex == nil or packIndex <= 0 or packIndex > enumItemPackType.COUNT then
		return false
	end

	local pack = self.pack_items[packIndex]
	local itemGuid = item:GetGUID()

	if pack[itemGuid] ~= nil then
		LuaLog.outError(string.format("为玩家:%s(guid=%.0f)添加物品(guid=%.0f.pacnIndex=%u)时,该位置已经有值", self.owner:GetName(),
						self.owner:GetGUID(), item:GetGUID(), packIndex))
		return false
	end
	-- 设置物品的背包索引
	item:SetPackIndex(packIndex)
	pack[itemGuid] = item
	
	return true
end

--[[ 从指定背包中移除物品
]]
function ItemInterface:RemoveItemFromSpecifyPack(item, packIndex, deleteFromMem, deleteFromDB)
	if packIndex == nil or packIndex <= 0 or packIndex > enumItemPackType.COUNT then
		return false
	end
	
	local packItems = self.pack_items[packIndex]
	local itemGuid = item:GetGUID()
	if packItems[itemGuid] == nil then
		LuaLog.outError(string.format("[物品系统]找不到玩家:%s(guid=%.0f)身上要移除的物品(guid=%.0f,packIndex=%u)", self.owner:GetName(), self.owner:GetGUID(), itemGuid, packIndex))
		return 
	end

	-- 要从数据库移除
	if deleteFromDB then
		item:DeleteFromDB()
	end

	-- 从内存中移除
	if deleteFromMem then
		item:Term()
	end
	
	packItems[itemGuid] = nil
end

--[[ 移除物品的检查
]]
function ItemInterface:RemoveItemFromNormalBackpackCheck(itemID, amount)
	local ret = false
	
	local curCount = self:GetItemCount(itemID, enumItemPackType.NORMAL)
	if curCount >= amount then
		ret = true
	end

	return ret
end

--[[ 移除物品
]]
function ItemInterface:RemoveItemFromNormalBackpack(itemID, amount, bindFirst, needNotice, itemRemoveType)
	LuaLog.outString(string.format("[背包系统]玩家:%s(guid=%.0f)试图移除物品(id=%u,数量=%u)", self.owner:GetName(), self.owner:GetGUID(),
					itemID, amount))

	-- body
	local proto = itemProtoConf.GetItemProto(itemID)
	if proto == nil or amount == nil or amount <= 0 then
		return 
	end

	local removeItems = self:GetItemObjectCanRemoveAmount(itemID, amount, bindFirst)
	if removeItems == nil then
		LuaLog.outError(string.format("[背包系统]玩家:%s(guid=%.0f)物品移除失败,没有足够数量可移除", self.owner:GetName(), self.owner:GetGUID()))
		return 
	end

	for k,v in pairs(removeItems) do
		local newCount = k:GetItemAmount() - v
		local itemGuid = k:GetGUID()
		if newCount <= 0 then
			LuaLog.outString(string.format("[背包系统]物品(proto=%u,guid=%.0f)数量(%u)被扣除为0,即将从内存和DB中移除", itemID, itemGuid, k:GetItemAmount()))
			self:RemoveItemFromSpecifyPack(k, k:GetPackIndex(), true, true)
			if needNotice then
				self.owner:SendRemoveItemNotice(itemGuid)
			end
		else
			LuaLog.outString(string.format("[背包系统]物品(proto=%u,guid=%.0f)被扣除指定数量(%u->%u)", itemID, itemGuid, k:GetItemAmount(), newCount))
			k:SetItemAmount(newCount)
			k:SaveToDB()
			if needNotice then
				self.owner:SendGetItemData(k)
			end
		end
	end

	if itemRemoveType ~= nil then

	end
end

--[[ 背包内交换物品
]]
function ItemInterface:SwapItem(itemGuid, dstPackIndex)
	local itemObj = self:GetItemByGuid(itemGuid)
	if itemObj == nil then
		return 
	end
	LuaLog.outString(string.format("玩家:%s(guid=%.0f)试图交换物品,源物品id=%.0f,目标背包索引=%u", self.owner:GetName(), self.owner:GetGUID(),
					itemGuid, dstPackIndex))

	local ret, existItem = self:CanSwapItemToSpecifyPack(itemObj, dstPackIndex)

	if ret < 0 then
		-- 发送错误消息到客户端
		LuaLog.outError(string.format("玩家:%s(guid=%.0f)交换物品检查失败,ret=%d", self.owner:GetName(), self.owner:GetGUID(), ret))
		return 
	end
	local itemProto = itemObj:GetProto()
	local srcPackIndex = itemObj:GetPackIndex()

	if itemProto.max_count <= 1 then
		-- 堆叠数量为1,直接从源背包中移除,放入新背包
		self:RemoveItemFromSpecifyPack(itemObj, srcPackIndex, false, false)
		self:AddItemToSpecifyPack(itemObj, dstPackIndex)
		-- 通知客户端
		self.owner:SendGetItemData(itemObj)
		-- 保存数据库
		itemObj:SaveToDB()
	else
		-- 堆叠数量大于1,那么查找新背包中能否容纳下该物品
		local containItems, remainCount = self:GetItemObjectCanContainItem(itemProto.item_id, itemObj:GetItemAmount(), dstPackIndex)
		for k,v in pairs(containItems) do
			-- 更新物品新数量
			local newAmount = k:GetItemAmount() + v
			k:SetItemAmount(newAmount)
			-- 通知客户端
			self.owner:SendGetItemData(k)
			-- 保存数据库
			k:SaveToDB()
		end

		if remainCount > 0 then
			-- 设置残留数量
			itemObj:SetItemAmount(remainCount)
			self:RemoveItemFromSpecifyPack(itemObj, srcPackIndex, false, false)
			self:AddItemToSpecifyPack(itemObj, dstPackIndex)
			-- 通知客户端
			self.owner:SendGetItemData(itemObj)
			-- 保存数据库
			itemObj:SaveToDB()
		else
			-- 原物品要被移除
			self:RemoveItemFromSpecifyPack(itemObj, srcPackIndex, true, true)
			self.owner:SendRemoveItemNotice(itemGuid)
		end
	end

	-- 如果是交换装备,则把旧装备放回源背包
	if existItem ~= nil then
		self:RemoveItemFromSpecifyPack(existItem, dstPackIndex, false, false)
		self:AddItemToSpecifyPack(existItem, srcPackIndex)
		-- 通知客户端
		self.owner:SendGetItemData(existItem)
		existItem:SaveToDB()
	end

	-- 通知客户端
	local msg6004 =
	{
		srcItemGuid = itemGuid,
		destPackIndex = dstPackIndex
	}
	self.owner:SendPacket(Opcodes.SMSG_006004, "UserProto006004", msg6004)

	LuaLog.outString(string.format("玩家:%s(guid=%.0f)交换物品成功", self.owner:GetName(), self.owner:GetGUID()))

	-- 穿/卸装备需要重新计算角色属性
	if srcPackIndex == enumItemPackType.EQUIPMENT or dstPackIndex == enumItemPackType.EQUIPMENT then

	end
end

-- 进行背包整理(主要处理物品合并)
function ItemInterface:MergePacketItems(packIndex)
	-- body
	if packIndex ~= enumItemPackType.NORMAL and packIndex ~= enumItemPackType.BANK then
		return 
	end

	local startTime = SystemUtil.getMsTime64()
	local packItems = self.pack_items[packIndex]
	if table.length(packItems) == 0 then
		return 
	end
	LuaLog.outDebug(string.format("[背包系统]为玩家:%s(guid=%.0f)进行背包(index=%u)整理", self.owner:GetName(), self.owner:GetGUID(), packIndex))

	local protoItems = {}	--以proto为key的二维数组
	for k,v in pairs(packItems) do
		-- k=guid, v=物品实例
		local proto = v:GetProto()
		if proto ~= nil and proto.max_count > 1 and v:GetItemAmount() < proto.max_count then
			if protoItems[proto] == nil then
				protoItems[proto] = {}
			end
			table.insert(protoItems[proto], v)
		end
	end

	local changeItems = {}
	local deleteItems = {}

	for k,v in pairs(protoItems) do -- k=proto,v=item数组
		-- 数组大于1,才有堆叠的价值
		if #v > 1 then
			local proto = k
			local frontItem = nil
			-- 先找第一个未放满的物品
			for i=1, #v do
				if v[i]:GetItemAmount() < proto.max_count then
					frontItem = v[i]
					LuaLog.outDebug(string.format("[整理]找到物品(guid=%.0f,个数=%u)做frontItem", frontItem:GetGUID(), frontItem:GetItemAmount()))
					break
				end
			end

			local arrayLen = #v
			for i=1, arrayLen do
				local currentItem = v[i]
				local currentItemAmount = currentItem:GetItemAmount()
				if currentItemAmount < proto.max_count and currentItem:GetGUID() ~= frontItem:GetGUID() then
					local remainNum = proto.max_count - frontItem:GetItemAmount()
					if remainNum <= 0 then
						-- 寻找下一个frontItem
					elseif remainNum >= currentItemAmount then
						-- 可以包含全部数量
						local newCount = frontItem:GetItemAmount() + currentItemAmount
						frontItem:SetItemAmount(newCount)
						changeItems[frontItem:GetGUID()] = frontItem
						currentItem:SetItemAmount(0)
						deleteItems[currentItem:GetGUID()] = currentItem
						v[i] = nil
					else
						-- 只能包含部分数量
						frontItem:SetItemAmount(proto.max_count)
						changeItems[frontItem:GetGUID()] = frontItem
						local newCount = currentItemAmount - remainNum
						currentItem:SetItemAmount(newCount)
						changeItems[currentItem:GetGUID()] = currentItem
					end
					
					-- 寻找新的frontItem
					if frontItem:GetItemAmount() >= proto.max_count then
						LuaLog.outDebug(string.format("[整理]FrontItem(guid=%.0f),个数已满,准备找下一个", frontItem:GetGUID()))
						frontItem = nil
						for j=1, arrayLen do
							if v[j] ~= nil and v[j]:GetItemAmount() < proto.max_count then
								frontItem = v[j]
								LuaLog.outDebug(string.format("[整理]找到新物品(guid=%.0f,个数=%u)做frontItem", frontItem:GetGUID(), frontItem:GetItemAmount()))
								break
							end
						end
					end

					-- 如果已经找不到到fronItem来包含后面的物品,则不再进行
					if frontItem == nil then
						LuaLog.outDebug(string.format("[整理]找不到物品做frontItem,准备结束该物品的合并"))
						break
					end
				end
			end
		end
	end
	
	-- 哪怕没有物品被合并或删除,客户端也需要一条协议来触发道具重排
	--[[local changeItemCount = table.length(changeItems)
	local deleteItemCount = table.length(deleteItems)

	if changeItemCount == 0 and deleteItemCount == 0 then
		return 
	end]]

	-- 返回至客户端
	local msg6018 =
	{
		packIndex = packIndex,
		changedItemDatas = {},
		deletedItemGuids = {}
	}

	for k,v in pairs(changeItems) do
		if v:GetItemAmount() <= 0 then
			deleteItems[v:GetGUID()] = v
		else
			LuaLog.outDebug(string.format("[整理]物品(guid=%.0f)数量被整理为:%u", v:GetGUID(), v:GetItemAmount()))
			v:SaveToDB()
			local changedData = 
			{
				itemGuid = v:GetGUID(),
				newItemCount = v:GetItemAmount()
			}
			table.insert(msg6018.changedItemDatas, changedData)
		end
	end

	for k,v in pairs(deleteItems) do
		if v:GetItemAmount() <= 0 then
			table.insert(msg6018.deletedItemGuids, v:GetGUID())
			LuaLog.outDebug(string.format("[整理]物品(guid=%.0f)数量为0,即将被删除", v:GetGUID()))
			self:RemoveItemFromSpecifyPack(v, packIndex, true, true)
		end
	end

	self.owner:SendPacket(Opcodes.SMSG_006018, "UserProto006018", msg6018)

	local timeDiff = SystemUtil.getMsTime64() - startTime
	LuaLog.outDebug(string.format("[整理]此次背包物品(更新数=%u,删除数=%u)整理耗时:%u ms", #msg6018.changedItemDatas, #msg6018.deletedItemGuids, timeDiff))
end

--[[ 清除背包内所有道具
]]
function ItemInterface:ClearAllPackItems(packIndex)
	local startIndex = enumItemPackType.NORMAL
	local endIndex = enumItemPackType.BANK
	if packIndex ~= nil then
		if packIndex == enumItemPackType.NORMAL then
			endIndex = enumItemPackType.NORMAL
		elseif packIndex == enumItemPackType.BANK then
			startIndex = enumItemPackType.BANK
		end
	end

	for i=startIndex,endIndex do
		local packItems = self.pack_items[i]
		for k,v in pairs(packItems) do
			local itemGuid = k
			-- 内存与DB中移除
			self:RemoveItemFromSpecifyPack(v, i, true, true)
			-- 通知到客户端
			self.owner:SendRemoveItemNotice(itemGuid)
		end
	end
end

--[[ 发送背包数据
]]
function ItemInterface:SendPackItemDatasToClient(packIndex)
	-- body
	if packIndex == nil or packIndex <= 0 or packIndex > enumItemPackType.COUNT then
		return
	end

	local packItems = self.pack_items[packIndex]
	local msg6002 =
	{
		packIndex = packIndex,
		packItems = {}
	}

	for k,v in pairs(packItems) do
		local data = v:FillItemClientData()
		table.insert(msg6002.packItems, data)
	end

	local cnt = #msg6002.packItems
	self.owner:SendPacket(Opcodes.SMSG_006002, "UserProto006002", msg6002)

	-- LuaLog.outString(string.format("向玩家:%s(guid=%.0f)发送背包(index=%u)中物品:%u个", self.owner:GetName(), self.owner:GetGUID(), packIndex, cnt))
end

--endregion
