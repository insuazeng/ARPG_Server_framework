#pragma once

#include "BaseLuaScript.h"
#include "ScriptParamArray.h"

class MasterScript : public BaseLuaScript
{
public:
	MasterScript(void);
	virtual ~MasterScript(void);

public:
	static const char* ms_szMasterScriptInitializeFunc;
	static const char* ms_szMasterScriptTerminateFunc;

public:
	virtual bool registLocalLibs();
	virtual bool callInitialize();			// 调用脚本初始化函数
	virtual bool callTerminate();

};

