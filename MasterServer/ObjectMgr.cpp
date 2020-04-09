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

#include "stdafx.h"
#include "ObjectMgr.h"
#include "ObjectPoolMgr.h"

#ifdef _WIN64
#define ToLower(yourstring) transform (yourstring.begin(),yourstring.end(), yourstring.begin(), tolower);
#else
#define ToLower(str) for(unsigned int i=0;i<str.size();i++) tolower(str[i]);
#endif
//add  by tangquan 这个函数不要用，在linxu没transform函数。
#define ToUpper(yourstring) transform (yourstring.begin(),yourstring.end(), yourstring.begin(), towupper);
//end

bool Rand(float chance)
{
	int32 val = RandomUInt(10000);
	int32 p = int32(chance * 100.0f);
	return p >= val;
}

bool Rand(uint32 chance)
{
	int32 val = RandomUInt(10000);
	int32 p = int32(chance * 100);
	return p >= val;
}

bool Rand(int32 chance)
{
	int32 val = RandomUInt(10000);
	int32 p = chance * 100;
	return p >= val;
}

ObjectMgr::ObjectMgr() :  m_hiItemGuid( 0 )
{
}

ObjectMgr::~ObjectMgr()
{
	sLog.outString("Deleting ObjectMgr Buffers...");
}

uint32 ObjectMgr::GenerateLowGuid( uint32 guidhigh )
{
	uint32 ret = 0;
    switch ( guidhigh )
    {
    default:
        break;
    }
	return ret;
}

void ObjectMgr::SetHighestGuids()
{

}

int ObjectMgr::Init(void)
{
	return 1;
}

int ObjectMgr::LoadBaseData(void)
{
	return 1;
}

void  ObjectMgr::ResetDailies() //重置日常事件
{
	// 遍历所有玩家，重置其日常任务
	/*for (;;)
	{
		//重置活动任务 add by liukai 2012-9-14
		pplayer->RemoveActivityQuest();
		//end
	}*/
}

void ObjectMgr::ResetTwoHours()//重置2小时事件循环任务重置
{

}

void ObjectMgr::Reload()
{

}






