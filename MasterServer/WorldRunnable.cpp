/****************************************************************************
 *
 * General Object Type File
 * Copyright (c) 2007 Team Ascent
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
 * License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
 * 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
 *
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
 * ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
 * ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 */

//
// WorldRunnable.cpp
//
#include "stdafx.h"

#include "WorldRunnable.h"
#include "GateCommHandler.h"
#include "PlayerMgr.h"
#include "Park.h"
#include "MongoServerData.h"

bool g_sReloadStaticDataBase = false;

#define WORLD_UPDATE_DELAY 50				// 每秒update20次

WorldRunnable::WorldRunnable(const int64 threadID) : UserThread(THREADTYPE_WORLDRUNNABLE, threadID)
{
	minparkdiff=100;
	maxparkdiff=0;
	totalparkdiff=0;
	totalupdate = 0;
	delete_after_use = false;
    //srand(time(NULL));    // 此处无效 by eeeo@2011.8.27
}

void WorldRunnable::run()
{
	ThreadRunning = true;
#ifdef _WIN64
	//::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL );
#endif
	SetThreadName("WorldRunnable (hohogame non inistance)");
	cLog.Success("WorldRunnable", "主逻辑处理线程启动.");

	uint64 LastWorldUpdate = getMSTime64();
	uint64 LastSessionsUpdate = getMSTime64();

	currenttime = UNIXTIME;
	dupe_tm_pointer(localtime(&currenttime), &local_currenttime);
	
	load_settings();
	set_tm_pointers();
    srand(time(NULL));                      // 设置随机数种子

	while(m_threadState != THREADSTATE_TERMINATE)
	{

		if(m_threadState == THREADSTATE_PAUSED)
		{
			while(m_threadState == THREADSTATE_PAUSED)
			{
				Sleep(200);
			}
		}
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		m_threadState = THREADSTATE_BUSY;

		currenttime = UNIXTIME;
		dupe_tm_pointer(localtime(&currenttime), &local_currenttime);
		if(has_timeout_expired(&local_currenttime, &local_last_daily_reset_time, DAILY))
		{
			update_daily();
			update_settings();
		}
		if (has_timeout_expired(&local_currenttime, &local_last_halfdaily_reset_time, HALFDAILY))
		{
			update_halfdaily();
			update_settings();
		}
		if(has_timeout_expired(&local_currenttime, &local_last_2hour_reset_time, TWOHOURLY))
		{
			update_2hour();
			update_settings();
		}
		if(has_timeout_expired(&local_currenttime, &local_last_2hour_reset_time, WEEKLY))
		{
			update_weekly();
			update_settings();
		}
		if(has_timeout_expired(&local_currenttime, &local_last_hour_reset_time, HOURLY))
		{
			update_hour();
			update_settings();
		}
		if (has_timeout_expired(&local_currenttime, &local_last_halfhour_reset_time, HALFHOURLY))
		{
			update_halfhour();
			update_settings();
		}
		if(has_timeout_expired(&local_currenttime, &local_last_minute_reset_time, MINUTELY))
		{
			update_minute();
			update_settings();
		}
        sPark.SetPassMinuteToday(local_currenttime.tm_hour * 60 + local_currenttime.tm_min);

        if ( g_sReloadStaticDataBase )
        {
            reload_database();
        }

		
		//calce time passed
		uint64 now,execution_start;
		now=getMSTime64();
		execution_start = now;
		uint32 diff = now - LastWorldUpdate;
		LastWorldUpdate=now;
		sPark.Update(diff);
		now=getMSTime64();

		//计算主逻辑的update时间和次数，统计最长和最短时间
		totalparkdiff+=diff;
		totalupdate++;
		minparkdiff=diff<minparkdiff?diff:minparkdiff;
		maxparkdiff = diff>maxparkdiff?diff:maxparkdiff;

		//数据包处理的update
		diff=now-LastSessionsUpdate;
		LastSessionsUpdate=now;
		
		sInfoCore.UpdateServerRecvPackets();
		sInfoCore.UpdateServerSendPackets();

		sPlayerMgr.UpdatePlayerServants(diff);
		sPlayerMgr.UpdateBlockChat( diff );
		
		now = getMSTime64();
		sMasterLuaTimer.Update(now);

		//计算整个update的时间。
		//we have to wait now 
		now = getMSTime64();
		diff=now-execution_start;//time used for updating 
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		m_threadState = THREADSTATE_SLEEPING;
		/*This is execution time compensating system
			if execution took more than default delay 
			no need to make this sleep*/
		if(diff<WORLD_UPDATE_DELAY)
			Sleep(WORLD_UPDATE_DELAY-diff);
	}
	ThreadRunning = false;
}

void WorldRunnable::dupe_tm_pointer(tm * returnvalue, tm * mypointer)
{
	memcpy(mypointer, returnvalue, sizeof(tm));
}

void WorldRunnable::set_tm_pointers()
{
	dupe_tm_pointer(localtime(&(sMongoServerData.last_2hour_reset_time)), &local_last_2hour_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_daily_reset_time)), &local_last_daily_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_halfdaily_reset_time)), &local_last_halfdaily_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_weekly_reset_time)),&local_last_weekly_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_hour_reset_time)), &local_last_hour_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_halfhour_reset_time)), &local_last_halfhour_reset_time);
	dupe_tm_pointer(localtime(&(sMongoServerData.last_minute_reset_time)), &local_last_minute_reset_time);
	//dupe_tm_pointer(localtime(&last_eventid_time), &local_last_eventid_time);
}

void WorldRunnable::load_settings()
{
	sMongoServerData.Initialize(MongoDB_Game);
}

