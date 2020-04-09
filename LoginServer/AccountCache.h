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

#ifndef __ACCOUNTCACHE_H
#define __ACCOUNTCACHE_H

typedef struct
{
	string strAccountName;		// 账号名
	string strLoginKey;			// 登陆key（内网不检测该key，外网一般由平台传入）
	string strPlatformAgentName;// 平台代理名
	uint32 nLoginCount;			// 登陆次数
	uint8 nBannedFlag;			// 封禁标记
	uint32 nGMFlags;			// gm标志
} Account;

class AccountMgr : public MongoTable, public Singleton < AccountMgr >
{
#ifdef _WIN64
    typedef HM_NAMESPACE::hash_map< std::string, Account > AccountMap;
#else
    typedef std::map< std::string, Account > AccountMap;
#endif

public:
	AccountMgr();
	virtual ~AccountMgr();

public:
	inline uint32 GetAccountCount() 
	{ 
		return AccountDatabase.size(); 
	}

	// void AddAccount(Field* field);
	void AddAccount(BSONObj &bsonObj);

	inline Account* GetAccountInfo(string& Name)
    {
        Account *pAccount = NULL;
		setBusy.Acquire();
		// this should already be uppercase!
        AccountMap::iterator itr = AccountDatabase.find( Name );
		if ( itr != AccountDatabase.end() )
        {
		    pAccount = &(itr->second);
        }
		setBusy.Release();
		return pAccount;
	}

	void InitConnectorData(MongoConnector *conn);

	bool LoadAccount(string Name);

	void AddNewAccount(string accname, int from = 0);

	void ReloadAccounts(bool silent);

	bool IsIpBanned(const char* strAccountIp);

	bool IsAccountBanned(string& accName);

	void RefreshIPBannedList();

	void UpdateLoginData(string accountName, const char* curIP);

protected:
	virtual void EnsureAllIndex();

private:
    AccountMap AccountDatabase;
	uint32 m_lastRefreshTime;
	
	Mutex m_iplistLock;
	std::map<uint32,uint32>	m_ipForbidden;				// ip封禁控制

protected:
	Mutex setBusy;
	Mutex accountMutex;
};

#define sAccountMgr AccountMgr::GetSingleton()

#endif
