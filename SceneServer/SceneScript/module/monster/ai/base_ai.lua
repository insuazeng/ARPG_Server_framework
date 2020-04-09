--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

module("SceneScript.module.monster.ai.base_ai", package.seeall)
require("SceneScript.module.skill.skill_def")

local oo = require("oo.oo")

local _G = _G
-- 进行怪物ai管理的模块
MonsterBaseAI = oo.Class(nil, "MonsterBaseAI")

-- 技能id（临时定义）
local monsterSkillID = 600101
-- 可攻击距离
local attackableRange = 3.0

local AIState =
{
    IDLE            = 0,    -- 空闲状态
    ATTACKING       = 1,    -- 攻击状态
    FOLLOW          = 2,    -- 追击
    EVADE           = 3,    -- 逃避（回到原位）
    MAX_COUNT       = 4,
}

local __updateThreatList = nil      -- 函数 更新仇恨列表
local __clearThreatInfo = nil       -- 函数 清除仇恨列表
local __setMovingPathInfo = nil     -- 函数 设置移动路径
local __clearMovingPathInfo = nil   -- 函数 清除移动路径
local __doPathMoving = nil          -- 函数 进行路径移动
local __findPathAndMove = nil       -- 函数 寻路并且移动
local __followAttackTarget = nil    -- 函数 跟随攻击目标
local __startAttack = nil           -- 函数 选择好目标,展开攻击
local __leaveCombat = nil           -- 函数 离开战斗
local __leaveCombatFinished = nil   -- 函数 脱战成功
local __onUpdateCombatTimer = nil   -- 函数 战斗更新（目标更替等判断）
local __addFindAttackableTargetTimer = nil  -- 函数 添加自动寻敌的定时器
local __addRandomMoveTimer = nil    -- 函数 添加随机移动的定时器
local __setCurAttackTargetInfo = nil        -- 函数 设置攻击目标信息
local __clearCurAttackTargetInfo = nil      -- 函数 清除攻击目标信息

--移除随机移动的定时器
local function __removeAITimer(timerID)
    if timerID ~= nil then
        gTimer.unRegCallback(timerID)
    end
end

-- 随机移动
local function __onRandMoveTimer(callbackID, curRepeatCount, monster)
    if monster.cpp_object == nil or monster:isDead() then
        return
    end

    if monster.cpp_object:isMoving() then
        return
    end

    local bornPosX, bornPosY = monster.cpp_object:getBornPos()
    if bornPosX == nil or bornPosY == nil then
        LuaLog.outDebug(string.format("出生位置无法获取,无法随机移动"))
        return 
    end

    local targetX = bornPosX + math.random(-1, 1)
    local targetY = bornPosY + math.random(-1, 1)
    if monster.cpp_object:isMovablePos(targetX, targetY) then
        monster:StartMoving(targetX, targetY, 0)
    end
    LuaLog.outDebug(string.format("怪物guid=%u准备开始移动,目标(%.1f,%.1f)", monster.object_guid, targetX, targetY))
end

-- 定时主动寻敌
local function __onFindAttackableTargetTimer(callbackID, curRepeatCount, ai)
    
    if ai.ai_state ~= AIState.IDLE then
        __removeAITimer(ai.find_target_ack_timer_id)
        ai.find_target_ack_timer_id = nil
    end

    -- 寻找身边可攻击的目标
    local monster = ai.owner_obj
    -- LuaLog.outDebug(string.format("怪物%u 开始寻找目标", monster.object_guid))

    local targetGuid, distance, targetX, targetY = monster.cpp_object:findAttackableTarget(512)
    if targetGuid == nil then
        return 
    end

    -- 移除寻敌定时器
    __removeAITimer(ai.find_target_ack_timer_id)
    ai.find_target_ack_timer_id = nil

    -- 设置当前目标
    __setCurAttackTargetInfo(ai, targetGuid, distance, targetX, targetY)
end

-- 定时展开攻击
local function __onDoAttackTimer(callbackID, curRepeatCount, ai)
    if ai.cur_ack_target_info.guid == 0 then
        return 
    end

    -- LuaLog.outDebug(string.format("怪物:%u 释放技能%u,目标%q", ai.owner_obj.object_guid, monsterSkillID, tostring(ai.cur_ack_target_info.guid)))
    -- 展开攻击
    ai.owner_obj:CastSkill(ai.cur_ack_target_info.guid, monsterSkillID)
