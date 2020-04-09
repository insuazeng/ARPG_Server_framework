#include "stdafx.h"
#include "OpDispatcher.h"

map<uint32, uint32> OpDispatcher::ms_OpcodeTransferDestServer;

OpDispatcher::OpDispatcher(void)
{

}

OpDispatcher::~OpDispatcher(void)
{

}

int OpDispatcher::regOpcodeTransferDest(lua_State *l)
{
	uint32 op = (uint32)lua_tonumber(l, 1);
	uint32 dst = (uint32)lua_tonumber(l, 2);

	OpDispatcher::ms_OpcodeTransferDestServer.insert(make_pair(op, dst));

	return 0;
}

int OpDispatcher::unregOpcodeTransferDest(lua_State *l)
{
	return 0;
}

int OpDispatcher::cleanOpcodeTransfer(lua_State *l)
{
	return 0;
}

int OpDispatcher::printOpcodeTransDetail(lua_State *l)
{
	map<uint32, uint32>::iterator it = OpDispatcher::ms_OpcodeTransferDestServer.begin();
	for (; it != OpDispatcher::ms_OpcodeTransferDestServer.end(); ++it)
	{
		sLog.outString("Opcode = %u, DestServer = %u", it->first, it->second);
	}
	return 0;
}

int OpDispatcher::luaTest(lua_State *l)
{
	/*int32 top = lua_gettop(l);
	uint32 tp = 0;
	if (lua_istable(l, 1))
	{
		uint32 arrSize = lua_objlen(l, 1);

		for (int i = 1; i <= arrSize; ++i)
		{
			lua_rawgeti(l, 1, i);
			tp = lua_type(l, -1);
			uint32 len = lua_objlen(l, -1);

			for (int j = 1; j <= len; ++j)
			{
				lua_rawgeti(l, -1, j);
				tp = lua_type(l, -1);
				lua_pop(l, 1);
			}
			lua_pop(l, 1);*/

			/*top = lua_gettop(l);
			lua_pushnil(l);

			lua_next(l, top);
			tp = lua_type(l, -1);
			lua_pop(l, 1);
			
			lua_next(l, top);
			tp = lua_type(l, -1);
			lua_pop(l, 1);
			
			lua_next(l, top);
			tp = lua_type(l, -1);
			lua_pop(l, 1);

			lua_next(l, top);
			tp = lua_type(l, -1);
			lua_pop(l, 1);
		}
	}*/
	return 0;
}

LUA_IMPL(OpDispatcher, OpDispatcher)

LUA_METD_START(OpDispatcher)
LUA_METD_END

LUA_FUNC_START(OpDispatcher)
L_FUNCTION(OpDispatcher, regOpcodeTransferDest)
L_FUNCTION(OpDispatcher, unregOpcodeTransferDest)
L_FUNCTION(OpDispatcher, cleanOpcodeTransfer)
L_FUNCTION(OpDispatcher, printOpcodeTransDetail)
L_FUNCTION(OpDispatcher, luaTest)
LUA_FUNC_END
