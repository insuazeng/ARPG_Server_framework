--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.skill.skill_system", package.seeall)
local skillProtoConf = require("gameconfig.skill_proto_conf")
local sSkillTargetChecker = require("SceneScript.module.skill.skill_check_target_handler")
local sSkillTargetFinder = require("SceneScript.module.skill.skill_find_target_handler")
local sSkillEffectMaker = require("SceneScript.module.skill.skill_effect_handler")
local levelUpPropConf = require("gameconfig.plr_level_up_prop_conf")

local _G = _G

local __calcuSkillHitResult = nil       -- 计算技能命中结果
local __doSkillEffect = nil             -- 让技能生效

-- 技能系统（处理技能释放、目标筛选、命中判定、伤害计算等）
__calcuSkillHitResult = function(casterObject, targetGuid, skillProto)
    -- 获取技能双方数据
    local targetObject = GetObjectData(targetGuid)

    if targetObject == nil then
        -- LuaLog.outError(string.format("无法找到施法者或目标对象,casterObj=%s,targetObj=%s", tostring)
        return false
    end

    if casterObject:isDead() or targetObject:isDead() then
        return false
    end

    local casterLevel = casterObject:GetLevel()
    local targetLevel = targetObject:GetLevel()
    
    local casterFightProp = casterObject.fight_props
    local targetFightProp = targetObject.fight_props
    
    -- 默认命中为普通命中
    local hitResult = enumSkillHitResult.NORMAL
    -- 普攻技能会检测是否产生闪避,技能攻击则必定命中
    if skillProto.type == enumSkillType.Normal then
        local casterHitKLV = levelUpPropConf.GetHitKLV(casterLevel)
        local targetDodgeKLV = levelUpPropConf.GetDodgeKLV(targetLevel)
        local casterHit = casterFightProp[enumFightProp.BASE_HIT] * (1.0 + casterFightProp[enumFightProp.AD_HIT_PERCENTAGE] / 10000)
        local targetDodge = targetFightProp[enumFightProp.BASE_DODGE] * (1.0 + targetFightProp[enumFightProp.AD_DODGE_PERCENTAGE] / 10000)
        -- 暴击率=守方闪避*守方闪避/(守方闪避KLV+守方闪避)/(0.01+攻方命中+攻方命中KLV)，但最大不能超过0.4
        local dodgeRate = targetDodge * targetDodge / (targetDodgeKLV + targetDodge) / (0.01 + casterHit + casterHitKLV)
        if dodgeRate > 0.4 then
            dodgeRate = 0.4
        end
        if math.random() < dodgeRate then
            hitResult = enumSkillHitResult.DODGE
        end
        -- 把己方命中/对方躲避/最终躲避率打印出来
        --[[if casterObject:isPlayer() then
            local notice = string.format("释放技能:%u,己方命中=%.2f,对方躲避=%.2f,躲避率=%f", skillProto.skill_id, casterHit, targetDodge, dodgeRate)
            casterObject:SendPopupNotice(notice)
        end]]
    end

    -- 判断有无机会产生暴击
    if hitResult == enumSkillHitResult.NORMAL then
        local casterCriKLV = levelUpPropConf.GetCriKLV(casterLevel)
        local targetCriReduceKLV = levelUpPropConf.GetCriReduceKLV(targetLevel)
        local casterCri = casterFightProp[enumFightProp.BASE_CRI] * (1.0 + casterFightProp[enumFightProp.AD_CRI_PERCENTAGE] / 10000)
        local targetTough = targetFightProp[enumFightProp.BASE_TOUGH] * (1.0 + targetFightProp[enumFightProp.AD_TOUGH_PRECENTAGE] / 10000)
        -- 根据KLV计算暴击率
        local criRate = casterCri * casterCri / (casterCriKLV + casterCri) / (0.01 + targetTough + targetCriReduceKLV)
        if criRate > 0.6 then
            criRate = 0.6
        end
        if math.random() < criRate then
            hitResult = enumSkillHitResult.CRITICAL
        end
        -- 把己方命中/对方躲避/最终躲避率打印出来
        if casterObject:isPlayer() then
            local notice = string.format("释放技能:%u,己方暴击=%.2f,对方抗暴=%.2f,暴击率=%f", skillProto.skill_id, casterCri, targetTough, criRate)
            casterObject:SendPopupNotice(notice)
        end
    end

    -- 根据伤害进行结算
    return true, hitResult, targetObject
end

--[[ 进行技能效果伤害结算
]]
__doSkillEffect = function(casterObject, targetObject, skillProto, hitResult)
    
    local hurtValue = 0

    if skillProto.effect_type ~= nil then
        local callRet, val = sSkillEffectMaker.HandleSkillEffect(skillProto.effect_type, skillProto, hitResult, casterObject, targetObject)
        if callRet then
            hurtValue = hurtValue + val
        end
    end

    -- 返回目标扣血,是否死亡等标志
    if hurtValue > 0 then
        if targetObject.entity_datas[UnitField.UNIT_FIELD_CUR_HP] > 0 then
            if targetObject.entity_datas[UnitField.UNIT_FIELD_CUR_HP] > hurtValue then
                targetObject:OnJustHurted(casterObject, hurtValue)
            else
                targetObject:OnJustDied(casterObject)
            end
        end
    else
        -- 治疗技能(进行加血)

    end

    return hurtValue, targetObject.entity_datas[UnitField.UNIT_FIELD_CUR_HP]
end

--[[ 技能释放
参数1:释放者guid
参数2:技能定义
参数3:目标guid列表
]]
function CastSkill(skillCastInfo, skillProto, curEffectIndex)

    local targetGuidList = skillCastInfo.target_guid_list
    if (skillCastInfo.main_target_guid == nil or skillCastInfo.main_target_guid == 0) and (targetGuidList == nil or #targetGuidList == 0) then
        -- 如果技能不允许被空放,则退出
        if not skillProto.no_target_cast then
            return 
        end
    end

    local casterObject = GetObjectData(skillCastInfo.caster_guid)
    if casterObject == nil or casterObject:isDead() then
        return
    end
    skillCastInfo.caster_object = casterObject

    -- 有些技能是每次结算时需要重新选目标
    if skillProto.rechoose_target_before_dmg then
        table.free(skillCastInfo.target_guid_list)
        local guidList = FillSkillTargetList(skillProto.fill_target_type, skillCastInfo, skillProto)
        skillCastInfo.target_guid_list = CheckTargetListValidation(casterObject, skillProto, guidList, skillCastInfo.main_target_guid)
    end

    LuaLog.outDebug(string.format("提交C++目标检测前,数量=%u个", #targetGuidList))
    -- 将目标列表检测一遍
    local ret, finalTargetList = sSkillTargetChecker.HandleSkillCheckTarget(skillProto.fill_target_type, skillCastInfo, skillProto, targetGuidList)
    if not ret or finalTargetList == nil then
        LuaLog.outError(string.format("技能(id=%u)释放目标检测失败,最终目标列表=%s", skillProto.skill_id, type(finalTargetList)))
        return 
    end
    
    targetGuidList = finalTargetList
    -- LuaLog.outDebug(string.format("提交C++目标检测完毕,最终数量=%u个", #targetGuidList))

    local msg3006 = {}
    msg3006.casterGuid = skillCastInfo.caster_guid
    msg3006.castResults = {}

    -- 同个时间段内可能会对目标造成多次伤害
    local dmgCount = 1
    if skillProto.dmg_effect_info[curEffectIndex] ~= nil and skillProto.dmg_effect_info[curEffectIndex][2] ~= nil then
        dmgCount = skillProto.dmg_effect_info[curEffectIndex][2]
    end

    LuaLog.outDebug(string.format("准备进行技能:%s(%u)的第%u次调用,会造成%u次伤害", skillProto.name, skillProto.skill_id, curEffectIndex, dmgCount))

    for k=1,dmgCount do
        for i=1, #targetGuidList do
            local tGuid = targetGuidList[i]
            local callRet, hitRet, targetObject = __calcuSkillHitResult(casterObject, tGuid, skillProto)
            if callRet then
                if hitRet == enumSkillHitResult.DODGE then
                    local t = 
                    {
                        targetGuid = tGuid,
                        castResult = SystemUtil.make32BitValue(hitRet, 0)
                    }
                    table.insert(msg3006.castResults, t)
                else
                    local hurtValue, targetCurHp = __doSkillEffect(casterObject, targetObject, skillProto, hitRet)
                    local hurtFlag = enumSkillHitEffect.NORMAL_HURT
                    LuaLog.outDebug(string.format("第%u次伤害,hurtValue=%u", k, hurtValue))
                    if targetCurHp <= 0 then
                        hurtFlag = enumSkillHitEffect.TARGET_DEAD
                    end
                    local t =
                    {
                        targetGuid = tGuid,
                        damageValue = hurtValue,
                        castResult = SystemUtil.make32BitValue(hitRet, hurtFlag)
                    }
                    table.insert(msg3006.castResults, t)
                end
            end
        end
    end

    -- 临时通过服务端把技能伤害数值发送至客户端
    if casterObject:isPlayer() then
        for i=1,#msg3006.castResults do
            local t = msg3006.castResults[i]
            if msg3006.castResults[i].damageValue ~= nil then
                local notice = string.format("技能:%u对目标guid=%.0f造成第%u次伤害: %u点", skillProto.skill_id, msg3006.castResults[i].targetGuid, i, msg3006.castResults[i].damageValue)
                casterObject:SendPopupNotice(notice)
            end
        end
    end

    casterObject:SendPacketToSet(Opcodes.SMSG_003006, "UserProto003006", msg3006, true)
end

--[[ 填充技能目标列表
]]
function FillSkillTargetList(index, skillCastInfo, skillProto)

    local ret = sSkillTargetFinder.HandleSkillFindTarget(index, skillCastInfo, skillProto)
    if ret == nil then
        ret = {}
    end

    return ret
end

--[[ 检测技能释放目标的合法性
]]
function CheckTargetListValidation(casterObject, skillProto, targetGuidList, mainTargetGuid)
    local targetObj = nil
    local targetGuidSet = {}
    local validGuidList = {}
    if mainTargetGuid == nil then
        mainTargetGuid = 0
    end
    local errCode = 0

    for i=1,#targetGuidList do
        local objGuid = targetGuidList[i]
        if objGuid ~= nil then
            if objGuid == mainTargetGuid then
                -- 主目标不用检测合法性,直接展开攻击
                table.insert(validGuidList, objGuid)
                targetGuidSet[objGuid] = 1
            else
                if targetGuidSet[objGuid] == nil then
                    targetObj = GetObjectData(objGuid)
                    -- LuaLog.outDebug(string.format("目标列表objGuid=%u,type=%s", objGuid, type(obj)))            
                    if targetObj ~= nil then
                        errCode = targetObj:CanBeAttack(casterObject, skillProto)
                        -- LuaLog.outDebug(string.format("目标CanBeAttack=%u", errCode))
                        if errCode == 0 then
                            table.insert(validGuidList, objGuid)
                            targetGuidSet[objGuid] = 1
                        end
                    end        
                end
            end
        end
    end
    return validGuidList
end

--[[获取技能原型定义
]]
function GetSkillProto(skillID)
    return skillProtoConf.GetSkillProto(skillID)
end

--endregion
