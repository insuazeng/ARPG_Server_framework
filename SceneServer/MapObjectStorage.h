#pragma once

template<typename T>
class CMapObjectStorage
{
	typedef HM_NAMESPACE::hash_map<uint64, T*> ObjStorageMap;
public:
	CMapObjectStorage(void)
	{

	}

	~CMapObjectStorage(void)
	{
		m_objStorage.clear();
	}

public:
	void AddObject(uint64 guid, T *pObj)
	{
		m_objStorage.insert(make_pair(guid, pObj));
	}
	void RemoveObject(uint64 guid)
	{
		m_objStorage.erase(guid);
	}
	void RemoveObject(typename ObjStorageMap::iterator it)
	{
		m_objStorage.erase(it);
	}
	T* GetObject(uint64 guid)
	{
		typename ObjStorageMap::iterator it = m_objStorage.find(guid);
		return it != m_objStorage.end() ? it->second : NULL ;
	}
	bool empty()
	{
		return m_objStorage.empty();
	}
	typename ObjStorageMap::iterator begin()
	{
		return m_objStorage.begin();
	}
	typename ObjStorageMap::iterator end()
	{
		return m_objStorage.end();
	}
	const uint32 size()
	{
		return uint32(m_objStorage.size());
	}
	void clear()
	{
		m_objStorage.clear();
	}
private:
	ObjStorageMap m_objStorage;	
};
