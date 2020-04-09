/****************************************************************************
*
* General Object Type File
* Copyright (c) 2007 Team Ascent
*
* This work is licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 3.0
* License. To view a copy of this license, visit
* http://creativecommons.org/licenses/by-nc-sa/3.0/ or send a letter to Creative Commons,
* 543 Howard Street, 5th Floor, San Francisco, California, 94105, USA.
*
* EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU
* ON ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES
* ARISING OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGES.
*
*/

#include "stdafx.h"
#include "AccountCache.h"
#include "LogonCommServer.h"
#include "BufferPool.h"

AccountMgr::AccountMgr(): m_lastRefreshTime(0)
{
}

AccountMgr::~AccountMgr()
{
}

void AccountMgr::ReloadAccounts(bool silent)
{
	setBusy.Acquire();
	if(!silent) sLog.outString("[AccountMgr] Reloading Accounts...");
	DBClientConnection *conn = GetClientConnection();
	string AccountName;
	set<string> account_list;
	Account *acct = NULL;

#ifdef _WIN64
	ASSERT(conn != NULL);
#else
	if (conn == NULL)
		return ;
#endif

	BEGIN_MONGO_CATCH
		auto_ptr<DBClientCursor> cursor = conn->query(sDBConfLoader.GetMongoTableName("accounts").c_str());
		while(cursor->more())
		{
			BSONObj obj = cursor->next();
			JUDGE_CONTINUE(obj.isEmpty() == false);

			AccountName = obj["account_name"].str();
			JUDGE_CONTINUE(AccountName.length() > 0);

			acct = GetAccountInfo(AccountName);
			if (acct == NULL)
			{
				AddAccount(obj);
			}
			else
			{

			}

			account_list.insert(AccountName);
		}
	END_MONGO_CATCH
	
	cLog.Notice("Account","%d Account Loaded:", account_list.size());

	// check for any purged/deleted accounts
	AccountMap::iterator itr = AccountDatabase.begin();
	AccountMap::iterator it2;
	for ( ; itr != AccountDatabase.end(); )
	{
		it2 = itr;
		++itr;

		if ( account_list.find(it2->first) == account_list.end() )
			AccountDatabase.erase(it2);
	}

	if(!silent) sLog.outString("[AccountMgr] Found %u account.", AccountDatabase.size());
	setBusy.Release();
}

/*void AccountMgr::AddAccount(Field* field)
{
	Account acct;
	acct.strAccountName		= field[0].GetString();
	acct.strLoginKey		= field[1].GetString();
	acct.strPlatformAgentName= field[2].GetString();
	acct.nLoginCount		= field[3].GetUInt32();
	acct.nBannedFlag		= field[4].GetUInt16();
	acct.nGMFlags			= field[5].GetUInt32();
	
	// Convert username/password to uppercase. this is needed ;)
	//transform(acct.Username.begin(), acct.Username.end(), acct.Username.begin(), towupper);
	transform(acct.strLoginKey.begin(), acct.strLoginKey.end(), acct.strLoginKey.begin(), towupper);

	setBusy.Acquire();
	AccountDatabase[acct.strAccountName] = acct;
	setBusy.Release();
}*/

void AccountMgr::AddAccount(BSONObj &bsonObj)
{
	Account acct;
	acct.strAccountName			= bsonObj["account_name"].str();
	acct.strLoginKey			= bsonObj["login_key"].str();
	acct.strPlatformAgentName	= bsonObj["platform_name"].str();
	acct.nLoginCount			= bsonObj["login_count"].numberInt();
	
	if (bsonObj.hasElement("banned"))
		acct.nBannedFlag = bsonObj["banned"].numberInt();
	else
		acct.nBannedFlag = 0;

	if (bsonObj.hasElement("gm_flags"))
		acct.nGMFlags = bsonObj["gm_flags"].numberInt();
	else
		acct.nGMFlags = 0;

	// Convert username/password to uppercase. this is needed ;)
	//transform(acct.Username.begin(), acct.Username.end(), acct.Username.begin(), towupper);
	transform(acct.strLoginKey.begin(), acct.strLoginKey.end(), acct.strLoginKey.begin(), towupper);

	setBusy.Acquire();
	AccountDatabase[acct.strAccountName] = acct;
	setBusy.Release();
}

void AccountMgr::InitConnectorData(MongoConnector *conn)
{
	SetConnection(conn);
	EnsureAllIndex();
}

