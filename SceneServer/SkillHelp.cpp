#include "stdafx.h"
#include "SkillHelp.h"

float SkillHelp::sectorRadianArray[GDirection16_Count][SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];
float SkillHelp::sectorDistanceArray[SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];

SkillHelp::SkillHelp(void)
{

}

SkillHelp::~SkillHelp(void)
{

}

int SkillHelp::fillMonsterGuidsAroundObject(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	if (obj == NULL)
		LuaReturn(l, top, 0);
	float radius = lua_tonumber(l, 2);
	if (!obj->IsInWorld() || obj->GetInRangeCount() == 0)
		LuaReturn(l, top, 0);

	vector<uint64> vecGuids;

	Object *target = NULL;
	InRangeSet::iterator itBegin = obj->GetInRangeSetBegin();
	InRangeSet::iterator itEnd = obj->GetInRangeSetEnd();
	for (; itBegin != itEnd; ++itBegin)
	{
		target = (*itBegin);
		if (target == NULL || !target->IsInWorld() || !target->isMonster() || !obj->CanSee(target))
			continue;

		float dis = obj->CalcDistance(target);
		if (dis > radius)
			continue; 

		vecGuids.push_back(target->GetGUID());
	}

	if (!vecGuids.empty())
		SkillHelp::Push64BitNumberArray(l, &vecGuids[0], vecGuids.size());
	else
		SkillHelp::Push64BitNumberArray(l, NULL, 0);

	LuaReturn(l, top, 1);
}

int SkillHelp::fillObjectGuidsAroundObject(lua_State *l)
{
	int32 top = lua_gettop(l);

	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	if (obj == NULL)
		LuaReturn(l, top, 0);

	float radius = lua_tonumber(l, 2);
	if (!obj->IsInWorld() || obj->GetInRangeCount() == 0)
		LuaReturn(l, top, 0);
	
	vector<uint64> vecGuids;
	Object *target = NULL;
	InRangeSet::iterator itBegin = obj->GetInRangeSetBegin();
	InRangeSet::iterator itEnd = obj->GetInRangeSetEnd();
	for (; itBegin != itEnd; ++itBegin)
	{
		target = (*itBegin);
		if (target == NULL || !target->IsInWorld() || !obj->CanSee(target))
			continue;

		float dis = obj->CalcDistance(target);
		if (dis > radius)
			continue; 

		vecGuids.push_back(target->GetGUID());
	}

	if (!vecGuids.empty())
		SkillHelp::Push64BitNumberArray(l, &vecGuids[0], vecGuids.size());
	else
		SkillHelp::Push64BitNumberArray(l, NULL, 0);

	LuaReturn(l, top, 1);
}

int SkillHelp::fillPlayerGuidsAroundObject(lua_State *l)
{
	int32 top = lua_gettop(l);
	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	if (obj == NULL)
		LuaReturn(l, top, 0);

	float radius = lua_tonumber(l, 2);
	if (!obj->IsInWorld())
		LuaReturn(l, top, 0);

	LuaReturn(l, top, 0);
}

int SkillHelp::findObjectAssaultTargetPos(lua_State *l)
{
	int32 top = lua_gettop(l);
	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	if (obj == NULL || !obj->IsInWorld())
		LuaReturn(l, top, 0);

	MapMgr *mgr = obj->GetMapMgr();
	uint64 targetGuid = (uint64)lua_tointeger(l, 2);
	FloatPoint targetPos;
	targetPos.m_fX = lua_tonumber(l, 3);
	targetPos.m_fY = lua_tonumber(l, 4);
	float assaultLength = lua_tonumber(l, 5);		// 冲刺长度
	// 如果lua传来了目标对象guid,那么冲刺的目标点设定为对象的坐标点
	Object *target = mgr->GetObject(targetGuid);
	if (target != NULL)
	{
		targetPos.m_fX = target->GetPositionX();
		targetPos.m_fY = target->GetPositionY();
	}
	// 如果与目标距离小于1米,则不进行冲锋
	FloatPoint curPos(obj->GetPositionX(), obj->GetPositionY());
	float dis = obj->CalcDistance(targetPos.m_fX, targetPos.m_fY);
	if (dis <= 1.0f)
	{
		lua_pushnumber(l, curPos.m_fX);
		lua_pushnumber(l, curPos.m_fY);
	}
	else
	{
		// 首先判断该条路径能否冲锋
		FloatPoint lastValidPos;
		bool canAssault = SceneMapMgr::IsMovableLineFromAToB(mgr->GetMapId(), curPos.m_fX, curPos.m_fY, targetPos.m_fX, targetPos.m_fY, &lastValidPos);
		if (canAssault)
		{
			float len = dis - 1.0f;
			if (len > assaultLength)
				len = assaultLength;
			// 得到向量
			FloatPoint vec = targetPos - curPos;
			vec.Normalize();
			curPos.m_fX += (vec.m_fX * len);
			curPos.m_fY += (vec.m_fY * len);
			lua_pushnumber(l, curPos.m_fX);
			lua_pushnumber(l, curPos.m_fY);
		}
		else
		{
			lua_pushnumber(l, lastValidPos.m_fX);
			lua_pushnumber(l, lastValidPos.m_fY);
		}
	}
	LuaReturn(l, top, 2);
}

