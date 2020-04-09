#include "stdafx.h"
#include "MasterLuaTimer.h"

MasterLuaTimer::MasterLuaTimer(void)
{

}

MasterLuaTimer::~MasterLuaTimer(void)
{

}

void MasterLuaTimer::onTimeOut(uint32 *pEventIDs, const uint32 eventCount)
{
	// 调用lua的定时器函数
	if (eventCount == 0)
		return ;

	ScriptParamArray params;
	if (eventCount == 1)
	{
		params << pEventIDs[0];
	}
	else
	{
		params.AppendArrayData(pEventIDs, eventCount);

#ifdef _WIN64
		ASSERT(params[0].getArraySize() == eventCount);
#endif // _WIN64

	}
	LuaScript.Call("OnEventTimeOut", &params, NULL);
}
