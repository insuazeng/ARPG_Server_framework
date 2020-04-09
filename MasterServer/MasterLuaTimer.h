#ifndef _MASTER_LUA_TIMER_H_
#define _MASTER_LUA_TIMER_H_

#include "Singleton.h"
#include "script/BaseScriptTimer.h"

class MasterLuaTimer : public BaseScriptTimer, public Singleton<MasterLuaTimer>
{
public:
	MasterLuaTimer(void);
	virtual ~MasterLuaTimer(void);

public:
	virtual void onTimeOut(uint32 *pEventIDs, const uint32 eventCount);
};

#define sMasterLuaTimer MasterLuaTimer::GetSingleton()

#endif
