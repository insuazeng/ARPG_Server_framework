--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- a much simple OO implementation base on loop.simple
module("oo.oo", package.seeall)

local _check = require("oo.oo_check")
_check.openFlag()

local classList = {}  --类列表
local classBase = {}  --基类

-- default construnctor
function classBase:__init(obj)
end

-- IsA operator
function classBase:IsA(cls)
	local c = ClassOf(self)
	while c and c ~= classBase do
		if c == cls then 
            return true 
        end
		c = SuperClassOf(c)
	end
	return false
end

local function new(cls, ...)
	local obj = {}
	local mt = rawget(cls, "_mt_")
	if not mt then
		mt = {
			__index		= cls,
			__tostring	= cls.__tostring,
			__gc		= cls.__gc,
		}
		rawset(cls, "_mt_", mt)
	end
	
	setmetatable(obj, mt)
	obj:__init(...)

	--check
	_check.checkObject(cls, obj)

	return obj
end

function Class(super, name)
	local cls = {}
	if type(name) == "string" then
		local class_name = name..'__'
		if classList[class_name] ~= nil then
			print(">>>>>>>>>>>>>oo.class repeat,is hot upgate? class:", name)
			cls = classList[class_name]
			for k,v in pairs(cls) do
				cls[k] = nil
			end
		else
			classList[class_name] = cls
		end

		--check
		_check.checkClass(cls, class_name)
	else
		error("Error:class name is error!")
	end

	super = super or classBase
	rawset(cls, "__super", super)
	setmetatable(cls, { __index = super, __call = new })
	return cls
end

function SuperClassOf(cls)
	return rawget(cls, "__super")
end

function ClassOf(obj)
	return getmetatable(obj).__index
end

function InstanceOf(obj, cls)
	return ((obj.IsA and obj:IsA(cls)) == true)
end


--endregion
