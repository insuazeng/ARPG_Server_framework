// sharedtest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <signal.h>

//database
#include "threading/UserThreadMgr.h"
#include "config/ConfigEnv.h"
#include "network/Network.h"

//threading
#include "WorldRunnable.h"

//network
#include "LogonCommServer.h"
#include "DirectSocket.h"

//accountmgr
#include "AccountCache.h"

//vld内存泄露测试工具
#ifdef _WIN64
#include "vld/vld.h"
#endif

#ifdef _WIN64
TextLogger * Crash_Log = NULL;
#endif 

WorldRunnable *WorldRun_Thread = NULL;
// MySQLDatabase * Database_Data = NULL;
MySQLDatabase * Database_Log = NULL;
MongoConnector * MongoDB_Data = NULL;

bool m_stopEvent = false;

const uint64 ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF;
const uint32 ILLEGAL_DATA_32 = 0xFFFFFFFF;

void _OnSignal(int s)
{
	switch (s)
	{
#ifndef _WIN64
	case SIGHUP:
		break;
	case SIGPIPE:
		sLog.outError("Error Receive SIGPIPE");
		break;
#endif
	case SIGINT:
	case SIGTERM:
	case SIGABRT:
#ifdef _WIN64
	case SIGBREAK:
#endif
		m_stopEvent = true;
		break;
	}
	signal(s, _OnSignal);
}

void _HookSignals()
{
	signal(SIGINT, _OnSignal);
	signal(SIGTERM, _OnSignal);
	signal(SIGABRT, _OnSignal);
#ifdef _WIN64
	signal(SIGBREAK, _OnSignal);
#else
	signal(SIGHUP, _OnSignal);
	signal(SIGPIPE, _OnSignal);
#endif
}

void _UnhookSignals()
{
	signal(SIGINT, 0);
	signal(SIGTERM, 0);
	signal(SIGABRT, 0);
#ifdef _WIN64
	signal(SIGBREAK, 0);
#else
	signal(SIGHUP, 0);
	signal(SIGPIPE, 0);
#endif
}

void OnCrash(bool Terminate)
{

}

