--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("MasterScript.module.player.lua_player", package.seeall)

local levelUpExpConf = require("gameconfig.plr_level_up_exp_conf")
local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")

ItemInterfaceModule = require("MasterScript.module.item.item_interface")
local itemProtoConf = require("gameconfig.item_proto_conf")

local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject

LuaPlayer = oo.Class(superClass, "LuaPlayer")

function LuaPlayer:__init(guid)
	
	superClass.__init(self, guid)

    self.cpp_servant = nil
    self.plr_info = nil
    self.item_interface = nil
end

function LuaPlayer:InitData(servant, plrInfo, lastOfflineTime)
    assert(servant ~= nil and plrInfo ~= nil)

    self.cpp_servant = servant
    self.plr_info = plrInfo
    
    self.login_count = 0
    self.last_save_time = Park.getCurUnixTime()     -- 上次保存数据的时间

    self.offline_data_save = false
    self.fight_props = {}
    for i=1,enumFightProp.COUNT do
        self.fight_props[i] = 0
    end

    -- 创建背包实体
    self.item_interface = ItemInterfaceModule.ItemInterface(self)

    -- 读取数据
    return self:LoadFromDB()
end

function LuaPlayer:Logout(curState, needSaveDB)
    -- body

    if self.reg_in_subsystem then
        self:LogoutFromSubSystem()
    end

    if self.plr_info ~= nil then
        self.plr_info:OnPlayerLogout()
    end

    -- 临时timer
    if self.tmp_timer ~= nil then
        gTimer.unRegCallback(self.tmp_timer)
        self.tmp_timer = nil
    end

    -- 保存离线数据
    if needSaveDB and not self.offline_data_save then
        -- 保存物品数据
        self:SaveItemDataToDB()
        -- 保存角色数据
        self:SavePlayerToDB(false, true)
    end

    self.plr_info = nil
    self.cpp_servant = nil
end

function LuaPlayer:Term()
    if self.item_interface ~= nil then
        self.item_interface:Term()
        self.item_interface = nil
    end
end

--[[ 获取角色当前所在场景服id
]]
function LuaPlayer:GetCurSceneServerID()
    local ret = nil
    if self.cpp_servant ~= nil then
        ret = self.cpp_servant:getCurSceneServerID()
    end
    return ret
end

--[[ 登陆至功能子系统
]]
function LuaPlayer:LoginToSubSystem()
    self.reg_in_subsystem = true
end

--[[ 从功能子系统登出
]]
function LuaPlayer:LogoutFromSubSystem()
    self.reg_in_subsystem = false
end

--[[ 填充进入游戏的角色数据
]]
function LuaPlayer:FillEnterGameData(packet)
    local info = self.plr_info

    packet:writeUInt32(Park.getServerGlobalID())
    packet:writeString(info.agent_name)
    packet:writeString(info.acc_name)

    packet:writeUInt64(self:GetGUID())
    packet:writeString(info.role_name)
    packet:writeUInt32(info.career)
    packet:writeUInt32(info.cur_level)

    packet:writeUInt32(self.cur_map_id)
    packet:writeFloat(self.cur_pos_x)
    packet:writeFloat(self.cur_pos_y)
    packet:writeUInt64(self.cur_exp)
    packet:writeUInt32(self.cur_hp)

    -- 金钱数据
    packet:writeUInt64(self.cur_coins)
    packet:writeUInt32(self.cur_bind_cash)
    packet:writeUInt32(self.cur_cash)

    -- 中央服战斗属性
    for i=1,enumFightProp.COUNT do
        packet:writeUInt32(self.fight_props[i])
    end
end

--[[ 获取角色等级
]]
function LuaPlayer:GetLevel()
    local ret = nil
    if self.plr_info ~= nil then
        ret = self.plr_info.cur_level
    end
    return ret
end

--[[ 获取姓名
]]
function LuaPlayer:GetName()
    local ret = ""
    
    if self.plr_info ~= nil then
        ret = self.plr_info.role_name
    end

    return ret
end

--[[ 是否死亡
]]
function LuaPlayer:isDead()
    local ret = false
    if self.cur_hp == 0 then
        ret = true
    end
    return ret
end

