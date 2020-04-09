#ifndef _SCENE_LUA_TIMER_H_
#define _SCENE_LUA_TIMER_H_

#include "Singleton.h"
#include "script/BaseScriptTimer.h"

class SceneLuaTimer : public BaseScriptTimer, public Singleton<SceneLuaTimer>
{
public:
	SceneLuaTimer(void);
	virtual ~SceneLuaTimer(void);

public:
	virtual void onTimeOut(uint32 *pEventIDs, const uint32 eventCount);
};

#define sSceneLuaTimer SceneLuaTimer::GetSingleton()

#endif

