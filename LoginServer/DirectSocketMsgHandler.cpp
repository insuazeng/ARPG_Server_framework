#include "stdafx.h"

#include "ServerCommonDef.h"
#include "DirectSocket.h"
#include "AccountCache.h"
#include "InformationCore.h"
#include "md5.h"

#define SEND_LOGIN_ERR_RET(errCode)		\
	WorldPacket packet(SMSG_001002, 4);	\
	packet.SetErrCode(errCode);	\
	SendPacket(&packet);	\
	return ; \


void DirectSocket::HandleLogin(WorldPacket& packet)
{
	string accountName;
	int32 parserErr = 0;
	pbc_rmessage *msg1001 = sPbcRegister.Make_pbc_rmessage("UserProto001001", (void*)packet.contents(), packet.size());
	if (msg1001 == NULL)
		return ;

	accountName = pbc_rmessage_string(msg1001, "accountName", 0, NULL);

	sLog.outString("[D_Socket] %p 请求登陆,账号=%s", this, accountName.c_str());
	if (accountName.length() == 0)
	{
		SEND_LOGIN_ERR_RET(LOGIN_ERROE_WRONG_NAME);
	}

	Account *pAccount = sAccountMgr.GetAccountInfo(accountName);
	if (pAccount == NULL)
	{
		// 需要创建新角色，检测角色名长度及屏蔽字
		sAccountMgr.AddNewAccount(accountName);
	}

	pAccount = sAccountMgr.GetAccountInfo(accountName);
	ASSERT(pAccount != NULL);

	if(sAccountMgr.IsIpBanned(GetIP()))
	{
		sLog.outError("[D_Socket]帐号 %s ip(%s) 被封禁,无法登陆", accountName.c_str(), GetIP());
		SEND_LOGIN_ERR_RET(LOGIN_ERROR_IPFORBID);
	}
	if (sAccountMgr.IsAccountBanned(accountName))
	{
		sLog.outError("[D_Socket]帐号 %s 被封禁,无法登陆", accountName.c_str());
		SEND_LOGIN_ERR_RET(LOGIN_ERROR_ACCOUNT_FORBID);
	}

	std::string md5string = sInfoCore.PHP_SECURITY;
	md5string += accountName;
	MD5 md5;
	md5.update(md5string);
	string sessionkey = md5.toString();

	sInfoCore.SetSessionKey(accountName, sessionkey, (time(NULL) + 3));		// 3秒内登陆有效
	string gatewayAddress;
	uint32 gatewayPort;
	sInfoCore.FillBestGatewayInfo(gatewayAddress, gatewayPort);
	/*const char* szAddress = gatwayAddress.c_str();
	sLog.outDebug("return gateway length=%u, content =", gatwayAddress.length());
	for (int i = 0; i < gatwayAddress.length(); ++i)
	{
	sLog.outDebug("%c", gatwayAddress[i]);
	}*/

	pbc_wmessage *msg1002 = sPbcRegister.Make_pbc_wmessage("UserProto001002");
	pbc_wmessage_string(msg1002, "gateServerIP", gatewayAddress.c_str(), 0);
	pbc_wmessage_integer(msg1002, "gateServerPort", gatewayPort, 0);
	pbc_wmessage_string(msg1002, "gateServerAuthKey", sessionkey.c_str(), 0);

	WorldPacket pck(SMSG_001002, 256);
	sPbcRegister.EncodePbcDataToPacket(pck, msg1002);
	SendPacket(&pck);

	sLog.outString("[D_Socket]玩家 %s 登录成功,key=%s,将登陆网关%s:%u", accountName.c_str(), sessionkey.c_str(), gatewayAddress.c_str(), gatewayPort);

	// 更新数据库中的上线时间
	sAccountMgr.UpdateLoginData(accountName, GetIP());

	DEL_PBC_R(msg1001);
	DEL_PBC_W(msg1002);
}

void DirectSocket::HandleAccountAutoRegister(WorldPacket &packet)
{
	/*string key;
	packet >> key;
	if (key != "arpg2016AutoRegKey")
	return ;

	char szAccountName[256] = { 0 };
	string strAccName;
	uint32 generateID = 0;
	uint32 accID = 0;
	Account *accInfo = NULL;

	do 
	{
	snprintf(szAccountName, 256, "arpg%.5u", ++generateID);
	strAccName = szAccountName;
	accInfo = sAccountMgr.GetAccountInfo(strAccName);
	} while (accInfo != NULL);

	sLog.outString("[D_Socket]准备自动注册账号 %s (ip=%s)", szAccountName, (char*)inet_ntoa(m_peer.sin_addr));

	sAccountMgr.AddNewAccount(strAccName);

	WorldPacket pck(LSMSG_AUTO_REGISTER, 128);
	pck << szAccountName;
	SendPacket(&pck);*/
}
