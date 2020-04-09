#ifndef _OP_DISPATCHER_H_
#define _OP_DISPATCHER_H_

#include "script/Lunar.h"

class OpDispatcher
{
public:
	OpDispatcher(void);
	~OpDispatcher(void);

public:
	// 导出给lua的接口
	static int regOpcodeTransferDest(lua_State *l);
	static int unregOpcodeTransferDest(lua_State *l);
	static int cleanOpcodeTransfer(lua_State *l);
	static int printOpcodeTransDetail(lua_State *l);
	static int luaTest(lua_State *l);

	LUA_EXPORT(OpDispatcher)

public:
	static map<uint32, uint32> ms_OpcodeTransferDestServer;
};

#endif