end

-- 战斗中更新状态
__onUpdateCombatTimer = function(callbackID, curRepeatCount, ai)
    if ai.ai_state == AIState.IDLE then
    
    elseif ai.ai_state == AIState.EVADE then
        -- 检测是否已经回到了原点(如果停止移动说明已经回到原点了)
        -- LuaLog.outDebug(string.format("脱战状态中 movingFlag = %q,当前坐标(%u,%u)", tostring(movingFlag), curPosX, curPosY))
        local bornPosX, bornPosY = ai.owner_obj:GetBornPos()
        local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
        local bornPosDis = gTool.CalcuDistance(bornPosX, bornPosY, curPosX, curPosY)
        if bornPosDis <= 16 then
            __leaveCombatFinished(ai)
        end
    elseif ai.ai_state == AIState.FOLLOW then
        -- 处于攻击/追击状态(目标死亡,仇恨目标切换,需要重新寻找)
        local dis, targetCurPosX, targetCurPosY = ai.owner_obj:CalcuDistance(ai.cur_ack_target_info.guid)
        local oldDis = ai.cur_ack_target_info.cur_distance
        ai.cur_ack_target_info.cur_distance = dis
        -- LuaLog.outDebug(string.format("怪物(guid=%u)处于追击状态,与目标当前距离=%u", ai.owner_obj.object_guid, dis))
        if dis < attackableRange then
            -- 停止移动,展开攻击
            if ai.owner_obj:IsMoving() then
                __clearMovingPathInfo(ai)
                ai.owner_obj:StopMoving()
            end
            __startAttack(ai)
        else
            if dis >= 65535.0 then
                -- 追击目标已与自己不在同场景,则放弃追击直接脱战
                __leaveCombat(ai)
            else
                local bornPosX, bornPosY = ai.owner_obj:GetBornPos()
                local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
                local bornPosDis = gTool.CalcuDistance(bornPosX, bornPosY, curPosX, curPosY)
                -- LuaLog.outDebug(string.format("怪物(guid=%u)追击中,与出生点距离=%u", ai.owner_obj.object_guid, bornPosDis))
                if bornPosDis > ai.owner_obj:GetLeaveCombatRange() then
                    -- LuaLog.outDebug(string.format("怪物(guid=%u)追击过远,准备脱战", ai.owner_obj.object_guid))
                    __leaveCombat(ai)
                else
                    -- 如果与目标之间距离拉长了,并且目标位置改变了,说明需要重新寻路,否则接着向目标走即可
                    local targetPosChanged = false
                    if ai.cur_ack_target_info.curPosX ~= targetCurPosX or ai.cur_ack_target_info.curPosY ~= targetCurPosY then
                        targetPosChanged = true
                    end
                    -- 只有当距离变大同时目标坐标也改变的情况下,才重新规划路线进行追击
                    if dis > oldDis and targetPosChanged then
                        ai.cur_ack_target_info.curPosX = targetCurPosX
                        ai.cur_ack_target_info.curPosY = targetCurPosY
                        local curMapID = ai.owner_obj:GetCurMapID()
                        if SceneMapMgr.isMovableLineFromAToB(curMapID, curPosX, curPosY, targetCurPosX, targetCurPosY) then
                            ai.owner_obj:StartMoving(targetCurPosX, targetCurPosY, 0)
                        else
                            __findPathAndMove(ai, curMapID, curPosX, curPosY, targetCurPosX, targetCurPosY, true)
                        end
                    end
                end
            end
        end
    elseif ai.ai_state == AIState.ATTACKING then
        -- 如果对象已不存在或是已死亡,则切换目标或脱战
        local targetObj = GetObjectData(ai.cur_ack_target_info.guid)
        if targetObj == nil or targetObj:isDead() or (targetObj:GetInstanceID() ~= ai.owner_obj:GetInstanceID()) then
            -- 脱战
            __leaveCombat(ai)
        else
            -- 大于脱战距离就脱战,大于可攻击距离就进行追击
            local bornPosX, bornPosY = ai.owner_obj:GetBornPos()
            local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
            local bornPosDis = gTool.CalcuDistance(bornPosX, bornPosY, curPosX, curPosY)
            if bornPosDis > ai.owner_obj:GetLeaveCombatRange() then
                __leaveCombat(ai)
            else
                -- 与目标距离大于可攻击距离,进行追击
                local dis = ai.owner_obj:CalcuDistance(ai.cur_ack_target_info.guid)
                if dis >= 65535.0 then
                    -- 目标已经与自己不在同场景,直接脱战
                    __leaveCombat(ai)
                else
                    -- 如果距离大于可攻击距离,则进行追击
                    if dis > attackableRange then
                        -- 准备追击,如果已经注册了自动攻击的定时器,则取消该定时器
                        __removeAITimer(ai.do_attack_timer_id)
                        ai.do_attack_timer_id = nil

                        __followAttackTarget(ai, ai.cur_ack_target_info.guid)
                    end
                end
            end
        end
    end
