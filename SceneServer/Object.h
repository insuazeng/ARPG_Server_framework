/****************************************************************************
*
* General Object Type File
* Copyright (c) 2007 Team Ascent
*
* This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
* License. To view a copy of this license, visit
* http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
* 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
*
* EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
* ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
* ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGES.
*
*/

#ifndef _OBJECT_H
#define _OBJECT_H
#include "BaseObject.h"
#include "FloatPoint.h"

//#include "Player.h"
//====================================================================
//  Object
//  Base object for every item, unit, player, corpse, container, etc
//====================================================================

#define TO_PLAYER(ptr) ((Player*)ptr)
#define TO_MONSTER(ptr) ((Monster*)ptr)
#define TO_OBJECT(ptr) ((Object*)ptr)
#define TO_PARTNER(ptr) ((Partner*)ptr)

// 移动类型
enum OBJ_MOVE_TYPE
{	
	OBJ_MOVE_TYPE_STOP_MOVING		= 0x00,		// 不动
	OBJ_MOVE_TYPE_START_MOVING		= 0x01,		// 移动中
};

enum DeathState
{
	ALIVE = 0,  // Unit is alive and well
	JUST_DIED,  // Unit has JUST died
	CORPSE,		// Unit has died but remains in the world as a corpse
	DEAD		// Unit is dead and his corpse is gone from the world
};

#define OBJ_MOVE_UPDATE_TIMER	100		// 100ms进行一次移动的update

class Unit;
class MapMgr;
class MapCell;
class Object;

typedef hash_set<Object*> InRangeSet;

class Object : public BaseObject, public EventableObject
{
public:
	virtual ~Object ( );

	void _Create(uint32 mapid, float x, float y, float dir);
	int32 event_GetInstanceID() { return m_event_Instanceid; }
	virtual void Update ( uint32 time ) { }
	virtual void Destructor() { delete this; }

public:
	//! True if object exists in world
	inline bool IsInWorld() { return m_mapMgr != NULL; }

	virtual void AddToWorld();
	virtual MapMgr* OnObjectGetInstanceFailed();
	void PushToWorld(MapMgr *);
	virtual void OnPushToWorld() { }
	virtual void OnPrePushToWorld() { }
	virtual void RemoveFromWorld(bool free_guid);
	virtual void OnPreRemoveFromWorld(MapMgr *mgr) { }
	virtual bool CanSee(Object *obj) { return true; }			// 能否看见某对象
	virtual void Init();
	virtual void Term() { }
	virtual void Delete();
	virtual void CheckErrorOnUpdateRange(uint32 cellX, uint32 cellY, uint32 mapId);
	virtual void OnValueFieldChange(const uint32 index);	// 调用SetxxxValue后的回调函数
	
	void SetPosition(float newX, float newY);
	void SetRespawnLocation(const FloatPoint &spawnPos, float spawnDir);
	void ModifyPosition(float modX, float modY);
	bool CheckPosition(uint16 x, uint16 y);

	inline bool isAlive() { return m_deathState == ALIVE; }
	inline bool isDead() { return  m_deathState != ALIVE; }
	inline void setDeathState(DeathState s) { m_deathState = s; }
	inline DeathState getDeathState() { return m_deathState; }

	inline bool isPlayer() { return m_uint64Values[OBJECT_FIELD_TYPE] == TYPEID_PLAYER; }
	inline bool isMonster() { return m_uint64Values[OBJECT_FIELD_TYPE] == TYPEID_MONSTER; }
	inline bool isPartner() { return m_uint64Values[OBJECT_FIELD_TYPE] == TYPEID_PARTNER; }

	inline bool isInDungeon() { return false; }
	inline bool isInNormalScene() { return true; }

	inline const float& GetPositionX( ) const { return m_position.m_fX; }
	inline const float& GetPositionY( ) const { return m_position.m_fY; }
	inline const uint32 GetGridPositionX() const { return (uint32)(m_position.m_fX / lengthPerGrid); }
	inline const uint32 GetGridPositionY() const { return (uint32)(m_position.m_fY / lengthPerGrid); }
	inline const float GetDirection( ) const { return m_direction; }
	inline void SetDirection( float newDir ) { m_direction = newDir; }

