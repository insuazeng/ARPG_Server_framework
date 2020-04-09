/*
* Ascent MMORPG Server
* Copyright (C) 2005-2010 Ascent Team <http://www.ascentemulator.net/>
*
* This software is  under the terms of the EULA License
* All title, including but not limited to copyrights, in and to the AscentNG Software
* and any copies there of are owned by ZEDCLANS INC. or its suppliers. All title
* and intellectual property rights in and to the content which may be accessed through
* use of the AscentNG is the property of the respective content owner and may be protected
* by applicable copyright or other intellectual property laws and treaties. This EULA grants
* you no rights to use such content. All rights not expressly granted are reserved by ZEDCLANS INC.
*
*/


#include "stdafx.h"
#include "MapSpawn.h"
#include "MapCell.h"
#include "MapMgr.h"

MapCell::MapCell()
{
	_forcedActive = false;
}

MapCell::~MapCell()
{
	RemoveAllObjects(true);
}

void MapCell::Init(uint32 x, uint32 y, MapMgr* mapmgr)
{
	_mapmgr = mapmgr;
	_forcedActive = false;
	if ( _mapmgr != NULL && !_mapmgr->GetMapInfo()->isNormalScene() )
	{
		_forcedActive = true;
	}
	_active = false;
	_loaded = false;
	_playerCount = 0;
	_cell_posX=x;
	_cell_posY=y;
	_objects.clear();
}

void MapCell::AddObject(Object* obj)
{
	if(obj->isPlayer())
		++_playerCount;

	_objects.insert(obj);
}

void MapCell::RemoveObject(Object* obj)
{
	if(obj->isPlayer())
		--_playerCount;

	_objects.erase(obj);
}

void MapCell::SetActivity(bool state)
{	
	if(!_active && state)
	{
		// Move all objects to active set.
		for(ObjectSet::iterator itr = _objects.begin(); itr != _objects.end(); ++itr)
		{
			if(!(*itr)->Active && (*itr)->CanActivate())
				(*itr)->Activate(_mapmgr);
		}
	} 
	else if(_active && !state)
	{
		// Move all objects from active set.
		for(ObjectSet::iterator itr = _objects.begin(); itr != _objects.end(); ++itr)
		{
			if((*itr)->Active)
				(*itr)->Deactivate(_mapmgr);
		}
	}

	_active = state; 

}
void MapCell::RemoveAllObjects(bool systemShutDown)
{
	ObjectSet::iterator itr;

	size_t objectCount = _respawnObjects.size();
	size_t curCount = 0;
	/* delete objects in pending respawn state */
	Object* pObject = NULL;
	for(itr = _respawnObjects.begin(); itr != _respawnObjects.end(); ++itr)
	{
		pObject = *itr;

		if (pObject != NULL)
		{
			switch(pObject->GetTypeID())
			{
			case TYPEID_MONSTER: 
				{
					// _mapmgr->_reusable_guids_creature.push_back( pObject->GetGUID());
					TO_MONSTER(pObject)->m_respawnCell = NULL;
					TO_MONSTER(pObject)->Destructor();
				}
				break;
			case TYPEID_PARTNER:
				break;
			default:
				pObject->Delete();
				break;
			}
		}

		if (++curCount >= objectCount)
			break;
	}
	_respawnObjects.clear();

	////This time it's simpler! We just remove everything :)
	Object* obj = NULL; //do this outside the loop!
	objectCount = _objects.size();
	curCount = 0;
	for(itr = _objects.begin(); itr != _objects.end(); )
	{
		obj = (*itr);
		++itr;

		if (obj != NULL)
		{
			if( obj->Active )
				obj->Deactivate( _mapmgr );

			if (systemShutDown && obj->isMonster())
			{
				TO_MONSTER(obj)->gcToMonsterPool();
			}
			else
			{
				if( obj->IsInWorld())
					obj->RemoveFromWorld( true );
				obj->Delete();
			}
		}

		// 不采用下面做法跳出循环的话,在本版本的vs内会出现崩溃
		if (++curCount >= objectCount)
			break;
	}
	_objects.clear();

	_playerCount = 0;
	_loaded = false;
}

void MapCell::LoadObjects(CellSpawns * sp)
{
	_loaded = true;
	Instance * pInstance = _mapmgr->pInstance;
	uint32 diff = _mapmgr->iInstanceDifficulty;
	if (sp->monsterBorns.empty())
		return ;

	Monster *m = NULL;
	for(vecMonsterBornList::iterator it = sp->monsterBorns.begin(); it != sp->monsterBorns.end(); it++)
	{
		// 一条配置可能会出生多个怪
		for (int i = 0; i < (*it)->born_count; ++i)
		{
			m = _mapmgr->CreateMonster((*it)->proto_id);
			if(m == NULL)
				continue;

			m->SetMapId(_mapmgr->GetMapId());
			m->SetInstanceID(_mapmgr->GetInstanceID());

			if(m->Load((*it), _mapmgr))
			{
				m->PushToWorld(_mapmgr);
			}
			else
			{
				m->gcToMonsterPool();
			}
		}
	}
}

bool MapCell::HaveActivityMonsters()
{
	if (_objects.empty())
		return false;

	Monster *m = NULL;
	for (ObjectSet::iterator it = _objects.begin(); it != _objects.end(); ++it)
	{
		if ((*it)->isMonster())
		{
			m = TO_MONSTER(*it);
			if (m->GetAIState() != AI_IDLE)
				return true;
		}
	}
	return false;
}

bool MapCell::HaveWaypointMovementCreatureNpc()
{
	return false;
}
