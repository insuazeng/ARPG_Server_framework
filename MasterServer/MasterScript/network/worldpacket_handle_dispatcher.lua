--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("MasterScript.network.worldpacket_handle_dispatcher", package.seeall)

local _G = _G

if _G.sWorldPacketDispatcher == nil then
    _G.sWorldPacketDispatcher = {}
end

local PacketDispatcher = _G.sWorldPacketDispatcher

PacketDispatcher.initProtobufProto = function()
    gLibPBC.register_file("Config/ProtobufData/UserProtoPublic.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto001.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto002.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto003.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto004.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto006.pb")
    gLibPBC.register_file("Config/ProtobufData/UserProto007.pb")
end

-- 注册网络包的响应函数
PacketDispatcher.registerPacketHandler = function (msg_id, pb_proto_name, func)
    if type(msg_id) ~= "number" then
        return 
    end

    if not PacketDispatcher[msg_id] then
        PacketDispatcher[msg_id] = { pb_proto_name, func }
    else
        PacketDispatcher[msg_id] = {}
        PacketDispatcher[msg_id] = { pb_proto_name, func }
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
PacketDispatcher.packetDispatcher = function (msg_id, guid, msg, msg_len)
    if PacketDispatcher[msg_id] == nil then
        LuaLog.outError(string.format("未找到消息号=%u的处理函数,guid=%.0f,msgLen=%u", msg_id, guid, msg_len))
        return 
    end
    -- LuaLog.outDebug(string.format("[WorldPacket]msg_len=%u, type=%s, type pbc=%s", msg_len, type(msg), type(gLibPBC)))
    -- 利用pb解析msg,而后把解析好的msg调用给相应函数
    local buf = nil 
    if msg_len ~= nil and msg_len > 0 then
        buf = gLibPBC.decode(PacketDispatcher[msg_id][1], msg, msg_len)
        if buf == nil then
            LuaLog.outError(string.format("libPBC解析protobuf数据失败,玩家guid=%.0f,opcode=%u,len=%u", guid, msg_id, msg_len))
            return 
        end    
    end
    -- LuaLog.outDebug(string.format("msg_id=%u, proto=%s type[2]=%s", msg_id, PacketDispatcher[msg_id][1], type(PacketDispatcher[msg_id][2])))
    PacketDispatcher[msg_id][2](guid, buf)
end

-- 在初始化函数中,对pb文件进行解析
table.insert(gInitializeFunctionTable, PacketDispatcher.initProtobufProto)

--endregion