void WorldRunnable::update_settings()
{
	sMongoServerData.SaveResetTimeValue();
}

bool WorldRunnable::has_timeout_expired(tm * now_time, tm * last_time, uint32 timeoutval)
{
	switch(timeoutval)
	{
	case WEEKLY:
		{
			return ( (now_time->tm_yday / 7) != (last_time->tm_yday / 7) );
		}
	case MONTHLY:
		return (now_time->tm_mon != last_time->tm_mon);
	case HOURLY:
		return ((now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case DAILY:
		return (now_time->tm_yday != last_time->tm_yday);
	case HALFDAILY:
		return ((now_time->tm_hour % 12 == 0) && (now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case MINUTELY:
		return ((now_time->tm_min != last_time->tm_min) || (now_time->tm_hour != last_time->tm_hour) || (now_time->tm_yday != last_time->tm_yday) );
	case TWOHOURLY:
		return ((now_time->tm_hour!=last_time->tm_hour)&&(now_time->tm_hour%2==0) || (now_time->tm_yday != last_time->tm_yday) );
	case HALFHOURLY:
		return ((now_time->tm_min != last_time->tm_min) && (now_time->tm_min % 30 == 0));
	}
	return false;
}

void WorldRunnable::update_daily()
{
	cLog.Notice("逻辑线程", "运行日常任务重置...");
	// 日常清理放到玩家登陆的时候判断

	objmgr.ResetDailies();
	sMongoServerData.last_daily_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_daily_reset_time)), &local_last_daily_reset_time);

	cLog.Notice("逻辑线程", "日常信息重置完毕,现在时间%d年%d月%d日...",local_last_daily_reset_time.tm_year+1900,local_last_daily_reset_time.tm_mon+1,local_last_daily_reset_time.tm_mday);
}

void WorldRunnable::update_halfdaily()
{
	cLog.Notice("逻辑线程", "运行12小时事件重置...");

	sMongoServerData.last_halfdaily_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_halfdaily_reset_time)), &local_last_halfdaily_reset_time);

	cLog.Notice("逻辑线程", "12小时信息重置完毕,现在时间%d:%d:%d...",local_last_halfdaily_reset_time.tm_hour,local_last_halfdaily_reset_time.tm_min,local_last_halfdaily_reset_time.tm_sec);
}

void WorldRunnable::update_2hour()
{
	cLog.Notice("逻辑线程", "运行2小时事件重置...");
	sMongoServerData.last_2hour_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_2hour_reset_time)), &local_last_2hour_reset_time);
	cLog.Notice("逻辑线程", "2小时信息重置完毕,现在时间%d:%d:%d...",local_last_2hour_reset_time.tm_hour,local_last_2hour_reset_time.tm_min,local_last_2hour_reset_time.tm_sec);
}

void WorldRunnable::update_hour()
{
	cLog.Notice("逻辑线程", "运行1小时事件重置...");
	sMongoServerData.last_hour_reset_time = UNIXTIME;

	dupe_tm_pointer(localtime(&(sMongoServerData.last_hour_reset_time)), &local_last_hour_reset_time);
	cLog.Notice("逻辑线程", "1小时信息重置完毕,现在时间%d:%d:%d...",local_last_hour_reset_time.tm_hour,local_last_hour_reset_time.tm_min,local_last_hour_reset_time.tm_sec);
}

void WorldRunnable::update_halfhour()
{
	cLog.Notice("逻辑线程", "运行30分钟事件重置...");
	sMongoServerData.last_halfhour_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_halfhour_reset_time)), &local_last_halfhour_reset_time);
	
	cLog.Notice("逻辑线程", "30分钟信息重置完毕,现在时间%d:%d:%d...",local_last_halfhour_reset_time.tm_hour,local_last_halfhour_reset_time.tm_min,local_last_halfhour_reset_time.tm_sec);
}

void WorldRunnable::update_minute()
{
	// cLog.Notice("逻辑线程", "运行1分钟事件重置");
	sMongoServerData.last_minute_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_minute_reset_time)), &local_last_minute_reset_time);
	// cLog.Notice("逻辑线程", "1分钟信息重置完毕,现在时间%d:%d:%d...",local_last_minute_reset_time.tm_hour,local_last_minute_reset_time.tm_min,local_last_minute_reset_time.tm_sec);
}

void WorldRunnable::update_weekly()
{
	cLog.Notice("逻辑线程", "运行1周事件重置...");
	sMongoServerData.last_weekly_reset_time = UNIXTIME;
	dupe_tm_pointer(localtime(&(sMongoServerData.last_weekly_reset_time)), &local_last_weekly_reset_time);
	cLog.Notice("逻辑线程", "1周信息重置完毕,现在时间%d年%d月%d日...",local_last_weekly_reset_time.tm_year+1900,local_last_weekly_reset_time.tm_mon+1,local_last_weekly_reset_time.tm_mday);
}

void WorldRunnable::reload_database()
{
    g_sReloadStaticDataBase = false;

/*#ifdef _DEBUG
	for(HM_NAMESPACE::hash_set<std::string>::iterator itr = sLang.m_missingkeys.begin();itr!=sLang.m_missingkeys.end();++itr)
	{
		Database_World->Execute("INSERT INTO language VALUES(0,'%s','%s(未设置)','未设置');",(*itr).c_str(),(*itr).c_str());
	};
	sLang.m_missingkeys.clear();
#endif*/   

	uint64 start_time = getMSTime64();
	sPark.LoadLanguage();
	objmgr.Reload();
	sDBConfLoader.Reload();
	sPark.LoadSceneMapControlConfig(true);

}
