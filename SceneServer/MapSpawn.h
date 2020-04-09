#pragma once

// 管理地图上怪物出生的类
struct MapInfo;
struct tagMonsterBornConfig;
typedef vector<tagMonsterBornConfig*> vecMonsterBornList;
// 怪物出生列表
struct CellSpawns
{
	vecMonsterBornList monsterBorns;
};

class MapSpawn
{
public:
	MapSpawn(uint32 mapid, MapInfo *inf);
	~MapSpawn();

	CellSpawns * GetSpawnsList(uint32 cellx,uint32 celly);
	CellSpawns * GetSpawnsListAndCreate(uint32 cellx, uint32 celly);

	void LoadSpawns(bool reload, MapInfo *mapInfo);			//set to true to make clean up
	void PrintCreatureSpawnCounter();

private:
	MapSpawn() { } 

private:
	uint32 _mapId;					
	CellSpawns **spawns[_sizeX];	// 每个格子的出生信息
};

