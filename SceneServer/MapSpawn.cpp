#include "stdafx.h"
#include "MapSpawn.h"

MapSpawn::MapSpawn(uint32 mapid, MapInfo * inf)
{
	memset(spawns, 0, sizeof(CellSpawns*) * _sizeX);
	_mapId = mapid;

	//new stuff Load Spawns
	LoadSpawns(false, inf);
	PrintCreatureSpawnCounter();
}


MapSpawn::~MapSpawn(void)
{
	cLog.Notice("MapSpawn", "~MapSpawn %u", this->_mapId);

	for(uint32 x=0;x<_sizeX;x++)
	{
		if(spawns[x])
		{
			for(uint32 y=0;y<_sizeY;y++)
			{
				if(spawns[x][y])
				{	
					CellSpawns * sp=spawns[x][y];
					for(vecMonsterBornList::iterator i = sp->monsterBorns.begin();i!=sp->monsterBorns.end();i++)
					{
						delete (*i);
					}

					delete sp;
					spawns[x][y]=NULL;
				}
			}
			delete [] spawns[x];
		}
	}
}

CellSpawns* MapSpawn::GetSpawnsList(uint32 cellx,uint32 celly)
{
	ASSERT(cellx < _sizeX);
	ASSERT(celly < _sizeY);
	if(spawns[cellx]==NULL) 
		return NULL;

	return spawns[cellx][celly];
}

CellSpawns* MapSpawn::GetSpawnsListAndCreate(uint32 cellx, uint32 celly)
{
	ASSERT(cellx < _sizeX);
	ASSERT(celly < _sizeY);
	if(spawns[cellx]==NULL)
	{
		spawns[cellx] = new CellSpawns*[_sizeX];
		memset(spawns[cellx],0,sizeof(CellSpawns*)*_sizeY);
	}

	if(spawns[cellx][celly] == 0)
		spawns[cellx][celly] = new CellSpawns;
	return spawns[cellx][celly];
}

void MapSpawn::LoadSpawns(bool reload, MapInfo *mapInfo)
{
	if(reload)
	{
		for(uint32 x=0;x<_sizeX;x++)
		{
			if (spawns[x] == NULL)
				continue;
			for(uint32 y=0;y<_sizeY;y++)
			{
				if(spawns[x][y])
				{	
					CellSpawns * sp=spawns[x][y];
					for(vecMonsterBornList::iterator i = sp->monsterBorns.begin(); i != sp->monsterBorns.end(); i++)
					{
						delete (*i);
					}
					delete sp;
					spawns[x][y]=NULL;
				}
			}
		}
	}

	Json::Value j_root;
	Json::Reader j_reader;
	int32 ret = JsonFileReader::ParseJsonFile(strMakeJsonConfigFullPath("monster_born/monster_born.json").c_str(), j_reader, j_root);
	if (ret != 0)
		return ;

	for (Json::Value::iterator it = j_root.begin(); it != j_root.end(); ++it)
	{
		const uint32 mapID = (*it)["map_id"].asUInt();
		if (mapID != this->_mapId)
			continue;

		const uint32 monsterID = (*it)["monster_id"].asUInt();
		uint32 confID = (*it)["id"].asUInt();

		tagMonsterProto * proto = objmgr.GetMonsterProto(monsterID);
		if (proto == NULL)
		{
			sLog.outError("A wrong monster spawn point has been detected conf_id=%u,monster_id=%u", confID, monsterID);
			continue;
		}

		// 出生数量为0 则不理会
		if ((*it)["born_count"].asInt() <= 0)
		{
			sLog.outError("A wrong monster born count(<=0) has been detected conf_id=%u,monster_id=%u", confID, monsterID);
			continue; 
		}
		
		tagMonsterBornConfig *conf = new tagMonsterBornConfig();
		conf->conf_id = (*it)["id"].asUInt();
		conf->proto_id = (*it)["monster_id"].asUInt();
		conf->born_center_pos.m_fX = (*it)["born_center_x"].asFloat();
		conf->born_center_pos.m_fY = (*it)["born_center_y"].asFloat();
		conf->born_radius = (*it)["born_radius"].asFloat();

		conf->born_count = (*it)["born_count"].asUInt();
		if ((*it).isMember("born_direction"))
		{
			conf->born_direction = (*it)["born_direction"].asUInt();
		}
		else
		{
			conf->born_direction = 0xFF;
		}
		conf->rand_move_percentage = (*it)["rand_move_percent"].asUInt();
		conf->active_ack_percentage = (*it)["active_ack_percent"].asUInt();

		uint32 cellx=CellHandler<MapMgr>::GetPosX(conf->GetSpawnGridPosX());
		uint32 celly=CellHandler<MapMgr>::GetPosY(conf->GetSpawnGridPosY());

		if(spawns[cellx]==NULL)
		{
			spawns[cellx]=new CellSpawns*[_sizeY];
			memset(spawns[cellx], 0, sizeof(CellSpawns*)*_sizeY);
		}

		if(!spawns[cellx][celly])
			spawns[cellx][celly] = new CellSpawns;

		spawns[cellx][celly]->monsterBorns.push_back(conf);
	}
}

void MapSpawn::PrintCreatureSpawnCounter()
{
	uint32 normalCreatureCounter = 0;

	for (int i = 0; i < _sizeX; ++i)
	{
		if (spawns[i] == NULL)
			continue ;
		for (int k = 0; k < _sizeY; ++k)
		{
			if (spawns[i][k] == NULL)
				continue ;

			normalCreatureCounter += spawns[i][k]->monsterBorns.size();
		}
	}


	sLog.outString("场景[%u] 怪物+NPC 配置条数=%u 个", _mapId, normalCreatureCounter);	
}