end

--添加随机移动的定时器
__addRandomMoveTimer = function(ai)
    -- body
    if ai.rand_move_timer_id ~= nil then
        return
    end
    ai.rand_move_timer_id = gTimer.regCallback(math.random(3000, 6000), ILLEGAL_DATA_32, __onRandMoveTimer, ai.owner_obj)
end

--添加主动寻敌的定时器
__addFindAttackableTargetTimer = function(ai)
    if ai.find_target_ack_timer_id ~= nil then
        LuaLog.outError(string.format("无法添加寻敌定时器,timerID=%u", ai.find_target_ack_timer_id))
        return 
    end

    -- LuaLog.outDebug(string.format("怪物%u 注册寻敌定时器", ai.owner_obj.object_guid))
    ai.find_target_ack_timer_id = gTimer.regCallback(300, ILLEGAL_DATA_32, __onFindAttackableTargetTimer, ai)
end

--展开攻击
__startAttack = function(ai)
    if ai.do_attack_timer_id ~= nil then
        return 
    end

    -- LuaLog.outDebug(string.format("怪物(guid=%u)展开攻击,目标guid=%q", ai.owner_obj.object_guid, tostring(ai.cur_ack_target_info.guid)))
    
    -- 设置ai状态
    ai:SetAIState(AIState.ATTACKING)

    -- 200毫秒后进行第一次攻击,之后每1000毫秒攻击一次
    ai.do_attack_timer_id = gTimer.regCallback(200, 1, function(callbackID, curRepeatCount)
            __onDoAttackTimer(callbackID, 1, ai)
            -- 自动释放技能
            local ackSpeed = ai.owner_obj:GetAttackSpeed()
            -- LuaLog.outDebug(string.format("怪物guid=%u, 攻击速度=%u", ai.owner_obj.object_guid, ackSpeed))
            ai.do_attack_timer_id = gTimer.regCallback(ackSpeed, ILLEGAL_DATA_32, __onDoAttackTimer, ai)
        end)
    
    -- 战斗更新
    if ai.update_combat_timer_id == nil then
        ai.update_combat_timer_id = gTimer.regCallback(50, ILLEGAL_DATA_32, __onUpdateCombatTimer, ai)
    end
end

--离开战斗
__leaveCombat = function(ai)
    -- 回到出生点
    local curMapID = ai.owner_obj:GetCurMapID()
    local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
    local bornPosX, bornPosY = ai.owner_obj:GetBornPos()
    -- LuaLog.outDebug(string.format("怪物(guid=%u)准备脱战,回到坐标(%u,%u)", ai.owner_obj.object_guid, bornPosX, bornPosY))
    
    if ai.owner_obj:IsMoving() then
        __clearMovingPathInfo(ai)
        ai.owner_obj:StopMoving()
    end

    -- 检测能否直接走回出生点
    if SceneMapMgr.isMovableLineFromAToB(curMapID, curPosX, curPosY, bornPosX, bornPosY) then
        -- LuaLog.outDebug(string.format("可以直接走回出生点"))
        ai.owner_obj:StartMoving(bornPosX, bornPosY, 0)
    else
        __findPathAndMove(ai, curMapID, curPosX, curPosY, bornPosX, bornPosY, true)
    end

    -- 清除当前目标
    __clearCurAttackTargetInfo(ai)
    -- 清除定时攻击的timer
    __removeAITimer(ai.do_attack_timer_id)
    ai.do_attack_timer_id = nil
    -- 设置脱战状态
    ai:SetAIState(AIState.EVADE)
    -- 清除仇恨信息
    __clearThreatInfo(ai)
