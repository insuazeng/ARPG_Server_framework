--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("lua_extends", package.seeall)

--string
function string.trim(s)
    return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

function string.split(szFullString, szSeparator, needtrim)
    local nFindStartIndex = 1
    local nSplitIndex = 1
    local nSplitArray = {}
    while true do
        local nFindLastIndex = string.find(szFullString, szSeparator, nFindStartIndex)
        if not nFindLastIndex then
            local str = string.sub(szFullString, nFindStartIndex, string.len(szFullString))
            if needtrim then str=string.trim(str) end
            nSplitArray[nSplitIndex] = str
            break
        end

        local str = string.sub(szFullString, nFindStartIndex, nFindLastIndex - 1)
        if needtrim then str=string.trim(str) end

        nSplitArray[nSplitIndex] = str
        nFindStartIndex = nFindLastIndex + string.len(szSeparator)
        nSplitIndex = nSplitIndex + 1
    end
    return nSplitArray
end

function string.append(...)
    local args = {...}
    local t = {}
    for _,v in ipairs(args)do
        table.insert(t, v)
    end
    return table.concat(t)
end

function string.startWith(str, key)
    return str:sub(0, #key) == key
end

function string.endWith(str, key)
    return str:sub(-#key) == key
end

function string.isNilOrEmpty (str)
    return str == nil or str == ""
end

--table
function table.isEmpty(tab)
    return not next(tab)
end

function table.length( tab )
    local length = 0
    for _, v in pairs( tab ) do
        length = length + 1
    end
    return length
end

function table.clone(tb)
    if type(tb) ~= "table" then return false end

    local rt = {}
    for i, v in pairs(tb) do
        if type(v) == "table" then
            rt[i] = table.clone(v)
        else
            rt[i] = v
        end
    end

    return rt
end

function table.free(t)
    if type(t) ~= "table" then
        return 
    end
    
    local k,v = next(t)
    while k do
        t[k] = nil
        k,v = next(t)
    end
    t.tag = "freed table"
end

function table.lightCopy(t)
    local copy = {}
    for k,v in pairs(t)do
        copy[k]=v
    end
    return copy
end

function table.removeAll(t, v)
    fassert(v ~= nil, "value is nil!")
    local n = #t
    for i = n, 1, -1 do
        if t[i] == v then
            t[i] = nil
        end
    end
end

--compat lua 5.1
table.unpack = unpack or table.unpack

table.tabToStr = function(value)
    assert( type(value) == "table" )
    return DataDumper(value)
end

local function strToTab( str )
    local chunk = nil
    if not string.startWith(str, "return") then
        chunk = loadstring("return "..str)
    else
        chunk = loadstring(str)
    end
    if chunk then
        local sandbox = {}
        setfenv(chunk, sandbox)
        return unpack({chunk()})
    else
        error(string.format("Invalid table string %s",str))
    end
end
table.strToTab = strToTab


--math
math.rad2deg = 57.2958
math.deg2rad = 0.0174533

function math.clamp(num, min, max)
    if num < min then num = min end
    if num > max then num = max end
    return num
end

--阶乘
local math = math
function math.fact(n)
    local result = 1
    for i=1,n,1 do
        result = result * i
    end
    return result
end

--排列，从n个中选m个
function math.combination(n, m)
    return math.fact (n) / (math.fact (m) * math.fact (n-m))
end

function sortpairs(tab, cmp)
    local orderArr = {}
    for k,v in pairs(tab)do
        table.insert(orderArr, {k,v})
    end
    table.sort(orderArr, cmp)

    local index = 0
    return function()
        index = index + 1
        local item = orderArr[index]
        if not item then
            return nil
        end
        return item[1],item[2]
    end
end


--endregion
