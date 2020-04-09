--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.player.lua_player", package.seeall)

--[[ 把数据保存回中央服
]]
function LuaPlayer:SaveBackDataToMaster(session)
	local pck = WorldPacket.newPacket(ServerOpcodes.SCENE_2_MASTER_SAVE_PLAYER_DATA, 256)
	pck:writeUInt32(0)			-- 发送数据至中央服
	pck:writeUInt32(session:getSrcPlatformID())
	pck:writeUInt32(Park.getSelfSceneServerID())
	pck:writeUInt64(self:GetGUID())
	pck:writeUInt32(session:getSessionIndex())
	pck:writeUInt32(session:getNextSceneServerID())
	self:WriteSaveDatas(pck)
	
	-- 发送至中央服
	local ret = session:sendServerPacket(pck)
	if not ret then
		Park.trySendPacket(session:getSrcPlatformID(), pck, ServerOpcodes.SCENE_2_MASTER_SAVE_PLAYER_DATA)
	end
end

--[[ 写入要保存到中央服的数据
]]
function LuaPlayer:WriteSaveDatas(pck)
	pck:writeUInt64(self:GetGUID())
	pck:writeUInt64(self.entity_datas[PLAYER_FIELD_CUR_EXP])
	local mapID = self.cpp_object:getMapID()
	pck:writeUInt32(mapID)
	local posX,posY = self:GetCurPixelPos()
	pck:writeFloat(posX)
	pck:writeFloat(posY)
	pck:writeUInt32(0)			-- instanceID
	pck:writeUInt32(0)			-- instanceExpireTime
	pck:writeUInt32(0)			-- lastMapID
	pck:writeUInt32(0.0)		-- lastMapPosX
	pck:writeUInt32(0.0)		-- lastMapPosY

	LuaLog.outString(string.format("玩家:%s(guid=%.0f)下线保存,当前场景坐标%u:(%.1f,%.1f),pckLen=%u", self.role_name, self:GetGUID(),
					mapID, posX, posY, pck:getCurWritePos()))
end

function LuaPlayer:SyncObjectDataToMaster(objType, op, objGuid, values)
	local encodeData = bson.encode(values)
	if encodeData == nil then
		LuaLog.outError(string.format("SyncObjectDataToMaster(obj=%u,op=%u) value(tp=%s) encode error, plrGuid=%.0f", objType, op, type(values), self:GetGUID()))
		return 
	end

	local pck = WorldPacket.newPacket(0)
	if pck == nil then
		LuaLog.outError(string.format("SyncObjectDataToMaster(obj=%u,op=%u) Packet failed, plrGuid=%.0f", objType, op, self:GetGUID()))
		return 
	end

	pck:writeUInt32(0)				-- 发送至中央服
	pck:writeUInt64(self:GetGUID())	-- 属主guid
	pck:writeUInt8(objType)
	pck:writeUInt8(op)
	pck:writeUInt64(objGuid)
	local len = pck:appendUserData(encodeData)
	if len == nil or len <= 0 then
		LuaLog.outError(string.format("SyncObjectDataToMaster(obj=%u,op=%u) appendUserData failed, plrGuid=%.0f", objType, op, self:GetGUID()))
	end
	
	self:SendServerPacket(pck, ServerOpcodes.SCENE_2_MASTER_SYNC_OBJ_UPDATE_DATA)
end

function LuaPlayer:SetDataSyncFlag()
	if not self.need_sync_data then
		self.need_sync_data = true
		self.cpp_object:setPlayerDataSyncFlag()
	end
end

--endregion
