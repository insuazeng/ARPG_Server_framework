--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

--region *.lua
--Date
--此文件由[BabeLua]插件自动生成

-- 玩家伙伴的功能
module("SceneScript.module.player.lua_player", package.seeall)

local PartnerModule = require("SceneScript.module.partner.lua_partner")

--[[创建一个伙伴
]]
function LuaPlayer:CreatePartner(protoID)
    LuaLog.outDebug(string.format("CreatePartner called, proto=%u", protoID))
    if self:GetPartnerByProto(protoID) ~= nil then
        return 
    end
    if self.cpp_object == nil then
        return 
    end

    -- 通过c++的接口创建伙伴
    local p = self.cpp_object:releasePartner(protoID)
    if p == nil then
        LuaLog.outError(string.format("无法为玩家:%q 创建伙伴:%u", tostring(self:GetGUID()), protoID))
        return
    end

    -- 创建lua的伙伴对象
    local luaPartner = PartnerModule.LuaPartner(p:getGuid(), protoID, self, p)
    self.partner_list[protoID] = luaPartner
end

--[[收回一个伙伴
]]
function LuaPlayer:RemovePartner(protoID)
    
    LuaLog.outDebug(string.format("RemovePartner called, proto=%u", protoID))

    local p = self:GetPartnerByProto(protoID)
    if p == nil then
        return 
    end

    if self.cpp_object == nil then
        return 
    end

    LuaLog.outDebug(string.format("玩家:%q 的伙伴(proto=%u,guid=%q)被从lua中移除", tostring(self:GetGUID()), protoID, p:GetGUID()))
    p:Term()
    self.partner_list[protoID] = nil


    local ret = self.cpp_object:revokePartner(protoID)
    if ret == nil then
        LuaLog.outError(string.format("玩家:%q C++移除伙伴(proto=%u)失败", tostring(self:GetGUID()), protoID))
        return 
    end

end

--[[获取伙伴
]]
function LuaPlayer:GetPartnerByProto(protoID)
    local ret = nil 

    if self.partner_list[protoID] ~= nil then
        ret = self.partner_list[protoID]
    end

    return ret
end

function LuaPlayer:GetPartnerByGuid(partnerGuid)

    local ret = nil 
    for k,v in pairs(self.partner_list) do
        local p = v
        if p:GetGUID() == partnerGuid then
            ret = v
            break
        end
    end
    return ret
end

--endregion


--endregion
