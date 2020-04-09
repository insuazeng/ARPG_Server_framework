#include "stdafx.h"
#include "Partner.h"

UpdateMask Partner::m_visibleUpdateMask;

void Partner::InitVisibleUpdateBits()
{
	Partner::m_visibleUpdateMask.SetCount(PARTNER_FIELD_END);

	Partner::m_visibleUpdateMask.SetBit(OBJECT_FIELD_GUID);
	Partner::m_visibleUpdateMask.SetBit(OBJECT_FIELD_TYPE);
	Partner::m_visibleUpdateMask.SetBit(OBJECT_FIELD_PROTO);

	Partner::m_visibleUpdateMask.SetBit(PARTNER_FIELD_OWNER);
	Partner::m_visibleUpdateMask.SetBit(PARTNER_FIELD_WEAPON);
	Partner::m_visibleUpdateMask.SetBit(PARTNER_FIELD_FASHION);
	Partner::m_visibleUpdateMask.SetBit(PARTNER_FIELD_WING);
	Partner::m_visibleUpdateMask.SetBit(PARTNER_FIELD_MOUNT);
}

Partner::Partner(void) : m_owner(NULL)
{
	m_valuesCount = PARTNER_FIELD_END;
	m_uint64Values = _fields;

	memset(m_uint64Values, 0, (PARTNER_FIELD_END) * sizeof(uint64));
	m_updateMask.SetCount(PARTNER_FIELD_END);
}

Partner::~Partner(void)
{
	m_owner = NULL;
}

void Partner::Initialize(uint64 guid, uint32 protoID, Player *owner)
{
	m_valuesCount = PARTNER_FIELD_END;
	m_uint64Values = _fields;

	memset(m_uint64Values, 0, (PARTNER_FIELD_END) * sizeof(uint64));
	m_updateMask.SetCount(PARTNER_FIELD_END);

	m_owner = owner;

	SetUInt64Value(OBJECT_FIELD_GUID, guid);
	SetUInt64Value(OBJECT_FIELD_TYPE, TYPEID_PARTNER);
	SetUInt64Value(OBJECT_FIELD_PROTO, protoID);

	SetUInt64Value(PARTNER_FIELD_OWNER, m_owner->GetGUID());
	// 默认时装为1
	SetUInt64Value(PARTNER_FIELD_FASHION, 1);

	Init();
}

void Partner::Term()
{
	Object::Term();
	m_owner = NULL;
}

void Partner::OnValueFieldChange(const uint32 index)
{
	if (m_owner == NULL || !m_owner->IsInWorld())
		return ;

	if(Partner::m_visibleUpdateMask.GetBit(index))
	{
		m_updateMask.SetBit( index );
		SetUpdatedFlag();
	}
}

void Partner::SetUpdatedFlag()
{
	// 对于伙伴来说,它的属性更新依赖于主人的属性更新,自己不用放入mapmgr的更新队列
	if (m_owner == NULL || !m_owner->IsInWorld())
		return ;

	if (!m_objectUpdated)
	{
		m_owner->SetUpdatedFlag();
		m_objectUpdated = true;
	}
}

