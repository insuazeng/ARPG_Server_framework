#include "stdafx.h"
#include "Partner.h"

LUA_IMPL(Partner, Partner)

LUA_METD_START(Partner)
// 继承自Object类的函数
L_METHOD(Monster, getUInt64Value)
L_METHOD(Monster, setUInt64Value)
L_METHOD(Monster, getFloatValue)
L_METHOD(Monster, setFloatValue)
L_METHOD(Partner, getGuid)
LUA_METD_END

LUA_FUNC_START(Partner)
LUA_FUNC_END