end

-- 脱战成功
__leaveCombatFinished = function(ai)
    -- body
    local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
    -- LuaLog.outDebug(string.format("怪物(guid=%u)脱战完成,当前坐标(%u,%u)", ai.owner_obj.object_guid, curPosX, curPosY))

    ai:SetAIState(AIState.IDLE)

    -- 将血量恢复满
    ai.owner_obj:OnLeaveCombatFinished()

    -- 移除战斗更新的定时器
    __removeAITimer(ai.update_combat_timer_id)
    ai.update_combat_timer_id = nil

    -- 移除路径行走定时器
    __clearMovingPathInfo(ai)

    -- 添加随机移动定时器
    if ai.move_type == enumMonsterMoveType.MONSTER_MOVE_RANDOM then
        __addRandomMoveTimer(ai)
    end
    -- 添加主动寻敌定时器
    if ai.attack_type == enumMonsterAttackMode.MONSTER_ACK_ACTIVE then
        __addFindAttackableTargetTimer(ai)
    end
end

function MonsterBaseAI:__init(monster, attackType, moveType)
	self.owner_obj = monster
	self.attack_type = attackType
	self.move_type = moveType

	self.rand_move_timer_id = nil       -- 随机移动定时器
    self.update_combat_timer_id = nil   -- 跟随移动定时器
    self.find_target_ack_timer_id = nil -- 寻敌定时器
    self.do_attack_timer_id = nil       -- 展开攻击定时器

    self.threat_list = {}               -- 仇恨列表
    self.max_threat_info = {}           -- 最大仇恨对象数据[1]=guid,[2]=仇恨值
    self.cur_ack_target_info =          -- 当前攻击目标信息
    { 
        guid = 0,
        curPosX = 0,
        curPosY = 0,
        cur_distance = 0                -- 当前与目标的距离
    }       
    self:SetAIState(AIState.IDLE)       -- ai状态
    
    -- 寻路数据相关
    self.cur_moving_path = nil
    self.cur_moving_path_index = 0

	LuaLog.outDebug(string.format("[怪物:%u] AI Create,ackType=%s, moveType=%s", monster.object_guid, tostring(attackType), tostring(moveType)))
end

function MonsterBaseAI:Term()

    __removeAITimer(self.rand_move_timer_id)
    self.rand_move_timer_id = nil
    __removeAITimer(self.update_combat_timer_id)
    self.update_combat_timer_id = nil
    __removeAITimer(self.find_target_ack_timer_id)
    self.find_target_ack_timer_id = nil
    __removeAITimer(self.do_attack_timer_id)
    self.do_attack_timer_id = nil

end

--[[ 设置aistate
]]
function MonsterBaseAI:SetAIState(newstate)
    if newstate >= AIState.MAX_COUNT then
        return 
    end

    self.ai_state = newstate
    if self.owner_obj ~= nil then
        self.owner_obj:SetAIState(newstate)
    end
end

--[[ 获取aistate
]]
function MonsterBaseAI:GetAIState()
    return self.ai_state
end

--[[ 能否被攻击
]]
function MonsterBaseAI:CanBeAttack(attacker)
    -- 脱战状态下无法被攻击
    if self.ai_state == AIState.EVADE then
        return enumCastSkillErrorCode.Bad_monster_state, "怪物脱战中"
    end

    return 0, ""
end

--[[ 被推放进世界
]]
function MonsterBaseAI:OnPushToWorld(mapid, instanceid)
	-- body
	
	-- 随机移动类型怪物,需要增加一个随机移动定时器
	if self.move_type == enumMonsterMoveType.MONSTER_MOVE_RANDOM then
		__addRandomMoveTimer(self)
	end

    -- 主动怪增加一个定时器进行寻敌攻击
    if self.attack_type == enumMonsterAttackMode.MONSTER_ACK_ACTIVE then
        __addFindAttackableTargetTimer(self)
    end

end