int SkillHelp::checkSectorTargetsValidtiy(lua_State *l)
{
	int32 top = lua_gettop(l);
	luaL_checktype(l, 1, LUA_TUSERDATA);
	Object *obj = *(Object**)lua_touserdata(l, 1);
	if (obj == NULL || !obj->IsInWorld())
		LuaReturn(l, top, 0);

	MapMgr *mgr = obj->GetMapMgr();
	float curDirection = lua_tonumber(l, 2);	// 当前方向
	float sectorAngle = lua_tonumber(l, 3);		// 扇形角度
	float radius = lua_tonumber(l, 4);			// 扇形半径

	if (lua_istable(l, 5) == 0)
		LuaReturn(l, top, 0);

	vector<uint64> targetGuids;
	int32 cnt = lua_objlen(l, 5);
	lua_pushnil(l);
	for (int i = 0; i < cnt; ++i)
	{
		lua_next(l, 5);
		uint64 guid = (uint64)lua_tointeger(l, -1);
		targetGuids.push_back(guid);
		lua_pop(l, 1);
	}
	lua_pop(l, 1);

	//OutputMsg(rmTip, "guid=%d 申请扇形检测(%s),角度=%.2f,长度=%.2f,施法=[%.2f,%.2f], 目标=[%.2f,%.2f]", nGuid, ss_target.str().c_str(), angle, radius, 
	//		castPosX, castPosY, targetPosX, targetPosY);

	// 有效的目标列表
	vector<uint64> validTargetGuids;
	// 采用查表法进行目标检测（效率会更高一些）
	SkillHelp::CheckSectorTargetsValidtiy(mgr, obj->GetPositionX(), obj->GetPositionY(), curDirection, sectorAngle, radius, targetGuids, validTargetGuids);
	
	if (!validTargetGuids.empty())
		SkillHelp::Push64BitNumberArray(l, &validTargetGuids[0], validTargetGuids.size());
	else
		SkillHelp::Push64BitNumberArray(l, NULL, 0);

	// OutputMsg(rmTip, "[查表法]检测合格目标有 %s", ss.str().c_str());
	LuaReturn(l, top, 1);
}

void SkillHelp::Push64BitNumberArray(lua_State *l, void *arr, const uint32 arrSize)
{
	if (arr == NULL || arrSize == 0)
	{
		lua_pushnil(l);
	}
	else
	{
		uint64 *pData = (uint64*)arr;
		lua_createtable(l, (int)arrSize, 0);
		for (int i = 1; i <= arrSize; ++i)
		{
			uint64 val = *pData;
			lua_pushinteger(l, (LUA_INTEGER)val);
			lua_rawseti(l, -2, i);
			pData++;
		}
	}
}

float SkillHelp::CalcuDistance(float startX, float startY, float endX, float endY)
{
	float x = endX - startX;
	float y = endY - startY;
	float fRet = 0.0f;
	if (fabs(x) >= 0.0001f || fabs(y) >= 0.0001f)
		fRet = sqrt(pow(x, 2) + pow(y, 2));
	return fRet;
}

float SkillHelp::CalcuDirection(float srcX, float srcY, float dstX, float dstY)
{
	double xDiff = dstX - srcX;
	double yDiff = dstY - srcY;

	// 先计算两点之间的角度范围
	double angle = atan2(yDiff, xDiff) * 180.0 / PI;
	if (angle > 90.0 && angle < 270.0)
		angle -= 360.0;

	double ret = 90.0 - angle;
	if (ret < 0.0)
		ret = 0.0;
	return float(ret);
}

