--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.skill.skill_def", package.seeall)

local _G = _G

-- 释放技能的错误码
_G.enumCastSkillErrorCode = 
{
	Bad_target					= 300201,		-- 指定了错误的目标
	Bad_monster_state			= 300202,		-- 脱战怪物无法被攻击
	Skill_cding					= 300203,		-- 技能正处于CD中
	Bad_normal_skill_id 		= 300204,		-- 错误的普攻技能id
	Bad_normal_skill_interval 	= 300205,		-- 错误的普攻攻击间隔
	Too_far_with_target			= 300206,		-- 距离目标太远
	
}

-- 技能分类
_G.enumSkillType =
{
	Normal				= 1,		-- 普通技能
	Active				= 2,		-- 主动技能
	Angry				= 3,		-- 怒气技能
	Passive				= 4,		-- 被动技能
}

-- 普通技能最大索引是4
_G.NORMAL_SKILL_MAX_INDEX = 4

-- 技能目标查找类型
_G.enumSkillTargetCheckType =
{
	Single_enemy				= 1,	-- 单个敌人
	Sector_from_caster			= 2,	-- 以施法者朝向为基础,半径为n,角度为r的扇形范围
	Round_with_caster			= 3,	-- 以施法者为中心,半径为r的圆形范围
	Round_front_caster			= 4,	-- 以施法者正前方n像素的坐标为中心,半径为r的圆形范围
	Rectangle_from_caster		= 5,	-- 以施法者朝向为基础,前方长x宽y的矩形范围
	Rectangle_with_caster		= 6,	-- 以施法者为中心,长x宽y的矩形范围
	Rectangle_front_caster		= 7,	-- 以施法者正前方n像素的坐标为中心,长x宽y的矩形范围
	Caster_self					= 8,	-- 施法者本人
}

-- 技能命中类型
_G.enumSkillHitResult = 
{
	DODGE			= 0,	-- 闪避
	NORMAL			= 1,	-- 普通命中
	CRITICAL		= 2,	-- 暴击命中
}

-- 技能命中效果类型
_G.enumSkillHitEffect =
{
	NORMAL_HURT		= 1,	-- 普通伤害
	TARGET_DEAD		= 2,	-- 目标死亡
}


-- 技能元素属性索引
_G.enumSkillElement =
{
	GOLD		= 1,	-- 金属性
	WOOD		= 2,	-- 木
	WATER		= 3,	-- 水
	FIRE		= 4,	-- 火
	EARTH		= 5,	-- 土
}

-- 技能释放开始事件
_G.enumSkillCastStartEvent =
{
	CASTER_ASSAULT		= 1,	-- 释放者进行冲锋
}

--endregion

