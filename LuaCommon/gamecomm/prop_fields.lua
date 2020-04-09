--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gamecomm.prop_fields", package.seeall)

local _G = _G

_G.ObjType =
{
	TYPE_PLAYER = 1,			-- 玩家
	TYPE_MONSTER = 2,			-- 怪物
	TYPE_PARTNER = 3,			-- 伙伴
}

_G.ObjField =
{
	OBJECT_FIELD_GUID		= 0,				-- 对象guid
	OBJECT_FIELD_TYPE		= 1,				-- 类型
	OBJECT_FIELD_PROTO		= 2,				-- 原型id
	
	OBJECT_FIELD_END		= 3,
}

_G.UnitField =
{
	UNIT_FIELD_LEVEL				= ObjField.OBJECT_FIELD_END + 0x0000,		-- 等级
	UNIT_FIELD_PHOTO_ID				= ObjField.OBJECT_FIELD_END + 0x0001,		-- 头像id
	UNIT_FIELD_MODEL_DISPLAY_ID		= ObjField.OBJECT_FIELD_END + 0x0002,		-- 模型id
	UNIT_FIELD_MAX_HP				= ObjField.OBJECT_FIELD_END + 0x0003,		-- 最大生命值
	UNIT_FIELD_CUR_HP				= ObjField.OBJECT_FIELD_END + 0x0004,		-- 当前生命值

	UNIT_FIELD_END = ObjField.OBJECT_FIELD_END + 0x0005,
}

_G.PartnerField =
{
	PARTNER_FIELD_OWNER			= UnitField.UNIT_FIELD_END + 0x0000,			-- 主人guid
	PARTNER_FIELD_WEAPON		= UnitField.UNIT_FIELD_END + 0x0001,			-- 伙伴武器
	PARTNER_FIELD_FASHION		= UnitField.UNIT_FIELD_END + 0x0002,			-- 伙伴时装
	PARTNER_FIELD_WING			= UnitField.UNIT_FIELD_END + 0x0003,			-- 伙伴翅膀
	PARTNER_FIELD_MOUNT			= UnitField.UNIT_FIELD_END + 0x0004,			-- 伙伴坐骑

	PARTNER_FIELD_END = UnitField.UNIT_FIELD_END + 0x0005,
}

--endregion
