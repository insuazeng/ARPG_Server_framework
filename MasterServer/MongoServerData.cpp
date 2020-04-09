#include "stdafx.h"
#include "MongoServerData.h"

MongoServerData::MongoServerData(void)
{

}

MongoServerData::~MongoServerData(void)
{
	SetConnection(NULL);
}

void MongoServerData::Initialize(MongoConnector *conn)
{
	SetConnection(conn);

	__LoadServerDatas();
}

void MongoServerData::SaveResetTimeValue()
{
	DBClientConnection *conn = GetClientConnection();
	string tableName = sDBConfLoader.GetMongoTableName("server_datas");

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_weekly_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_weekly_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_dailies_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_daily_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_halfdaily_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_halfdaily_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_2hour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_2hour_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_hour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_hour_reset_time)), true);

	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_halfhour_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_halfhour_reset_time)), true);
	
	conn->update(tableName.c_str(), mongo::Query(BSON("setting_id" << "last_minute_reset_time")), 
				BSON("$set" << BSON("setting_value" << (uint32)last_minute_reset_time)), true);
}

void MongoServerData::EnsureAllIndex()
{

}

void MongoServerData::__LoadServerDatas()
{
	BSONObj bsonObj;
	
	// 每周更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_weekly_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化日常更新..\n");
		last_weekly_reset_time = 0;
	}
	else
	{
		last_weekly_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 每日更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_dailies_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化日常更新..\n");
		last_daily_reset_time = 0;
	}
	else
	{
		last_daily_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 半日更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_halfdaily_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化12小时更新..\n");
		last_halfdaily_reset_time = 0;
	}
	else
	{
		last_halfdaily_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 每2小时更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_2hour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化2小时更新..\n");
		last_2hour_reset_time = 0;
	}
	else
	{
		last_2hour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 每1小时更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_hour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化1小时更新..\n");
		last_hour_reset_time = 0;
	}
	else
	{
		last_hour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 每半小时更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_halfhour_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化0.5小时更新..\n");
		last_halfhour_reset_time = 0;
	}
	else
	{
		last_halfhour_reset_time = bsonObj["setting_value"].numberLong();
	}

	// 每分钟更新
	bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("server_datas").c_str(), mongo::Query(BSON("setting_id" << "last_minute_reset_time")));
	if (bsonObj.isEmpty())
	{
		cLog.Success("逻辑线程", "初始化0.5小时更新..\n");
		last_minute_reset_time = 0;
	}
	else
	{
		last_minute_reset_time = bsonObj["setting_value"].numberLong();
	}
}
