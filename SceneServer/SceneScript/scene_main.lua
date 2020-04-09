--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

local _G = _G
local xpcall = xpcall
local _lua_error_handle = _lua_error_handle

-- 全局变量
gInitializeFunctionTable = {}       -- 初始化函数列表

function InitPackagePath()
    package.path = package.path .. ";./SceneScript/?/.lua;./SceneScript/module/?.lua;./LuaCommon/?.lua;"
end

InitPackagePath()

print("Start load scene_main.lua..")
print("Current Lua Path = " .. package.path)

-- 公共定义
require("gamecomm.common_defines")
require("gamecomm.item_defines")
require("gamecomm.opcodes")
require("gamecomm.server_opcodes")
require("gamecomm.error_codes")
require("gamecomm.gm_cmd_dispatcher")
require("lua_extends")
require("SceneScript.module.common.scene_defines")
require("SceneScript.module.player.player_fields")
require("SceneScript.module.partner.partner_fields")

-- reload注册
require("gamecomm.reloader")

-- 网络协议监听注册
require("SceneScript.network.worldpacket_handle_dispatcher")
require("SceneScript.network.serverpacket_handle_dispatcher")
require("SceneScript.network.server_packet_handler")
require("SceneScript.module.gmcommand.gm_packet_handler")
require("SceneScript.module.skill.skill_packet_handler")
require("SceneScript.module.player.player_packet_handler")
require("SceneScript.module.partner.partner_packet_handler")
print(55)
-- 单例
sPlayerMgr = require("SceneScript.module.player.player_manager")
sMonsterMgr = require("SceneScript.module.monster.monster_manager")

-- 全局变量
gOO = require("oo.oo")
gTimer = require("timer.timer")
gTool = require("gamecomm.tool_functions")
gLibPBC = require("protobuf")
gLibCJson = require("cjson")

-- 局部变量
local __onGCEventTimeOut = nil
local __initPlayerVisibleMaskBits = nil     -- 初始化玩家的可见标记
local __initMonsterVisibleMaskBits = nil    -- 初始化怪物的可见标记
local __initPartnerVisibleMaskBits = nil    -- 初始化怪物的可见标记

-- 脚本初始化函数
function SceneServerInitialize()
    print("SceneServer Initialize..")
    -- 初始化随机种子
    math.randomseed(os.time())

    __initPlayerVisibleMaskBits()
    __initMonsterVisibleMaskBits()
    __initPartnerVisibleMaskBits()

    -- 对初始化函数列表中的函数进行初始化
    local initFuncCount = table.getn(gInitializeFunctionTable)
     print("initFunctionCount = " .. initFuncCount)
    for i = 1, initFuncCount do
    print(i)
        gInitializeFunctionTable[i]()
    end

    --[[if sWorldPacketDispatcher == nil then
        print("dispatcher is null")
    else
        print(string.format("dispatcher is %s", type(sWorldPacketDispatcher)))
        for k, v in pairs(sWorldPacketDispatcher) do
            print(string.format("worldPack, k=%s,type(v)=%s", k, type(v)))
        end
    end]]

    --[[local test = 
    {
        propIndex = 15,
        propValue = 200,
    }
    local buf = gLibPBC.encode("tagProtoObjectSinglePropData", test)
    LuaLog.outDebug(string.format("type buf=%s", type(buf)))
    local t = gLibPBC.decode("tagProtoObjectSinglePropData", buf);
    LuaLog.outDebug(string.format("index=%u, val=%u", t.propIndex, t.propValue))]]

    -- for test
    --[[local pck = WorldPacket.newPacket(128, 128)
    LuaLog.outDebug(string.format("type pck=%s, metatable=%s", type(pck), tostring(getmetatable(pck))))
    pck:writeUInt32(560001)
    pck:writeString("hello,world")
    
    local val_1 = pck:readUInt32()
    local val_2 = pck:readString()
    WorldPacket.delPacket(pck)
    pck = nil

    LuaLog.outDebug(string.format("val_1=%u, val_2=%s", val_1, val_2))]]

    local curUsed = collectgarbage("count") / 1024.0
    LuaLog.outString(string.format("初始化时,内存使用=%.2f MB", curUsed))
    -- 注册gc的定时器
    gTimer.regCallback(120000, ILLEGAL_DATA_32, __onGCEventTimeOut)