const uint32 SkillHelp::CalcuDireciton16(float fDir)
{
	// 以正上方开始,向右旋转,为0-360的区间范围
	// 每22.5度为一个方向,共16个方向
	static const uint32 maxIndexCount = 32;
	static const uint32 szDirection16[maxIndexCount] = 
	{
		GDirection16_Up,
		GDirection16_RightUpUp,
		GDirection16_RightUpUp,
		GDirection16_RightUp,
		GDirection16_RightUp,
		GDirection16_RightUpRight,
		GDirection16_RightUpRight,
		GDirection16_Right,

		GDirection16_Right,
		GDirection16_RightDownRight,
		GDirection16_RightDownRight,
		GDirection16_RightDown,
		GDirection16_RightDown,
		GDirection16_RightDownDown,
		GDirection16_RightDownDown,
		GDirection16_Down,

		GDirection16_Down,
		GDirection16_LeftDownDown,
		GDirection16_LeftDownDown,
		GDirection16_LeftDown,
		GDirection16_LeftDown,
		GDirection16_LeftDownLeft,
		GDirection16_LeftDownLeft,
		GDirection16_Left,

		GDirection16_Left,
		GDirection16_LeftUpLeft,
		GDirection16_LeftUpLeft,
		GDirection16_LeftUp,
		GDirection16_LeftUp,
		GDirection16_LeftUpUp,
		GDirection16_LeftUpUp,
		GDirection16_Up
	};

	uint32 index = fDir / 11.25f;
	if (index >= maxIndexCount)
		index = 0;

	return szDirection16[index];
}

int32 SkillHelp::CheckSectorTargetsValidtiy(MapMgr *mgr, float pixelPosX, float pixelPosY, float dir, float sectorAngle, float sectorRadius, 
										   vector<uint64> &inGuids, vector<uint64> &outGuids)
{
	static const char* szDirection16Name[GDirection16_Count] = 
	{
		"上", "右上偏上", "右上", "右上偏右",
		"右", "右下偏右", "右下", "右下偏下",
		"下", "左下偏下", "左下", "左下偏左",
		"左", "左上偏左", "左上", "左上偏上"
	};

	// 把技能释放的起点与终点坐标设置在格子的中间点
	float castGridPosX = floor(pixelPosX / lengthPerGrid) + 0.5f;
	float castGridPosY = floor(pixelPosY / lengthPerGrid) + 0.5f;
	
	// 传入的扇形检测角度需要除以2(对半分，但允许有一定误差左右+5度)
	sectorAngle = sectorAngle / 2.0f + 5.0f;
	
	const uint32 dir16 = SkillHelp::CalcuDireciton16(dir);
	Object *obj = NULL;

	// sLog.outDebug("本次扇形检测,方向数据表%u- %s,施法坐标(%.2f,%.2f),扇形角度=%.2f,距离=%u", dir16, szDirection16Name[dir16], castGridPosX, castGridPosY, sectorAngle, sectorRadius);
	// 2.根据方向取得对应角度、距离检测表，根据角度、距离检测表来逐一判断对象是否处于合法范围内

	float enemyGridPosX = 0.0f, enemyGridPosY = 0.0f;
	for (int i = 0; i < inGuids.size(); ++i)
	{
		uint64 guid = inGuids[i];
		obj = mgr->GetObject(guid);
		if (obj == NULL)
			continue ;

		/*enemyGridPosX = obj->GetGridPositionX() + 0.5f;
		enemyGridPosY = obj->GetGridPositionY() + 0.5f;*/
		enemyGridPosX = obj->GetGridPositionX();
		enemyGridPosY = obj->GetGridPositionY();

		// 3.计算目标坐标与施法坐标之间的x,y差值，如果超出查表范围，则认为超出施法距离
		int xDiff = snTargetCheckerCoordCentrePosX + int(castGridPosY) - int(enemyGridPosY);
		int yDiff = snTargetCheckerCoordCentrePosY + int(enemyGridPosX) - int(castGridPosX);
		if (xDiff < 0 || xDiff > 40 || yDiff < 0 || yDiff > 40)
			continue ;

		// 4.判断给定角度、距离是否在查表角度、距离范围内
		float tableRadianValue = sectorRadianArray[dir16][xDiff][yDiff];
		// float tableDistanceValue = sectorDistanceArray[xDiff][yDiff];
		float realDistance = obj->CalcDistance(pixelPosX, pixelPosY);
		if ((tableRadianValue <= sectorAngle && realDistance <= sectorRadius) ||
			(xDiff == snTargetCheckerCoordCentrePosX && yDiff == snTargetCheckerCoordCentrePosY))
		{
			// 目标有效
			outGuids.push_back(guid);
			// sLog.outDebug("[成功],guid="I64FMTD",表角度=%.2f,表距离=%.2f,下标[%d,%d]", guid, tableRadianValue, tableDistanceValue, xDiff, yDiff);
		}
		else
		{
			/*sLog.outDebug("[失败]guid="I64FMTD",坐标(%.2f, %.2f),表角度=%.2f,表距离=%.2f,下标[%d,%d]", guid, 
						enemyGridPosX, enemyGridPosY, tableRadianValue, tableDistanceValue, xDiff, yDiff);*/
		}
	}

	return inGuids.size() - outGuids.size();
}

