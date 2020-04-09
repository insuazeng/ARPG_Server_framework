--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.common.scene_defines", package.seeall)

local _G = _G
print(0)
_G.enumDeathState = 
{
	ALIVE 			= 0,	-- 存活
	JUST_DIED		= 1,	-- 刚死
	CORPSE			= 2,	-- 尸体状态
	DEAD			= 3,	-- 死亡
}

_G.enumMonsterAttackMode = 
{
	MONSTER_ACK_PASSIVE		= 0,	-- 被动攻击
	MONSTER_ACK_ACTIVE		= 1,	-- 主动攻击
	MONSTER_ACK_PEACE		= 2,	-- 和平模式
}

_G.enumMonsterMoveType =
{
	MONSTER_MOVE_NONE		= 0,	-- 不进行移动
	MONSTER_MOVE_RANDOM		= 1,	-- 随机移动
	MONSTER_MOVE_WAYPOINT	= 2,	-- 按路点移动
}

_G.enumSceneFightObjectType = 
{
	OBJECT			= 0,		-- 普通对象
	PLAYER			= 1,		-- 玩家
	MONSTER			= 2,		-- 怪物
}

-- 16方向
_G.Direction16 =
{
	Up					= 0,
	RightUpUp			= 1,
	RightUp				= 2,
	RightUpRight		= 3,

	Right				= 4,
	RightDownRight		= 5,
	RightDown			= 6,
	RightDownDown		= 7,

	Down				= 8,
	LeftDownDown		= 9,
	LeftDown			= 10,
	LeftDownLeft		= 11,

	Left				= 12,
	LeftUpLeft			= 13,
	LeftUp				= 14,
	LeftUpUp			= 15,
}

--endregion
