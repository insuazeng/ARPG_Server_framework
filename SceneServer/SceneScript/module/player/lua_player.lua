--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.player.lua_player", package.seeall)

local oo = require("oo.oo")
local ooSUPER = require("SceneScript.module.common.scene_fight_object")
local superClass = ooSUPER.SceneFightObject

local PartnerModule = require("SceneScript.module.partner.lua_partner")

local NORMAL_SKILL_START_ID = 100101
local NORMAL_SKILL_MAX_ID = 100104

LuaPlayer = oo.Class(superClass, "LuaPlayer")

function LuaPlayer:__init(guid)
    superClass.__init(self, guid, enumSceneFightObjectType.PLAYER, 0)

    LuaLog.outDebug(string.format("LuaPlayer(guid=%q)被创建", tostring(guid)))
    -- LuaLog.outDebug(string.format("LuaPlayer(guid=%q)被创建, typeObj=%s", tostring(guid), type(obj)))
end

function LuaPlayer:InitData(platformID, guid, session, plrData, dataLen)
    local startTime = SystemUtil.getMsTime64()

    -- 初始化entityData数据
    for i=1,PlayerField.PLAYER_FIELD_END do
        self.entity_datas[i-1] = 0
    end

    local dataBuffer = WorldPacket.newPacket(0)
    dataBuffer:writeLString(dataLen, plrData)

    LuaLog.outDebug(string.format("准备创建玩家guid:%.0f的C++对象", guid))
    local cppPlayer = Player.newCppPlayerObject()
    if cppPlayer == nil then
        return false
    end

    cppPlayer:initData(session, guid, PlayerField.PLAYER_FIELD_END)
    superClass.SetCppObject(self, cppPlayer)

    -- 从dataBuf中读取玩家数据
    self:SetEntityData(ObjField.OBJECT_FILED_GUID, dataBuffer:readUInt64())
    self.role_name = dataBuffer:readString()
    cppPlayer:setPlayerUtf8Name(self.role_name)
    self:SetEntityData(PlayerField.PLAYER_FIELD_PLATFORM, platformID)
    self:SetEntityData(PlayerField.PLAYER_FIELD_CAREER, dataBuffer:readUInt32())
    self:SetEntityData(UnitField.UNIT_FIELD_LEVEL, dataBuffer:readUInt32())

    local mapID = dataBuffer:readUInt32()
    local posX = dataBuffer:readFloat()
    local posY = dataBuffer:readFloat()
    cppPlayer:initMapPosData(mapID, posX, posY, math.random(0, 255))
    
    self:SetEntityData(PlayerField.PLAYER_FIELD_CUR_EXP, dataBuffer:readUInt64())
    local curHp = dataBuffer:readUInt32()
    if curHp == 0 then
        curHp = 1
    end
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, curHp)
    self.cur_coins = dataBuffer:readUInt64()
    self.cur_bind_cash = dataBuffer:readUInt32()
    self.cur_cash = dataBuffer:readUInt32()

    -- 读取中央服战斗属性
    self.master_fight_props = {}
    for i=1,enumFightProp.COUNT do
        self.master_fight_props[i] = dataBuffer:readUInt32()
    end
    
    -- 设置默认时装
    if self.entity_datas[PlayerField.PLAYER_FIELD_FASHION] == 0 then
        self:SetEntityData(PlayerField.PLAYER_FIELD_FASHION, 1)
    end

    self.exp_add_value = 0          -- 经验增加值
    self.need_sync_data = false     -- 数据同步标记
    self.fight_power = 0            -- 战斗力

    -- 普通技能使用信息记录
    self.normal_skill_info = { 0, 0, nil }   -- [1]:上次使用的普攻技能id,[2]:上次进行普攻的毫秒时间,[3]:技能proto
    -- 伙伴列表
    self.partner_list = { }
    
    WorldPacket.delPacket(dataBuffer)

    -- 计算战斗属性
    self:CalcuProperties(false)

    local timeDiff = SystemUtil.getMsTime64() - startTime
    LuaLog.outDebug(string.format("玩家:%s(guid=%.0f)上线,职业=%u,hp=%u,lv=%u,上线时场景坐标(%u:%.1f,%.1f),元宝=%u,绑元=%u,耗时%u毫秒", self.role_name, self:GetGUID(), self.entity_datas[PlayerField.PLAYER_FIELD_CAREER], 
                    self.entity_datas[UnitField.UNIT_FIELD_CUR_HP], self:GetLevel(), mapID, posX, posY, self.cur_cash, self.cur_bind_cash, timeDiff))

    return true