function MonsterBaseAI:OnBeAttacked(attacker, damage)
	-- 受击时取消随机移动
	__removeAITimer(self.rand_move_timer_id)
    self.rand_move_timer_id = nil

    if self.attack_type ~= enumMonsterAttackMode.MONSTER_ACK_PEACE then
        -- 更新仇恨列表
        __updateThreatList(self, attacker, damage)
        -- 准备开始反击
        if self.ai_state == AIState.IDLE then
            local ackerGuid = attacker:GetGUID()
            -- LuaLog.outDebug(string.format("怪物=%q 受击,准备开始回击,目标guid=%q", tostring(self.owner_obj.object_guid), tostring(ackerGuid)))
            -- 设置当前目标
            __setCurAttackTargetInfo(self, ackerGuid)
        end
    end
end

function MonsterBaseAI:OnBeKilled(killer)

	-- 把各定时器都取消掉,准备复活
    __removeAITimer(self.rand_move_timer_id)
    self.rand_move_timer_id = nil
    __removeAITimer(self.update_combat_timer_id)
    self.update_combat_timer_id = nil
    __removeAITimer(self.find_target_ack_timer_id)
    self.find_target_ack_timer_id = nil
    __removeAITimer(self.do_attack_timer_id)
    self.do_attack_timer_id = nil

    -- 清楚仇恨信息
    __clearThreatInfo(self)
    -- 清除目标信息
    __clearCurAttackTargetInfo(self)
    -- 清除路径移动信息
    __clearMovingPathInfo(self)
    -- 重置ai状态
    self:SetAIState(AIState.IDLE)

end

--[[移动被停止
]]
function MonsterBaseAI:OnMovingStopped(stopPosX, stopPosY)
    LuaLog.outDebug(string.format("怪物=%.0f 移动被中止,当前坐标(%.1f,%.1f)", self.owner_obj:GetGUID(), stopPosX, stopPosY))
    -- 如果处于寻路移动状态下,那么就进行下一个点的移动
    if self.cur_moving_path ~= nil then
        local ret = __doPathMoving(self)
        if not ret then
            -- 已经走到了路径的最后一点,到达了目的地
            __clearMovingPathInfo(self)
        end
    end
end

--[[进入激活模式
]]
function MonsterBaseAI:EnterActiveMode()
    -- 主动怪开始寻敌
    if self.attack_type == enumMonsterAttackMode.MONSTER_ACK_ACTIVE and self.find_target_ack_timer_id == nil then
        __addFindAttackableTargetTimer(self)
    end

    -- 随机移动怪开始随机移动
    if self.move_type == enumMonsterMoveType.MONSTER_MOVE_RANDOM and self.rand_move_timer_id == nil then
        __addRandomMoveTimer(self)
    end
end

--[[进入非激活模式
]]
function MonsterBaseAI:EnterDeactiveMode()
    -- 取消主动寻敌以及随机移动
    if self.find_target_ack_timer_id ~= nil then
        __removeAITimer(self.find_target_ack_timer_id)
        self.find_target_ack_timer_id = nil
    end

    if self.rand_move_timer_id ~= nil then
        __removeAITimer(self.rand_move_timer_id)
        self.rand_move_timer_id = nil
    end
end

--[[ 更新仇恨列表
]]
__updateThreatList = function(ai, attacker, damage)

    if ai.threat_list[attacker] == nil then
        ai.threat_list[attacker] = damage
    else
        ai.threat_list[attacker] = ai.threat_list[attacker] + damage
    end

    local curThreat = ai.threat_list[attacker]

    -- 更新最大仇恨者的信息
    if ai.max_threat_info[1] == nil then
        ai.max_threat_info = { attacker, curThreat }
    else
        -- 判断能否更换仇恨目标(不能更换就更新仇恨值)
        if ai.max_threat_info[1] == attacker then
            ai.max_threat_info[2] = curThreat
        else
            if curThreat > ai.max_threat_info[2] then
                ai.max_threat_info[1] = attacker
                ai.max_threat_info[2] = curThreat
            end
        end
    end

end

--[[ 清除仇恨信息
]]
__clearThreatInfo = function(ai)
    table.free(ai.threat_list)
    table.free(ai.max_threat_info)
end

--[[ 设置移动路径
]]
__setMovingPathInfo = function(ai, path)
    ai.cur_moving_path = path
    ai.cur_moving_path_index = 0
