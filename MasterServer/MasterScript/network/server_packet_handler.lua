--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 处理服务端之间的消息通信
module("MasterScript.network.server_packet_handler", package.seeall)
local _G = _G

--[[ 从场景服同步对象位置数据
]]
local function HandleSyncPostionDataFromScene(msg, msg_len)
	-- LuaLog.outDebug(string.format("HandleSyncPostionDataFromScene called, type msg=%s, msg_len=%u", type(msg), msg_len))
	local undef = msg:readUInt32()
	local mapCount = msg:readUInt16()
	local plrCount, plrGuid, mapID, posValue, posX, posY
	local plr = nil

	for i=1, mapCount do
		mapID = msg:readUInt32()
		plrCount = msg:readUInt16()
		for j=1, plrCount do
			plrGuid = msg:readUInt64()
			posX = msg:readFloat()
			posY = msg:readFloat()
			plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
			if plr ~= nil then
				plr.cur_map_id = mapID
				plr.cur_pos_x = posX
				plr.cur_pos_y = posY				
				-- LuaLog.outDebug(string.format("玩家 guid=%.0f 的坐标被场景服同步为 %u(%.1f,%.1f)", plrGuid, mapID, posX, posY))
			end
		end
	end
end

--[[ 从场景服同步对象数据
]]
local function HandleSyncObjectDataFromScene(msg, msg_len)
	-- LuaLog.outDebug(string.format("HandleSyncObjectDataFromScene called, type msg=%s, msgLen=%u", type(msg), msg_len))
	local var = msg:readUInt32()				-- 中央服id
	local plrGuid = msg:readUInt64()
	local plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
	if plr == nil then
		LuaLog.outError(string.format("无法找到玩家(guid=%.0f)对象,SyncObjectData失败", plrGuid))
		return 
	end

	local objType = msg:readUInt8()
	if objType == enumSyncObject.PLAYER_DATA_STM then
		var = msg:readUInt8()		-- 操作类型
		var = msg:readUInt64()
		local bsonData, len = msg:readUserData() -- 读取同步来的bson数据
		if len == nil then 
			return  
		end
		local decodeData = bson.decode(bsonData)
		if decodeData == nil then
			return 
		end

		if decodeData.addExp > 0 then
			plr:UpdateLevel(decodeData.addExp, true)
		end

		if decodeData.curHp ~= nil then
			plr.cur_hp = decodeData.curHp
			-- LuaLog.outDebug(string.format("中央服玩家:%s(guid=%.0f)血量被设置为=%u", plr:GetName(), plr:GetGUID(), plr.cur_hp))
			if plr:isDead() then
				LuaLog.outDebug(string.format("玩家:%s(guid=%.0f)死亡", plr:GetName(), plr:GetGUID()))
			end
		end
	end
end

--[[ 通知角色进入场景
]]
local function HandleEnterSceneNotice(msg, msg_len)
	-- LuaLog.outDebug(string.format("HandleEnterSceneNotice called, type msg=%s", type(msg)))
	
	local guid = msg:readUInt64()
	local sceneServerID = msg:readUInt32()
	local enterTime = msg:readUInt32()
	local mapID = msg:readUInt32()
	local curPosX = msg:readFloat()
	local curPosY = msg:readFloat()
	local loginIp = msg:readString()

	local servant = CPlayerMgr.findPlayerServant(guid)
	local plr = sPlayerMgr.GetPlayerByGuid(guid)
	if servant == nil or plr == nil then
		LuaLog.outError(string.format("未找到合法的servant或plr对象(guid=%q,servant=%s,plr=%s),设置登陆成功信息失败", tostring(guid), type(servant), type(plr)))
		return 
	end
	local plrInfo = plr.plr_info
	assert(plrInfo ~= nil)

	if plr.cur_login_time == nil or plr.cur_login_time == 0 then
		plr.cur_login_time = Park.getCurUnixTime()
	end

	-- 设置servant的进入信息
	servant:onEnterSceneNotice(sceneServerID, enterTime)
	LuaLog.outString(string.format("中央服确认玩家:%s(guid=%.0f)进入场景服%u(mapID=%u,pos=%.1f,%.1f)", plrInfo.role_name, plr:GetGUID(), 
					sceneServerID, mapID, curPosX, curPosY))
