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

#ifndef _PLAYER_H
#define _PLAYER_H

#include "Object.h"

class GameSession;
class Partner;
typedef map<uint64, Partner*> PlayerPartnerMap;

class Player: public Object
{
public:
	Player ();
	~Player ();

public:
    /**
    * Init Player对象初始化，新增的数据成员必须添加相应的初始化代码
    * @param guid: 玩家角色id
    */
	void Init(GameSession *session, uint64 guid, const uint32 fieldCount);

	// 从字节流中读取玩家属性数据
	// bool LoadFromByteBuffer(uint32 platformServerID, uint64 playerGuid, ByteBuffer &buffer);

    /**
    * Term 对象清理，以便回收重用，新增的数据成员必须添加相应的清理代码
    */
    void Term();

    /** 
    * Update 定时更新操作
    * @param time: 本次更新检测与上次操作的时间间隔
    */
    void Update(uint32 time);

	void SyncUpdateDataMasterServer();							// 同步更新数据到中央服
	void ResetSyncObjectDatas();							    // 重置同步对象数据

public:
    /************************************************************************/
    /* Player对象对应的客户端会话对象的设置及获取接口                       */
    /************************************************************************/
    inline GameSession *GetSession() const { return m_session; }
    /************************************************************************/
    /* 角色id，名字等获取接口                                               */
    /************************************************************************/
    inline const std::string &strGetUtf8NameString() const { return m_NickName; }
    virtual const char* szGetUtf8Name() const { return m_NickName.c_str(); }
    inline void SetPlayerTransferStatus(uint8 pStatus) { m_transfer_status = pStatus; }
    inline uint8 GetPlayerTransferStatus() const { return m_transfer_status; }
	inline uint32 GetPlatformID() { return GetUInt32Value(PLAYER_FIELD_PLATFORM); }
	
	inline pbc_wmessage* GetObjCreationBuffer() { return m_pbcCreationDataBuffer; }
	inline pbc_wmessage* GetObjUpdateBuffer() { return m_pbcPropUpdateDataBuffer; }
	inline tagLastMapData* GetLastMapData() { return &m_lastMapData; }
	inline uint32 GetLevel() { return GetUInt32Value(UNIT_FIELD_LEVEL); }
	inline uint32 GetCareer() { return GetUInt32Value(PLAYER_FIELD_CAREER); }

public:
	string strGetGbkNameString();
    /************************************************************************/
    /* 角色游戏状态相关接口                                                 */
    /************************************************************************/
    uint32 GetGameStates();                                 // 获取玩家当前游戏状态
   
    /************************************************************************/
    /* 地图相关接口                                                         */
    /************************************************************************/
    //override
    virtual void AddToWorld();                                              // 加入到预设的地图中
	virtual MapMgr* OnObjectGetInstanceFailed();							// 重新试图获取之前获取不到的mapmgr
    virtual void OnPushToWorld();                                           // 进入地图时的处理
    virtual void RemoveFromWorld();                                         // 离开当前地图
    
    /**
    * SafeTeleport 传送到某地图的某个坐标位置
    * @param MapID: 指定的地图id
    * @param InstanceID: 地图实例的id，若为0则寻找合适的地图进行加入
    * @param X: 传送目的地x轴坐标
    * @param Y: 传送目的地y轴坐标
    * Return: 是否传送成功
    */
    bool SafeTeleport(uint32 MapID, uint32 InstanceID, float X, float Y);
	
	/*
	*/
	bool CanEnterThisScene(uint32 mapId);

	/*
	* 处理场景传送请求的接口，一般从外部调用
	*/
	bool HandleSceneTeleportRequest(uint32 destServerIndex, uint32 destMapID, FloatPoint destPos);

    /**
    * _Relocate 传送到某地图的某个坐标位置
    * @param mapid: 指定的地图id
    * @param v: 坐标点信息
    * @param force_new_world: 是否强制进入新地图，即副本中进入同一个地图的新实例
    * @param instance_id: 地图实例id
    * Return: 是否传送成功
    */
    bool _Relocate(uint32 mapid, const FloatPoint &v, bool force_new_world, uint32 instance_id);

