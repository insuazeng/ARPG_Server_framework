#ifndef _MONGO_SERVER_DATA_H_
#define _MONGO_SERVER_DATA_H_

class MongoTable;
class MongoConnector;

// 针对"server_datas"表的读写
class MongoServerData : public MongoTable, public Singleton<MongoServerData>
{
public:
	MongoServerData(void);
	~MongoServerData(void);

public:
	void Initialize(MongoConnector *conn);
	void SaveResetTimeValue();

public:
	virtual void EnsureAllIndex(void);

private:
	void __LoadServerDatas();

public:
	// 一些服务器持有的变量
	time_t last_daily_reset_time;
	time_t last_2hour_reset_time;
	time_t last_halfdaily_reset_time;
	time_t last_weekly_reset_time;
	time_t last_hour_reset_time;
	time_t last_halfhour_reset_time;
	time_t last_minute_reset_time;
};

#define sMongoServerData MongoServerData::GetSingleton()

#endif

