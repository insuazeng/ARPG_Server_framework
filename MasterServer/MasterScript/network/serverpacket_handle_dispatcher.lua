--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("MasterScript.network.serverpacket_handle_dispatcher", package.seeall)

local _G = _G

if _G.sServerPacketDispatcher == nil then
    _G.sServerPacketDispatcher = {}
end

local PacketDispatcher = _G.sServerPacketDispatcher

PacketDispatcher.initProtobufProto = function()
    -- gLibPBC.register_file("Config/ProtobufData/UserProtoPublic.pb")
    
end

-- 注册网络包的响应函数
PacketDispatcher.registerPacketHandler = function (msg_id, func)
    if type(msg_id) ~= "number" then
        return 
    end

    if not PacketDispatcher[msg_id] then
        PacketDispatcher[msg_id] = func
    else
        PacketDispatcher[msg_id] = {}
        PacketDispatcher[msg_id] = func
    end

end

-- 反注册
PacketDispatcher.unregisterPacketHandler = function (msg_id)
    if type(msg_id) ~= "number" then
        return 
    end

    if PacketDispatcher[msg_id] ~= nil then
        PacketDispatcher[msg_id] = nil
    end
    
end

-- 消息包分发
PacketDispatcher.packetDispatcher = function (msg_id, msg, msg_len)
    if PacketDispatcher[msg_id] == nil then
        LuaLog.outError(string.format("[ServerPacket]未找到msgID=%u的处理函数,msgLen=%u", msg_id, msg_len))
        return 
    end

    -- 临时做法,用msg数据构建一个pck包,而后把pck对象交给各函数去执行
    local pck = WorldPacket.newPacket(msg_id)
    -- 通过LString的方式把数据写入WorldPacket
    local len = pck:writeLString(msg_len, msg)
    -- LuaLog.outDebug(string.format("Write len=%u, msg_id=%u, type[msg_id]=%s", len, msg_id, type(PacketDispatcher[msg_id])))
    PacketDispatcher[msg_id](pck, msg_len)
    WorldPacket.delPacket(pck)
    -- PacketDispatcher[msg_id](msg, msg_len)
end

-- 在初始化函数中,对pb文件进行解析
table.insert(gInitializeFunctionTable, PacketDispatcher.initProtobufProto)

--endregion
