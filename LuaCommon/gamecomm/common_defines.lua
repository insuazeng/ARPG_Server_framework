--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 游戏内通用定义
module("gamecomm.common_defines", package.seeall)

local _G = _G

_G.ILLEGAL_DATA_32 = 0xFFFFFFFF
_G.ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF
_G.MIN_PLAYER_GUID = 0x00000002541B2640

-- 暂定60级
_G.MAX_PLAYER_LEVEL = 60

-- 单格地图像素
-- _G.PIXEL_PER_GRID = 64

-- 职业定义s
_G.PLAYER_CAREER = 
{
	NONE			= 0,	-- 无
	ZS				= 1,	-- 战士
	FS 				= 2,	-- 法师

	COUNT			= 3,
}

-- 游戏货币类型
_G.enumMoneyType =
{
	COIN 		= 1,	-- 铜币
	BIND_CASH	= 2,	-- 绑定元宝
	CASH 		= 3,	-- 非绑定元宝
}

-- 角色属性索引
_G.enumFightProp = 
{
	BASE_HP						= 1,		-- 基础生命值
	BASE_ACK					= 2,		-- 基础攻击值
	BASE_DEF					= 3,		-- 基础防御值
	BASE_CRI					= 4,		-- 基础暴击值
	BASE_CRI_DMG				= 5,		-- 基础暴击伤害值
	BASE_CRI_DMG_PERCENTAGE		= 6,		-- 基础暴击伤害百分比
	BASE_CRI_DMG_REDUCE			= 7,		-- 基础暴击伤害减免值
	BASE_CRI_DMG_REDUCE_PERCENTAGE = 8,		-- 基础暴击伤害减免百分比
	BASE_TOUGH					= 9,		-- 基础韧性值
	BASE_HIT					= 10,		-- 基础命中值
	BASE_DODGE					= 11,		-- 基础闪避值
	BASE_ACK_SPEED				= 12,		-- 基础攻击速度值
	BASE_MOVE_SPEED				= 13,		-- 基础移动速度值

	AD_ACK_PERCENTAGE			= 14,		-- 攻击提升百分比
	AD_ACK_FIXED				= 15,		-- 攻击提升固定值
	AD_DMG_PERCENTAGE			= 16,		-- 伤害提升百分比
	AD_DMG_FIXED				= 17,		-- 伤害提升固定值
	AD_DEF_PERCENTAGE			= 18,		-- 防御提升百分比
	AD_DEF_FIXED				= 19,		-- 防御提升固定值
	AD_IGNORE_DEF_PERCENTAGE	= 20,		-- 无视防御百分比
	AD_IGNORE_DEF_FIXED			= 21,		-- 无视防御固定值
	AD_DMG_REDUCE_PERCENTAGE	= 22,		-- 减免伤害百分比
	AD_DMG_REDUCE_FIXED			= 23,		-- 减免伤害固定值
	AD_HIT_PERCENTAGE			= 24,		-- 命中提升百分比
	AD_DODGE_PERCENTAGE			= 25,		-- 闪避提升百分比
	AD_CRI_PERCENTAGE			= 26,		-- 暴击提升百分比
	AD_TOUGH_PRECENTAGE			= 27,		-- 韧性提升百分比

 	SP_ZHICAN_FIXED				= 28,		-- 致残(降低对方移速)固定值
 	SP_BINGHUAN_FIXED			= 29,		-- 冰缓(降低对方移速)固定值
 	SP_SLOWDOWN_PERCENTAGE		= 30,		-- 减速效果百分比
 	SP_FAINT_FIXED				= 31,		-- 眩晕固定值
 	SP_FAINT_TIME				= 32,		-- 眩晕时间
 	SP_WEAK_PERCENTAGE			= 33,		-- 虚弱百分比
 	SP_WEAK_FIXED				= 34,		-- 虚弱固定值
 	SP_POJIA_PERCENTAGE			= 35,		-- 破甲百分比
 	SP_POJIA_FIXED				= 36,		-- 破甲固定值
 	SP_SLIENT_FIXED				= 37,		-- 沉默固定值
 	SP_SLIENT_TIME				= 38,		-- 沉默时间
 	SP_JINSHEN_FIXED			= 39,		-- 金身固定值
 	SP_BINGPO_FIXED				= 40,		-- 冰魄固定值
 	SP_JIANDING_FIXED			= 41,		-- 坚定固定值
 	SP_JIANTI_FIXED				= 42,		-- 健体固定值
 	SP_WUJIN_FIXED				= 43,		-- 无禁固定值
 	SP_DMG_BOSS_PERCENTAGE		= 44,		-- boss伤害加成百分比
 	SP_DMG_PVP_PERCENTAGE		= 45,		-- pvp伤害加成百分比
 	SP_DMG_PVP_REDUCE_PERCENTAGE= 46,		-- pvp伤害减免加成百分比

 	ELE_GOLD_ACK_FIXED			= 47,		-- 金属性攻击加成固定值
 	ELE_GOLD_ACK_PERCENTAGE		= 48,		-- 金属性攻击加成百分比
 	ELE_WOOD_ACK_FIXED			= 49,		-- 木属性攻击加成固定值
	ELE_WOOD_ACK_PERCENTAGE		= 50,		-- 木属性攻击加成百分比
 	ELE_WATER_ACK_FIXED			= 51,		-- 水属性攻击加成固定值
 	ELE_WATER_ACK_PERCENTAGE	= 52,		-- 水属性攻击加成百分比
 	ELE_FIRE_ACK_FIXED			= 53,		-- 火属性攻击加成固定值
 	ELE_FIRE_ACK_PERCENTAGE 	= 54,		-- 火属性攻击加成百分比
 	ELE_EARTH_ACK_FIXED			= 55,		-- 土属性攻击加成固定值
 	ELE_EARTH_ACK_PERCENTAGE	= 56,		-- 土属性攻击加成百分比

 	ELE_GOLD_RESIST_FIXED		= 57,	-- 金属性抗性固定值
 	ELE_GOLD_RESIST_PERCENTAGE	= 58,	-- 金属性抗性百分比
 	ELE_WOOD_RESIST_FIXED		= 59,	-- 木属性抗性固定值
	ELE_WOOD_RESIST_PERCENTAGE	= 60,	-- 木属性抗性百分比
 	ELE_WATER_RESIST_FIXED		= 61,	-- 水属性抗性固定值
 	ELE_WATER_RESIST_PERCENTAGE	= 62,	-- 水属性抗性百分比
 	ELE_FIRE_RESIST_FIXED		= 63,	-- 火属性抗性固定值
 	ELE_FIRE_RESIST_PERCENTAGE 	= 64,	-- 火属性抗性百分比
 	ELE_EARTH_RESIST_FIXED		= 65,	-- 土属性抗性固定值
 	ELE_EARTH_RESIST_PERCENTAGE	= 66,	-- 土属性抗性百分比
	
	COUNT = 66,
}

-- 对象数据同步类型
_G.enumObjDataSyncOp =
{
	ADD 		= 0,			-- 新增
	UPDATE		= 1,			-- 数值更新
	DELETE		= 2,			-- 移除
}

-- 需要同步的对象类型
_G.enumSyncObject =
{
	PLAYER_DATA_STM			= 1,		-- 玩家数据(场景->中央服)
	PLAYER_DATA_MTS			= 2,		-- 玩家数据(中央->场景服)
	FIGHT_PROPS_MTS			= 3,		-- 战斗属性(中央->场景服)
}


--endregion
