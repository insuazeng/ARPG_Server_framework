--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.shop.shop_system", package.seeall)
local _G = _G

--[[ 物品快速购买的检测
参数 plr
参数 itemID
参数 itemCount
参数 priceType
参数 defPricePerUnit
参数 remainMinCount
]]
function ItemQuickBuyCheck(plr, itemID, itemCount, priceType, defPricePerUnit, remainMinCount)
	return false
end

--[[ 进行一次快速购买
]]
function ItemQuickBuy(plr, itemID, itemCount)
	-- body

end

--[[ 获取物品快速购买的价格
]]
function GetItemQuickBuyCost(itemID, itemCount)
	-- body
end

--endregion
