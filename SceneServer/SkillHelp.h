#ifndef _SKILL_HELP_H_
#define _SKILL_HELP_H_

// 用于为lua提供技能辅助判断的类
#include "script/Lunar.h"

// 以(20,20)为圆心,最多检测半径20格(640像素)的目标
#define SKILL_HELP_MAX_CHECK_GRID	41

class SkillHelp
{
public:
	SkillHelp(void);
	~SkillHelp(void);

public:
	// 导出给lua的
	static int fillMonsterGuidsAroundObject(lua_State *l);
	static int fillObjectGuidsAroundObject(lua_State *l);
	static int fillPlayerGuidsAroundObject(lua_State *l);

	static int findObjectAssaultTargetPos(lua_State *l);
	static int checkSectorTargetsValidtiy(lua_State *l);		// 检测扇形目标的有效性

public:
	static void Push64BitNumberArray(lua_State *l, void *arr, const uint32 arrSize);
	static float CalcuDistance(float startX, float startY, float endX, float endY);
	static float CalcuDirection(float srcX, float srcY, float dstX, float dstY);
	static const uint32 CalcuDireciton16(float fDir);
	static int32 CheckSectorTargetsValidtiy(MapMgr *mgr, float pixelPosX, float pixelPosY, float dir, float sectorAngle, float sectorRadius, 
											vector<uint64> &inGuids, vector<uint64> &outGuids);
	
	static void InitAttackableTargetCheckerTable();
	// 初始化扇形目标检测数据表
	static void InitSectorTargetCheckerTable();
	// 初始化矩形目标检测数据表
	static void InitRectangleTargetCheckerTable();


	LUA_EXPORT(SkillHelp)

private:
	// 根据扇形构建的16个方向的,以[20,20]为中心点,构建的长度最多为20的角度、距离数据
	static float sectorRadianArray[GDirection16_Count][SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];
	static float sectorDistanceArray[SKILL_HELP_MAX_CHECK_GRID][SKILL_HELP_MAX_CHECK_GRID];
	static const int snTargetCheckerCoordCentrePosX = 20;
	static const int snTargetCheckerCoordCentrePosY = 20;
};


#endif // !_SKILL_HELP_H_