    void EjectFromInstance();											// 从当前场景中被弹出

	// 伙伴相关
	Partner* GetPartnerByProto(uint32 protoID);
	Partner* GetPartnerByGuid(uint64 partnerGuid);

public:
	static UpdateMask m_visibleUpdateMask;                  // 他人可见属性配置信息

	// lua导出相关
	static int newCppPlayerObject(lua_State *l);
	static int delCppPlayerObject(lua_State *l);

	int initData(lua_State *l);
	int getPlayerUtf8Name(lua_State *l);
	int setPlayerUtf8Name(lua_State *l);
	int getPlayerGbkName(lua_State *l);
	int removeFromWorld(lua_State *l);
	int sendPacket(lua_State *l);
	int sendServerPacket(lua_State *l);
	int setPlayerDataSyncFlag(lua_State *l);

	int releasePartner(lua_State *l);		// 放出一个伙伴
	int revokePartner(lua_State *l);		// 收回某个伙伴

	LUA_EXPORT(Player)

private:
	virtual void OnValueFieldChange(const uint32 index);
    //设置玩家需要同步的数据
	virtual void _SetCreateBits(UpdateMask *updateMask, Player* target) const;
	virtual	void _SetUpdateBits(UpdateMask *updateMask, Player* target) const;

private:
	GameSession *m_session;                                 // 客户端映射会话指针
    uint32 m_gamestates;                                    // 对应SessionStatus的会话状态
    std::string m_NickName;                                 // 角色名
    uint8 m_transfer_status;                                // 当前传送状态

	tagLastMapData m_lastMapData;							// 最后一处所在普通场景的数据
	PlayerPartnerMap m_Partners;							// 玩家的伙伴 可有多个,map<伙伴guid,实例>

    //周围object更新信息打包发送的相关接口
    bool bProcessPending;                                   // 是否已在待处理队列中
    bool bMoveProcessPending;                               // 是否已在移动消息待处理队列中
	bool bRemoveProcessPending;								// 是否已在移除消息待处理队列中
	bool bCreateProcessPending;								// 是否已在创建消息待处理队列中

    Mutex _bufferS;
	pbc_wmessage *m_pbcMovmentDataBuffer;				// 对象移动协议数据缓冲区
	pbc_wmessage *m_pbcCreationDataBuffer;				// 对象创建协议数据缓冲区
	pbc_wmessage *m_pbcRemoveDataBuffer;				// 对象移除协议数据缓冲区
	uint32 m_pbcRemoveDataCounter;						// 对象移除数量
	pbc_wmessage *m_pbcPropUpdateDataBuffer;			// 对象属性更新协议数据缓冲区

public:
	// 对象数据同步
	uint32 m_nSyncDataLoopCounter;			// 循环计数器（并非每次update都同步数据）
	bool m_bNeedSyncPlayerData;				// 需要同步玩家数据
	
	virtual void BuildCreationDataBlockForPlayer( Player* target );
	virtual bool BuildValuesUpdateBlockForPlayer( Player *target );
	virtual void BuildRemoveDataBlockForPlayer( Player *target );
	virtual void ClearUpdateMask();

    // 角色属性同步、移动包同步相关接口
	void PushSingleObjectPropUpdateData();
	void PushCreationData();
	void PushOutOfRange(const uint64 &guid, uint32 objtype);
	void PushMovementData(uint64 objguid, float srcX, float srcY, float srcHeight, float destX, float destY, float destHeight, uint8 mode);
	
	void ProcessObjectPropUpdates();			// 推送属性更新包
	void ProcessObjectRemoveUpdates();			// 推送对象移除包
	void ProcessObjectMovmentUpdates();			// 推送对象移动包
	void ProcessObjectCreationUpdates();		// 推送对象创建包
	
	void ClearAllPendingUpdates();
};

#endif

