#ifndef STATISTICSSYSTEM_HEAD_FILE
#define STATISTICSSYSTEM_HEAD_FILE

class Instance;
class Player;

class StatisticsSystem : public Singleton<StatisticsSystem>
{
public:
	StatisticsSystem();
	~StatisticsSystem();

private:
	tm m_tmLastOnlineUpdate;          //在线人数更新
	tm m_tmStatDataTime;              //游戏时段统计
	bool m_bNewRecord;                //Insert还是Update

public:
	void Init();                      //初始化
	void DailyEvent();                //日常事件
	void WeeklyEvent();               //一周事件
	void SaveAll();
	void Update(time_t diff);
	
	void OnShutDownServer();//在关闭之前统计一些数据

	void OnInstanceCreated(Instance *in);
	void OnInstancePassed(Instance *in);
	void OnPlayerLeaveUndercity(Instance *in, Player *plr);
};

#define sStatisticsSystem StatisticsSystem::GetSingleton()

#endif
