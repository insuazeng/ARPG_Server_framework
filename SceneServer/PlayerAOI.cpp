#include "stdafx.h"
#include "Player.h"

// 与场景aoi相关的部分函数
void Player::BuildCreationDataBlockForPlayer( Player* target )
{
	// 把自己的角色创建数据推给目标玩家
	Object::BuildCreationDataBlockForPlayer(target);
	// 将伙伴数据一并推送给玩家
	if (!m_Partners.empty())
	{
		PlayerPartnerMap::iterator it = m_Partners.begin();
		Partner *p = NULL;
		for (; it != m_Partners.end(); ++it)
		{
			p = it->second;
			p->BuildCreationDataBlockForPlayer(target);
		}
	}
}

bool Player::BuildValuesUpdateBlockForPlayer( Player *target )
{
	bool selfUpdate = Object::BuildValuesUpdateBlockForPlayer(target);
	bool partnerUpdate = false;
	PlayerPartnerMap::iterator it = m_Partners.begin();
	Partner *p = NULL;
	for (; it != m_Partners.end(); ++it)
	{
		p = it->second;
		if (p == NULL || !p->BuildValuesUpdateBlockForPlayer(target))
			continue ;
		partnerUpdate = true;
	}

	// 玩家或伙伴有任意一个将更新数据写入了target的buffer,都认为设置更新数据成功
	return (selfUpdate || partnerUpdate) ? true : false;
}

void Player::BuildRemoveDataBlockForPlayer( Player *target )
{
	// 如果存在伙伴的话,先将伙伴从目标的视野中移除
	Partner *p = NULL;
	PlayerPartnerMap::iterator it = m_Partners.begin();
	for (; it != m_Partners.end(); ++it)
	{
		p = it->second;
		p->BuildRemoveDataBlockForPlayer(target);
	}
	// 再移除自身
	Object::BuildRemoveDataBlockForPlayer(target);
}

void Player::ClearUpdateMask()
{
	Object::ClearUpdateMask();
	// 取消伙伴的更新码
	PlayerPartnerMap::iterator it = m_Partners.begin();
	for (; it != m_Partners.end(); ++it)
	{
		it->second->ClearUpdateMask();
	}
}

void Player::PushSingleObjectPropUpdateData()
{
	_bufferS.Acquire();

	if (m_mapMgr && !bProcessPending)
	{
		bProcessPending = true;
		m_mapMgr->PushToProcessed(this);
	}

	_bufferS.Release();
}

void Player::PushCreationData()
{
	// imagine the bytebuffer getting appended from 2 threads at once! :D
	_bufferS.Acquire();

	// add to process queue
	if(m_mapMgr && !bCreateProcessPending)
	{
		bCreateProcessPending = true;
		m_mapMgr->PushToCreationProcessed(this);
	}

	_bufferS.Release();
}

void Player::PushMovementData(uint64 objguid, float srcX, float srcY, float srcHeight, float destX, float destY, float destHeight, uint8 mode)
{
	// 可以对目标点以及当前点进行判断，如果范围太远，则不需要push到该玩家的视野内
	/*if(abs(x-(uint16)GetPositionX())>23||abs(y-(uint16)GetPositionY())>23)
	return;*/

	if (pbc_wmessage_size(m_pbcMovmentDataBuffer, "objMovmentList") > 400)
		return ;
	
	_bufferS.Acquire();

	pbc_wmessage *data = pbc_wmessage_message(m_pbcMovmentDataBuffer, "objMovmentList");
	if (data != NULL)
	{
		sPbcRegister.WriteUInt64Value(data, "objGuid", objguid);
		pbc_wmessage_real(data, "srcPosX", srcX);
		pbc_wmessage_real(data, "srcPosY", srcY);
		pbc_wmessage_real(data, "srcHeight", srcHeight);
		pbc_wmessage_real(data, "destPosX", destX);
		pbc_wmessage_real(data, "destPosY", destY);
		pbc_wmessage_real(data, "destHeight", destHeight);
		pbc_wmessage_integer(data, "moveType", (uint32)mode, 0);

		if(m_mapMgr && !bMoveProcessPending)
		{
			bMoveProcessPending = true;
			m_mapMgr->PushToMoveProcessed(this);
		}
	}
	
	_bufferS.Release();
}

void Player::PushOutOfRange(const uint64 &guid, uint32 objtype)
{
	_bufferS.Acquire();

	sPbcRegister.WriteUInt64Value(m_pbcRemoveDataBuffer, "removeObjectGuids", guid);
	++m_pbcRemoveDataCounter;

	// add to process queue
	if(m_mapMgr && !bRemoveProcessPending)
	{
		bRemoveProcessPending = true;
		m_mapMgr->PushToProcessed(this);
	}
	_bufferS.Release();
}

