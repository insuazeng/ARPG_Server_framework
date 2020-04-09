--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("oo.oo_check", package.seeall)

--************内存检测***********

local classNameList = {}    --类列表 table->name
local objList = {}          --对象列表 table->name

setmetatable(objList, {["__mode"]='k'})

local objCountList = {}     --对象创建次数列表 name->count
local checkCount = 0        --check打印次数
local checkFlag = false     --check开关

function openFlag()
	checkFlag = true
end

function closeFlag()
	checkFlag = false
end

--标记类
function checkClass(clz, classNm)
	if checkFlag then
		classNameList[clz] = classNm
	end
end

--标记对象
function checkObject(clz, obj)
	if checkFlag then
		local name = classNameList[clz] or "none"
		objList[obj] = name
		objCountList[name] = (objCountList[name] or 0) + 1
	end
end

function checkPrint(logFunc)
	collectgarbage("collect")

	local c = 0
	local list = {}
	for k,nm in pairs(objList) do
		c = c + 1
		list[nm] = (list[nm] or 0) + 1
	end

	checkCount = checkCount + 1
	local str = string.format("$$$$$$$$$$obj size is %d, check count is %d\n", c, checkCount)

	for nm,cn in pairs(objCountList) do
		if list[nm] ~= nil and list[nm] > 1 then
			str = str .. string.format("\t\t%s create %d time,now exist %d!\n", 
			nm, cn, list[nm] or 0)
		end
	end

	--log
	if logFunc ~= nil then
		logFunc(str)
	else
		print(str, string.len(str))
	end
end


--endregion
