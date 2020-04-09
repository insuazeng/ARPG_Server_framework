--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.common.scene_fight_object", package.seeall)
require("SceneScript.module.skill.skill_def")

local auraProtoConf = require("SceneScript.config.aura_proto_conf")

local oo = require("oo.oo")
local ooSUPER = require("oo.base_lua_object")
local superClass = ooSUPER.BaseLuaObject
SceneFightObject = oo.Class(superClass, "SceneFightObject")

local buffEffectHandler = require("SceneScript.module.aura.buff_effect_handler")
local debuffEffectHandler = require("SceneScript.module.aura.debuff_effect_handler")
local AuraModule = require("SceneScript.module.aura.lua_aura")

function SceneFightObject:__init(guid, typeID, entry)
    superClass.__init(self, guid)
    
    self.type_id = typeID
    self.entry = entry

    -- 对应的C++的object对象
    self.cpp_object = nil
    -- 实体数据
    self.entity_datas = {}
    -- 设定血量和等级
    self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] = 0
    self.entity_datas[UnitField.UNIT_FIELD_LEVEL] = 1
    -- 战斗属性
    self.fight_props = {}
    self.fight_states = 0
    -- 初始化战斗属性
    for i=1,enumFightProp.COUNT do
        self.fight_props[i] = 0
    end
    -- 技能CD列表
    self.skill_cd_list = {}
    -- buff/debuff/被动技能效果列表
    self.buff_list = {}
    self.cur_buff_count = 0
    self.debuff_list = {}
    self.cur_debuff_count = 0
    
end

-- 设置C++对象指针
function SceneFightObject:SetCppObject(obj)
    self.cpp_object = obj
end

function SceneFightObject:GetCppObject()
    return self.cpp_object
end

function SceneFightObject:GetType()
    return self.type_id
end

function SceneFightObject:GetEntry()
    return self.entry
end

function SceneFightObject:isPlayer()
    return false
end

function SceneFightObject:isMonster()
    return false
end

function SceneFightObject:isBossMonster()
    return false
end

--[[ 获取场景实例id ]]
function SceneFightObject:GetInstanceID()
    local ret = -1
    if self.cpp_object ~= nil then
        ret = self.cpp_object:getInstanceID()
    end
    return ret
end

--[[ 获取当前地图id ]]
function SceneFightObject:GetCurMapID()
    local ret = 0
    if self.cpp_object ~= nil then
        ret = self.cpp_object:getMapID()
    end
    return ret
end

function SceneFightObject:Term()
    self.cpp_object = nil
    table.free(self.entity_datas)
    table.free(self.fight_props)
    self.fight_states = 0
    table.free(self.skill_cd_list)

    for k,v in pairs(self.buff_list) do
        local buff = v
        buff:Term()
    end
    table.free(self.buff_list)
    self.cur_buff_count = 0

    for k,v in pairs(self.debuff_list) do
        local buff = v
        buff:Term()
    end
    table.free(self.debuff_list)
    self.cur_debuff_count = 0

end

-- 是否死亡
function SceneFightObject:isDead()
    if self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] <= 0 then
        return true
    end

    return false
end

-- 能否被攻击
function SceneFightObject:CanBeAttack(attacker, skillProto)
    -- 死亡不可以被攻击
    if self:isDead() then
        return enumCastSkillErrorCode.Bad_target, "目标已死亡"
    end

    -- 不在同个场景内无法被攻击
    if self:GetInstanceID() ~= attacker:GetInstanceID() then
        return enumCastSkillErrorCode.Too_far_with_target, "目标已在其他场景"
    end

    -- 技能距离打不到的话,无法被攻击
    if skillProto.cast_range > 0 then
        local dis = self:CalcuDistance(attacker:GetGUID())
        if dis > skillProto.cast_range then
            LuaLog.outDebug(string.format("对象%u 与攻击者距离(%.2f)过远,技能范围=%.2f", self:GetGUID(), dis, skillProto.cast_range))
            return enumCastSkillErrorCode.Too_far_with_target, "距离目标太远"
        end
    end

    return 0, ""
end

-- 获取等级
function SceneFightObject:GetLevel()
    return self.entity_datas[UnitField.UNIT_FIELD_LEVEL]
end

--[[技能cd相关
]]
-- 添加技能cd
function SceneFightObject:AddSkillCD(skillid, cdTime)
    local idx = math.floor(skillid / 100)
    self.skill_cd_list[idx] = SystemUtil.getMsTime64() + cdTime