end

--[[ 场景服通知保存玩家数据(下线保存或切换至下一场景服)
]]
local function HandleSavePlayerDataReq(msg, msg_len)
	local srcPlatformID = msg:readUInt32()
	srcPlatformID = msg:readUInt32()
	local srcSceneServerID = msg:readUInt32()
	local plrGuid = msg:readUInt64()
	local sessionIndex = msg:readUInt32()
	local selfPlatformID = Park.getServerGlobalID()

	if srcPlatformID ~= selfPlatformID then
		LuaLog.outError(string.format("场景服:%u试图保存一个来自非本平台(self=%u,传参=%u)玩家guid=%.0f数据", srcSceneServerID, selfPlatformID, srcPlatformID, plrGuid))
		return 
	end

	local servant = CPlayerMgr.findPlayerServant(plrGuid)
	local plr = sPlayerMgr.GetPlayerByGuid(plrGuid)
	if servant == nil or plr == nil then
		LuaLog.outError(string.format("保存数据时未找到Servant或LuaPlayer对象(guid=%.0f,tpS=%s,tpP=%s)", plrGuid, type(servant), type(plr)))
		return 
	end

	if sessionIndex ~= servant:getSessionIndex() then
		LuaLog.outError(string.format("玩家(guid=%.0f)sessionIndex不匹配(当前=%u,传入=%u),无法进行数据保存处理)", plrGuid, servant:getSessionIndex(), sessionIndex))
		return 
	end

	local nextSceneServerID = msg:readUInt32()
	-- 如果下一个场景ID不为0,说明需要传送至下一场景服
	local saveToDB = true
	if nextSceneServerID ~= 0 then
		saveToDB = false
	end

	-- 同步从场景服传来的玩家数据
	plr:AsyncDataFromSceneServer(msg, saveToDB)

	if nextSceneServerID ~= 0 then
		-- 将数据传递至下一场景服
		local pck = WorldPacket.newPacket(ServerOpcodes.SMSG_MASTER_2_GATE_CHANGE_SCENE_NOTICE, 256)
		if pck == nil then
			return 
		end
		pck:writeUInt64(plrGuid)
		pck:writeUInt32(nextSceneServerID)
		pck:writeUInt32(plr.cur_map_id)
		pck:writeFloat(plr.cur_pos_x)
		pck:writeFloat(plr.cur_pos_y)
        pck:writeFloat(SceneMapMgr.getPositionHeight(plr.cur_pos_x, plr.cur_pos_y))
		pck:writeUInt32(Park.getCurUnixTime())
		plr:FillEnterGameData(pck)
		servant:sendServerPacket(pck)
		WorldPacket.delPacket(pck)
	else
		-- 玩家已保存好数据,下线
		LuaLog.outString(string.format("[下线]场景服%u 请求保存玩家(guid=%.0f,index=%u)数据并下线", srcSceneServerID, plrGuid, sessionIndex))
		CPlayerMgr.invalidServant(plrGuid, sessionIndex, false, true, true)
	end
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.SCENE_2_MASTER_SYNC_OBJ_POS_DATA, HandleSyncPostionDataFromScene)       -- 从场景服同步位置数据
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.SCENE_2_MASTER_SYNC_OBJ_UPDATE_DATA, HandleSyncObjectDataFromScene)     -- 从场景服同步对象数据
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.CMSG_GATE_2_MASTER_ENTER_SCENE_NOTICE, HandleEnterSceneNotice)       	-- 通知中央服进入场景
    sServerPacketDispatcher.registerPacketHandler(ServerOpcodes.SCENE_2_MASTER_SAVE_PLAYER_DATA, HandleSavePlayerDataReq)       		-- 保存玩家数据
end)

--endregion
