--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("gamecomm.server_opcodes", package.seeall)

local _G = _G

_G.ServerOpcodes =
{
    --网关连接中央服务器和场景服务器时，验证和注册的包
	CMSG_GATE_2_MASTER_AUTH_REQ				= 1,		-- 验证服务器（网关->中央）
	SMSG_MASTER_2_GATE_AUTH_RESULT			= 2,		-- 验证服务器（中央->网关）

	CMSG_GATE_2_LOGIN_AUTH_REQ				= 3,		-- 验证服务器（网关->登陆）
	SMSG_LOGIN_2_GATE_AUTH_RESULT			= 4,		-- 验证服务器（登陆->网关）

	CMSG_GATE_2_SCENE_AUTH_REQ				= 5,		-- 验证服务器（网关->场景）
	SMSG_SCENE_2_GATE_AUTH_RESULT			= 6,		-- 验证服务器（场景->网关）

	CMSG_GATE_2_MASTER_PING					= 7,		-- PING（网关->中央）
	SMSG_MASTER_2_GATE_PONG					= 8,		-- PONG（中央->网关）

	CMSG_GATE_2_LOGIN_PING					= 9,		-- PING（网关->登陆）
	SMSG_LOGIN_2_GATE_PONG					= 10,		-- PING（登陆->网关）

	CMSG_GATE_2_SCENE_PING					= 11,		-- PING（网关->场景）
	SMSG_SCENE_2_GATE_PONG					= 12,		-- PING（场景->网关）

	CMSG_GATE_2_LOGIN_ACCOUNT_AUTH_REQ		= 13,		-- 验证账户（网关->登陆）
	SMSG_LOGIN_2_GATE_ACCOUNT_AUTH_RET		= 14,		-- 账户验证结果（登陆->网关）
	
	-- 请求账号角色列表
	CMSG_GATE_2_MASTER_ROLELIST_REQ			= 15,		-- 请求获取角色列表（网关->中央）
	-- SMSG_MASTER_2_GATE_ROLELIST_RET		= 16,		-- 角色列表信息（中央->网关），现在直接由中央服直接推送至客户端
	
	-- 创建中央服会话（选取角色创建会话)
	SMSG_MASTER_2_GATE_ROLE_CHOOSE_RET		= 17,		-- 选角结果（中央->网关）
	CMSG_GATE_2_MASTER_ENTER_SCENE_NOTICE	= 18,		-- 玩家成功进入场景后对中央服进行通知（网关->中央）
	
	-- 创建游戏服会话（选取角色进入游戏）
	CMSG_GATE_2_SCENE_ENTER_GAME_REQ		= 19,		-- 选角成功后请求进入游戏创建会话（网关->场景服）
	SMSG_SCENE_2_GATE_ENTER_GAME_RET		= 20,		-- 进入游戏结果（游戏-网关）

	CMSG_GATE_2_MASTER_SESSIONING			= 21,		-- 中央服玩家会话包请求
	SMSG_MASTER_2_GATE_SESSIONING_RET		= 22,		-- 中央服玩家会话包响应

	CMSG_GATE_2_SCENE_SESSIONING			= 23,		-- 场景服玩家会话包请求
	SMSG_SCENE_2_GATE_SESSIONING_RET		= 24,		-- 场景服玩家会话包响应

	CMSG_GATE_2_SCENE_SERVER_CONNECTION_STATUS = 25,	-- 服务器连接状态（网关->场景服）
	CMSG_GATE_2_MASTER_SERVER_CONNECTION_STATUS= 26,	-- 服务器连接状态（网关->中央服）

	CMSG_GATE_2_SCENE_CLOSE_SESSION_REQ		= 27,		-- 网关发起关闭会话
	CMSG_GATE_2_MASTER_CLOSE_SESSION_REQ	= 28,		-- 网关发起关闭会话

	SMSG_MASTER_2_GATE_CLOST_SESSION_NOTICE	= 29,		-- 中央服通知关闭会话
	SMSG_SCENE_2_GATE_CLOSE_SESSION_NOTICE	= 30,		-- 场景服通知关闭会话

	SMSG_SCENE_2_GATE_CHANGE_SCENE_NOTICE	= 31,		-- 通知切换场景（场景->网关）
	CMSG_GATE_2_MASTER_CHANGE_SCENE_NOTICE	= 32,		-- 通知切换场景（网关->中央）
	SMSG_MASTER_2_GATE_CHANGE_SCENE_NOTICE	= 33,		-- 通知切换场景（中央->网关）

	MASTER_2_SCENE_SYNC_OBJ_DATA			= 39,		-- 同步对象数据（中央服->场景服）
	SCENE_2_MASTER_SAVE_PLAYER_DATA			= 40,		-- 保存玩家数据
	SCENE_2_MASTER_SYNC_OBJ_POS_DATA		= 41,		-- 同步玩家在地图上的数据
	SCENE_2_MASTER_SYNC_OBJ_UPDATE_DATA		= 42,		-- 同步玩家对象更新数据（场景服->中央服）
	MASTER_2_SCENE_GM_COMMAND_LINE			= 43,		-- gm命令同步

	SCENE_2_MASTER_SERVER_MSG_EVENT			= 44,		-- 系统消息事件（场景服->中央服）
	MASTER_2_SCENE_SERVER_MSG_EVENT			= 45,		-- 系统消息事件（中央服->场景服）
	SCENE_2_MASTER_PLAYER_MSG_EVENT			= 46,		-- 玩家消息事件（场景服->中央服）
	MASTER_2_SCENE_PLAYER_MSG_EVENT			= 47,		-- 玩家消息事件（中央服->场景服）
	SCENE_2_SCENE_SERVER_MSG_EVENT			= 48,		-- 系统消息事件（场景服A->场景服B）
}

--endregion
