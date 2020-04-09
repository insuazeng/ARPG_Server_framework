--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gamecomm.tool_functions", package.seeall)

local _G = _G

--[[ 加载模块
]]

function require_ex(module_name)
	LuaLog.outString(string.format("准备 require_ex = %s", module_name))
  	if package.loaded[module_name] then
   		LuaLog.outString(string.format("%s已被加载,准备重置", module_name))
  	end

  	package.loaded[module_name] = nil

  	return require(module_name)
end

--[[ 计算两点间距离
]]
function CalcuDistance(x1, y1, x2, y2)
	local xDiff = x1 - x2
	local yDiff = y1 - y2
	return math.sqrt(math.pow(xDiff, 2) + math.pow(yDiff, 2))
end

--[[ 计算游戏内方向
]]
function CalcuDirection(x1, y1, x2, y2)
	
end


--endregion
