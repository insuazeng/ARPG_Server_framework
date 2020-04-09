#include "stdafx.h"
#include "GameSession.h"

int GameSession::getSessionIndex(lua_State *l)
{
	int32 top = lua_gettop(l);
	
	lua_pushinteger(l, m_SessionIndex);

	LuaReturn(l, top, 1);
}

int GameSession::getNextSceneServerID(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushinteger(l, m_NextSceneServerID);

	LuaReturn(l, top, 1);
}

int GameSession::setNextSceneServerID(lua_State *l)
{
	int32 top = lua_gettop(l);

	uint32 id = (uint32)lua_tointeger(l, 1);
	SetNextSceneServerID(id);

	LuaReturn(l, top, 0);
}

int GameSession::getSrcPlatformID(lua_State *l)
{
	int32 top = lua_gettop(l);

	lua_pushinteger(l, _srcPlatformID);

	LuaReturn(l, top, 1);
}

int GameSession::onSceneLoaded(lua_State *l)
{
	int32 top = lua_gettop(l);
	Player *plr = GetPlayer();

	sLog.outString("玩家%s loading地图完成 准备加入场景%u(%.1f,%.1f)", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetPositionX(), plr->GetPositionY());	
	plr->AddToWorld();

	// 如果玩家并未成功加入场景中
	if ( !plr->IsInWorld() )
	{
		sLog.outError("玩家 %s("I64FMTD") loading地图%u(instance=%u)失败 被断开", plr->strGetGbkNameString().c_str(), plr->GetGUID(), plr->GetMapId(), plr->GetInstanceID());
		LogoutPlayer(false);
		Disconnect();
		lua_pushboolean(l, 0);
		LuaReturn(l, top, 1);
	}

	// 推送玩家创角数据
	pbc_wmessage *msg2002 = sPbcRegister.Make_pbc_wmessage("UserProto002002");
	if (msg2002 == NULL)
	{
		lua_pushboolean(l, 0);
		LuaReturn(l, top, 1);
	}
	pbc_wmessage *creationBuffer = pbc_wmessage_message(msg2002, "playerCreationData");
	if (creationBuffer == NULL)
	{
		lua_pushboolean(l, 0);
		LuaReturn(l, top, 1);
	}

	plr->BuildCreationDataBlockToBuffer(creationBuffer, plr);
	WorldPacket packet(SMSG_002002, 256);
	sPbcRegister.EncodePbcDataToPacket(packet, msg2002);
	uint32 packSize = packet.size();
	SendPacket(&packet);

	DEL_PBC_W(msg2002);
	sLog.outString("玩家 %s 成功加入场景 %u(InstanceID = %u),创角BufferSize=%u", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetInstanceID(), packSize);

	lua_pushboolean(l, 1);
	LuaReturn(l, top, 1);
}

int GameSession::sendServerPacket(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	ByteBuffer *data = *(ByteBuffer**)lua_touserdata(l, 1);
	WorldPacket *pck = dynamic_cast<WorldPacket*>(data);
	if (pck == NULL)
		LuaReturn(l, top, 0);

	if (GetGateServerSocket() == NULL)
	{
		lua_pushboolean(l, 0);
	}
	else
	{
		SendServerPacket(pck);
		lua_pushboolean(l, 1);
	}

	LuaReturn(l, top, 1);
}

LUA_IMPL(GameSession, GameSession)

LUA_METD_START(GameSession)
L_METHOD(GameSession, getSessionIndex)
L_METHOD(GameSession, getNextSceneServerID)
L_METHOD(GameSession, setNextSceneServerID)
L_METHOD(GameSession, getSrcPlatformID)
L_METHOD(GameSession, onSceneLoaded)
L_METHOD(GameSession, sendServerPacket)
LUA_METD_END

LUA_FUNC_START(GameSession)
LUA_FUNC_END
