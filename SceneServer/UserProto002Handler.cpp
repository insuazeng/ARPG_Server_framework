#include "stdafx.h"
#include "GameSession.h"

// 处理角色场景加载、请求移动、请求地图传送等协议
void GameSession::HandleSessionPing(WorldPacket &recvPacket)
{

}

void GameSession::HandlePlayerMove(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();
	if (!plr->IsInWorld())
		return;

	pbc_rmessage *msg2003 = sPbcRegister.Make_pbc_rmessage("UserProto002003", (void*)recvPacket.contents(), recvPacket.size());
	if (msg2003 == NULL)
		return ;

	FloatPoint srcPos, destPos;
	srcPos.m_fX = pbc_rmessage_real(msg2003, "srcPosX", 0);
	srcPos.m_fY = pbc_rmessage_real(msg2003, "srcPosY", 0);
	destPos.m_fX = pbc_rmessage_real(msg2003, "destPosX", 0);
	destPos.m_fY = pbc_rmessage_real(msg2003, "destPosY", 0);

	// 写一个函数检测该条路径是否可走
	if (srcPos.m_fX <= 0.0f || srcPos.m_fY <= 0.0f || destPos.m_fX <= 0.0f || destPos.m_fY <= 0.0f)
		return ;
	if (srcPos.m_fX > mapLimitWidth || srcPos.m_fY > mapLimitLength || destPos.m_fX > mapLimitWidth || destPos.m_fY > mapLimitLength)
		return ;
	// 如果起点坐标与当前坐标相差太远,则无法行走
	float dist = plr->CalcDistance(srcPos.m_fX, srcPos.m_fY);
	if (dist >= 8.0f)
	{
		sLog.outError("[移动]玩家:%s(guid="I64FMTD")请求移动(起点:%.1f,%.1f)与当前坐标(%.1f,%.1f)距离(%.1f)太远,不处理请求", plr->strGetGbkNameString().c_str(), plr->GetGUID(),
						srcPos.m_fX, srcPos.m_fY, plr->GetPositionX(), plr->GetPositionY(), dist);
		return ;
	}

	// 如果源目标与目标坐标相等，说明停止移动，某则说明启动移动或者进行转向
	bool stopMoving = false;
	if (srcPos == destPos)
		stopMoving = true;

	if (stopMoving)
	{
		// 停止移动
		plr->StopMoving(false, destPos);
	}
	else
	{
		uint8 moveMode = OBJ_MOVE_TYPE_START_MOVING;
		plr->StartMoving(srcPos.m_fX, srcPos.m_fY, destPos.m_fX, destPos.m_fY, moveMode);
	}

	DEL_PBC_R(msg2003);
}

void GameSession::HandlePlayerSceneTransfer(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();
	if (!plr->IsInWorld())
		return ;

	pbc_rmessage *msg2005 = sPbcRegister.Make_pbc_rmessage("UserProto002005", (void*)recvPacket.contents(), recvPacket.size());
	if (msg2005 == NULL)
		return ;

	uint32 destScene = pbc_rmessage_integer(msg2005, "destSceneID", 0, NULL);
	FloatPoint destPos;
	destPos.m_fX = pbc_rmessage_integer(msg2005, "destPosX", 0, NULL);
	destPos.m_fY = pbc_rmessage_integer(msg2005, "destPosY", 0, NULL);

	plr->HandleSceneTeleportRequest(ILLEGAL_DATA_32, destScene, destPos);
	DEL_PBC_R(msg2005);
}

/*void GameSession::HandleGameLoadedOpcode(WorldPacket & recvPacket)
{
	Player *plr = GetPlayer();

	sLog.outString("玩家%s loading地图完成 准备加入场景%u(%.1f,%.1f)", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetPositionX(), plr->GetPositionY());	
	plr->AddToWorld();

	// 如果玩家并未成功加入场景中
	if ( !plr->IsInWorld() )
	{
		sLog.outError("玩家 %s("I64FMTD") loading地图%u(instance=%u)失败 被断开", plr->strGetGbkNameString().c_str(), plr->GetGUID(), plr->GetMapId(), plr->GetInstanceID());
		LogoutPlayer(false);
		Disconnect();
		return;
	}

	// 推送玩家对象的创建信息到客户端
	pbc_wmessage *msg2002 = sPbcRegister.Make_pbc_wmessage("UserProto002002");
	if (msg2002 == NULL)
		return ;
	pbc_wmessage *creationBuffer = pbc_wmessage_message(msg2002, "playerCreationData");
	if (creationBuffer == NULL)
		return ;

	plr->BuildCreationDataBlockToBuffer(creationBuffer, plr);
	WorldPacket packet(SMSG_002002, 256);
	sPbcRegister.EncodePbcDataToPacket(packet, msg2002);
	uint32 packSize = packet.size();
	SendPacket(&packet);

	DEL_PBC_W(msg2002);
	sLog.outString("玩家 %s 成功加入场景 %u(InstanceID = %u),创角BufferSize=%u", plr->strGetGbkNameString().c_str(), plr->GetMapId(), plr->GetInstanceID(), packSize);
	//if (!plr->IsLoaded())
	//{
	//	// 通知到中央服玩家已经第一次进入本场景进程
	//	WorldPacket centrePacket(GAME_TO_CENTER_PLAYER_OPERATION);
	//	centrePacket << uint32(0) << sInfoCore.GetSelfSceneServerIndex();
	//	centrePacket << GetAccountId() << plr->GetGUID();
	//	centrePacket << uint32(STC_Player_op_first_loaded);
	//	SendServerPacket(&centrePacket);

	//	// 通知到其他场景服玩家已经第一次进入本场景进程
	//	WorldPacket scenePacket(GAME_TO_GAME_SERVER_MSG);
	//	scenePacket << uint32(ILLEGAL_DATA) << sInfoCore.GetSelfSceneServerIndex();
	//	scenePacket << uint32(STS_Server_msg_op_player_first_loaded);
	//	scenePacket << plr->GetGUID();
	//	SendServerPacket(&scenePacket);
	//}
}*/
