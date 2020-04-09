#pragma once

#ifndef GATEWAY_SERVER
#define GATEWAY_SERVER
#endif

#include "CommonTypes.h"
#include "Singleton.h"
#include "WorldPacket.h"
#include "BufferPool.h"
#include "LanguageMgr.h"
#include "network/Network.h"
#include "threading/Timer.h"
#include "log/NGLog.h"
#include "log/Log.h"
#include "PbcDataRegister.h"

#include "ServerCommonDef.h"
#include "PbDataLoader.h"
#include "GatewayScript.h"

extern const uint64 ILLEGAL_DATA_64;
extern const uint32 ILLEGAL_DATA_32;

extern GatewayScript* g_pGatewayScript;
#define LuaScript (*g_pGatewayScript)
