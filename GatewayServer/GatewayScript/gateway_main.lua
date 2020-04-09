--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

local _G = _G
local xpcall = xpcall
local _lua_error_handle = _lua_error_handle

function InitPackagePath()
    package.path = package.path .. ";./LuaCommon/?.lua;"
end

InitPackagePath()

print("Start load gateway_main.lua ...")
print("Current Lua Path = " .. package.path)

--公共定义
require("gamecomm.opcodes")

local op = _G.Opcodes
local reg = OpDispatcher.regOpcodeTransferDest

local GATEWAY = 1	-- 网关
local MASTER = 2	-- 中央服
local SCENE = 4		-- 场景服

local function regOpcodeTransferDestServer()
	-- 网关服处理的opcode
	reg(op.CMSG_001003, GATEWAY)
	reg(op.CMSG_001011, GATEWAY)

	-- 中央服
	reg(op.CMSG_001007, MASTER)
	reg(op.CMSG_001009, MASTER)
	reg(op.CMSG_001015, MASTER)
	reg(op.CMSG_003009, MASTER)
	reg(op.CMSG_003011, MASTER)
	reg(op.CMSG_003013, MASTER)
	reg(op.CMSG_006001, MASTER)
	reg(op.CMSG_006003, MASTER)
	reg(op.CMSG_006005, MASTER)
	reg(op.CMSG_006007, MASTER)
	reg(op.CMSG_006009, MASTER)
	reg(op.CMSG_006011, MASTER)
	reg(op.CMSG_006017, MASTER)
	reg(op.CMSG_007001, MASTER)
	reg(op.CMSG_007003, MASTER)
	reg(op.CMSG_007005, MASTER)
	reg(op.CMSG_007007, MASTER)

	-- 场景服
	reg(op.CMSG_001017, SCENE)
	reg(op.CMSG_001019, SCENE)
	reg(op.CMSG_001021, SCENE)
	reg(op.CMSG_002001, SCENE)
	reg(op.CMSG_002003, SCENE)
	reg(op.CMSG_002005, SCENE)
	reg(op.CMSG_003001, SCENE)
	reg(op.CMSG_003007, SCENE)
	reg(op.CMSG_004001, SCENE)
	reg(op.CMSG_004003, SCENE)
end

-- 脚本初始化函数
function GatewayServerInitialize()
	-- 注册给C++
	regOpcodeTransferDestServer()
	OpDispatcher.printOpcodeTransDetail()
end

-- 脚本终止函数
function GatewayServerTerminate()
    print("GatewayServer lua being Terminate..\n")
end

--endregion