--[[ 添加经验
]]
function LuaPlayer:UpdateLevel(addExp, notice)
    if addExp == nil or addExp <= 0 then return end
    if levelUpExpConf == nil then return end

    local plrInfo = self.plr_info
    if plrInfo == nil then return end
    
    local guid = self:GetGUID()
    LuaLog.outDebug(string.format("AddPlayerExp call,guid=%0.f,当前等级=%u,当前经验=%u,addValue=%u", guid, plrInfo.cur_level, self.cur_exp, addExp))
    
    self.cur_exp = self.cur_exp + addExp

    if plrInfo.cur_level >= MAX_PLAYER_LEVEL then
        return 
    end

    -- self:SendPopupNotice(string.format("杀怪增加经验=%u,当前经验=%u", addExp, self.cur_exp))

    local nextLevelExp = levelUpExpConf.GetLevelUpNeedExp(plrInfo.cur_level + 1)

    -- 循环升级
    if self.cur_exp < nextLevelExp then
        -- 通知到客户端处
        if notice then
            self:SendExpUpdateNotice(addExp)
        end
    else

        local oldLevel = plrInfo.cur_level
        while self.cur_exp >= nextLevelExp do
            self.cur_exp = self.cur_exp - nextLevelExp
            plrInfo.cur_level = plrInfo.cur_level + 1
            nextLevelExp = levelUpExpConf.GetLevelUpNeedExp(plrInfo.cur_level + 1)

            if nextLevelExp == nil or nextLevelExp == 0 then
                break
            end
        end

        LuaLog.outString(string.format("玩家:%s(guid=%.0f)升级(%u->%u)", plrInfo.role_name, guid, oldLevel, plrInfo.cur_level))

        -- 将等级同步至场景服
        local t =
        {
            curLevel = plrInfo.cur_level,
            curExp = self.cur_exp
        }
        self:SyncObjectDataToScene(enumSyncObject.PLAYER_DATA_MTS, enumObjDataSyncOp.UPDATE, guid, t)

        -- 保存到DB
        local querySql = string.format("{player_guid:%.0f}", guid)
        local updateSql = string.format("{$set:{cur_level:%u,cur_exp:%.0f}}", plrInfo.cur_level, self.cur_exp)
        sMongoDB.Update("character_datas", querySql, updateSql, true)

        -- 通知到客户端处
        if notice then
            self:SendExpUpdateNotice(addExp)
        end

        -- 通知客户端升级
        self:SendLevelUpNotice(oldLevel, plrInfo.cur_level)
    end
end

--[[ 发送数据包到玩家处
]]
function LuaPlayer:SendPacket(opcode, protoName, msg, errCode)
    -- LuaLog.outDebug(string.format("SendPacket called, op=%u, proto=%s, type(msg)=%s", opcode, protoName, type(msg)))
    
    local pck = WorldPacket.newPacket(opcode)
    if errCode ~= nil and errCode ~= 0 then
        pck:luaSetErrorCode(errCode)
    end

    local ret = gLibPBC.encode(protoName, msg, function(data, len)
        return pck:appendProtobufData(data, len)
    end)
    
    if self.cpp_servant ~= nil then
        self.cpp_servant:sendResultPacket(pck)
        WorldPacket.delPacket(pck)
        -- LuaLog.outDebug(string.format("SendPacket 成功"))
    else
        LuaLog.outError(string.format("lua 发包失败,op=%u,玩家guid=%q", opcode, tostring(self:GetGUID())))
        WorldPacket.delPacket(pck)
    end
    pck = nil 
end

--[[ 发送系统消息包
]]
function LuaPlayer:SendServerPacket(pck, serverOpCode)

    if serverOpCode ~= nil then
        pck:luaSetServerOpcode(serverOpCode)
    end

    if self.cpp_servant ~= nil then
        self.cpp_servant:sendServerPacket(pck)
    else
        LuaLog.outError(string.format("LuaPlayer SendServerPacket 失败,serverOp=%u,玩家guid=%.0f", serverOpCode, self:GetGUID())) 
    end

    WorldPacket.delPacket(pck)
    pck = nil
end

-- 发送弹框消息
function LuaPlayer:SendPopupNotice(msg)
    local msg1014 = 
    {
        notice = msg
    }
    self:SendPacket(Opcodes.SMSG_001014, "UserProto001014", msg1014)
end

--[[ 将玩家数据同步至场景服
]]
function LuaPlayer:SyncObjectDataToScene(objType, op, objGuid, values)

    if self.cpp_servant == nil then
        LuaLog.outError(string.format("SyncObjectDataToScene(obj=%u,op=%u) value(tp=%s) servant error, plrGuid=%.0f", objType, op, type(values), self:GetGUID()))
        return 
    end

    local encodeData = bson.encode(values)
    if encodeData == nil then
        LuaLog.outError(string.format("SyncObjectDataToScene(obj=%u,op=%u) value(tp=%s) encode error, plrGuid=%.0f", objType, op, type(values), self:GetGUID()))
        return 
    end

    local pck = WorldPacket.newPacket(0)
    if pck == nil then
        LuaLog.outError(string.format("SyncObjectDataToScene(obj=%u,op=%u) Packet failed, plrGuid=%.0f", objType, op, self:GetGUID()))
        return 
    end

    pck:writeUInt32(self.cpp_servant:getCurSceneServerID())              -- 发送至对应的场景服
    pck:writeUInt64(self:GetGUID()) -- 属主guid
    pck:writeUInt8(objType)
    pck:writeUInt8(op)
    pck:writeUInt64(objGuid)
    local len = pck:appendUserData(encodeData)
    if len == nil or len <= 0 then
        LuaLog.outError(string.format("SyncObjectDataToScene(obj=%u,op=%u) appendUserData failed, plrGuid=%.0f", objType, op, self:GetGUID()))
    end
    
    self:SendServerPacket(pck, ServerOpcodes.MASTER_2_SCENE_SYNC_OBJ_DATA)
end

--endregion
