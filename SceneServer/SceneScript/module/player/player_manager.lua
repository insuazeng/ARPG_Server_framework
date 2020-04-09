--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 玩家管理器,对lua内的玩家数据进行管理/初始化/卸载等操作
module("SceneScript.module.player.player_manager", package.seeall)
local PlayerModule = require("SceneScript.module.player.lua_player")
require("SceneScript.module.player.player_sync")
require("SceneScript.module.player.player_props")
require("SceneScript.module.player.player_combat")
require("SceneScript.module.player.player_partner")

local _G = _G

if _G.playerDataList == nil then
    _G.playerDataList = {}
end

local playerDataList = _G.playerDataList

-- 获取lua中的player对象
function GetPlayerByGuid(guid)
    return playerDataList[guid]
end

-- 移除lua中的player对象
function RemovePlayerByGuid(guid)
    if playerDataList[guid] ~= nil then
        -- 析构掉玩家对象
        playerDataList[guid] = nil
    end
end

-- 创建一个player对象
function CreatePlayer(platformID, guid, session, plrData, dataLen)
    local plr = PlayerModule.LuaPlayer(guid)
    if plr == nil then 
        return false
    end

    local ret = plr:InitData(platformID, guid, session, plrData, dataLen)
    if not ret then
        return false
    end

    -- 添加到玩家列表
    if playerDataList[guid] ~= nil then
        LuaLog.outError(string.format("[PlayerMgr]添加进玩家列表时,table内已存在数据(guid=%.0f,plr=%s)", guid, type(playerDataList[guid])))
    end
    
    playerDataList[guid] = plr

    return true
end

--endregion
