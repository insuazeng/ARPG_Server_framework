#ifndef _MONSTER_DEF_H_
#define _MONSTER_DEF_H_

// 移动类型
enum MonsterMoveType
{
	Monster_move_none			= 0,
	Monster_move_random			= 1,	// 随机移动
	Monster_move_waypoint		= 2,	// 路点移动
};

enum MonsterAttackType
{
	Monster_ack_passive		= 0,	// 被动（受击后还击）
	Monster_ack_active		= 1,	// 主动展开攻击
	Monster_ack_peace		= 2,	// 和平（受击后不还击）
};

enum MonsterAIState
{
	AI_IDLE            = 0,    // 空闲状态
	AI_ATTACKING       = 1,    // 攻击状态
	AI_FOLLOW          = 2,    // 追击
	AI_EVADE           = 3,    // 逃避（回到原位）

	AI_STATE_COUNT,
};

// 怪物原型定义
struct tagMonsterProto
{
	uint32 m_ProtoID;						//原型id
	string m_MonsterName;					//怪物名
	string m_CreatureTitle;					//怪物头衔
	uint32 m_ResModelId;					//模型资源id
	uint32 m_RespawnTime;					//刷新时间
	uint32 m_Level;							//等级
	uint32 m_Exp;							//死亡后获得经验
	uint32 m_HP;							//生命值
	float  m_BaseMoveSpeed;					//基础移动速度(单位:米)
	uint32 m_CreatureModelSize;				//怪物的模型大小
};

struct tagMonsterBornConfig
{
	inline uint32 GetSpawnGridPosX() { return (uint32)born_center_pos.m_fX; }
	inline uint32 GetSpawnGridPosY() { return (uint32)born_center_pos.m_fY; }

	uint32	conf_id;				// 配置id
	uint32	proto_id;				// 怪物原型id
	FloatPoint born_center_pos;		// 出生位置中心点
	uint16	born_count;				// 出生个数
	uint16	born_direction;			// 出生方向
	float born_radius;
	uint8	rand_move_percentage;	// 随机移动的概率
	uint8	active_ack_percentage;	// 主动攻击的概率
};

// 怪物出生配置
struct tagMonsterSpwanData
{
	tagMonsterSpwanData() : config_id( 0 ), proto_id( 0 ), born_dir(0.0f), move_type( Monster_move_none ),
		spawn_count( 0 ), attack_type( Monster_ack_passive )
	{

	}
	inline void InitData(uint32 sqlID, uint32 spawnID, uint32 protoID)
	{
		config_id = sqlID;
		spawn_id = spawnID;
		proto_id = protoID;
	}
	inline uint32 GetSpawnGridPosX() { return uint32(born_pos.m_fX); }
	inline uint32 GetSpawnGridPosY() { return uint32(born_pos.m_fY); }

	uint32	config_id;			// 对应的sqlid
	uint32	spawn_id;			// 出生配置id（每个怪唯一）
	uint32	proto_id;			// 原型id
	FloatPoint born_pos;		// 实际出生位置
	float	born_dir;			// 方位
	uint8	move_type;			// 移动类型
	uint8	attack_type;		// 攻击类型
	uint8	spawn_count;		// 出生次数
};

#endif
