--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.item.equipment_item", package.seeall)

local oo = require("oo.oo")
local ooSUPER = require("MasterScript.module.item.base_item")
local superClass = ooSUPER.BaseItem

EquipmentItem = oo.Class(superClass, "EquipmentItem")

function EquipmentItem:__init(guid, protoID, ownerGuid)
	-- body
    superClass.__init(self, guid, protoID, ownerGuid)
end

-- 能否装备
function EquipmentItem:CanBeEquip()
    return true
end

-- 能否使用
function EquipmentItem:CanBeUse()
	return false
end

--endregion