end

-- 脚本终止函数
function SceneServerTerminate()
    print("SceneServer termination..")
end

--[[ 创建玩家对象
]]
function CreatePlayer(platformID, guid, session, plrData, dataLen)
    LuaLog.outDebug(string.format("CreatePlayer Called,platform=%u,guid=%.0f,session=%s,plrData=%s,len=%u", platformID, guid, type(session), type(plrData), dataLen))
    return sPlayerMgr.CreatePlayer(platformID, guid, session, plrData, dataLen)
end

--[[ 登出玩家对象
]]
function LogoutPlayer(guid, saveDataToMaster)
    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    local session = Park.getSessionByGuid(guid)
    if plr == nil or session == nil then
        LuaLog.outError(string.format("无法找到要登出的玩家或Session对象,guid=%.0f,plr=%s,session=%s", guid, type(plr), type(session)))
        return
    end
    
    -- 登出
    plr:Logout(session, saveDataToMaster)
    sPlayerMgr.RemovePlayerByGuid(guid)
end

--[[ 将玩家改变数据同步至中央服
]]
function SyncPlayerDataToMaster(plrGuid)
    local plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
    if plr ~= nil then
        local t =
        {
            addExp = plr.exp_add_value,
            curHp = plr.entity_datas[UnitField.UNIT_FIELD_CUR_HP]
        }
        plr:SyncObjectDataToMaster(enumSyncObject.PLAYER_DATA_STM, enumObjDataSyncOp.UPDATE, plr:GetGUID(), t)
        -- 清空经验增加量
        plr.exp_add_value = 0
        plr.need_sync_data = false
    end
end

function CreateMonster(guid, protoID, attackMode, moveType) 
    sMonsterMgr.CreateMonster(guid, protoID, attackMode, moveType)
end

function RemoveMonster(guid) 
    sMonsterMgr.RemoveMonster(guid)
end

--[[批量激活怪物
]]
function ActiveMonsters(guids)
    for i=1,#guids do
        local m = sMonsterMgr.GetMonsterByGuid(guids[i])
        if m ~= nil then
            m:EnterActiveMode()
        end
    end
end

--[[批量反激活怪物
]]
function DeactiveMonsters(guids)
    for i=1,#guids do
        local m = sMonsterMgr.GetMonsterByGuid(guids[i])
        if m ~= nil then
            m:EnterDeactiveMode()
        end
    end
end

--[[ 获取对象数据（怪物或玩家）
]]
function GetObjectData(objGuid)
    local ret = nil 
    if objGuid < MIN_PLAYER_GUID then
        ret = sMonsterMgr.GetMonsterByGuid(objGuid)
    else
        ret = sPlayerMgr.GetPlayerByGuid(objGuid)
    end
    return ret
end

--[[怪物被推放进场景
]]
function OnMonsterPushToWorld(guid, mapid, instanceid)
    local monster = sMonsterMgr.GetMonsterByGuid(guid)
    if monster ~= nil then
        monster:OnPushToWorld(mapid, instanceid)
    else
        LuaLog.outError(string.format("PushtoWorld未找到对应怪物对象(instanceid=%u,guid=%u)", instanceid, guid))
    end
end

--[[对象停止移动
]]
function OnMovingStopped(guid, stopPosX, stopPosY)
    local obj = GetObjectData(guid)
    if obj ~= nil then
        obj:OnMovingStopped(stopPosX, stopPosY)
    end
end

