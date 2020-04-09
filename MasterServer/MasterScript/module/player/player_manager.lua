--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 玩家管理器,对lua内的玩家数据进行管理/初始化/卸载等操作
module("MasterScript.module.player.player_manager", package.seeall)

local PlayerModule = require("MasterScript.module.player.lua_player")
require("MasterScript.module.player.player_db")
require("MasterScript.module.player.player_event")
require("MasterScript.module.player.player_money")
require("MasterScript.module.player.player_skill")
require("MasterScript.module.player.player_props")
require("MasterScript.module.player.player_item")

local PlayerInfoModule = require("MasterScript.module.player.player_info")
local MapInfoConf = require("gameconfig.map_info_conf")

local _G = _G

-- 保存PlayerInfo以及LuaPlayer对象的列表
if _G.gPlayerDataList == nil then
    _G.gPlayerDataList = {}
end

-- 账号角色列表
if _G.gAccountRoleList == nil then
    _G.gAccountRoleList = {}
end

local playerDataList = _G.gPlayerDataList
local accountRoleList = _G.gAccountRoleList

local __playerGuidHigh = 0           -- 玩家最大guid
local __loadPlayerMaxGuid = nil
local __generatePlayerMaxGuid = nil

local __itemGuidHigh = 0
local __loadItemMaxGuid = nil

function Initialize()
    -- 读取角色最大id
    __loadPlayerMaxGuid()
    
    -- 读取playerinfo
    LoadPlayerInfos()

    -- 读取物品最大id
    __loadItemMaxGuid()
end

-- 获取lua中的player对象
function GetPlayerByGuid(guid)
    local ret = nil
    if playerDataList[guid] ~= nil and playerDataList[guid].obj ~= nil then
        ret = playerDataList[guid].obj
    end
    return ret
end

-- 移除lua中的player对象
function RemovePlayerByGuid(guid)
    if playerDataList[guid] ~= nil and playerDataList[guid].obj ~= nil then
        -- 析构掉玩家对象
        playerDataList[guid].obj:Term()
        playerDataList[guid].obj = nil
    end
end

-- 创建一个player对象
function CreateLuaPlayer(guid, servant, info)
    local plr = PlayerModule.LuaPlayer(guid)
    if plr == nil then 
        return nil 
    end

    local ret = plr:InitData(servant, info, info.last_online_time)
    if not ret then
        return nil
    end

    playerDataList[guid].obj = plr

    return plr
end

-- 初始化player对象数据
function InitPlayerData(guid)
    local plr = GetPlayerByGuid(guid)
    if plr == nil then
        return false
    end
    
    -- LuaLog.outDebug(string.format("准备对玩家guid=%q进行数据初始化", tostring(guid)))
    return plr:InitData()
end

-- 获取PlayerInfo对象
function GetPlayerInfoByGuid(guid)
    local ret = nil
    if playerDataList[guid] ~= nil and playerDataList[guid].info ~= nil then
        ret = playerDataList[guid].info
    end
    return ret
end

-- 
function GetPlayerInfoByName(roleName)
    -- body
    local ret = nil
    
    for k,v in pairs(playerDataList) do
        if v.info ~= nil and v.info.role_name == roleName then
            ret = v.info
            break
        end
    end

    return ret
end

-- 读取PlayerInfo对象
function LoadPlayerInfos()
    local infos = sMongoDB.Query("character_datas", "{}")
    local cnt = 0
    local createdCnt = 0
    accountRoleList = {}
    if infos ~= nil then
        for ret in infos:results() do
            assert(ret.player_guid ~= nil and ret.player_guid > 0)
            -- LuaLog.outDebug(string.format("guid=%q, name=%s", tostring(ret.player_guid), ret.nick_name))
            local plrInfo = PlayerInfoModule.PlayerInfo(ret.player_guid, ret.nick_name)
            assert(plrInfo ~= nil)
            -- 读取已创号playerInfo的数据
            if ret.create_time ~= nil and ret.create_time > 0 then
                plrInfo.acc_name = ret.account_name
                plrInfo.agent_name = ret.agent_name
                
                plrInfo.career = ret.career
                plrInfo.create_time = ret.create_time
                plrInfo.cur_level = ret.cur_level
                
                plrInfo.last_online_time = ret.last_online_time
                
                if ret.first_enter_scene_time ~= nil then
                    plrInfo.first_enter_scene_time = ret.first_enter_scene_time
                end
                
                if ret.last_join_guild_time ~= nil then
                    plrInfo.last_join_guild_time = ret.last_join_guild_time
                end

                if ret.ban_expire_time ~= nil then
                    plrInfo.ban_expire_time = ret.ban_expire_time
                end
                
                createdCnt = createdCnt + 1
                -- 添加到PlayerMgr中
                AddPlayerInfo(plrInfo, ret.player_guid)
            else

            end
            cnt = cnt + 1
        end
    end
    LuaLog.outString(string.format("共读取%u条PlayerInfo数据,其中已创号数据%u条", cnt, createdCnt))
