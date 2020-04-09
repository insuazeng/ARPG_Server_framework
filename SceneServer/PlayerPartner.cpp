#include "stdafx.h"
#include "Player.h"

Partner* Player::GetPartnerByProto(uint32 protoID)
{
	Partner *ret = NULL;
	PlayerPartnerMap::iterator it = m_Partners.begin();
	for (; it != m_Partners.end(); ++it)
	{
		if (it->second->GetEntry() == protoID)
		{
			ret = it->second;
			break;
		}
	}
	return ret;
}

Partner* Player::GetPartnerByGuid(uint64 partnerGuid)
{
	Partner *ret = NULL;
	
	if (m_Partners.find(partnerGuid) != m_Partners.end())
		ret = m_Partners[partnerGuid];

	return ret;
}
