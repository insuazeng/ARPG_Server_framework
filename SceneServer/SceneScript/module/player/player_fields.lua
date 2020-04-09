--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.player.player_fields", package.seeall)
require("gamecomm.prop_fields")

local _G = _G

_G.PlayerField =
{
	PLAYER_FIELD_PLATFORM			= UnitField.UNIT_FIELD_END + 0x0000,			-- 平台id
	PLAYER_FIELD_CAREER				= UnitField.UNIT_FIELD_END + 0x0001,			-- 职业
	PLAYER_FIELD_CUR_EXP			= UnitField.UNIT_FIELD_END + 0x0002,			-- 当前经验

	PLAYER_FIELD_WEAPON				= UnitField.UNIT_FIELD_END + 0x0003,			-- 武器id
	PLAYER_FIELD_FASHION			= UnitField.UNIT_FIELD_END + 0x0004,			-- 时装id
	PLAYER_FIELD_WING				= UnitField.UNIT_FIELD_END + 0x0005,			-- 翅膀id
	PLAYER_FIELD_MOUNT				= UnitField.UNIT_FIELD_END + 0x0006,			-- 坐骑id

    PLAYER_FIELD_END = UnitField.UNIT_FIELD_END + 0x0008,
}

--endregion