end

-- 添加PlayerInfo对象
function AddPlayerInfo(info, guid)
    if playerDataList[guid] == nil then
        playerDataList[guid] = {}
    end

    playerDataList[guid].info = info

    -- 以代理名+账号名做key,将角色信息添加进列表
    local k = info.agent_name .. info.acc_name
    if accountRoleList[k] == nil then
        accountRoleList[k] = {}
    end
    table.insert(accountRoleList[k], info)
end

-- 生成玩家guid
__generatePlayerMaxGuid = function()
    __playerGuidHigh = __playerGuidHigh + 1
    return __playerGuidHigh
end

-- 读取玩家最大id
__loadPlayerMaxGuid = function()
    
    local maxGuid = sMongoDB.FindOne("character_datas", "{query:{}, orderby:{player_guid:-1}}")

    if maxGuid ~= nil and maxGuid.player_guid then
        __playerGuidHigh = maxGuid.player_guid
    end
    
    if __playerGuidHigh == 0 then
        __playerGuidHigh = Park.getServerGlobalID() * 1000000
    end

    LuaLog.outString(string.format("当前最大PlayerGuid=%q", tostring(__playerGuidHigh)))
end

-- 生成物品最大guid
function GenerateItemMaxGuid()
    -- body
    __itemGuidHigh = __itemGuidHigh + 1
    return __itemGuidHigh
end

-- 读取物品最大guid
__loadItemMaxGuid = function()
    -- body
    local maxGuid = sMongoDB.FindOne("player_items", "{query:{}, orderby:{item_guid:-1}}")

    if maxGuid ~= nil and maxGuid.item_guid then
        __itemGuidHigh = maxGuid.item_guid
    end
    
    if __itemGuidHigh == 0 then
        __itemGuidHigh = Park.getServerGlobalID() * 1000000000
    end

    LuaLog.outString(string.format("当前最大ItemGuid=%q", tostring(__itemGuidHigh)))
end

--[[ 创建一个新角色
]]
function CreateNewPlayer(agentName, accName, roleName, roleCareer)
    -- 把名字中的空格去除掉
    LuaLog.outString("等待解决")

    -- 创建PlayerInfo对象
    local newPlayerGuid = __generatePlayerMaxGuid()
    local plrInfo = PlayerInfoModule.PlayerInfo(newPlayerGuid, roleName)
    if plrInfo == nil then
        return nil
    end

    plrInfo.acc_name = accName
    plrInfo.agent_name = agentName
    plrInfo.career = roleCareer
    plrInfo.create_time = Park.getCurUnixTime()
    local mapID, posX, posY = GetBornMapPos()

    -- 添加到保存的列表
    AddPlayerInfo(plrInfo, newPlayerGuid)

    -- 保存至数据库
    local sql = string.format("{player_guid:%.0f,account_name:'%s',agent_name:'%s',nick_name:'%s',create_time:%u,career:%u,cur_level:1,cur_exp:0,cur_hp:500,map_id:%u,pos_x:%.1f,pos_y:%.1f}", 
                            newPlayerGuid, accName, agentName, roleName, plrInfo.create_time, roleCareer, mapID, posX, posY)
    sMongoDB.Insert("character_datas", sql)

    return plrInfo
end

--[[ 填充指定账号的角色列表信息
]]
function FillAccountRoleListInfo(accID, agentName, accName)
    -- LuaLog.outDebug(string.format("FillAccountRoleListInfo called, agent=%s,acc=%s", agentName, accName))

    local servant = CPlayerMgr.findPlayerServant(accID)
    if servant == nil then return end

    local ret = {}
    local k = agentName .. accName
    -- LuaLog.outDebug(string.format("accountRoleList[k] = %s", type(accountRoleList[k])))

    if accountRoleList[k] ~= nil then
        local infoList = accountRoleList[k]
        -- LuaLog.outDebug(string.format("账号 %s 名下角色=%s", k, type(infoList)))
        for i=1,#infoList do
            local t = { infoList[i]:GetGUID(), infoList[i].role_name, infoList[i].cur_level, infoList[i].career, infoList[i].ban_expire_time }
            table.insert(ret, t)
        end
    end
    
    servant:setRoleListInfo(ret)
end

