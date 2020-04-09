#include "stdafx.h"
#include "Object.h"
#include "SkillHelp.h"

int Object::getUInt64Value(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 index = (uint32)lua_tointeger(l, 1);
	uint64 val = GetUInt64Value(index);
	lua_pushinteger(l, (LUA_INTEGER)val);
	LuaReturn(l, top, 1);
}

int Object::setUInt64Value(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 index = (uint32)lua_tointeger(l, 1);
	uint64 val = (uint64)lua_tointeger(l, 2);
	SetUInt64Value(index, val);
	LuaReturn(l, top, 0);
}

int Object::getFloatValue(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 index = (uint32)lua_tointeger(l, 1);
	float val = GetFloatValue(index);
	lua_pushnumber(l, (LUA_NUMBER)val);
	LuaReturn(l, top, 1);
}

int Object::setFloatValue(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 index = (uint32)lua_tointeger(l, 1);
	float val = (float)lua_tonumber(l, 2);
	SetFloatValue(index, val);
	LuaReturn(l, top, 0);
}

int Object::isAlive(lua_State *l)
{
	int32 top = lua_gettop(l);
	int alive = isAlive() ? 1 : 0;
	lua_pushboolean(l, alive);
	LuaReturn(l, top, 1);
}

int Object::isDead(lua_State *l)
{
	int32 top = lua_gettop(l);
	int dead = isDead() ? 1 : 0;
	lua_pushboolean(l, dead);
	LuaReturn(l, top, 1);
}

int Object::setDeathState(lua_State *l)
{
	int32 top = lua_gettop(l);
	int state = (int)lua_tonumber(l, 1);
	setDeathState((DeathState)state);
	LuaReturn(l, top, 0);
}

int Object::isMoving(lua_State *l)
{
	int32 top = lua_gettop(l);
	int moving = isMoving() ? 1 : 0;
	lua_pushboolean(l, moving);
	LuaReturn(l, top, 1);
}

int Object::getGuid(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint64 guid = GetGUID();
	lua_pushinteger(l, (LUA_INTEGER)guid);

	LuaReturn(l, top, 1);
}

int Object::getObjType(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 type = GetTypeID();
	lua_pushinteger(l, (LUA_INTEGER)type);
	LuaReturn(l, top, 1);
}

int Object::isInWorld(lua_State *l)
{
	int32 top = lua_gettop(l);
	int in = IsInWorld() ? 1 : 0;
	lua_pushboolean(l, in);
	LuaReturn(l, top, 1);
}

int Object::getMapID(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 val = GetMapId();
	lua_pushinteger(l, (LUA_INTEGER)val);
	LuaReturn(l, top, 1);
}

int Object::setMapID(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	m_mapId = lua_tointeger(l, 1);

	LuaReturn(l, top, 0);
}

int Object::getInstanceID(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 val = GetInstanceID();
	lua_pushinteger(l, (LUA_INTEGER)val);
	LuaReturn(l, top, 1);
}

int Object::getCurPixelPos(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushnumber(l, GetPositionX());
	lua_pushnumber(l, GetPositionY());
	
	LuaReturn(l, top, 2);
}

int Object::setCurPos(lua_State *l)
{
	int32 top = lua_gettop(l);

	float posX = lua_tonumber(l, 1);
	float posY = lua_tonumber(l, 2);
	SetPosition(posX, posY);
	
	LuaReturn(l, top, 0);
}

int Object::getCurDirection(lua_State *l)
{
	int32 top = lua_gettop(l);
	lua_pushnumber(l, m_direction);
	LuaReturn(l, top, 1);
}

int Object::calcuDistance(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint64 objGuid = (uint64)lua_tointeger(l, 1);
	float dis = 65536.0f;
	float posX = 0.0f, posY = 0.0f;
	if (IsInWorld())
	{
		Object *obj = GetMapMgr()->GetObject(objGuid);
		if (obj != NULL)
		{
			posX = obj->GetPositionX();
			posY = obj->GetPositionY();
			dis = CalcDistance(obj);
		}
	}
	lua_pushnumber(l, dis);
	lua_pushnumber(l, posX);
	lua_pushnumber(l, posY);

	LuaReturn(l, top, 3);
}

int Object::calcuDirectionByGuid(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint64 objGuid = (uint64)lua_tointeger(l, 1);
	if (!IsInWorld())
		LuaReturn(l, top, 0);

	Object *obj = GetMapMgr()->GetObject(objGuid);
	if (obj == NULL)
		LuaReturn(l, top, 0);

	float dir = SkillHelp::CalcuDirection(GetPositionX(), GetPositionY(), obj->GetPositionX(), obj->GetPositionY());
	uint8 dir_16 = SkillHelp::CalcuDireciton16(dir);

	lua_pushnumber(l, dir);
	lua_pushnumber(l, (LUA_NUMBER)dir_16);

	LuaReturn(l, top, 2);
}

int Object::setDirectionToDestObject(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint64 objGuid = (uint64)lua_tointeger(l, 1);
	if (!IsInWorld())
		LuaReturn(l, top, 0);
	Object *obj = GetMapMgr()->GetObject(objGuid);
	if (obj == NULL)
		LuaReturn(l, top, 0);
	
	float dir = SkillHelp::CalcuDirection(GetPositionX(), GetPositionY(), obj->GetPositionX(), obj->GetPositionY());
	SetDirection(dir);

	// uint32 dir16 = SkillHelp::CalcuDireciton16(dir);
	/*if (isPlayer())
	{
	Player *plr = TO_PLAYER(this);
	sLog.outDebug("玩家%s 当前方向被设置为%.2f, dir16=%u", plr->strGetGbkNameString().c_str(), dir, dir16);
	}*/
	lua_pushnumber(l, dir);
	LuaReturn(l, top, 1);
}

int Object::setDirectionToDestCoord(lua_State *l)
{
	int32 top = lua_gettop(l);
	if (!IsInWorld())
		LuaReturn(l, top, 0);

	float dstPosX = lua_tonumber(l, 1);
	float dstPosY = lua_tonumber(l, 2);

	float dir = SkillHelp::CalcuDirection(GetPositionX(), GetPositionY(), dstPosX, dstPosY);
	SetDirection(dir);

	lua_pushnumber(l, dir);
	LuaReturn(l, top, 1);
}

int Object::initMapPosData(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint32 mapID = lua_tointeger(l, 1);
	float posX = lua_tonumber(l, 2);
	float posY = lua_tonumber(l, 3);
	float dir = lua_tonumber(l, 4);
	
	_Create(mapID, posX, posY, dir);

	LuaReturn(l, top, 0);
}

int Object::startMoving(lua_State *l)
{
	int32 top = lua_gettop(l);
	float dstPosX = lua_tonumber(l, 1);
	float dstPosY = lua_tonumber(l, 2);
	uint8 mode = (uint8)lua_tonumber(l, 3);

	float costTime = StartMoving(GetPositionX(), GetPositionY(), dstPosX, dstPosY, mode);
	lua_pushnumber(l, (LUA_NUMBER)costTime);
	
	LuaReturn(l, top, 1);
}

int Object::stopMoving(lua_State *l)
{
	int32 top = lua_gettop(l);
	StopMoving(true, GetPosition());
	LuaReturn(l, top, 0);
}

int Object::sendMessageToSet(lua_State *l)
{
	int32 top = lua_gettop(l);
	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	bool toSelf = lua_toboolean(l, 2);

	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 1);

	SendMessageToSet(pck, toSelf);
	LuaReturn(l, top, 0);
}

