#include "stdafx.h"
#include "Monster.h"

int Monster::getMonsterObject(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 mapID = (uint32)lua_tonumber(l, 1);
	uint32 instanceID = (uint32)lua_tonumber(l, 2);
	uint64 guid = (uint64)lua_tonumber(l, 3);
	MapMgr *mgr = sInstanceMgr.GetInstanceMapMgr(mapID, instanceID);
	if (mgr == NULL)
	{
		LuaReturn(l, top, 0);
	}

	Monster *m = mgr->GetMonster(guid);
	if (m == NULL)
	{
		LuaReturn(l, top, 0);
	}

	Lunar<Monster>::push(l, m, false);
	LuaReturn(l, top, 1);
}

int Monster::getBornPos(lua_State *l)
{
	int32 top = lua_gettop(l);
	if (m_spawnData == NULL)
		LuaReturn(l, top, 0);
	
	lua_pushnumber(l, m_spawnData->born_pos.m_fX);
	lua_pushnumber(l, m_spawnData->born_pos.m_fY);

	LuaReturn(l, top, 2);
}

int Monster::onMonsterJustDied(lua_State *l)
{
	int32 top = lua_gettop(l);
	m_corpseEvent = true;		// 启动重生事件
	setDeathState(CORPSE);

	LuaReturn(l, top, 0);
}

int Monster::findAttackableTarget(lua_State *l)
{
	int32 top = lua_gettop(l);
	// 根据传入的警戒范围找到玩家目标
	uint32 range = (uint32)lua_tonumber(l, 1);
	Player *plr = FindAttackablePlayerTarget(range);
	if (plr == NULL)
		LuaReturn(l, top, 0);

	if (plr != NULL)
	{
		float dis = CalcDistanceWithModelSize(TO_OBJECT(plr));
		// 写入该目标id以及距离
		lua_pushinteger(l, (LUA_INTEGER)plr->GetGUID());
		lua_pushnumber(l, (LUA_NUMBER)dis);
		lua_pushinteger(l, (LUA_INTEGER)plr->GetPositionX());
		lua_pushinteger(l, (LUA_INTEGER)plr->GetPositionY());
	}

	LuaReturn(l, top, 4);
}

int Monster::isMovablePos(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 mapID = GetMapId();
	float posX = lua_tonumber(l, 1);
	float posY = lua_tonumber(l, 2);
	bool ret = sSceneMapMgr.IsCurrentPositionMovable(mapID, posX, posY);

	lua_pushboolean(l, ret ? 1 : 0);
	LuaReturn(l, top, 1);
}

int Monster::setAIState(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint8 state = (uint8)lua_tonumber(l, 1);
	if (state < AI_STATE_COUNT)
	{
		m_AIState = state;
		// sLog.outDebug("怪物guid=%u,ai状态被设置为:%u", (uint32)GetGUID(), (uint32)state);
	}
	LuaReturn(l, top, 0);
}


LUA_IMPL(Monster, Monster)

LUA_METD_START(Monster)
// 从obejct继承的一些函数
L_METHOD(Monster, getUInt64Value)
L_METHOD(Monster, setUInt64Value)
L_METHOD(Monster, getFloatValue)
L_METHOD(Monster, setFloatValue)
L_METHOD(Monster, isAlive)
L_METHOD(Monster, isDead)
L_METHOD(Monster, setDeathState)
L_METHOD(Monster, isMoving)
L_METHOD(Monster, getGuid)
L_METHOD(Monster, getObjType)
L_METHOD(Monster, isInWorld)
L_METHOD(Monster, getMapID)
L_METHOD(Monster, setMapID)
L_METHOD(Monster, getInstanceID)
L_METHOD(Monster, getCurPixelPos)
L_METHOD(Monster, setCurPos)
L_METHOD(Monster, getCurDirection)
L_METHOD(Monster, startMoving)
L_METHOD(Monster, stopMoving)
L_METHOD(Monster, sendMessageToSet)
L_METHOD(Monster, calcuDistance)
L_METHOD(Monster, setDirectionToDestObject)
L_METHOD(Monster, setDirectionToDestCoord)
L_METHOD(Monster, initMapPosData)
// monster类自己的函数
L_METHOD(Monster, getBornPos)
L_METHOD(Monster, onMonsterJustDied)
L_METHOD(Monster, findAttackableTarget)
L_METHOD(Monster, isMovablePos)
L_METHOD(Monster, setAIState)
LUA_METD_END

LUA_FUNC_START(Monster)
L_FUNCTION(Monster, getMonsterObject)
LUA_FUNC_END