--[[ 能否选择指定角色进入游戏
]]
function CanChoosePlayerEnterGame(tempAccountID, choosePlayerGuid)
    local servant = CPlayerMgr.findPlayerServant(tempAccountID)
    if servant == nil then
        return nil
    end

    local info = GetPlayerInfoByGuid(choosePlayerGuid)
    if info == nil then
        return nil
    end

    -- 比较两者的帐号名,代理名是否一致
    if info.acc_name ~= servant:getAccountName() or info.agent_name ~= servant:getAgentName() then
        return nil
    end

    -- 检测角色是否已被封
    if info.ban_expire_time ~= nil and info.ban_expire_time > Park.getCurUnixTime() then
        return nil
    end

    return info
end

--[[ 选择角色进入游戏
]]
function ChoosePlayerEnterGame(tempAccountID, plrInfo)
    local servant = CPlayerMgr.findPlayerServant(tempAccountID)
    assert(servant ~= nil and plrInfo ~= nil)
    
    local chooseStartTime = SystemUtil.getMsTime64()
    local plrGuid = plrInfo:GetGUID()

    -- 在PlayerMgr的在线列表中更新servant的guid
    CPlayerMgr.updatePlayerServantRealGuid(tempAccountID, plrGuid)
    
    -- 创建并初始化LuaPlayer对象,并添加进链表
    local plr = CreateLuaPlayer(plrGuid, servant, plrInfo)
    if plr == nil then
        return 
    end

    -- 读取物品数据
    plr:LoadItemDataFromDB()
    
    -- 设置PlayerInfo数据
    plrInfo:OnPlayerLogin(plr)
    
    -- 登陆至子系统
    plr:LoginToSubSystem()
    
    -- 
    OnPlayerLogin(plrInfo)

    -- 首次计算玩家玩家中央服战斗属性,但不用同步至场景服
    plr:CalcuProperties(false)

    -- 检测上次在线与当前时间是否有跨天,如果有则处理跨天事件

    -- 在数据库设置玩家在线标记/本地登陆时间/总登陆次数
    local querySql = string.format("{player_guid:%.0f}", plrGuid)
    local updateSql = string.format("{$set:{online_flag:1,last_login_time:%u},$inc:{login_count:1}}", Park.getCurUnixTime())
    sMongoDB.Update("character_datas", querySql, updateSql, true)

    -- 获取玩家即将要登陆到的场景服id
    local targetSceneServerID = Park.getSceneServerIDOnLogin(plr.cur_map_id)
    
    -- 通知网关,选角成功准备进入游戏
    local pck = WorldPacket.newPacket(ServerOpcodes.SMSG_MASTER_2_GATE_ROLE_CHOOSE_RET, 256)
    pck:writeUInt64(tempAccountID)
    pck:writeUInt64(plrGuid)
    pck:writeUInt32(targetSceneServerID)
    pck:writeUInt32(curUnixTime)
    -- 填充进入游戏的数据
    plr:FillEnterGameData(pck)
    servant:sendServerPacket(pck)

    WorldPacket.delPacket(pck)

    local timeDiff = SystemUtil.getMsTime64() - chooseStartTime
    LuaLog.outDebug(string.format("选角成功(name=%s,guid=%.0f),准备登入场景服%u,耗时%u毫秒", plr:GetName(), plrGuid, targetSceneServerID, timeDiff))
end

--[[ 玩家登出
]]
function LogoutPlayer(guid, curState, needSaveDB)
    -- body
    local plr = GetPlayerByGuid(guid)
    -- LuaLog.outDebug(string.format("LogoutPlayer Called, guid=%.0f tpPlr=%s", guid, type(plr)))
    
    if plr == nil then
        LuaLog.outError(string.format("无法找到需要下线处理的对象guid=%.0f", guid))
        return
    end
    
    OnPlayerLogout(plr.plr_info)

    plr:Logout(curState, needSaveDB)
    
    RemovePlayerByGuid(guid)

    LuaLog.outString(string.format("玩家guid=%.0f 下线成功并保存数据", guid))
end

--[[ 
]]
function OnPlayerLogin(plrInfo)
end

--[[
]]
function OnPlayerLogout(plrInfo)
end

--[[
]]
function OnPlayerLevelUp(plrInfo, newLevel)
    -- body
end

--[[ 获取游戏角色出生地图及坐标
]]
function GetBornMapPos()
    local mapIDs =
    {
        1001, 1001, 1001
    }

    local bornID = mapIDs[math.random(1, 3)]
    local info = MapInfoConf.GetMapInfo(bornID)
    assert(info ~= nil)

    -- local xOffset = math.random(-3, 3)
    -- local yOffset = math.random(-3, 3)

    return bornID, info.born_pos_x, info.born_pos_y
end

--endregion
