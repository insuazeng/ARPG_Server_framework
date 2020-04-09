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
#include "MersenneTwister.h"

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

ObjectMgr::ObjectMgr() : m_crFindPathFrequencyCounter(0),
						m_crFindPathTotalCostTime(0)
{
}

ObjectMgr::~ObjectMgr()
{
	cLog.Notice("ObjectMgr", "Deleting m_MonsterProtoList ...");
	for (map<uint32, tagMonsterProto*>::iterator it = m_MonsterProtoList.begin(); it != m_MonsterProtoList.end(); ++it)
	{
		delete (*it).second;
	}
	m_MonsterProtoList.clear();
}

int ObjectMgr::Init(void)
{
	
	return 1;
}

int ObjectMgr::LoadBaseData(void)
{	
	LoadMonsterProto();
	return 1;
}

void ObjectMgr::LoadMonsterProto()
{
	// 怪物原型分布配置在地图中,读取每张地图上的怪物配置
	MapInfoConfigs::iterator itBegin = sSceneMapMgr.GetMapInfoBegin();
	MapInfoConfigs::iterator itEnd = sSceneMapMgr.GetMapInfoEnd();
	for (; itBegin != itEnd; ++itBegin)
	{
		const uint32 mapID = itBegin->first;
		char szFileName[128] = { 0 };
		snprintf(szFileName, 128, "Config/JsonConfigs/monster_proto/%u.json", mapID);
		Json::Value j_root;
		Json::Reader j_reader;
		int32 ret = JsonFileReader::ParseJsonFile(szFileName, j_reader, j_root);
		if (ret == -2)
		{
#if defined(_WIN32) || defined(_WIN64)
			ASSERT(false && "怪物表配置出错!");
#else
			sLog.outError("怪物原型表文件 %s 解析失败", szFileName);
			return ;
#endif
		}

		tagMonsterProto * pn = NULL;
		uint32 monsterID = 0;

		for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
		{
			monsterID = (*it)["id"].asUInt();
			pn = GetMonsterProto( monsterID );
			if ( pn == NULL )
			{
				pn = new tagMonsterProto();
				pn->m_ProtoID = monsterID;
				m_MonsterProtoList[monsterID] = pn;
			}

			pn->m_MonsterName = (*it)["monster_name"].asString();
			// pn->m_CreatureTitle = (*it)["id"].asUInt();
			pn->m_ResModelId = (*it)["model_id"].asUInt();
			pn->m_RespawnTime = (*it)["relive_interval"].asUInt();
			pn->m_Level = (*it)["level"].asUInt();
			pn->m_Exp = (*it)["exp"].asUInt();
			pn->m_HP = (*it)["hp"].asUInt();
			pn->m_CreatureModelSize = (*it)["model_size"].asUInt();
			pn->m_BaseMoveSpeed = (*it)["move_speed"].asFloat();
		}
	}

	cLog.Success( "数据库", "怪物表读取成功 %d 条", m_MonsterProtoList.size() );
}

void  ObjectMgr::ResetDailies() //重置日常事件
{

}

void ObjectMgr::ResetTwoHours()//重置2小时事件循环任务重置
{

}

tagMonsterProto* ObjectMgr::GetMonsterProto(uint32 entry)
{
	std::map<uint32, tagMonsterProto*>::iterator iter = m_MonsterProtoList.find(entry);
	if ( iter != m_MonsterProtoList.end() )
	{
		return iter->second;
	}
	return NULL;
}

void ObjectMgr::Reload()
{

}

