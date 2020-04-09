--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("SceneScript.module.monster.lua_monster", package.seeall)
local _G = _G
local oo = require("oo.oo")
local ooSUPER = require("SceneScript.module.common.scene_fight_object")
local superClass = ooSUPER.SceneFightObject
LuaMonster = oo.Class(superClass, "LuaMonster")

local MonsterAIModule = require("SceneScript.module.monster.ai.base_ai")
local sSkillSystem = require("SceneScript.module.skill.skill_system")
local monsterProtoConf = require("SceneScript.config.monster_proto_conf")

-- 玩家类初始化函数
function LuaMonster:__init(guid, protoID, attackMode, moveType)
    superClass.__init(self, guid, enumSceneFightObjectType.MONSTER, 0)
    self.proto_id = protoID
    -- self.ai_interface = MonsterAIModule.MonsterBaseAI(self, enumMonsterAttackMode.MONSTER_ACK_PASSIVE, enumMonsterMoveType.MONSTER_MOVE_NONE)
    self.ai_interface = MonsterAIModule.MonsterBaseAI(self, enumMonsterAttackMode.MONSTER_ACK_PEACE, moveType)

    LuaLog.outDebug(string.format("LuaMonster(guid=%q,proto=%u)被创建,攻击模式=%u,移动类型=%u", tostring(guid), protoID, attackMode, moveType))
end

function LuaMonster:InitData(mapid, instanceid)
    self.instance_id = instanceid
    
    local cppMonster = Monster.getMonsterObject(mapid, instanceid, self.object_guid)
    if cppMonster == nil then
        LuaLog.outError(string.format("怪物 protoID=%u,guid=%.0f InitData失败,找不到对应的cpp对象", self.proto_id, self:GetGUID()))
        return false
    end

    local proto = monsterProtoConf.GetMonsterProto(self.proto_id)
    if proto == nil then
        LuaLog.outError(string.format("怪物 protoID=%u,guid=%.0f InitData失败,找不到对应的proto定义", self.proto_id, self:GetGUID()))
        return false
    end

    superClass.SetCppObject(self, cppMonster)

    -- 设置血量等数据
    self:SetEntityData(UnitField.UNIT_FIELD_MAX_HP, proto.hp)
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, proto.hp)

    -- 初始化战斗数据
    self.fight_props = { }          -- 战斗属性
    local fProps = self.fight_props
    for i=1, enumFightProp.COUNT do
        if proto.fight_props[i] ~= nil then
            fProps[i] = proto.fight_props[i]
        else
            fProps[i] = 0
        end
    end

    -- 初始化ai数据
    LuaLog.outDebug(string.format("怪物guid=%.0f lua数据初始化完成,hp=%u", self.object_guid, self.entity_datas[UnitField.UNIT_FIELD_CUR_HP]))

    return true
end

function LuaMonster:OnPushToWorld(mapid, instanceid)
    -- 如果cpp_object还未被初始化,则先初始化
    if self.cpp_object == nil then
        local ret = self:InitData(mapid, instanceid)
        if not ret then
            LuaLog.outError(string.format("无法初始化cppMonster对象,guid=%q,mapid=%u,instanceid=%u", tostring(self.object_guid), mapid, instanceid))
        end
    end

    -- 重新设置一次战斗属性(方便数据更新)
    local proto = monsterProtoConf.GetMonsterProto(self.proto_id)
    if proto ~= nil then
        -- 设置血量等数据
        self:SetEntityData(UnitField.UNIT_FIELD_MAX_HP, proto.hp)
        self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, proto.hp)

        self.fight_props = { }          -- 战斗属性
        local fProps = self.fight_props
        for i=1, enumFightProp.COUNT do
            if proto.fight_props[i] ~= nil then
                fProps[i] = proto.fight_props[i]
            else
                fProps[i] = 0
            end
        end   
    end

    -- ai推放场景
    self.ai_interface:OnPushToWorld(mapid, instanceid)
    
    self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] = self.cpp_object:getUInt64Value(UnitField.UNIT_FIELD_CUR_HP)
end

function LuaMonster:Term()
    superClass.Term(self)
    self.ai_interface:Term()
    self.ai_interface = nil
end

--[[ 设置实体数据
]]
function LuaMonster:SetEntityData(index, val)
    -- body
    if index == nil or index < 0 or index >= UnitField.UNIT_FIELD_END then
        return 
    end

    self.entity_datas[index] = val
    -- 设置到C++处
    self.cpp_object:setUInt64Value(index, val)
end

--[[ 是否怪物
]]
function LuaMonster:isMonster()
    return true
end

--[[ 是否Boss怪
]]
function LuaMonster:isBossMonster()
    return false
end

--[[ 设置当前的ai状态
]]
function LuaMonster:SetAIState(state)
    if self.cpp_object ~= nil then
        self.cpp_object:setAIState(state)
    end
end

--[[ 获取原型数据
]]
function LuaMonster:GetProto()
    return monsterProtoConf.GetMonsterProto(self.proto_id)
end

--[[获取攻击频率
]]
function LuaMonster:GetAttackSpeed()
    -- 默认1000毫秒攻击一次
    local ret = 1000
    local proto = monsterProtoConf.GetMonsterProto(self.proto_id)
    if proto ~= nil and proto.attack_interval ~= nil then
        ret = proto.attack_interval
    end
    return ret
