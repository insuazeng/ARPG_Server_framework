--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
-- 物品相关

module("MasterScript.module.player.lua_player", package.seeall)

--[[ 获取背包实例
]]
function LuaPlayer:GetItemInterface()
	-- body
	return self.item_interface
end

--[[ 发送物品信息(新增物品/数量改变/绑定改变/数据改变都通过本函数发送)
]]
function LuaPlayer:SendGetItemData(item)

	local msg6014 = {}
	msg6014.itemData = item:FillItemClientData()
	self:SendPacket(Opcodes.SMSG_006014, "UserProto006014", msg6014)
end

--[[ 发送物品移除通知
]]
function LuaPlayer:SendRemoveItemNotice(itemGuid)
	-- body
	local msg6016 =
	{
		itemGuid = itemGuid
	}
	self:SendPacket(Opcodes.SMSG_006016, "UserProto006016", msg6016)
end

--endregion
