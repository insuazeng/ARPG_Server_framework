--region *.lua
--Date
--此文件由[BabeLua]插件自动生成
module("SceneScript.module.skill.skill_packet_handler", package.seeall)

local _G = _G
local sSkillSystem = require("SceneScript.module.skill.skill_system")

local function __SendCastSkillResult(plrCaster, errCode)
    -- 通知自己技能释放结果
    local msg3002 = {}
    plrCaster:SendPacket(Opcodes.SMSG_003002, "UserProto003002", msg3002, errCode)
end

-- 请求释放技能
local function HandleCastSkill(guid, pb_msg)
    LuaLog.outDebug(string.format("HandleCastSkill called, skillId=%u,targetGuid=%u", pb_msg.skillID, pb_msg.mainTargetGuid))
    local skillID = pb_msg.skillID
    local mainTargetGuid = pb_msg.mainTargetGuid
    if skillID == nil then return end
    
    local proto = sSkillSystem.GetSkillProto(skillID)
    if proto == nil then return end

    local plr = sPlayerMgr.GetPlayerByGuid(guid)
    if plr == nil then return end

    -- 检测玩家是否有条件释放该技能
    local desc = ""
    local errCode = 0
    errCode, desc = plr:CanCastSkill(proto)
    if errCode ~= 0 then
        LuaLog.outError(string.format("玩家guid=%.0f 释放技能skillID=%u,能否释放检测结果=%u", guid, skillID, errCode))
        __SendCastSkillResult(plr, errCode)
        local notice = string.format("技能无法使用:%s", desc)
        plr:SendPopupNotice(notice)
        return 
    end

    if mainTargetGuid == nil then
        mainTargetGuid = 0 
    end
    
    local mainTargetObject = GetObjectData(mainTargetGuid)
    if mainTargetObject == nil then
        -- 判断该技能能否空放,如果不允许空放,则直接返回
        if not proto.no_target_cast then
            return 
        end
    else
        -- 判断主目标能否被攻击,如果是的话才继续
        errCode, desc = mainTargetObject:CanBeAttack(plr, proto)
        if errCode ~= 0 then
            LuaLog.outError(string.format("玩家guid=%.0f 释放技能skillID=%u,主目标可攻击检测结果=%s", guid, skillID, desc))
            __SendCastSkillResult(plr, errCode)
            local notice = string.format("技能无法使用:%s", desc)
            plr:SendPopupNotice(notice)
            -- 主目标不可攻击,但认为玩家已经发招完成,一样添加技能cd
            -- plr:OnSkillCasted(proto)
            return 
        end
    end

    local skillCastInfo = 
    {
        caster_guid = guid,
        caster_object = plr,
        cast_direction = plr:SetDirectionToObject(mainTargetGuid),
        main_target_guid = mainTargetGuid
    }
    skillCastInfo.caster_posx, skillCastInfo.caster_posy = plr:GetCurPixelPos()
    if mainTargetObject ~= nil then
        skillCastInfo.target_posx, skillCastInfo.target_posy = mainTargetObject:GetCurPixelPos()
    else
        -- 此处应该等于协议传输过来的目标坐标x,y
        if pb_msg.targetPos ~= nil then
            skillCastInfo.target_posx = pb_msg.targetPos.posX
            skillCastInfo.target_posy = pb_msg.targetPos.posY
        else
            skillCastInfo.target_posx = skillCastInfo.caster_posx
            skillCastInfo.target_posy = skillCastInfo.caster_posy
        end
    end

    -- 获取技能攻击的所有目标
    local targetGuidSet = { }        -- 去掉重复的目标guid
    local clientCommitGuids = {}
    if pb_msg.otherTargetGuids ~= nil then
        clientCommitGuids = pb_msg.otherTargetGuids 
    end

    -- 临时做法,需要从C++处填充群攻的目标列表(如果最大目标个数大于1)
	if #clientCommitGuids == 0 and proto.max_target > 1 then        
		clientCommitGuids = sSkillSystem.FillSkillTargetList(proto.fill_target_type, skillCastInfo, proto)
		LuaLog.outDebug(string.format("技能=%u 从C++获取周边(%.1f米内)目标共%u个", proto.skill_id, proto.cast_range, #clientCommitGuids))
	end

    -- 遍历客户端提交过来的其他目标guid,将不合适的排除掉,主目标不理会
    skillCastInfo.target_guid_list = sSkillSystem.CheckTargetListValidation(plr, proto, clientCommitGuids, mainTargetGuid)
    LuaLog.outDebug(string.format("技能=%u 过滤后目标数量共%u个", proto.skill_id, #skillCastInfo.target_guid_list))
    -- 通知自己释放成功
    __SendCastSkillResult(plr, 0)

    -- 通知周围玩家技能释放成功
    local msg3004 = 
    {
        casterGuid = guid,
        castSkillID = skillID
    }

    if proto.cast_start_event ~= nil then
        if proto.cast_start_event == enumSkillCastStartEvent.CASTER_ASSAULT then
            -- 释放者进行冲刺,填充冲刺消息
            local targetX, targetY = SkillHelp.findObjectAssaultTargetPos(plr:GetCppObject(), mainTargetGuid, skillCastInfo.target_posx, skillCastInfo.target_posy, proto.cast_range)
            LuaLog.outDebug(string.format("释放者(guid=%.0f,(%.1f,%.1f))可冲刺终点为(%.1f,%.1f)", guid, skillCastInfo.caster_posx, skillCastInfo.caster_posy, targetX, targetY))
            skillCastInfo.target_posx = targetX
            skillCastInfo.target_posy = targetY
            local msg = 
            {
                assaultObjectGuid = guid,
                startPosX = skillCastInfo.caster_posx,      -- 原起始坐标x
                startPosY = skillCastInfo.caster_posy,      -- 原起始坐标y
                endPosX = skillCastInfo.target_posx,
                endPosY = skillCastInfo.target_posy,
                endPosHeight = SceneMapMgr.getPositionHeight(plr:GetCurMapID(), targetX, targetY)
            }
            LuaLog.outDebug(string.format("释放者(guid=%.0f)进行冲刺(%.1f,%.1f)->(%.1f,%.1f)", guid, msg.startPosX, msg.startPosY, msg.endPosX, msg.endPosY))
            msg3004.assaultData = msg
            -- 设置释放者新坐标
            plr:SetCurPosition(targetX, targetY)
            skillCastInfo.caster_posx = targetX
            skillCastInfo.caster_posy = targetY
        end
    end

    plr:SendPacketToSet(Opcodes.SMSG_003004, "UserProto003004", msg3004, false)

    -- LuaLog.outDebug(string.format("玩家guid=%q 释放技能skillID=%u,目标guid=%q", tostring(guid), skillID, tostring(mainTargetGuid)))
    
    -- 注册技能生效定时器(根据技能的配置类决定)
    if proto.dmg_effect_count > 0 then
        for i=1,proto.dmg_effect_count do
            local time = proto.dmg_effect_info[i][1]
            if time == nil then
                time = 1000
            end
            
            if time > 0 then
                gTimer.regCallback(time, 1, function(callbackID, curRepeatCount)
                    sSkillSystem.CastSkill(skillCastInfo, proto, i)
                end)
            else
                sSkillSystem.CastSkill(skillCastInfo, proto, i)
            end
        end    
    end
    
    -- 释放结束,添加cd等信息
    plr:OnSkillCasted(proto)
end

table.insert(gInitializeFunctionTable, function()
    -- print(string.format("type worldpacket= %s,regFunc=%s", type(sWorldPacketDispatcher), type(sWorldPacketDispatcher.registerPacketHandler)))
    sWorldPacketDispatcher.registerPacketHandler(Opcodes.CMSG_003001, "UserProto003001", HandleCastSkill)       -- 请求释放技能
end)

--endregion
