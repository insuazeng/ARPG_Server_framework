--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gamecomm.reloader", package.seeall)

local _G = _G

if _G.sGameReloader == nil then
    _G.sGameReloader = {}
end

-- 添加配置热更
sGameReloader.regConfigReloader = function(module_name, reloadFunc)
    if sGameReloader.configReloadFuncList == nil then
        sGameReloader.configReloadFuncList = {}
    end
    local funcList = sGameReloader.configReloadFuncList
    if funcList[module_name] == nil then
        funcList[module_name] = {}
    end
    table.insert(funcList[module_name], reloadFunc)
end

-- 进行配置热更
sGameReloader.doConfigReload = function(module_name)
	local reloadConfTable = sGameReloader.configReloadFuncList
    if reloadConfTable == nil then
        return false
    end

    if module_name == nil then
        -- 所有配置表进行热更
        LuaLog.outString(string.format("准备进行所有模块的配置热更"))
        for k,v in pairs(reloadConfTable) do
            local reloadFuncList = v
            for i=1, #reloadFuncList do
                reloadFuncList[i]()
            end
            LuaLog.outString(string.format("模块:%s 配置数据reload完成", k))
        end
    else
        LuaLog.outString(string.format("准备进行模块:%s 的配置热更", module_name))
        -- 指定模块的配置表进行热更
        if reloadConfTable[module_name] ~= nil then
            local reloadFuncList = reloadConfTable[module_name]
            for i=1, #reloadFuncList do
                reloadFuncList[i]()
            end
        else
            return false
        end
    end
    return true
end

-- 添加模块功能热更
sGameReloader.regModuleLogicReloader = function(module_name, reloadFunc)
    if sGameReloader.logicalReloadFuncList == nil then
        sGameReloader.logicalReloadFuncList = {}
    end
    local funcList = sGameReloader.logicalReloadFuncList
    if funcList[module_name] == nil then
        funcList[module_name] = {}
    end
    table.insert(funcList[module_name], reloadFunc)
end

-- 进行模块逻辑热更
sGameReloader.doLogicalReload = function(module_name)
    
end

--endregion
