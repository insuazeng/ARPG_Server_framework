--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.item.base_item", package.seeall)

local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject

BaseItem = oo.Class(superClass, "BaseItem")

local itemProtoConf = require("gameconfig.item_proto_conf")

function BaseItem:__init(guid, protoID, ownerGuid)
	-- body
    superClass.__init(self, guid)
    
    self.proto_id = protoID
    self.owner_guid = ownerGuid
    self.pack_index = 0
    self.is_dirty = false
    self.bind_flag = false
    self.amount = 0

end

function BaseItem:Term()
	-- body
	-- LuaLog.outString(string.format("BaseItem(guid=%.0f,proto=%u)Term Call", self:GetGUID(), self.proto_id))
end

-- 读取数据
function BaseItem:LoadFromDB(dbData)
	-- body
	self.owner_guid = dbData.owner_guid
	self.proto_id = dbData.proto_id
	self.pack_index = dbData.pack_index
	self.amount = dbData.amount
	self.bind_flag = false
	if dbData.bind_flag == 1 then
		self.bind_flag = true
	end
end

-- 保存数据
function BaseItem:SaveToDB()
	-- body
	if not self.is_dirty then
		return
	end

	LuaLog.outDebug(string.format("BaseItem:SaveToDB called, itemGuid=%.0f", self:GetGUID()))
	
	local querySql = string.format("{item_guid:%.0f}", self:GetGUID())
	
	local bindFlag = 0
	if self.bind_flag then
		bindFlag = 1
	end

    local updateSql = string.format("{$set:{item_guid:%.0f,owner_guid:%.0f,proto_id:%u,pack_index:%u,amount:%u,bind_flag:%u}}", self:GetGUID(), self.owner_guid,
    								self.proto_id, self.pack_index, self.amount, bindFlag)
    sMongoDB.Update("player_items", querySql, updateSql, true)

	self.is_dirty = false
end

-- 从DB中移除
function BaseItem:DeleteFromDB()
	-- body
	local querySql = string.format("{item_guid:%.0f}", self:GetGUID())
	sMongoDB.Delete("player_items", querySql, false)
end

-- 获取原型配置
function BaseItem:GetProto()
	-- body
	return itemProtoConf.GetItemProto(self.proto_id)
end

-- 能否装备
function BaseItem:CanBeEquip()
    return false
end

-- 能否使用
function BaseItem:CanBeUse()
	return true
end

-- 是否为绑定物
function BaseItem:IsBindItem()
	return self.bind_flag
end

-- 设置绑定标记
function BaseItem:SetBindFlag(flag)
	if self.bind_flag ~= flag then
		self.bind_flag = true
		self.is_dirty = true
	end
end

-- 设置物品数量
function BaseItem:SetItemAmount(amount)
	if self.amount ~= amount then
		self.amount = amount
		self.is_dirty = true
	end
end

-- 获取物品数量
function BaseItem:GetItemAmount()
	return self.amount
end

-- 设置物品背包索引
function BaseItem:SetPackIndex(index)

	-- body
	if self.pack_index ~= index then
		self.pack_index = index
		self.is_dirty = true
	end
end

-- 获取物品背包索引
function BaseItem:GetPackIndex()
	-- body
	return self.pack_index
end

-- 填充物品的保存数据
function BaseItem:FillItemSaveData()
	-- body
	local data = {}
end

-- 填充物品的发送数据
function BaseItem:FillItemClientData()
	-- body
	local data = 
	{
		itemGuid = self:GetGUID(),
		itemProtoID = self.proto_id,
		packIndex = self.pack_index,
		count = self.amount,
		bindFlag = self.bind_flag
	}
	
	return data
end

--endregion