end

-- 移除技能cd
function SceneFightObject:RemoveSkillCD(skillid)
    local idx = math.floor(skillid / 100)
    self.skill_cd_list[idx] = nil
end

-- 获取CD到期时间
function SceneFightObject:GetSkillCDExpiredTime(skillid)
    local idx = math.floor(skillid / 100)
    return self.skill_cd_list[idx]
end

-- 技能cd的序列化(需要保存至db中的)

--[[ buff/debuff相关
]]

-- 添加buff
function SceneFightObject:AddAura(casterGuid, auraID)
    -- body
    local proto = auraProtoConf.GetAuraProto(auraID)
    if proto == nil then 
        return 
    end

    -- 添加到buff列表中,并让其生效
end

-- 移除debuff
function SceneFightObject:RemoveAura(auraID)
    local proto = auraProtoConf.GetAuraProto(auraID)
    if proto == nil then 
        return 
    end

end

-- 移除aura（特定条件）
function SceneFightObject:RemoveAuraByCondtion(cond)
    
end

--从C++对象处获取的一些状态数据
--[[是否处于移动状态
]]
function SceneFightObject:IsMoving()
    local ret = false
    if self.cpp_object ~= nil and self.cpp_object:isMoving() then
        ret = true
    end
    return ret
end

--[[开始移动
]]
function SceneFightObject:StartMoving(destPosX, destPosY, mode)
    return self.cpp_object:startMoving(destPosX, destPosY, mode)
end

--[[停止移动
]]
function SceneFightObject:StopMoving()
    self.cpp_object:stopMoving()
end

--[[移动被停止
]]
function SceneFightObject:OnMovingStopped(stopPosX, stopPosY)
    
end

--[[设置朝向冲着某个对象
]]
function SceneFightObject:SetDirectionToObject(targetObject)
    return self.cpp_object:setDirectionToDestObject(targetObject)
end

--[[设置自身朝向冲着某个坐标
]]
function SceneFightObject:SetDirectionToCoord(dstX, dstY)
    return self.cpp_object:setDirectionToDestCoord(dstX, dstY)
end

--[[获取当前坐标
]]
function SceneFightObject:GetCurPixelPos()
    return self.cpp_object:getCurPixelPos()
end

--[[设置当前目标
]]
function SceneFightObject:SetCurPosition(newPosX, newPosY)
    if self.cpp_object ~= nil then
        self.cpp_object:setCurPos(newPosX, newPosY)
    end
end

--[[获取当前朝向角度
]]
function SceneFightObject:GetCurDirection()
    return self.cpp_object:getCurDirection()
end

--[[添加战斗状态
]]
function SceneFightObject:AddFightState(state)
    self.fight_states = SystemUtil.bitMakeAND(self.fight_states, state)
end

--[[移除战斗状态
]]
function SceneFightObject:RemoveFightState(state)
    local notState = SystemUtil.bitMakeNOT(state)
    self.fight_states = SystemUtil.bitMakeAND(self.fight_states, notState)
end

--[[是否处于某战斗状态中
]]
function SceneFightObject:InFightState(state)
    local ret = false
    if SystemUtil.bitHaveAND(self.fight_states, state) then
        ret = true
    end
    return ret
end

--[[发包到周围
]]
function SceneFightObject:SendPacketToSet(opcode, protoName, msg, toSelf, errCode)
    
    -- LuaLog.outDebug(string.format("SendPacketToSet called, op=%u, proto=%s, type(msg)=%s", opcode, protoName, type(msg)))
    local pck = WorldPacket.newPacket(opcode)
    if errCode ~= nil and errCode ~= 0 then
        pck:luaSetErrorCode(errCode)
    end

    local ret = gLibPBC.encode(protoName, msg, function(data, len)
        return pck:appendProtobufData(data, len)
    end)
    if self.cpp_object ~= nil then
        self.cpp_object:sendMessageToSet(pck, toSelf)
        WorldPacket.delPacket(pck)
        -- LuaLog.outDebug(string.format("SendPacketToSet 成功"))
    else
        LuaLog.outError(string.format("Lua SendPacketToSet失败,op=%u,guid=%q", opcode, tostring(self:GetGUID())))
        WorldPacket.delPacket(pck)
    end
    pck = nil 
end

--[[计算与对象的距离
]]
function SceneFightObject:CalcuDistance(objGuid)
    return self.cpp_object:calcuDistance(objGuid)
end

--endregion
