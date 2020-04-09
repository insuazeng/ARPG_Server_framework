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

//
// CellHandler.h
//

#ifndef __CELLHANDLER_H
#define __CELLHANDLER_H

class MapSpawn;

template < class Class >
class CellHandler
{
public:
	CellHandler(MapSpawn *spawn);
	~CellHandler();

	Class *GetCell(uint32 x, uint32 y);
	Class *GetCellByCoords(uint32 x, uint32 y);
	Class *Create(uint32 x, uint32 y);
	Class *CreateByCoords(uint32 x, uint32 y);
	void Remove(uint32 x, uint32 y);

	inline bool Allocated(uint32 x, uint32 y) { return _cells[x][y] != NULL; }

	static uint32 GetPosX(uint32 x); 
	static uint32 GetPosY(uint32 y);

protected:
	void _Init();
	Class ***_cells;

	MapSpawn *m_mapSpawn;
};

template <class Class>
CellHandler<Class>::CellHandler(MapSpawn *spawn)
{
	m_mapSpawn = spawn;
	_Init();
}

template <class Class>
void CellHandler<Class>::_Init()
{
	_cells = new Class**[_sizeX];
	ASSERT(_cells);
	for (uint32 i = 0; i < _sizeX; i++)
	{
		_cells[i]=NULL;
	}
}

template <class Class>
CellHandler<Class>::~CellHandler()
{
	if(_cells)
	{
		for (uint32 i = 0; i < _sizeX; i++)
		{
			if(!_cells[i])
				continue;

			for (uint32 j = 0; j < _sizeY; j++)
			{
				if(_cells[i][j])
					delete _cells[i][j];
			}
			delete [] _cells[i];	
		}
		delete [] _cells;
	}
}

template <class Class>
Class* CellHandler<Class>::Create(uint32 x, uint32 y)		// 根据cellpos(x,y)创建对应的cell
{
	if( x >= _sizeX ||  y >= _sizeY ) 
		return NULL; 

	if(!_cells[x])
	{
		_cells[x] = new Class*[_sizeY];
		memset(_cells[x],0,sizeof(Class*)*_sizeY);
	}

	ASSERT(_cells[x][y] == NULL);

	Class *cls = new Class;
	_cells[x][y] = cls;

	return cls;
}

template <class Class>
Class* CellHandler<Class>::CreateByCoords(uint32 x, uint32 y)	// 根据gridpos(x,y)创建对应的cell
{
	return Create(GetPosX(x),GetPosY(y));
}

template <class Class>
void CellHandler<Class>::Remove(uint32 x, uint32 y)			// 移除cellpos(x,y)对应的cell
{
	if( x >= _sizeX ||  y >= _sizeY ) 
		return; 

	if(!_cells[x]) return;
	ASSERT(_cells[x][y] != NULL);

	Class *cls = _cells[x][y];
	_cells[x][y] = NULL;

	delete cls;
}

template <class Class>
Class* CellHandler<Class>::GetCell(uint32 x, uint32 y)			// 根据cellpos(x,y)获取对应的cell
{
	if( x >= _sizeX ||  y >= _sizeY ) 
		return NULL; 
	if(!_cells[x]) return NULL;
	return _cells[x][y];
}

template <class Class>
Class* CellHandler<Class>::GetCellByCoords(uint32 x, uint32 y)	// 根据gridpos(x,y)获取对应的cell
{
	return GetCell(GetPosX(x),GetPosY(y));
}

template <class Class>
uint32 CellHandler<Class>::GetPosX(uint32 x)	// 根据grid_pos_x获取对应的cellPosX
{
	ASSERT((x >= _minX) && (x <= _maxX));
	return (uint32)((x-_minX)/_cellSize);
}

template <class Class>
uint32 CellHandler<Class>::GetPosY(uint32 y)	// 根据grid_pos_y获取对应的cellPosY
{
	ASSERT((y >= _minY) && (y <= _maxY));
	return (uint32)((y-_minY)/_cellSize);
}

#endif