void Player::ProcessObjectMovmentUpdates()
{
	_bufferS.Acquire();
	
	int32 cnt = pbc_wmessage_size(m_pbcMovmentDataBuffer, "objMovmentList");
	if (cnt <= 0)
	{
		_bufferS.Release();
		return;
	}

	WorldPacket packet(SMSG_002004, 256);
	sPbcRegister.EncodePbcDataToPacket(packet, m_pbcMovmentDataBuffer);
	m_session->SendPacket(&packet);

	// sLog.outDebug("向玩家%s("I64FMTD")发送%d个[移动]通知包", strGetGbkNameString().c_str(), GetGUID(), cnt);
	
	bMoveProcessPending = false;

	DEL_PBC_W(m_pbcMovmentDataBuffer);
	m_pbcMovmentDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002004");
	
	_bufferS.Release();
}

void Player::ProcessObjectCreationUpdates()
{
	_bufferS.Acquire();
	
	int32 cnt = pbc_wmessage_size(m_pbcCreationDataBuffer, "createObjectList");
	if (cnt <= 0)
	{
		_bufferS.Release();
		return ;
	}
	
	WorldPacket packet(SMSG_002008, 2048);
	sPbcRegister.EncodePbcDataToPacket(packet, m_pbcCreationDataBuffer);
	m_session->SendPacket(&packet);

	// sLog.outDebug("向玩家%s("I64FMTD")发送%d个[创建]通知包", strGetGbkNameString().c_str(), GetGUID(), cnt);

	bCreateProcessPending = false;

	DEL_PBC_W(m_pbcCreationDataBuffer);
	m_pbcCreationDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002008");

	_bufferS.Release();
}

void Player::ProcessObjectPropUpdates()
{
	_bufferS.Acquire();

	if (pbc_wmessage_size(m_pbcPropUpdateDataBuffer, "updateObjects") <= 0)
	{
		bProcessPending = false;
		_bufferS.Release();
		return ;
	}

	WorldPacket packet(SMSG_002012, 128);
	sPbcRegister.EncodePbcDataToPacket(packet, m_pbcPropUpdateDataBuffer);
	m_session->SendPacket(&packet);

	// sLog.outDebug("向玩家%s("I64FMTD")发送%u个[属性更新]通知包", strGetGbkNameString().c_str(), GetGUID(), m_objPropUpdateProtoDataBuffer.updateobjects_size());

	DEL_PBC_W(m_pbcPropUpdateDataBuffer);
	m_pbcPropUpdateDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002012");
	bProcessPending = false;

	_bufferS.Release();

}

void Player::ProcessObjectRemoveUpdates()
{
	_bufferS.Acquire();
	// int32 cnt = pbc_wmessage_size(m_pbcRemoveDataBuffer, "removeObjectGuids");
	if (m_pbcRemoveDataCounter <= 0)
	{
		bRemoveProcessPending = false;
		_bufferS.Release();
		return ;
	}

	WorldPacket packet(SMSG_002010, 128);
	sPbcRegister.EncodePbcDataToPacket(packet, m_pbcRemoveDataBuffer);
	m_session->SendPacket(&packet);

	sLog.outDebug("向玩家%s("I64FMTD")发送%u个[移除]通知包", strGetGbkNameString().c_str(), GetGUID(), m_pbcRemoveDataCounter);

	DEL_PBC_W(m_pbcRemoveDataBuffer);
	m_pbcRemoveDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002010");
	m_pbcRemoveDataCounter = 0;

	bRemoveProcessPending = false;
	_bufferS.Release();
}

void Player::ClearAllPendingUpdates()
{
	_bufferS.Acquire();

	bProcessPending = false;
	bMoveProcessPending = false;
	bRemoveProcessPending = false;
	bCreateProcessPending = false;

	DEL_PBC_W(m_pbcMovmentDataBuffer);
	m_pbcMovmentDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002004");
	DEL_PBC_W(m_pbcCreationDataBuffer);
	m_pbcCreationDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002008");
	DEL_PBC_W(m_pbcRemoveDataBuffer);
	m_pbcRemoveDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002010");
	m_pbcRemoveDataCounter = 0;
	DEL_PBC_W(m_pbcPropUpdateDataBuffer);
	m_pbcPropUpdateDataBuffer = sPbcRegister.Make_pbc_wmessage("UserProto002012");

	_bufferS.Release();
}

