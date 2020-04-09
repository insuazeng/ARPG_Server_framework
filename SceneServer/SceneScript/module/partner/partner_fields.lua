--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.partner.partner_fields", package.seeall)
require("gamecomm.prop_fields")

local _G = _G

_G.PartnerField =
{
	PARTNER_FIELD_OWNER				= UnitField.UNIT_FIELD_END + 0x0000,			-- 主人guid
	PARTNER_FIELD_WEAPON			= UnitField.UNIT_FIELD_END + 0x0001,			-- 伙伴武器
	PARTNER_FIELD_FASHION			= UnitField.UNIT_FIELD_END + 0x0002,			-- 伙伴时装
	PARTNER_FIELD_WING				= UnitField.UNIT_FIELD_END + 0x0003,			-- 伙伴翅膀
	PARTNER_FIELD_MOUNT				= UnitField.UNIT_FIELD_END + 0x0004,			-- 伙伴坐骑

    PARTNER_FIELD_END = UnitField.UNIT_FIELD_END + 0x0005,
}

--endregion
