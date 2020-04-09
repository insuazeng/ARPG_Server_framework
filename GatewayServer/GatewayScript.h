#ifndef _GATEWAY_SCRIPT_H_
#define _GATEWAY_SCRIPT_H_

#include "BaseLuaScript.h"
#include "ScriptParamArray.h"

class GatewayScript : public BaseLuaScript
{
public:
	GatewayScript(void);
	virtual ~GatewayScript(void);

public:
	static const char* ms_szGatewayScriptInitializeFunc;
	static const char* ms_szGatewayScriptTerminateFunc;

public:
	virtual bool registLocalLibs();
	virtual bool callInitialize();			// 调用脚本初始化函数
	virtual bool callTerminate();
};

#endif


