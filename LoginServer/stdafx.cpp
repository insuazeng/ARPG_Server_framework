#include "stdafx.h"

string strMakeJsonConfigFullPath(const char* fileName)
{
	string f = fileName;
	string ret = "Config/JsonConfigs/" + f;
	return ret;
}

string strMakeLuaConfigFullPath(const char* fileName)
{
	string f = fileName;
	string ret = "Config/LuaConfigs/" + f;
	return ret;
}
