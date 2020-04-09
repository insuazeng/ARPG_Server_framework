--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.player.lua_player", package.seeall)

local mapInfoConf = require("gameconfig.map_info_conf")
local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")
--[[ 接收从场景服传递过来的玩家个人数据
]]
function LuaPlayer:AsyncDataFromSceneServer(msg, saveToDB)
	-- body
	local guid = msg:readUInt64()
    local curExp = msg:readUInt64()
    self.cur_map_id = msg:readUInt32()
    self.cur_pos_x = msg:readFloat()
    self.cur_pos_y = msg:readFloat()
    local curInstanceID = msg:readUInt32()
    local instanceExpireTime = msg:readUInt32()
    local lastMapID = msg:readUInt32()
    local lastPosX = msg:readFloat()
    local lastPosY = msg:readFloat()

    if saveToDB then
        LuaLog.outString(string.format("玩家:%s(guid=%.0f)下线,当前地图坐标[%u:(%.1f,%.1f)],备份地图坐标[%u:(%.1f,%.1f)],场景实例id=%u,到期时间=%u",
                        self.plr_info.role_name, self:GetGUID(), self.cur_map_id, self.cur_pos_x, self.cur_pos_y, lastMapID, lastPosX, lastPosY, curInstanceID, instanceExpireTime))
        -- 保存DB
        self:SavePlayerToDB(false, true)
    else
        -- 准备切换至下一个场景服
        LuaLog.outString(string.format("[场景服切换]玩家:%s(guid=%.0f)当前地图坐标[%u:(%.1f,%.1f)],备份地图坐标[%u:(%.1f,%.1f)],场景实例id=%u",
                        self.plr_info.role_name, self:GetGUID(), self.cur_map_id, self.cur_pos_x, self.cur_pos_y, lastMapID, lastPosX, lastPosY, curInstanceID))
    end
end

--[[ 读取角色db数据
]]
function LuaPlayer:LoadFromDB()
    
    local info = self.plr_info
    -- LuaLog.outString(string.format("LoadFromDB, guid=%.0f, type=%s", info.object_guid, type(info)))
    self.cur_coins = 0
    self.cur_bind_cash = 0
    self.cur_cash = 0

    if info.last_online_time == nil or info.last_online_time == 0 then
        -- 初次创角登陆
        self.login_count = 1
        self.total_online_time = 0
        self.cur_hp = 100
        self.cur_exp = 0
        -- 获取初次登陆游戏的场景id与坐标
        self.cur_map_id, self.cur_pos_x, self.cur_pos_y = sPlayerMgr.GetBornMapPos()
        local lvConf = levelUpPropConf.GetLevelProp(info.career, 1)
        if lvConf ~= nil then
            self.cur_hp = lvConf[1]
        end
    else
        -- 多次登陆,从DB中取出数据
        local sql = string.format("{query:{player_guid:%.0f}}", info.object_guid)
        local data = sMongoDB.FindOne("character_datas", sql)

        if data == nil or data.player_guid ~= info.object_guid then
            LuaLog.outError(string.format("Player login query failed, guid=%.0f", info.object_guid))
            return false
        end

        self.login_count = data.login_count + 1
        self.total_online_time = data.total_online_time
        self.cur_exp = data.cur_exp
        self.cur_hp = data.cur_hp
        self.cur_map_id = data.map_id
        self.cur_pos_x = data.pos_x
        self.cur_pos_y = data.pos_y
        
        if data.cur_coins ~= nil then
            self.cur_coins = data.cur_coins
        end
        if data.cur_bind_cash ~= nil then
            self.cur_bind_cash = data.cur_bind_cash
        end
        if data.cur_cash ~= nil then
            self.cur_cash = data.cur_cash
        end
        
        if data.hp_pool_data ~= nil then
            self.hp_pool_data = {}
            self.hp_pool_data.cur_store_value = data.hp_pool_data.cur_store_value
            self.hp_pool_data.last_used_time = data.hp_pool_data.last_used_time
        end
        
        if data.mount_advanced_data ~= nil then
            self.mount_advanced_data = data.mount_advanced_data
            if data.mount_advanced_data.hided_flag > 0 then
                self.mount_advanced_data.hided_flag = true
            else
                self.mount_advanced_data.hided_flag = false
            end
        end

        -- 如果该坐标已经不可站立,则获取该场景的合法坐标
        if not SceneMapMgr.isPositionMovable(self.cur_map_id, self.cur_pos_x, self.cur_pos_y) then
            local mapInfo = mapInfoConf.GetMapInfo(self.cur_map_id)
            if mapInfo == nil then
                self.cur_map_id, self.cur_pos_x, self.cur_pos_y = sPlayerMgr.GetBornMapPos()
            else
                self.cur_pos_x = mapInfo.born_pos_x
                self.cur_pos_y = mapInfo.born_pos_y
            end
        end
    end

    self.last_save_time = Park.getCurUnixTime()
    
    LuaLog.outString(string.format("玩家:%s(guid=%.0f)准备登陆在地图%u:(%.1f,%.1f),curHp=%u,lv=%u", info.role_name, 
                    self:GetGUID(), self.cur_map_id, self.cur_pos_x, self.cur_pos_y, self.cur_hp, info.cur_level))

    return true
end

--[[ 保存数据到DB
]]
function LuaPlayer:SavePlayerToDB(saveAtOnce, offlineSave)
	-- body
    local onlineFlag = 1
    if offlineSave then
        onlineFlag = 0
        self.offline_data_save = true
    end

    -- LuaLog.outDebug(string.format("SavePlayerToDB called"))

    local info = self.plr_info

    local querySql = string.format("{player_guid:%.0f}", self:GetGUID())
    local updateSql = string.format("{$set:{online_flag:%u,login_count:%u,last_online_time:%u,total_online_time:%u,cur_level:%u,cur_exp:%.0f,cur_hp:%u,map_id:%u,pos_x:%.1f,pos_y:%.1f}}",
                                    onlineFlag, self.login_count, info.last_online_time, self.total_online_time, info.cur_level, self.cur_exp, self.cur_hp, self.cur_map_id, self.cur_pos_x, self.cur_pos_y)

    sMongoDB.Update("character_datas", querySql, updateSql, true)
end

--[[ 读取玩家物品数据
]]
function LuaPlayer:LoadItemDataFromDB()
    -- body
    if self.item_interface == nil then
        return 
    end
    
    self.item_interface:LoadItemsFromDB()
end

--[[ 保存玩家物品数据
]]
function LuaPlayer:SaveItemDataToDB()
    -- body
    if self.item_interface == nil then
        return 
    end
    self.item_interface:SaveItemsToDB()
end


--endregion
