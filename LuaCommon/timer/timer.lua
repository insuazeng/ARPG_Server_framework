--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("timer.timer", package.seeall)

local callbackIDs = {}              -- 事件表
local lastCallbackID = 0            -- 回调事件id
local maxRepeatCount = 0xFFFFFFFF   -- 最大回调次数

-- 生成回调事件id
local function generateCallbackID()
    local callbackID = lastCallbackID

    repeat
        callbackID = (callbackID + 1) % 0x10000000;     -- 在2亿多内循环
        if callbackIDs[callbackID] == nil then
            lastCallbackID = callbackID
            return callbackID
        end
    until callbackID == lastCallbackID

    return nil
end

--[[事件到时
参数1:事件id
]]
onEventTimer = function(callbackID)

    local info = callbackIDs[callbackID]
    -- 事件已经被取消则不理会
    if info == nil then
        return 
    end

    -- 回调次数加1
    info[3] = info[3] + 1

    -- 进行函数回调(参数1:回调id,参数2:第x次调用,参数3:对象)
    -- LuaLog.outString(string.format("event:%u Prepare called", callbackID))
    info[4](callbackID, info[3], info[5])
    -- LuaLog.outString(string.format("event:%u called", callbackID))

    -- 如果次数已经到达指定回调次数,则取消该定时器,否则继续进行注册
    if info[2] ~= maxRepeatCount and info[3] >= info[2] then
        unRegCallback(callbackID)
    else
        -- LuaLog.outDebug(string.format("准备进行下一次注册,间隔=%u ms,cbID=%u", info[1], callbackID))
        LuaTimer.addTimer(info[1], callbackID)
    end

end

--[[注册回调事件
参数1:间隔事件
参数2:重复次数
参数3:回调函数
参数4:对象实体
]]
regCallback = function(intervalMsTime, repeatCount, callbackFunc, obj)
    local callbackID = generateCallbackID()
    if callbackID == nil then
        LuaLog.outError(string.format("无法为lua定时器申请生成事件id"))
        return 
    end

    if repeatCount <= 0 then
        repeatCount = 1
    end

    --                          间隔时间,       重复次数, 当前次数,  回调函数
    callbackIDs[callbackID] = { intervalMsTime, repeatCount, 0, callbackFunc, obj }

    -- 注册到C++
    LuaTimer.addTimer(intervalMsTime, callbackID)

    -- 返回注册的callbackID
    return callbackID
end

-- 取消回调事件
unRegCallback = function(callbackID)
    -- LuaLog.outDebug(string.format("取消定时器事件,id=%u", callbackID))
    callbackIDs[callbackID] = nil
end


--endregion
