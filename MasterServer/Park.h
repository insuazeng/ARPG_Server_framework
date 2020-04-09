#pragma once

class CPlayerServant;
// class PlayerEntity;

class Park : public Singleton<Park>
{
public:
	Park(void);
	~Park(void);
public:
	inline void SetPlayerLimit(int limit) { m_nMaxPlayerLimit = limit; }
	inline int GetPlayerLimit() { return m_nMaxPlayerLimit; }
	inline uint32 GetPlatformID() { return m_PlatformID; }
	inline uint32 GetServerFullID() { return m_ServerGlobalID; }
	inline uint32 GetServerIndex() { return m_ServerIndex; }
	inline uint32 GetPassMinuteToday() { return m_pass_min_today; }
	inline void SetPassMinuteToday(uint32 data) { m_pass_min_today = data; }

public:
	void Update(uint32 diff);

	void Rehash(bool load = false);
	void Initialize();
	void SaveAllPlayers();
	void ShutdownClasses();

	// 语言翻译相关
	void LoadLanguage();

	//平台名相关
	void LoadPlatformNameFromDB();
	std::string GetPlatformNameById(uint32 platformId = 0);//默认获取本平台名称

	bool isValidablePlayerName(string& utf8_name);
	
    // 场景控制相关
	void LoadSceneMapControlConfig(bool reloadFlag = false);
	const uint32 GetHandledServerIndexBySceneID(uint32 sceneID);
	uint32 GetSceneIDOnPlayerLogin(uint32 targetMapID);					// 玩家上线时挑选一个合适的场景服进程索引
	uint32 GetMinPlayerCounterServerIndex(uint32 defaultServerIndex);	// 获取人数最少的场景服进程索引
	bool IsValidationSceneServerIndex(uint32 serverIndex);				// 是否为一个有效的场景服索引
	
    // 场景服场景人数统计更新
	void UpdatePlayerMapCounterData(uint32 serverIndex, WorldPacket &recvData);
	void GetSceneServerConnectionStatus(WorldPacket &recvData);

public:
	// lua导出相关
	static int getMongoDBName(lua_State *l);
	static int getServerGlobalID(lua_State *l);
	static int getCurUnixTime(lua_State *l);
	static int getSceneServerIDOnLogin(lua_State *l);

	LUA_EXPORT(Park)

public:
	uint32 flood_lines;
	uint32 flood_seconds;
	bool flood_message;

private:
    void _DeleteOldDatabaseLogs();

protected:
    uint32 m_PlatformID;
	uint32 m_ServerGlobalID;	// 
	uint32 m_ServerIndex;		// 服务器编号(几服)

	int m_nMaxPlayerLimit;			// 最多可容纳的玩家数
    uint32 m_pass_min_today;        // 今天第几分钟 

	// 平台名称
	std::map<uint32, string> m_platformNames;
	// 场景控制
	std::map<uint32, set<uint32> > m_sceneControlConfigs;
	typedef std::map<uint32, int32> ScenePlayerCounterMap;
	// 场景服场景人数统计
	std::map<uint32, ScenePlayerCounterMap> m_sceneServerMapPlayerCounter;		// map<场景服索引,map<场景id,场景人数>>
	// 场景服人数统计
	std::map<uint32, int32> m_sceneServerPlayerCounter;							// map<场景服索引,该场景服人数>
};

#define sPark Park::GetSingleton()
