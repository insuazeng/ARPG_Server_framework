#pragma once

// #define EPOLL_TEST

#include "CommonTypes.h"
#include "WorldPacket.h"
#include "log/NGLog.h"
#include "log/Log.h"
#include "database/mysql/DatabaseEnv.h"
#include "database/mongodb/MongoConnector.h"
#include "database/mongodb/MongoTable.h"
#include "network/Network.h"
#include "config/ConfigEnv.h"
#include "PbcDataRegister.h"
#include "BufferPool.h"

#include "PbDataLoader.h"
#include "DBConfigLoader.h"

#include "AccountCache.h"
#include "InformationCore.h"

extern const uint64 ILLEGAL_DATA_64;
extern const uint32 ILLEGAL_DATA_32;

// extern MySQLDatabase *Database_Data;
extern MySQLDatabase *Database_Log;

extern MongoConnector *MongoDB_Data;

// 生成json的配置文件读取路径
string strMakeJsonConfigFullPath(const char* fileName);
// 生成lua的配置文件读取路径
string strMakeLuaConfigFullPath(const char* fileName);