bool AccountMgr::LoadAccount(string Name)
{
	BEGIN_MONGO_CATCH
		BSONObjBuilder b;
		b.append("account_name", Name);
		BSONObj bsonObj = GetClientConnection()->findOne(sDBConfLoader.GetMongoTableName("accounts").c_str(), b.obj());
		if (bsonObj.isEmpty())
			return false;
		AddAccount(bsonObj);
	END_MONGO_CATCH

	return true;

}

void AccountMgr::AddNewAccount(string accname, int from)
{
	Account acct;
	acct.strAccountName		= accname;
	acct.strLoginKey		= "123456";
	acct.nGMFlags			= 0;
	acct.nLoginCount		= 0;

	transform(acct.strLoginKey.begin(), acct.strLoginKey.end(), acct.strLoginKey.begin(), towupper);

	setBusy.Acquire();
	AccountDatabase[acct.strAccountName] = acct;
	setBusy.Release();
	
	BSONObjBuilder b;
	b.append("account_name", accname);
	b.append("login_key", acct.strLoginKey);
	b.append("platform_name", "test");
	b.appendIntOrLL("login_count", 0);
	b << "reg_time" <<  mongo::DATENOW;

	GetClientConnection()->insert(sDBConfLoader.GetMongoTableName("accounts").c_str(), b.obj());
}

//ip封禁刷新时间，单位秒
#define IP_BAN_REFRESHTIME 5
bool AccountMgr::IsIpBanned(const char* strAccountIp)
{	
	uint32 ip = (uint32)inet_addr(strAccountIp);
	uint32 forbidtime = 0;
	m_iplistLock.Acquire();
	std::map<uint32,uint32>::iterator itr = m_ipForbidden.find(ip);
	if(itr!=m_ipForbidden.end())
	{
		forbidtime = itr->second;
	}
	m_iplistLock.Release();
	return forbidtime>UNIXTIME;

	//QueryResult * result = Database_Log->Query("Select forbid_result From game_forbid_ip Where forbid_ip = '%s' And forbid_state = 1 And end_time>NOW()",strAccountIp);
	//if(result != NULL)
	//{
	//	delete result;
	//	return true;
	//}
	//return false;
}

bool AccountMgr::IsAccountBanned(string& accName)
{
	return false;
}

void AccountMgr::RefreshIPBannedList()
{
	if(UNIXTIME<m_lastRefreshTime+5)
		return;

	QueryResult * presult = NULL;
	//0表示已经失效的封禁。1表示正在封禁中的，2表示取消封禁的
	if(m_lastRefreshTime==0)
	{
		presult = Database_Log->Query("Select forbid_ip,forbid_state,end_time From game_forbid_ip Where forbid_state = 1 And end_time>NOW()");
	}
	else
	{
		//查找新增的封禁和取消的封禁
		std::string lasttime = ConvertTimeStampToMYSQL(m_lastRefreshTime);
		presult = Database_Log->Query("Select forbid_ip,forbid_state,end_time From game_forbid_ip Where forbid_state = 1 AND processing_time>'%s' OR forbid_state = 2;",lasttime.c_str());
	}
	m_lastRefreshTime = UNIXTIME;
	if(presult!=NULL)
	{
		uint32 ip = 0;
		uint32 end_time = 0;
		uint32 forbid_state = 0;
		Field * field = NULL;
		m_iplistLock.Acquire();
		do 
		{
			field = presult->Fetch();
			forbid_state = field[1].GetUInt32();
			if(forbid_state==1)
			{
				std::string strtemp = field[2].GetString();
				end_time = convertMYSQLTimeToUNIXTIME(strtemp);
				ip = (uint32)inet_addr(field[0].GetString());
				m_ipForbidden[ip] = end_time;
			}
			else
			{
				ip = (uint32)inet_addr(field[0].GetString());
				std::map<uint32,uint32>::iterator itr = m_ipForbidden.find(ip);
				if(itr!=m_ipForbidden.end())
				{
					m_ipForbidden.erase(itr);
				}
			}
		} while (presult->NextRow());
		m_iplistLock.Release();
		delete presult;
		Database_Log->Execute("Update game_forbid_ip Set forbid_state = 0 Where forbid_state = 1 And end_time<NOW() or forbid_state = 2");
		sLog.outString("IP 封禁记录刷新，共%u条。",(uint32)m_ipForbidden.size());
	}
}

void AccountMgr::UpdateLoginData(string accountName, const char* curIP)
{
	GetClientConnection()->update(sDBConfLoader.GetMongoTableName("accounts").c_str(), mongo::Query(BSON("account_name" << accountName)),
				BSON("$set" << BSON("last_login_time" << mongo::DATENOW << "last_ip" << curIP) << "$inc" << BSON("login_count" << 1)));
}

void AccountMgr::EnsureAllIndex()
{

}