void SkillHelp::InitAttackableTargetCheckerTable()
{
	SkillHelp::InitSectorTargetCheckerTable();
	SkillHelp::InitRectangleTargetCheckerTable();
}

// 初始化扇形目标检测数据表
void SkillHelp::InitSectorTargetCheckerTable()
{
	const float RADIAN_STEP = 22.5f;
	float centerPosX = (float)snTargetCheckerCoordCentrePosX;
	float centerPosY = (float)snTargetCheckerCoordCentrePosY;

	for (int index = 0; index < GDirection16_Count; ++index)
	{
		for (int x = 0; x < SKILL_HELP_MAX_CHECK_GRID; ++x)
		{
			for (int y = 0; y < SKILL_HELP_MAX_CHECK_GRID; ++y)
			{
				if (x == snTargetCheckerCoordCentrePosX && y == snTargetCheckerCoordCentrePosY)
					continue; 
				else
				{
					sectorRadianArray[index][x][y] = atan2f(((float)y - centerPosY), ((float)x - centerPosX)) * 180.0f / PI + 180.0f + index * RADIAN_STEP;
					
					if (sectorRadianArray[index][x][y] > 360.0f)
						sectorRadianArray[index][x][y] -= 360.0f;
					
					if (sectorRadianArray[index][x][y] >= 180.0f)
					{
						sectorRadianArray[index][x][y] = fabs(sectorRadianArray[index][x][y] - 360.0f);
					}
					/*
					if (sectorRadianArray[index][x][y] >= 90.99f)
					{
						if (sectorRadianArray[index][x][y] <= 269.0f)
						{
							// 在一个方向上，只生成平面180度的检索数据，因此在180度外的角度数据将被设置为0，距离值设置为999（一定能超出范围）
							sectorRadianArray[index][x][y] = 0.0f;
							sectorDistanceArray[index][x][y] = 999.0f;
						}
						else
						{
							sectorRadianArray[index][x][y] = fabs(sectorRadianArray[index][x][y] - 360.0f);
						}
					}*/
				}
			}
		}
	}
	for (int x = 0; x < SKILL_HELP_MAX_CHECK_GRID; ++x)
	{
		for (int y = 0; y < SKILL_HELP_MAX_CHECK_GRID; ++y)
		{
			if (x == snTargetCheckerCoordCentrePosX && y == snTargetCheckerCoordCentrePosY)
				sectorDistanceArray[x][y] = 0.0f;
			else
			{
				float fX = (float)x + 0.5f;
				float fY = (float)y + 0.5f;
				float dis = SkillHelp::CalcuDistance(centerPosX, centerPosY, fX, fY);
				sectorDistanceArray[x][y] = dis * lengthPerGrid;
			}
		}
	}
}

// 初始化矩形目标检测数据表
void SkillHelp::InitRectangleTargetCheckerTable()
{

}


LUA_IMPL(SkillHelp, SkillHelp)

LUA_METD_START(SkillHelp)
LUA_METD_END

LUA_FUNC_START(SkillHelp)
L_FUNCTION(SkillHelp, fillMonsterGuidsAroundObject)
L_FUNCTION(SkillHelp, fillObjectGuidsAroundObject)
L_FUNCTION(SkillHelp, findObjectAssaultTargetPos)
L_FUNCTION(SkillHelp, checkSectorTargetsValidtiy)
LUA_FUNC_END

