--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gamecomm.item_defines", package.seeall)
local _G = _G

-- 物品品质定义
_G.ITEM_QUALITY =
{
    WHITE			= 0,	-- 白
    GREEN			= 1,	-- 绿
    BLUE			= 2,	-- 蓝
    PURPLE			= 3,	-- 紫
    ORANGE			= 4,	-- 橙
    RED 			= 5,	-- 红

    COUNT			= 6,
}

-- 背包格子最大数量
_G.enumItemPackGridLimit =
{
    NORMAL          = 100,  -- 普通背包100
    BANK            = 100,  -- 仓库背包100
}

-- 物品类型定义
_G.enumItemMainClass =
{
	COMSUME			= 1,	-- 消耗类物品
    EQUIPMENT		= 2,	-- 装备类物品
    MATERIAL		= 3,	-- 材料类物品
}

-- 消耗类物品子类定义
_G.enumComsumeItemSubClass =
{
    HP_PACKAGE      = 1,    -- 血包道具
}

-- 背包类型定义
_G.enumItemPackType = 
{
    EQUIPMENT       = 1,        -- 装备背包
    NORMAL          = 2,        -- 普通背包
    BANK            = 3,        -- 仓库背包

    COUNT = 3,
}

-- 背包绑定类型
_G.enumItemBindType =
{
    DEFAULT_BIND    = 0,    -- 获取即绑定
    USE_TO_BIND     = 1,    -- 使用后才绑定
    NOT_BIND        = 5,    -- 无论如何都不绑定
}

--endregion