end

--[[ 清除移动路径
]]
__clearMovingPathInfo = function(ai)
    table.free(ai.cur_moving_path)
    ai.cur_moving_path = nil
    ai.cur_moving_path_index = 0
end

--[[ 进行路径移动
]]
__doPathMoving = function(ai)
    if ai.cur_moving_path == nil or ai.cur_moving_path_index >= #ai.cur_moving_path then
        return false
    end
    
    ai.cur_moving_path_index = ai.cur_moving_path_index + 1
    
    local dstX = ai.cur_moving_path[ai.cur_moving_path_index].x
    local dstY = ai.cur_moving_path[ai.cur_moving_path_index].y

    -- LuaLog.outDebug(string.format("怪物 %u准备向第%u个路点(%u,%u)进行移动", ai.owner_obj:GetGUID(), ai.cur_moving_path_index, dstX, dstY))
    ai.owner_obj:StartMoving(dstX, dstY, 0)

    return true
end

--[[ 寻路并尝试移动
]]
__findPathAndMove = function(ai, mapID, srcPosX, srcPosY, dstPosX, dstPosY, ingoreError)
    local path = SceneMapMgr.findMovePath(mapID, srcPosX, srcPosY, dstPosX, dstPosY, 300)
    if path == nil then
        if ingoreError then
            -- 如果忽略寻路失败的错误,那么直接让对象移动到指定坐标
            ai.owner_obj:StartMoving(dstPosX, dstPosY, 0)
            return true
        else
            -- 不忽略则return
            return false
        end
    else
        __setMovingPathInfo(ai, path)
        -- 如果怪物当前处于移动状态,则不进行移动,等下一次停止移动时通过回调激活再次移动
        if not ai.owner_obj:IsMoving() then
           __doPathMoving(ai)
        end
    end
    return true
end

--[[ 设置攻击目标信息
]]
__setCurAttackTargetInfo = function(ai, targetGuid, dis, targetPosX, targetPosY)

    ai.cur_ack_target_info.guid = targetGuid
    if dis == nil then
        dis, targetPosX, targetPosY = ai.owner_obj:CalcuDistance(targetGuid)
    end

    ai.cur_ack_target_info.curPosX = targetPosX
    ai.cur_ack_target_info.curPosY = targetPosY
    ai.cur_ack_target_info.cur_distance = dis

    -- 设置状态
    if dis < attackableRange then
        -- 直接攻击
        __startAttack(ai)
    else
        -- 获取一个攻击的坐标,并向该方位移动
        __followAttackTarget(ai, targetGuid)
    end
end

--[[ 清除目标信息
]]
__clearCurAttackTargetInfo = function(ai)
    -- body
    ai.cur_ack_target_info.guid = 0
    ai.cur_ack_target_info.curPosX = 0
    ai.cur_ack_target_info.curPosY = 0
    ai.cur_ack_target_info.cur_distance = 0
end

--[[ 追击攻击目标
]]
__followAttackTarget = function(ai, targetGuid)
    -- body

    local target = GetObjectData(targetGuid)
    if target == nil then
        return 
    end

    -- 获取目标当前坐标
    local curMapID = ai.owner_obj:GetCurMapID()
    local curPosX, curPosY = ai.owner_obj:GetCurPixelPos()
    local dstPosX, dstPosY = target:GetCurPixelPos()
    -- 检测能否直接到达该攻击点,如果不行的话,进行寻路后开始攻击
    if SceneMapMgr.isMovableLineFromAToB(curMapID, curPosX, curPosY, dstPosX, dstPosY) then
        -- 直接可走
        -- LuaLog.outDebug(string.format("可直接走到攻击点进行攻击"))
        ai.owner_obj:StartMoving(dstPosX, dstPosY, 0)
    else
        if not __findPathAndMove(ai, curMapID, curPosX, curPosY, dstPosX, dstPosY, false) then
            return 
        end
    end

    ai:SetAIState(AIState.FOLLOW)
    -- 添加一个用于检测的定时器,检测是否已经到达了可攻击范围
    if ai.update_combat_timer_id == nil then
        ai.update_combat_timer_id = gTimer.regCallback(50, ILLEGAL_DATA_32, __onUpdateCombatTimer, ai)
    end
end

--endregion
