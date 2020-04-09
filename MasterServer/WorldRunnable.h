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
// WorldRunnable.h
//

#ifndef __WORLDRUNNABLE_H
#define __WORLDRUNNABLE_H
//#include "CommonTypes.h"

#include "threading/UserThread.h"

enum DAYWATCHERSETTINGS
{
	WEEKLY		= 1,
	DAILY		= 2,
	HALFDAILY	= 3,
	MONTHLY		= 4,
	HOURLY		= 5,
	MINUTELY	= 6,
	TWOHOURLY	= 7,
	HALFHOURLY	= 8,
};

class WorldRunnable : public UserThread
{
public:
	WorldRunnable(const int64 threadID);
	void run();

	uint32 minparkdiff;
	uint32 maxparkdiff;
	uint32 totalparkdiff;
	uint32 totalupdate;

	bool ThreadRunning;
	//日常更新相关参数和变量
	time_t currenttime;
	tm local_currenttime;
	// time_t last_daily_reset_time;
	tm local_last_daily_reset_time;
	// time_t last_2hour_reset_time;
	tm local_last_2hour_reset_time;
	// time_t last_halfdaily_reset_time;
	tm local_last_halfdaily_reset_time;
	// time_t last_weekly_reset_time;
	tm local_last_weekly_reset_time;
	// time_t last_hour_reset_time;
	tm local_last_hour_reset_time;
	// time_t last_halfhour_reset_time;
	tm local_last_halfhour_reset_time;
	// time_t last_minute_reset_time;
	tm local_last_minute_reset_time;

	void update_minute();
	void update_hour();
	void update_halfhour();
	void load_settings();
	void update_settings();
	bool has_timeout_expired(tm * now_time, tm * last_time, uint32 timeoutval);
	void update_daily();
	void update_halfdaily();
	void update_2hour();
	void update_weekly();
	void dupe_tm_pointer(tm * returnvalue, tm * mypointer);
	void set_tm_pointers();
	void reload_database();
};

#endif