--[[ 处理网络协议包
]]
function HandleWorldPacket(msg_id, guid, msg, msg_len)
    -- LuaLog.outDebug(string.format("HandleWorldPacket called, guid=%q, msgLen=%u, msgID=%u, type(msg)=%s", tostring(guid), msg_len, msg_id, type(msg)))
    sWorldPacketDispatcher.packetDispatcher(msg_id, guid, msg, msg_len)
end

--[[ 处理服务端之间的网络消息包
]]
function HandleServerPacket(msg_id, msg, msg_len)
    -- LuaLog.outDebug(string.format("HandleServerPacket called, msgLen=%u, msgID=%u, type(msg)=%s", msg_len, msg_id, type(msg)))
    sServerPacketDispatcher.packetDispatcher(msg_id, msg, msg_len)
end

-- 事件调用
function OnEventTimeOut(events)
    if type(events) == "number" then
        -- 单个事件回调
        gTimer.onEventTimer(events)
    else
        -- 数组
        for i = 1, #events do
            gTimer.onEventTimer(events[i])
        end
    end

    -- LuaLog.outDebug(string.format("本次回调事件触发数量=%u", #eventIDs))
end

-- 内存gc函数的调用
__onGCEventTimeOut = function(callbackID, curRepeatCount)
    local curUsed = collectgarbage("count") / 1024.0
    LuaLog.outString(string.format("OnGCEventTimeOut called, 当前虚拟机内存用量=%.2f MB", curUsed))
end

__initPlayerVisibleMaskBits = function()
    local t =
    {
        ObjField.OBJECT_FIELD_GUID,

        UnitField.UNIT_FIELD_MAX_HP,
        UnitField.UNIT_FIELD_CUR_HP,

        PlayerField.PLAYER_FIELD_PLATFORM,
        PlayerField.PLAYER_FIELD_CAREER,
        PlayerField.PLAYER_FIELD_WEAPON,
        PlayerField.PLAYER_FIELD_FASHION,
        PlayerField.PLAYER_FIELD_WING,
        PlayerField.PLAYER_FIELD_MOUNT
    }
    Park.setPlayerVisibleUpdateMaskBits(PlayerField.PLAYER_FIELD_END, t)
end

__initMonsterVisibleMaskBits = function()
    local t =
    {
        ObjField.OBJECT_FIELD_GUID,
        ObjField.OBJECT_FIELD_PROTO,

        UnitField.UNIT_FIELD_MAX_HP,
        UnitField.UNIT_FIELD_CUR_HP
    }
    Park.setMonsterVisibleUpdateMaskBits(UnitField.UNIT_FIELD_END, t)
end

__initPartnerVisibleMaskBits = function()
    local t =
    {
        ObjField.OBJECT_FIELD_GUID,
        ObjField.OBJECT_FIELD_TYPE,
        ObjField.OBJECT_FIELD_PROTO,

        PartnerField.PARTNER_FIELD_OWNER,
        PartnerField.PARTNER_FIELD_WEAPON,
        PartnerField.PARTNER_FIELD_FASHION,
        PartnerField.PARTNER_FIELD_WING,
        PartnerField.PARTNER_FIELD_MOUNT,
    }
    Park.setPartnerVisibleUpdateMaskBits(PartnerField.PARTNER_FIELD_END, t)
end

-- 进行配置热更
function ReloadGameConfig(module_name)
    local startTime = SystemUtil.getMsTime64()
    local ret = sGameReloader.doConfigReload(module_name)
    if ret then
        local timeDiff = SystemUtil.getMsTime64() - startTime
        LuaLog.outString(string.format("配置热更 成功, 耗时:%u 毫秒", timeDiff))
    else
        LuaLog.outString(string.format("配置热更 失败"))
    end
    return ret
end

-- 进行逻辑热更
function ReloadGameLogical(module_name)
   local ret = sGameReloader.doLogicalReload(module_name)
    if ret then
        LuaLog.outString(string.format("逻辑热更 成功"))
    else
        LuaLog.outString(string.format("逻辑热更 失败"))
    end
    return ret 
end

--endregion
