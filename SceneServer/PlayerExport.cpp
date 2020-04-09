#include "stdafx.h"
#include "Player.h"

int Player::newCppPlayerObject(lua_State *l)
{
	int32 top = lua_gettop(l);

	Player *plr = new Player();
	int ret = 0;
	if (plr != NULL)
	{
		Lunar<Player>::push(l, plr, false);
		ret = 1;
	}

	LuaReturn(l, top, ret);
}

int Player::delCppPlayerObject(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	Player *plr = dynamic_cast<Player*>(obj);
	
	if (plr != NULL)
	{
		plr->Term();
		SAFE_DELETE(plr);
	}
	LuaReturn(l, top, 0);
}

int Player::initData(lua_State *l)
{
	int32 top = lua_gettop(l);

	GameSession *session = (GameSession*)lua_topointer(l, 1);
	uint64 guid = (uint64)lua_tointeger(l, 2);
	uint32 fieldCount = lua_tointeger(l, 3);
	
	Init(session, guid, fieldCount);

	LuaReturn(l, top, 0);
}

int Player::getPlayerUtf8Name(lua_State *l)
{
	int32 top = lua_gettop(l);
	string name = strGetUtf8NameString();
	lua_pushstring(l, name.c_str());

	LuaReturn(l, top, 1);
}

int Player::setPlayerUtf8Name(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	string name = lua_tostring(l, 1);
	m_NickName = name;

	LuaReturn(l, top, 0);
}

int Player::getPlayerGbkName(lua_State *l)
{
	int32 top = lua_gettop(l);
	string gbkName = strGetGbkNameString();
	lua_pushstring(l, gbkName.c_str());
	LuaReturn(l, top, 1);
}

int Player::removeFromWorld(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	RemoveFromWorld();

	LuaReturn(l, top, 0);
}

int Player::sendPacket(lua_State *l)
{
	int32 top = lua_gettop(l);
	GameSession *session = GetSession();
	if (session == NULL)
	{
		sLog.outError("luaSendPacket失败,玩家(guid="I64FMTD")Session为NULL", GetGUID());
		LuaReturn(l, top, 0);
	}

	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 0);

	sLog.outDebug("[Lua发包]玩家:%s(guid="I64FMTD")发包成功(op=%u,len=%u,err=%u)", strGetGbkNameString().c_str(), GetGUID(), pck->GetOpcode(), pck->size(), pck->GetErrorCode());
	session->SendPacket(pck);

	LuaReturn(l, top, 0);
}

int Player::sendServerPacket(lua_State *l)
{
	int32 top = lua_gettop(l);
	GameSession *session = GetSession();
	if (session == NULL)
	{
		sLog.outError("luaSendServerPacket失败,玩家(guid="I64FMTD")Session为NULL", GetGUID());
		LuaReturn(l, top, 0);
	}

	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 0);

	session->SendServerPacket(pck);

	LuaReturn(l, top, 0);
}

int Player::setPlayerDataSyncFlag(lua_State *l)
{
	int32 top = lua_gettop(l);

	m_bNeedSyncPlayerData = true;

	LuaReturn(l, top, 0);
}

int Player::releasePartner(lua_State *l)
{
	int32 top = lua_gettop(l);
	uint32 partnerProto = (uint32)lua_tointeger(l, 1);

	// 检测该proto是否存在
	if (GetPartnerByProto(partnerProto) != NULL)
		LuaReturn(l, top, 0);

	// 创建一个partner对象
	Partner *p = new Partner();
	if (p == NULL)
		LuaReturn(l, top, 0);

	p->Initialize(sInstanceMgr.GenerateMonsterGuid(), partnerProto, this);
	m_Partners.insert(make_pair(p->GetGUID(), p));

	// 推送给周围的玩家
	if (IsInWorld())
	{
		// 创建给自己
		p->BuildCreationDataBlockForPlayer(this);

		// 创建给周围其他玩家
		if (GetInRangePlayersCount() > 0)
		{
			Player *target = NULL;
			hash_set<Player*  >::iterator itBegin = GetInRangePlayerSetBegin();
			hash_set<Player*  >::iterator itEnd = GetInRangePlayerSetEnd();
			for (; itBegin != itEnd; ++itBegin)
			{
				target = *itBegin;
				if (target == NULL)
					continue; 
				p->BuildCreationDataBlockForPlayer(target);
			}
		}
	}

	Lunar<Partner>::push(l, p, false);

	LuaReturn(l, top, 1);
}

int Player::revokePartner(lua_State *l)
{
	int32 top = lua_gettop(l);
	if (m_Partners.empty())
		LuaReturn(l, top, 0);

	uint32 partnerProto = (uint32)lua_tointeger(l, 1);
	Partner *p = GetPartnerByProto(partnerProto);
	if (p == NULL)
		LuaReturn(l, top, 0);

	uint64 partnerGuid = p->GetGUID();
	uint32 tp = p->GetTypeID();

	m_Partners.erase(partnerGuid);

	if (IsInWorld())
	{
		p->BuildRemoveDataBlockForPlayer(this);
		if (GetInRangePlayersCount() > 0)
		{
			Player *target = NULL;
			hash_set<Player*  >::iterator itBegin = GetInRangePlayerSetBegin();
			hash_set<Player*  >::iterator itEnd = GetInRangePlayerSetEnd();
			for (; itBegin != itEnd; ++itBegin)
			{
				target = *itBegin;
				if (target == NULL)
					continue;
				p->BuildRemoveDataBlockForPlayer(target);
			}
		}
	}

	p->Term();
	SAFE_DELETE(p);

	lua_pushboolean(l, 1);

	LuaReturn(l, top, 1);
}


LUA_IMPL(Player, Player)

LUA_METD_START(Player)
// 从obejct继承的一些函数
L_METHOD(Player, getUInt64Value)
L_METHOD(Player, setUInt64Value)
L_METHOD(Player, getFloatValue)
L_METHOD(Player, setFloatValue)
L_METHOD(Player, isAlive)
L_METHOD(Player, isDead)
L_METHOD(Player, setDeathState)
L_METHOD(Player, isMoving)
L_METHOD(Player, getGuid)
L_METHOD(Player, getObjType)
L_METHOD(Player, isInWorld)
L_METHOD(Player, getMapID)
L_METHOD(Player, setMapID)
L_METHOD(Player, getInstanceID)
L_METHOD(Player, getCurPixelPos)
L_METHOD(Player, setCurPos)
L_METHOD(Player, getCurDirection)
L_METHOD(Player, startMoving)
L_METHOD(Player, stopMoving)
L_METHOD(Player, sendMessageToSet)
L_METHOD(Player, calcuDistance)
L_METHOD(Player, setDirectionToDestObject)
L_METHOD(Player, setDirectionToDestCoord)
L_METHOD(Player, initMapPosData)
// player类自己的函数
L_METHOD(Player, initData)
L_METHOD(Player, getPlayerUtf8Name)
L_METHOD(Player, setPlayerUtf8Name)
L_METHOD(Player, getPlayerGbkName)
L_METHOD(Player, removeFromWorld)
L_METHOD(Player, sendPacket)
L_METHOD(Player, sendServerPacket)
L_METHOD(Player, setPlayerDataSyncFlag)
L_METHOD(Player, releasePartner)
L_METHOD(Player, revokePartner)
LUA_METD_END

LUA_FUNC_START(Player)
L_FUNCTION(Player, newCppPlayerObject)
L_FUNCTION(Player, delCppPlayerObject)
LUA_FUNC_END
