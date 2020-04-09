--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

local _G = _G
local xpcall = xpcall
local _lua_error_handle = _lua_error_handle

-- 全局变量
gInitializeFunctionTable = {}       -- 初始化函数列表

function InitPackagePath()
    package.path = package.path .. ";./MasterScript/?/.lua;./MasterScript/module/?.lua;./LuaCommon/?.lua;"
end

InitPackagePath()

print("Start load master_main.lua..")
print("Current Lua Path = " .. package.path)

-- 公共定义
require("gamecomm.common_defines")
require("gamecomm.item_defines")
require("gamecomm.opcodes")
require("gamecomm.server_opcodes")
require("gamecomm.error_codes")
require("gamecomm.gm_cmd_dispatcher")
require("lua_extends")

-- reload注册
require("gamecomm.reloader")

-- 网络协议监听注册
require("MasterScript.network.worldpacket_handle_dispatcher")
require("MasterScript.network.serverpacket_handle_dispatcher")
require("MasterScript.network.server_packet_handler")

require("MasterScript.module.advanced.mount_packet_handler")
require("MasterScript.module.charge.charge_packet_handler")
require("MasterScript.module.gmcommand.gm_packet_handler")
require("MasterScript.module.guild.guild_packet_handler")
require("MasterScript.module.item.item_packet_handler")
require("MasterScript.module.mail.mail_packet_handler")
require("MasterScript.module.player.character_packet_handler")
require("MasterScript.module.quest.quest_packet_handler")
require("MasterScript.module.shop.shop_packet_handler")
require("MasterScript.module.skill.skill_packet_handler")
require("MasterScript.module.social.social_packet_handler")
require("MasterScript.module.team.team_packet_handler")

-- 单例
sPlayerMgr = require("MasterScript.module.player.player_manager")
-- MongoDB实例
sMongoDB = require("MasterScript.database.mongo_db")

-- 全局变量
gOO = require("oo.oo")
gTimer = require("timer.timer")
gTool = require("gamecomm.tool_functions")
gLibPBC = require("protobuf")
gLibCJson = require("cjson")

-- 局部变量
local __onGCEventTimeOut = nil

-- 脚本初始化函数
function MasterServerInitialize()
    
    -- 初始化随机种子
    math.randomseed(os.time())
    
    local curUsed = collectgarbage("count") / 1024.0
    LuaLog.outString(string.format("初始化时,内存使用=%.2f MB", curUsed))
    -- 注册gc的定时器
    gTimer.regCallback(120000, ILLEGAL_DATA_32, __onGCEventTimeOut)

    -- 对初始化函数列表中的函数进行初始化
    local initFuncCount = table.getn(gInitializeFunctionTable)
    -- print("initFunctionCount = " .. initFuncCount)
    for i = 1, initFuncCount do
        gInitializeFunctionTable[i]()
    end

    -- 初始化mongodb数据库
    sMongoDB.InitMongoDatabase("localhost", 27017)
    
    -- 初始化playerMgr
    sPlayerMgr.Initialize()
end

-- 脚本终止函数
function MasterServerTerminate()
    print("MasterServer termination..")
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

--[[ 获取账号角色列表数据
]]
function FillAccountRoleListData(accID, agentName, accName)
    sPlayerMgr.FillAccountRoleListInfo(accID, agentName, accName)
end

--[[ 玩家登出
]]
function LogoutPlayer(guid, curState, needSaveData)
    sPlayerMgr.LogoutPlayer(guid, curState, needSaveData)
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
