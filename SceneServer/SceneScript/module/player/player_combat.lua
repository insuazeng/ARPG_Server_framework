--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.player.lua_player", package.seeall)

local ooSUPER = require("SceneScript.module.common.scene_fight_object")
local superClass = ooSUPER.SceneFightObject

--[[ 能否释放技能
]]
function LuaPlayer:CanCastSkill(skillProto)

    -- 检查玩家有无释放该钟技能的条件(cd/时间)等
    if skillProto.type == enumSkillType.Normal then
        -- 检测玩家能否释放下一个普攻技能
        --[[local ret = self:CanCastNextNormalSkill(skillProto.skill_id)
        if ret ~= 0 then
            return ret
        end]]
    else
        -- 判断CD是否已经到达
        if skillProto.cd_time > 0 then
            local cdExpireTime = self:GetSkillCDExpiredTime(skillProto.skill_id)
            if cdExpireTime ~= nil and SystemUtil.getMsTime64() < cdExpireTime then
                return enumCastSkillErrorCode.Skill_cding, "技能cd中"
            end
        end
    end

    return 0, ""
end

--[[能否被攻击
]]
function LuaPlayer:CanBeAttack(attacker, skillProto)
    local ret = 0
    local desc = ""
    ret, desc = superClass.CanBeAttack(self, attacker, skillProto)
    if ret ~= 0 then
        return ret, desc
    end
    return 0, ""
end

--[[技能释放完成
]]
function LuaPlayer:OnSkillCasted(skillProto)
    -- 进行添加能cd等操作
    if skillProto.type == enumSkillType.Normal then
        self.normal_skill_info[1] = skillProto.skill_id
        self.normal_skill_info[2] = SystemUtil.getMsTime64()
        self.normal_skill_info[3] = skillProto
    else
        -- 添加cd时间
        if skillProto.cd_time ~= nil and skillProto.cd_time > 0 then
            self:AddSkillCD(skillProto.skill_id, skillProto.cd_time)
        end
    end
end

--[[ 能否释放下一个普攻技能
]]
function LuaPlayer:CanCastNextNormalSkill(nextSkillID)
    local curNormalSkill = self.normal_skill_info[1]
    -- 根据职业来获取对应的普攻技能
    if curNormalSkill == 0 then
        curNormalSkill = NORMAL_SKILL_START_ID
    else
        local proto = self.normal_skill_info[3]
        local curMsTime = SystemUtil.getMsTime64()
        local prevNormalSkillEndTime = self.normal_skill_info[2] + proto.action_play_time - 200
        -- 如果上一个普攻技能尚未结束释放,则不允许释放新技能
        if curMsTime < prevNormalSkillEndTime then
            return enumCastSkillErrorCode.Bad_normal_skill_interval
        end

        -- 以上一个技能释放结束时间为基础,前200毫秒至后500毫秒这段期间(但服务端放宽至后700毫秒)
        local diff = curMsTime - prevNormalSkillEndTime
        local resetNormalSkill = true
        
        if diff < 900 then
            -- 900毫秒之内使用连招
            curNormalSkill = curNormalSkill + 1
            if curNormalSkill <= NORMAL_SKILL_MAX_ID then
                resetNormalSkill = false
            end
        end

        if resetNormalSkill then
            curNormalSkill = NORMAL_SKILL_START_ID
        end
    end

    if nextSkillID ~= curNormalSkill and nextSkillID ~= NORMAL_SKILL_START_ID then
        return enumCastSkillErrorCode.Bad_normal_skill_id
    end

    return 0
end

--[[受击后受伤
参数1:
参数2:
]]
function LuaPlayer:OnJustHurted(attacker, damage)
    -- 设置Player对象当前血量
    local curHp = self.entity_datas[UnitField.UNIT_FIELD_CUR_HP] - damage
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, curHp)
end

--[[受击后刚死亡
参数1:
]]
function LuaPlayer:OnJustDied(killer)
    self:SetEntityData(UnitField.UNIT_FIELD_CUR_HP, 0)
    if killer ~= nil then
        killer:KilledObject(self)
    end
end

--[[击杀目标
]]
function LuaPlayer:KilledObject(obj)
    local objType = obj:GetType()
    if objType == enumSceneFightObjectType.MONSTER then
        -- 击杀怪物,获得经验
        local proto = obj:GetProto()
        if proto == nil then
            return 
        end
        -- 玩家击杀怪物
        -- LuaLog.outDebug(string.format("怪物%u(%s)被玩家%q击杀", obj:GetGUID(), proto.monster_name, tostring(self:GetGUID())))
        if proto.exp ~= nil and proto.exp > 0 then
            self:AddPlayerExp(proto.exp)
        end
    else
        -- LuaLog.outError(string.format("对象%u被玩家%q击杀,不是怪物", obj:GetGUID(), tostring(self:GetGUID())))
    end
end

--endregion