bool _StartDB()
{
	string hostname, username, password, database,charset;
	int port = 0;
	int type = 1;
	bool result = false;

	result = Config.GlobalConfig.GetString("MongoDatabase", "Username", &username);
	Config.GlobalConfig.GetString("MongoDatabase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("MongoDatabase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("MongoDatabase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("MongoDatabase", "Port", &port);

	if(result == false)
	{
		sLog.outError("sql: mainconfig.conf配置文件中缺少MongoDatabase参数.");
		return false;
	}
	
	// 初始化mongo环境
	mongo::client::initialize();

	MongoDB_Data = new MongoConnector();
	if (MongoDB_Data == NULL)
		return false;

	int ret = MongoDB_Data->ConnectMongoDB(hostname.c_str(), port);
	if (ret == 0)
	{
		cLog.Success("Database", "MongoDB Connections established.");
	}
	else
	{
		cLog.Error("Database", "One or more errors occured while connecting to MongoDB..");
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}
	
	// Configure Main Database
	/*result = Config.GlobalConfig.GetString("GameDatabase", "Username", &username);
	Config.GlobalConfig.GetString("GameDatabase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Type", &type);
	charset = Config.GlobalConfig.GetStringDefault("GameDatabase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: mainconfig.conf配置文件中缺少GameDatabase参数.");
		return false;
	}

	Database_Data = new MySQLDatabase(sThreadMgr.GenerateThreadId());

	if(Database_Data->Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),password.c_str(), database.c_str(), 1, 1000, charset.c_str()))
	{
		cLog.Success("Database", "Connections established.");
	}
	else
	{
		cLog.Error("Database", "One or more errors occured while connecting to databases.");
		DestroyDatabaseInterface(Database_Data);//实例销毁
		Database_Data = NULL;
		return false;
	}*/

	//hohogamelog数据库
	result = Config.GlobalConfig.GetString("LogDataBase", "Username", &username);
	Config.GlobalConfig.GetString("LogDataBase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Type", &type);
	charset = Config.GlobalConfig.GetStringDefault("LogDataBase", "CharSet", "UTF8");
	if(result == false)
	{
		sLog.outError("sql: loginserver.conf配置文件中缺少LogDatabase参数.");
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}

	Database_Log = new MySQLDatabase(sThreadMgr.GenerateThreadId());

	if(Database_Log->Initialize(hostname.c_str(), (unsigned int)port, username.c_str(), password.c_str(), database.c_str(), 1, 1000, charset.c_str()))
	{
		cLog.Success("LogDatabase", "Connections established.");
	}
	else
	{
		cLog.Error("LogDatabase", "One or more errors occured while connecting to databases.");
		/*DestroyDatabaseInterface(Database_Data); //实例销毁
		Database_Data = NULL;*/
		DestroyDatabaseInterface(Database_Log);  //实例销毁
		Database_Log  = NULL;
		SAFE_DELETE(MongoDB_Data);
		mongo::client::shutdown();
		return false;
	}

	return true;
}

//ListenSocket<PhpLogonSocket> * g_s1;
ListenSocket<LogonCommServerSocket> * g_s2;
ListenSocket<DirectSocket> * g_s3;

bool _StartListen()
{
	uint32 wport =sInfoCore.GetStartParmInt("webport");
	uint32 sport = sInfoCore.GetStartParmInt("serverport");
	string webhost = Config.GlobalConfig.GetStringDefault("LoginServer", "PHPHost", "127.0.0.1");
	string shost = Config.GlobalConfig.GetStringDefault("LoginServer", "GSHost", "127.0.0.1");
	uint32 authport =sInfoCore.GetStartParmInt("clientport");
	string authhost = Config.GlobalConfig.GetStringDefault("LoginServer", "AuthHost", "0.0.0.0");

	bool listenfailed = false;
	/*g_s1 = new ListenSocket<PhpLogonSocket>();
	if(g_s1->Open(webhost.c_str(),wport))
	sLog.outString("监听PHP连接，IP:%s Port:%d",webhost.c_str(),wport);
	else
	listenfailed = true;*/

	g_s2 = new ListenSocket<LogonCommServerSocket>();
	if(g_s2->Open(shost.c_str(),sport))
		sLog.outString("监听GateServer连接，IP:%s Port:%d",shost.c_str(),sport);
	else
		listenfailed = true;

	g_s3 = new ListenSocket<DirectSocket>();
	if(g_s3->Open(authhost.c_str(), authport))
		sLog.outString("监听客户端验证连接，IP:%s Port:%d",authhost.c_str(),authport);
	else
		listenfailed = true;

	return !listenfailed;
}

void _StopListen()
{
	//g_s1->Disconnect();
	g_s2->Disconnect();
	g_s3->Disconnect();
}

#ifdef _WIN64
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#else
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#endif

bool _LoadConfig()
{
	//取配置文件
	cLog.Notice("Config", "Loading Config Files...\n");
	if(Config.GlobalConfig.SetSource(global_config_file))
	{
		cLog.Success("Config", ">> ../arpg2016_config/mainconfig.conf");
	}
	else
	{
		cLog.Error("Config", ">> ../arpg2016_config/mainconfig.conf");
		return false;
	}

	sInfoCore.SetStartParm("webport", Config.GlobalConfig.GetStringDefault("LoginServer","WebListPort","2445"));
	sInfoCore.SetStartParm("serverport", Config.GlobalConfig.GetStringDefault("LoginServer","ServerPort","8293"));
	sInfoCore.SetStartParm("clientport", Config.GlobalConfig.GetStringDefault("LoginServer","AuthPort","8294"));
	sInfoCore.SetStartParm("consolename", Config.GlobalConfig.GetStringDefault("LoginServer","Name","登录服"));

	return true;
}

int RunLS(int argc, char* argv[])
{
#ifndef _WIN64
	SetEXEPathAsWorkDir();
#endif

	UNIXTIME = time(NULL);
	UNIXMSTIME = getMSTime64();
	launch_thread(new TextLoggerThread);//文本记录的线程启动

#ifdef _WIN64
	Crash_Log = new TextLogger("logs", "logonCrashLog", false);
#endif

	//取配置文件
	if(!_LoadConfig())
		return 0;

	if(!sInfoCore.LoadStartParam(argc,argv))
		return 0;

	sLog.Init(Config.GlobalConfig.GetIntDefault("LoginLogLevel", "File", -1),Config.GlobalConfig.GetIntDefault("LoginLogLevel", "Screen", 1), "LoginServerLog");

	string logon_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "LoginServer", "change_me_logon");
	sInfoCore.PHP_SECURITY = Config.GlobalConfig.GetStringDefault("Global", "PHPKey", "请设置密钥");
	char szLogonPass[20] = { 0 };
	snprintf(szLogonPass, 20, "%s", logon_pass.c_str());
	SHA_1 sha1;
	sha1.SHA1Reset(&(sInfoCore.m_gatewayAuthSha1Data));
	sha1.SHA1Input(&(sInfoCore.m_gatewayAuthSha1Data), (uint8*)szLogonPass, 20);

#ifdef _WIN64
	::SetConsoleTitle(sInfoCore.GetStartParm("consolename"));
#endif

#ifndef EPOLL_TEST
	cLog.Line();
	cLog.Notice("Database", "Connecting to database...");

	//连接数据库
	if(!_StartDB())
		return 0;
#endif

	// 读取sp的配置
	sInfoCore.LoadSpConfig();
	
	// 读取server_setting配置信息
	sDBConfLoader.LoadDBConfig();
	
#ifndef EPOLL_TEST
	// 初始化AccountMgr
	sAccountMgr.InitConnectorData(MongoDB_Data);

	// 读取配置
	sLog.outColor(TNORMAL, " >> precaching accounts...\n");
	sAccountMgr.ReloadAccounts(true);
	sLog.outColor(TNORMAL, " >> finished...\n");
#endif

	// 初始化数据包和sql执行缓冲的池
	g_worldPacketBufferPool.Init();
	g_sqlExecuteBufferPool.Init();

	// 初始化pbc的解析器
	sPbcRegister.Init();
	RegistProtobufConfig(&(sPbcRegister));

	// 初始化处理包的数组
	LogonCommServerSocket::InitLoginServerPacketHandler();

	//network
	CreateSocketEngine();
	SocketEngine::getSingleton().SpawnThreads();
	WorldRun_Thread = new WorldRunnable(sThreadMgr.GenerateThreadId());
	launch_thread(WorldRun_Thread);

	bool bListSucess = _StartListen();
	if(!bListSucess)
		return 0;

	cLog.Line();
	//查寻方法
	sLog.outColor(TNORMAL, " >> Netword Started...\n");

#ifdef _WIN64
	_HookSignals();
	while(!m_stopEvent)
	{
		int input = getchar();
		switch(input)
		{
		case 's':
			printf(sThreadMgr.ShowStatus().c_str());
			break;
		case 'r':
			sAccountMgr.ReloadAccounts(true);
			break;
		}
		Sleep(1000);
	}
	_UnhookSignals();

#else
	_HookSignals();
	while(!m_stopEvent)//退出
	{
		Sleep(1000);
	}
	_UnhookSignals();
#endif

	_StopListen();

#ifndef EPOLL_TEST

	// 关闭mongo数据库
	SAFE_DELETE(MongoDB_Data);
	mongo::client::shutdown();

	//关闭data数据库
	/*((MySQLDatabase*)Database_Data)->SetThreadState(THREADSTATE_TERMINATE);
	// send a query to wake up condition//需要激活线程才能关闭
	((MySQLDatabase*)Database_Data)->Execute("SELECT count(account_name) from accounts");
	cLog.Notice("Database","Waiting for database to close..");
	while(((MySQLDatabase*)Database_Data)->ThreadRunning)
		Sleep(100);
	cLog.Success("Database","database has been shutdown");
	DestroyDatabaseInterface(Database_Data);//实例销毁
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Data));*/

	//关闭log数据库
	((MySQLDatabase*)Database_Log)->SetThreadState(THREADSTATE_TERMINATE);
	Database_Log->Execute("Update game_forbid_ip Set forbid_state = 0 Where forbid_state = 1 And end_time<NOW() or forbid_state = 2");
	cLog.Notice("LogDatabase","Waiting for database to close..");
	while(((MySQLDatabase*)Database_Log)->ThreadRunning)
		Sleep(100);

	cLog.Success("LogDatabase","database has been shutdown");
	DestroyDatabaseInterface(Database_Log);
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Log));
#endif

	WorldRun_Thread->SetThreadState(THREADSTATE_TERMINATE);
	Sleep(100);

	sThreadMgr.RemoveThread(WorldRun_Thread);
	sThreadMgr.Shutdown();
	Sleep(500);

	printf("Network Engine Shutting down..\n");
	SocketEngine::getSingleton().Shutdown();

	printf("delete AccountMgr ..\n");
	delete AccountMgr::GetSingletonPtr();

	printf("delete ThreadMgr ..\n");
	delete ThreadMgr::GetSingletonPtr();

	printf("delete pbcRegister ..\n");
	delete PbcDataRegister::GetSingletonPtr();

	printf("delete DBConfigLoader ..\n");
	delete DBConfigLoader::GetSingletonPtr();

	delete InformationCore::GetSingletonPtr();

	printf("delete CLog\n");
	delete CLog::GetSingletonPtr();

	printf("TextLogger::Thread->Die()\n");
	TextLogger::Thread->Die();
	Sleep(100);

	printf("delete oLog\n");
	delete oLog::GetSingletonPtr();
	
#ifdef _WIN64
	WSACleanup();
#endif

	printf("g_worldPacketBufferPool.Stats\n");
	g_worldPacketBufferPool.Stats();
	g_worldPacketBufferPool.Destroy();

	printf("g_sqlExecuteBufferPool.Stats\n");
	g_sqlExecuteBufferPool.Stats();
	g_sqlExecuteBufferPool.Destroy();

	printf("程序安全退出，按任意键结束进程\n");
	return 0;
}

int main(int argc, char** argv)
{
	THREAD_TRY_EXECUTION
	{
		RunLS(argc, argv);
	}
	THREAD_HANDLE_CRASH
}
