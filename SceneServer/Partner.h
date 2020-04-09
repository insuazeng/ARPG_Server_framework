#ifndef _PARTNER_H_
#define _PARTNER_H_

#include "Object.h"
#include "PartnerDef.h"

class Player;

class Partner : public Object
{
public:
	Partner(void);
	virtual ~Partner(void);
public:
	void Initialize(uint64 guid, uint32 protoID, Player *owner);

public:
	virtual void Term();
	virtual void OnValueFieldChange(const uint32 index);
	virtual void SetUpdatedFlag();

public:
	// static
	static void InitVisibleUpdateBits();
	static UpdateMask m_visibleUpdateMask;

public:
	// µ¼³ö¸ølua
	LUA_EXPORT(Partner)

private:
	uint64 _fields[PARTNER_FIELD_END];
	Player* m_owner;
};

#endif

