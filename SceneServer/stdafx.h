#pragma once

#ifndef SCENE_SERVER
#define SCENE_SERVER
#endif

#include "CommonTypes.h"

#include "log/NGLog.h"
#include "log/Log.h"
#include "log/TextLogger.h"
#include "network/Network.h"
#include "threading/Timer.h"
#include "threading/UserThreadMgr.h"
#include "database/mysql/DatabaseEnv.h"
#include "config/ConfigEnv.h"
#include "config/JsonFileReader.h"

#include "BufferPool.h"
#include "WorldPacket.h"
#include "MersenneTwister.h"
#include "PbcDataRegister.h"

#include "GameCommonDefine.h"
#include "PlayerCommonDef.h"
#include "BaseObject.h"
#include "SynchronizedObject.h"
#include "EventableObject.h"
#include "EventMgr.h"
#include "DBConfigLoader.h"
#include "PbDataLoader.h"
#include "LanguageMgr.h"

#include "SceneScript.h"
#include "Park.h"
#include "ObjectMgr.h"
#include "GateCommHandler.h"
#include "GameSession.h"
#include "Player.h"
#include "Monster.h"
#include "Partner.h"
#include "SceneMapMgr.h"
#include "MapMgr.h"
#include "InstanceMgr.h"
#include "StatisticsSystem.h"

extern const uint64 ILLEGAL_DATA_64;
extern const uint32 ILLEGAL_DATA_32;

//extern MySQLDatabase * Database_Game;
//#define GameDatabase (*Database_Game)

//extern MySQLDatabase *Database_World;
//#define WorldDatabase (*Database_World)

extern MySQLDatabase * Database_Log;
#define LogDatabase (*Database_Log)

extern SceneScript* g_pSceneScript;
#define LuaScript (*g_pSceneScript)

// 生成json的配置文件读取路径
string strMakeJsonConfigFullPath(const char* fileName);
// 生成lua的配置文件读取路径
string strMakeLuaConfigFullPath(const char* fileName);