end

function LuaPlayer:Term()
    superClass.Term(self)

    -- 清除掉伙伴数据
    for k,v in pairs(self.partner_list) do
        v:Term()
    end
    self.partner_list = {}
end

--[[ 角色登出前的操作
]]
function LuaPlayer:BeforeLogout()

end

--[[ 从子系统中登出
]]
function LuaPlayer:LogoutDataFromSubSystem()

end

--[[ 角色登出
]]
function LuaPlayer:Logout(session, saveDataToMaster)
    LuaLog.outDebug(string.format("LuaPlayer(guid=%.0f) Logout called, session=%s", self:GetGUID(), type(session)))
    -- 登出前的处理
    self:BeforeLogout()
    
    -- 把数据保存回中央服
    if saveDataToMaster then
        self:SaveBackDataToMaster(session)
        session:setNextSceneServerID(0)
    end

    -- 退出当前场景
    if self.cpp_object:isInWorld() then
        self.cpp_object:removeFromWorld()
    end

    self:LogoutDataFromSubSystem()

    -- 析构Player对象
    Player.delCppPlayerObject(self.cpp_object)
    self:SetCppObject(nil)
end

--[[ 设置角色数据
]]
function LuaPlayer:SetEntityData(index, val)
    -- body
    if index == nil or index < 0 or index >= PlayerField.PLAYER_FIELD_END then
        return 
    end
    self.entity_datas[index] = val

    -- 设置到C++处
    self.cpp_object:setUInt64Value(index, val)

    -- 如果是当前血量变化,需要同步至中央服(未来此处会优化一下)
    if index == UnitField.UNIT_FIELD_CUR_HP then
        self:SetDataSyncFlag()
    end
end

--[[ 是否玩家
]]
function LuaPlayer:isPlayer()
    return true
end

--[[ 增加经验
]]
function LuaPlayer:AddPlayerExp(addExp)
    if addExp <= 0 then return end

    self.exp_add_value = self.exp_add_value + addExp
    self:SetDataSyncFlag()
end

--[[ 升级
]]
function LuaPlayer:OnPlayerLevelUp(newLevel)

    local fProp = self.fight_props
    local oldLevel = self:GetLevel()
    self:SetEntityData(UnitField.UNIT_FIELD_LEVEL, newLevel)
    
    -- 重新计算战斗属性
    self:CalcuProperties(true)
    
    if self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] == 0 then
        -- 发送复活消息
    end

    -- 血量补满
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, self.entity_datas[UnitField.UNIT_FIELD_MAX_HP])
    LuaLog.outDebug(string.format("场景服玩家:%s(guid=%.0f)升级,%u->%u,血量=%u", self.role_name, self:GetGUID(), oldLevel, 
                    newLevel, self.entity_datas[UnitField.UNIT_FIELD_CUR_HP]))
end

--[[ 普通复活
]]
function LuaPlayer:OnNormalRelive()
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, self.entity_datas[UnitField.UNIT_FIELD_MAX_HP])
    self.cpp_object:setDeathState(enumDeathState.ALIVE)
    return self.entity_datas[UnitField.UNIT_FIELD_CUR_HP]
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
    
    if self.cpp_object ~= nil then
        self.cpp_object:sendPacket(pck)
        -- LuaLog.outDebug(string.format("SendPacket 成功"))
    else
        LuaLog.outError(string.format("LuaPlayer SendPacket 失败,op=%u,玩家guid=%.0f", opcode, self:GetGUID()))
    end

    WorldPacket.delPacket(pck)
    pck = nil 
end

--[[ 发送系统消息包
]]
function LuaPlayer:SendServerPacket(pck, serverOpCode)

    if serverOpCode ~= nil then
        pck:luaSetServerOpcode(serverOpCode)
    end

    if self.cpp_object ~= nil then
        self.cpp_object:sendServerPacket(pck)
    else
        LuaLog.outError(string.format("LuaPlayer SendServerPacket 失败,serverOp=%u,玩家guid=%.0f", serverOpCode, self:GetGUID())) 
    end

    WorldPacket.delPacket(pck)
    pck = nil
end

--[[ 发送飘字提示
]]
function LuaPlayer:SendPopupNotice(msg)
    local msg1014 = 
    {
        notice = msg
    }
    self:SendPacket(Opcodes.SMSG_001014, "UserProto001014", msg1014)
end


--endregion
