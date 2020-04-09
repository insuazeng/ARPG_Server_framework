--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 处理服务端之间的消息通信
module("SceneScript.network.server_packet_handler", package.seeall)
local _G = _G

--[[ 接收中央服同步过来的对象数据
]]
local function HandleSyncObjectDataFromMaster(msg, msg_len)
	LuaLog.outDebug(string.format("HandleSyncObjectDataFromMaster called, type msg=%s, msgLen=%u", type(msg), msg_len))

	local var = msg:readUInt32()				-- 场景服id
	local plrGuid = msg:readUInt64()
	local plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
	if plr == nil then
		LuaLog.outError(string.format("无法找到玩家(guid=%.0f)对象,SyncObjectData失败", plrGuid))
		return 
	end

	local objType = msg:readUInt8()
	if objType == enumSyncObject.PLAYER_DATA_MTS then
		var = msg:readUInt8()		-- 操作类型
		var = msg:readUInt64()
		local bsonData, len = msg:readUserData() -- 读取同步来的bson数据
		if len == nil then return end
		
		local decodeData = bson.decode(bsonData)
		if decodeData == nil then return end

		if decodeData.curLevel ~= nil then
			-- 更新最新的等级和经验
			local oldLevel = plr:GetLevel()
			if decodeData.curLevel > oldLevel then
				plr:OnPlayerLevelUp(decodeData.curLevel)
			end
		end
		if decodeData.curCoins ~= nil then
			plr.cur_coins = decodeData.curCoins
			LuaLog.outDebug(string.format("玩家(guid=%.0f)[铜币]被设置为=%.0f", plrGuid, plr.cur_coins))
		end
		if decodeData.curBindCash ~= nil then
			plr.cur_bind_cash = decodeData.curBindCash
			LuaLog.outDebug(string.format("玩家(guid=%.0f)[绑元]被设置为=%u", plrGuid, plr.cur_bind_cash))
		end
		if decodeData.curCash ~= nil then
			plr.cur_cash = decodeData.curCash
			LuaLog.outDebug(string.format("玩家(guid=%.0f)[元宝]被设置为=%u", plrGuid, plr.cur_cash))
		end

	elseif objType == enumSyncObject.FIGHT_PROPS_MTS then
		var = msg:readUInt8()		-- 操作类型
		var = msg:readUInt64()
		local bsonData, len = msg:readUserData() -- 读取同步来的bson数据
		if len == nil then return end
		
		local decodeData = bson.decode(bsonData)
		if decodeData == nil then return end
		-- LuaLog.outDebug("中央服战斗属性更新")
		for k,v in pairs(decodeData) do
			-- LuaLog.outDebug(string.format("idx=%u,val=%u,%s,%s", k, v, type(k), type(v)))
			plr.master_fight_props[tonumber(k)] = v
		end
		-- 重新计算场景服属性
		plr:CalcuProperties(true)
	else

	end
end

--[[ 玩家事件
]]
local function HandlePlayerEventFromMaster(msg, msg_len)

end

--[[ 中央服系统事件
]]
local function HandleSystemEventFromMaster(msg, msg_len)

end

--[[ 场景服系统事件(由其他场景服推送过来)
]]
local function HandleSystemEventFromOtherScene(msg, msg_len)

end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.MASTER_2_SCENE_SYNC_OBJ_DATA, HandleSyncObjectDataFromMaster)
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.MASTER_2_SCENE_PLAYER_MSG_EVENT, HandlePlayerEventFromMaster)
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.MASTER_2_SCENE_SERVER_MSG_EVENT, HandleSystemEventFromMaster)
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.SCENE_2_SCENE_SERVER_MSG_EVENT, HandleSystemEventFromOtherScene)
end)

--endregion
