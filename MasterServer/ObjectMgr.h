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

#ifndef _OBJECTMGR_H
#define _OBJECTMGR_H

#include "threading/RWLock.h"

class CPlayerServant;

/************************************************************************/
/* 主要用于存放物体（物品，人物等）相关的静态表                         */
/************************************************************************/
class SERVER_DECL ObjectMgr : public Singleton < ObjectMgr >
{
public:
	ObjectMgr();
	~ObjectMgr();

public:
    void Reload();

	void SetHighestGuids();
	uint32 GenerateLowGuid(uint32 guidhigh);

	void ResetDailies();//重置日常事件
	void ResetTwoHours();//重置2小时事件循环任务重置
	
protected:
	Mutex m_guidGenMutex;
	uint32 m_hiItemGuid;
	std::list<uint32> m_itemidbuf;

public:
	int Init(void);
	int LoadBaseData(void);
};

#define objmgr ObjectMgr::GetSingleton()

#endif

