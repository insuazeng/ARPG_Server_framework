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

#include "network/SocketEngine.h"
#include "threading/UserThreadMgr.h"
#include "threading/Timer.h"
#include "WorldRunnable.h"

// 每秒update50次
#define WORLD_UPDATE_DELAY 20

WorldRunnable::WorldRunnable(const int64 threadID) : UserThread(THREADTYPE_WORLDRUNNABLE, threadID)
{
	SetThreadState(THREADSTATE_AWAITING);
	m_threadStartTime = time(NULL);
	delete_after_use = false;
	sThreadMgr.AddThread(this);
}

WorldRunnable::~WorldRunnable()
{
	if(delete_after_use)
		return; // we're deleted manually
	
	delete_after_use = true;
	sThreadMgr.RemoveThread(this);
}

void WorldRunnable::run()
{
	uint64 LastWorldUpdate = getMSTime64();

	while(m_threadState != THREADSTATE_TERMINATE)
	{
		// Provision for pausing this thread.
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

		//calce time passed
		uint64 execution_start = getMSTime64();

		sInfoCore.TimeoutSockets(execution_start - LastWorldUpdate);
		sInfoCore.UpdateRegisterPacket();
		sSocketDeleter.Update();

#ifndef EPOLL_TEST
		sAccountMgr.RefreshIPBannedList();
#endif		

		uint64 now = getMSTime64();
		uint32 diff = now - LastWorldUpdate;
		LastWorldUpdate = now;
		
		UNIXMSTIME = now;
		UNIXTIME = time(NULL);

		if(m_threadState == THREADSTATE_TERMINATE)
			break;
		m_threadState = THREADSTATE_SLEEPING;

		// 固定每秒帧数
		if(diff < WORLD_UPDATE_DELAY)
		{
			Sleep(WORLD_UPDATE_DELAY - diff);
		}
		else
		{
			Sleep(1);
		}
	}

	sLog.outString("WorldRunnable Thread(id=%u) Stop Running, will exit..", uint32(m_threadID));
}