	inline const float& GetSpawnX( ) const { return m_spawnLocation.m_fX; }
	inline const float& GetSpawnY( ) const { return m_spawnLocation.m_fY; }
	inline const uint32 GetGridSpwanX() const { return (uint32)(m_spawnLocation.m_fX / lengthPerGrid); }
	inline const uint32 GetGridSpwanY() const { return (uint32)(m_spawnLocation.m_fY / lengthPerGrid); }
	inline const float GetSpawnDir( ) const { return m_spawndir; }
	inline void SetSpawnX(float x) { m_spawnLocation.m_fX = x; }
	inline void SetSpawnY(float y) { m_spawnLocation.m_fY = y; }
	// inline virtual const uint32 GetMoveSpeed() { return sDBConfLoader.GetUInt32Value("object_base_speed"); }
	inline virtual const float GetMoveSpeed() { return sDBConfLoader.GetUInt32Value("object_base_speed") / 100.0f; }
	inline bool isMoving() { return m_bMoving; }

	virtual const char* szGetUtf8Name() const;

	const FloatPoint & GetPosition() { return m_position; }
	FloatPoint & GetPositionNC() { return m_position; }
	FloatPoint * GetPositionV() { return &m_position; }

	//Distance Calculation
	float CalcDistance(Object* Ob);
	float CalcDistance(float ObX, float ObY);
	float CalcDistance(Object* Oa, Object* Ob);
	float CalcDistance(Object* Oa, float ObX, float ObY);
	float CalcDistance(float OaX, float OaY, float ObX, float ObY);
	float CalcDistance(FloatPoint &pos);
	float CalcDistanceSQ(Object *obj);

	//Distance Calculation with model size
	virtual float GetModelRadius() const { return 0; }
	float CalcDistanceWithModelSize(Object *obj);
	float CalcDistanceWithModelSize(float x, float y);

	//use it to check if a object is in range of another
	bool isInRange(Object* target, float range);

	// 移动相关
	float StartMoving(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 moveMode);
	void StopMoving(bool systemStop, const FloatPoint &curPos);
	void BoardCastMovingStatus(float srcPosX, float srcPosY, float destPosX, float destPosY, uint8 mode);
	void UpdateMoving(uint32 timeDiff);		// 更新移动状态

	//活动相关的接口，怪物没有人的时候就不用活动了
	bool Active;
	bool CanActivate();
	void Activate(MapMgr* mgr);
	void Deactivate(MapMgr* mgr);

	//! Only for MapMgr use
	inline MapCell* GetMapCell() const { return m_mapCell; }
	//! Only for MapMgr use
	inline void SetMapCell(MapCell* cell) { m_mapCell = cell; }
	//! Only for MapMgr use
	inline MapMgr* GetMapMgr() const { return m_mapMgr; }

	inline void SetMapId(uint32 newMap) { m_mapId = newMap; }
	inline const uint32 GetMapId( ) const { return m_mapId; }
	inline bool InSpecifiedScene(uint32 mapId) { return m_mapId == mapId; }

	inline void SetInstanceID(int32 instance) { m_instanceId = instance; }
	inline int32 GetInstanceID() { return m_instanceId; }

	//通用数据模式
	//创建object的创建信息（告诉客户端创建这个object所需要的信息）
	virtual void	BuildCreationDataBlockForPlayer( Player* target );
	virtual void	BuildCreationDataBlockToBuffer( pbc_wmessage *data, Player *target );
	bool 			BuildValuesUpdateBlockToBuffer( pbc_wmessage *msg, Player *target );
	virtual bool	BuildValuesUpdateBlockForPlayer( Player *target );
	virtual void	BuildRemoveDataBlockForPlayer( Player *target );
	virtual void	ClearUpdateMask();
	virtual void	SetUpdatedFlag();		// 设置更新标记

	//处理周围object的相关函数
	inline bool IsInRangeSet( Object* pObj )
	{
		return !( m_objectsInRange.find( pObj ) == m_objectsInRange.end() );
	}
	virtual void AddInRangeObject(Object* pObj)
	{
		if( pObj == NULL )
			return;

		m_objectsInRange.insert( pObj );
		if( pObj->isPlayer() )
			m_inRangePlayers.insert( TO_PLAYER(pObj) );
	}
	inline void RemoveInRangeObject( Object* pObj )
	{
		if( pObj == NULL )
			return;
		OnRemoveInRangeObject( pObj );
		m_objectsInRange.erase( pObj );
	}

	virtual void OnRemoveInRangeObject( Object* pObj )
	{
		if( pObj->isPlayer() )
			m_inRangePlayers.erase( TO_PLAYER(pObj) );
	}

