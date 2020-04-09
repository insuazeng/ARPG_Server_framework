#ifndef _MONSTER_H_
#define _MONSTER_H_

#include "Object.h"
#include "MonsterDef.h"

class MapMgr;
class MapCell;
struct tagMonsterBornConfig;

class Monster : public Object
{
public:
	Monster(void);
	virtual ~Monster(void);

public:
	inline tagMonsterProto* GetProto()	{ return m_protoConfig; }
	inline void SetProto(tagMonsterProto *proto) { m_protoConfig = proto; }
	inline tagMonsterSpwanData* GetSpawnData() { return m_spawnData; }
	inline const uint8 GetAIState() { return m_AIState; }

public:
	void Initialize(uint64 guid);		// 从pool中初始化
	void Terminate();					// 删除回pool

	virtual void gcToMonsterPool();
	virtual void Update( uint32 time );

	virtual void OnPushToWorld();
	virtual void OnPreRemoveFromWorld(MapMgr *mgr);
	virtual const float GetMoveSpeed();

public:
	bool Load( tagMonsterBornConfig *bornConfig, MapMgr *bornMapMgr );
	void Load( tagMonsterProto *proto_, float x, float y, float direction, MapMgr *bornMapMgr );
	void RemoveFromWorld(bool addrespawnevent, bool free_guid);
	void OnRemoveCorpse();
	void OnRespawn(MapMgr* m);
	void Despawn(uint32 delay, uint32 respawntime);
	void SafeDelete();
	void DeleteMe();//请求删除这个怪物，safedelete的回调

public:
	virtual void _SetCreateBits(UpdateMask *updateMask, Player* target) const;
	virtual void _SetUpdateBits(UpdateMask *updateMask, Player* target) const;
	// static
	static void InitVisibleUpdateBits();
	static UpdateMask m_visibleUpdateMask;

	// 与AI有关的部分
	Player* FindAttackablePlayerTarget(uint32 alertRange);

public:
	static int getMonsterObject(lua_State *l);

	int getBornPos(lua_State *l);
	int onMonsterJustDied(lua_State *l);
	int findAttackableTarget(lua_State *l);
	int isMovablePos(lua_State *l);
	int setAIState(lua_State *l);

	// 导出给lua
	LUA_EXPORT(Monster)

public:
	MapCell *m_respawnCell;				// 出生的cell

private:
	bool m_corpseEvent;					// 死亡事件的处理标记
	uint8 m_AIState;					// ai状态(空闲,战斗,追击等)
	
	tagMonsterProto		*m_protoConfig;
	tagMonsterSpwanData *m_spawnData;

	uint64 _fields[UNIT_FIELD_END];
};

#endif