end

--[[获取脱战距离
]]
function LuaMonster:GetLeaveCombatRange()
    -- 默认9米距离脱战
    local ret = 9.0
    local proto = monsterProtoConf.GetMonsterProto(self.proto_id)
    if proto ~= nil and proto.leave_combat_range ~= nil then
        ret = proto.leave_combat_range
    end
    return ret
end


--[[受击后受伤
参数1:
参数2:
]]
function LuaMonster:OnJustHurted(attacker, damage)
    -- 设置血量
    local newHp = self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] - damage
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, newHp)
    -- 管理仇恨列表,以及进行反击
    self.ai_interface:OnBeAttacked(attacker, damage)
end

--[[受击后刚死亡
参数1:
]]
function LuaMonster:OnJustDied(killer)
    -- 怪物死亡了
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, 0)
    self.ai_interface:OnBeKilled(killer)
    self.cpp_object:onMonsterJustDied()
    if killer ~= nil then
        killer:KilledObject(self)
    end
end

--[[击杀目标
]]
function LuaMonster:KilledObject(obj)
    
end

--[[脱战成功
]]
function LuaMonster:OnLeaveCombatFinished()
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, self.entity_datas[UnitField.UNIT_FIELD_MAX_HP])
end

--[[进入激活状态
]]
function LuaMonster:EnterActiveMode()
    -- body
    self.ai_interface:EnterActiveMode()
end

--[[选择可释放的技能
]]
function LuaMonster:FindCastableSkillID()
    -- 如果proto里有配置技能id,那么在这些id中寻找可释放的,否则固定选择一个普攻技能

end

--[[进入非激活状态
]]
function LuaMonster:EnterDeactiveMode()
    -- body
    self.ai_interface:EnterDeactiveMode()
end

--[[释放技能
]]
function LuaMonster:CastSkill(mainTargetGuid, skillID, otherTargetList)

    local proto = sSkillSystem.GetSkillProto(skillID)
    if proto == nil then
        LuaLog.outError(string.format("怪物 %u(proto=%u)试图释放不存在的怪物技能:%u", self:GetGUID(), self:GetEntry(), skillID))
        return 
    end

    local casterInfo =
    {
        caster_guid = self.object_guid,
        caster_posx, caster_posy = self:GetCurPixelPos(),
        main_target_guid = mainTargetGuid,
        cast_direction = self:SetDirectionToObject(mainTargetGuid),
        target_guid_list = {}
    }
    
    if otherTargetList ~= nil then
        casterInfo.target_guid_list = otherTargetList
    end

    table.insert(casterInfo.target_guid_list, mainTargetGuid)
    
    -- 遍历列表内所有目标,检测对方是否可以被攻击
    local validTargetGuids = {}
    for i=1, #casterInfo.target_guid_list do
        local objGuid = casterInfo.target_guid_list[i]
        local obj = GetObjectData(objGuid)
            -- LuaLog.outDebug(string.format("目标列表objGuid=%u,type=%s", objGuid, type(obj)))            
        if obj ~= nil then
            errCode = obj:CanBeAttack(self, proto)
            -- LuaLog.outDebug(string.format("目标CanBeAttack=%u", errCode))
            if errCode == 0 then
                table.insert(validTargetGuids, objGuid)
            else
                if objGuid == mainTargetGuid then
                    -- 主目标无法攻击的话,则不进行技能释放
                    return 
                end
            end
        end
    end
    
    casterInfo.target_guid_list = validTargetGuids

    -- 发送技能释放的广播
    local t = 
    {
        casterGuid = self.object_guid,
        castSkillID = skillID
    }
    self:SendPacketToSet(Opcodes.SMSG_003004, "UserProto003004", t, false)

    -- 一个技能会在多个时间段造成伤害
    for i=1,proto.dmg_effect_count do
        local time = proto.dmg_effect_info[i][1]
        if time == nil then
            time = 1000
        end
        
        -- 添加定时器
        if time > 0 then
            gTimer.regCallback(time, 1, function(callbackID, curRepeatCount)
                sSkillSystem.CastSkill(casterInfo, proto, i)
            end)
        else
            sSkillSystem.CastSkill(casterInfo, proto, i)
        end
    end

    -- 添加CD
    if proto.cd_time ~= nil and proto.cd_time > 0 then
        self:AddSkillCD(proto.skill_id, proto.cd_time)
    end
end

--[[能否被攻击
]]
function LuaMonster:CanBeAttack(attacker, skillProto)
    local ret = 0
    local desc = ""
    ret, desc = superClass.CanBeAttack(self, attacker, skillProto)
    if ret ~= 0 then
        return ret, desc
    end

    if self.ai_interface ~= nil then
        return self.ai_interface:CanBeAttack(attacker)
    end

    return 0, ""
end

--[[移动被停止
]]
function LuaMonster:OnMovingStopped(stopPosX, stopPosY)

    if self.ai_interface ~= nil then
        self.ai_interface:OnMovingStopped(stopPosX, stopPosY)
    end
    
end

--[[获取出生点坐标
]]
function LuaMonster:GetBornPos()
    return self.cpp_object:getBornPos()
end


--endregion