	virtual void ClearInRangeSet()
	{
		m_objectsInRange.clear();
		m_inRangePlayers.clear();
	}

	inline size_t GetInRangeCount() { return m_objectsInRange.size(); }
	inline InRangeSet::iterator GetInRangeSetBegin() { return m_objectsInRange.begin(); }
	inline InRangeSet::iterator GetInRangeSetEnd() { return m_objectsInRange.end(); }
	inline InRangeSet* GetInRangeSetObejct() { return &m_objectsInRange; }
	inline InRangeSet::iterator FindInRangeSet(Object* obj) { return m_objectsInRange.find(obj); }
	inline bool HasInRangeObjects() { return !m_objectsInRange.empty(); }

	void WriteCurPos(WorldPacket &packet)
	{
		packet << m_position.m_fX << m_position.m_fY;
	}
	void OnMgrTerm()
	{
		m_mapMgr = NULL;
	}

	inline hash_set<Player*  >::iterator GetInRangePlayerSetBegin() { return m_inRangePlayers.begin(); }
	inline hash_set<Player*  >::iterator GetInRangePlayerSetEnd() { return m_inRangePlayers.end(); }
	inline hash_set<Player*  > * GetInRangePlayerSet() { return &m_inRangePlayers; };
	inline bool IsInRangePlayerSet(Player *player) { return m_inRangePlayers.find(player) != m_inRangePlayers.end(); }
	inline bool HasInRangePlayers() { return !m_inRangePlayers.empty(); }
	inline size_t GetInRangePlayersCount() { return m_inRangePlayers.size();}

	void SendMessageToSet(WorldPacket *data, bool self);

	FloatPoint &GetDirePostion()	{return m_moveDirection;}

public:
	// 导出给lua的函数
	int getUInt64Value(lua_State *l);
	int setUInt64Value(lua_State *l);
	int getFloatValue(lua_State *l);
	int setFloatValue(lua_State *l);
	
	int isAlive(lua_State *l);
	int isDead(lua_State *l);
	int setDeathState(lua_State *l);
	int isMoving(lua_State *l);
	
	int getGuid(lua_State *l);
	int getObjType(lua_State *l);
	int isInWorld(lua_State *l);
	int getMapID(lua_State *l);
	int setMapID(lua_State *l);
	int getInstanceID(lua_State *l);
	int getCurPixelPos(lua_State *l);
	int setCurPos(lua_State *l);
	int getCurDirection(lua_State *l);
	int calcuDistance(lua_State *l);
	int calcuDirectionByGuid(lua_State *l);
	int setDirectionToDestObject(lua_State *l);
	int setDirectionToDestCoord(lua_State *l);
	int initMapPosData(lua_State *l);
	
	int startMoving(lua_State *l);
	int stopMoving(lua_State *l);
	int sendMessageToSet(lua_State *l);

protected:
	Object (  );

	//! Mark values that need updating for specified player.
	virtual void _SetUpdateBits(UpdateMask *updateMask, Player* target) const;
	//! Mark values that player should get when he/she/it sees object for first time.
	virtual void _SetCreateBits(UpdateMask *updateMask, Player* target) const;

	void _BuildValuesCreation( pbc_wmessage *createData, UpdateMask *updateMask );
	void _BuildValuesUpdate( pbc_wmessage *propData, UpdateMask *updateMask );

	//! Continent/map id.
	uint32 m_mapId;
	//! Map manager
	MapMgr *m_mapMgr;
	//! Current map cell
	MapCell *m_mapCell;

	float		m_direction;			// 当前方向
	FloatPoint	m_position;		        // 当前位置
	FloatPoint	m_lastMapUpdatePosition;
	FloatPoint	m_spawnLocation;
	float		m_spawndir;

	// about obj move
	FloatPoint m_moveDestPosition;		// 移动目标
	FloatPoint m_moveDirection;			// 方向向量
	bool m_bMoving;						// 是否在移动
	uint32 m_nMovingUpdateTimer;		// 移动定时器
	uint64 m_nStartMovingTime;			// 开始移动的时间
	DeathState m_deathState;			// 死亡状态
	//! True if object was updated
	bool m_objectUpdated;
	int32 m_instanceId;

	//add object 所有的对象都会保存在这个集合中
	hash_set<Object* > m_objectsInRange;
	//add in range when player type is TYPEID_PLAYER by tangquan 2011/10/18
	hash_set<Player* > m_inRangePlayers;
};

#endif


