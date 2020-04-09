// MainServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "WorldRunnable.h"
#include "UpdateThread.h"
#include "MasterCommServer.h"
#include "GateCommHandler.h"
#include "PlayerServant.h"
#include "Park.h"
#include <signal.h>

MongoConnector* MongoDB_Game = NULL;
// MySQLDatabase* Database_Game = NULL;
MySQLDatabase* Database_Log = NULL;
// MySQLDatabase* Database_World = NULL;
MasterScript* g_pMasterScript = NULL;

#ifdef _WIN64
#include "vld/vld.h"
TextLogger * Crash_Log = NULL;
#endif

bool m_stopEvent = false;
bool m_ShutdownEvent = false;
uint32 m_ShutdownTimer = 30000;

int g_serverDaylightBias;
int g_serverTimeOffset;//时差

const uint64 ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF;
const uint32 ILLEGAL_DATA_32 = 0xFFFFFFFF;

void CheckTimeZone()
{
#ifdef _WIN64
	TIME_ZONE_INFORMATION   tzi; 
	GetSystemTime(&tzi.StandardDate); 
	GetTimeZoneInformation(&tzi); 
	setlocale(LC_CTYPE, "chs");
	wprintf( L"%s:时区%d,%s:时差%d小时 \n",tzi.StandardName,tzi.Bias /-60,tzi.DaylightName,tzi.DaylightBias   /   -60); 
	g_serverDaylightBias = tzi.DaylightBias;
#else
	g_serverDaylightBias = 0;
#endif
	time_t timeNow = time(NULL);   
	struct tm* pp = gmtime(&timeNow);   
	time_t timeNow2 = mktime(pp);   
	g_serverTimeOffset = (timeNow-timeNow2)/60;
	cLog.Notice("TimeZone", "Current TimeOffset = %d min", g_serverTimeOffset);
}

#ifdef _WIN64
Mutex m_crashedMutex;

void OnCrash(bool Terminate)
{
	sLog.outString( "Advanced crash handler initialized." );

	if( !m_crashedMutex.AttemptAcquire() )
		TerminateThread( GetCurrentThread(), 0 );

	try
	{
	}
	catch(...)
	{
		sLog.outString( "Threw an exception while attempting to save all data." );
	}

	sLog.outString( "Closing." );

	// beep
	//printf("\x7");

	// Terminate Entire Application
	if( Terminate )
	{
		HANDLE pH = OpenProcess( PROCESS_TERMINATE, TRUE, GetCurrentProcessId() );
		TerminateProcess( pH, 1 );
		CloseHandle( pH );
	}
}
#endif

bool _StartDB()
{
	//return true;//暂时不连接数据库
	string hostname, username, password, database;
	int port = 0;
	int type = 1;
	bool result = false;
	uint32 server_id = Config.GlobalConfig.GetIntDefault("Global", "ServerID", 0);
	int connectcount = 1;
	std::string charset;

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

	MongoDB_Game = new MongoConnector();
	if (MongoDB_Game == NULL)
		return false;

	int ret = MongoDB_Game->ConnectMongoDB(hostname.c_str(), port);
	if (ret == 0)
	{
		cLog.Success("Database", "MongoDB Connections established.");
	}
	else
	{
		cLog.Error("Database", "One or more errors occured while connecting to MongoDB..");
		SAFE_DELETE(MongoDB_Game);
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
	int connectcount = Config.GlobalConfig.GetIntDefault("GameDatabase", "ConnectionCount", 3);
	std::string charset = Config.GlobalConfig.GetStringDefault("GameDatabase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: One or more parameters were missing from WorldDatabase directive.");
		return false;
	}

	Database_Game = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());
	if(!GameDatabase.Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),
		password.c_str(), database.c_str(), connectcount,16384,charset.c_str()))
	{
		sLog.outError("sql: Main database initialization failed. Exiting.");
		DestroyDatabaseInterface(Database_Game);
		Database_Game=NULL;
		return false;
	}*/
	//静态数据库
	/*username = Config.GlobalConfig.GetStringDefault("WorldDatabase", "Username","root");
	password = Config.GlobalConfig.GetStringDefault("WorldDatabase", "Password","root");
	hostname =  Config.GlobalConfig.GetStringDefault("WorldDatabase", "Hostname","127.0.0.1");
	database =  Config.GlobalConfig.GetStringDefault("WorldDatabase", "Name", "arpg2016world");
	port =  Config.GlobalConfig.GetIntDefault("WorldDatabase", "Port", 3306);
	type =  Config.GlobalConfig.GetIntDefault("WorldDatabase", "Type", 1);
	Database_World = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());
	charset = Config.GlobalConfig.GetStringDefault("WorldDatabase", "CharSet", "UTF8");

	if(!Database_World->Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),
		password.c_str(), database.c_str(),1,16384,charset.c_str()))
	{
		sLog.outError("sql: World database initialization failed. Exiting.");
		DestroyDatabaseInterface(Database_World);

		GameDatabase.Shutdown();
		DestroyDatabaseInterface(Database_Game);

		Database_World = NULL;
		Database_Game = NULL;
		return false;
	}
	if(charset=="utf8")
		LanguageMgr::GetSingleton().UseUTF8();*/

	//hohogamelog数据库
	result = Config.GlobalConfig.GetString("LogDataBase", "Username", &username);
	Config.GlobalConfig.GetString("LogDataBase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("LogDataBase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("LogDataBase", "Type", &type);
	connectcount = Config.GlobalConfig.GetIntDefault("LogDataBase", "ConnectionCount", 3);
	charset = Config.GlobalConfig.GetStringDefault("LogDataBase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: One or more parameters were missing from GameLogDB directive.");
		SAFE_DELETE(MongoDB_Game);
		mongo::client::shutdown();
		return false;
	}

	Database_Log = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());

	if(!LogDatabase.Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),
		password.c_str(), database.c_str(), connectcount, 16384, charset.c_str()))
	{
		sLog.outError("sql: Log database initialization failed. Exiting.");
		DestroyDatabaseInterface(Database_Log);
		Database_Log = NULL;

		/*GameDatabase.Shutdown();
		DestroyDatabaseInterface(Database_Game);
		Database_Game = NULL;*/

		SAFE_DELETE(MongoDB_Game);
		mongo::client::shutdown();

		return false;
	}
	return true;
}

void _StopDB()
{
	sLog.outString("Stoping MySQL..");

	/*GameDatabase.Shutdown();
	DestroyDatabaseInterface(Database_Game);*/

	LogDatabase.Shutdown();
	DestroyDatabaseInterface(Database_Log);

	sLog.outString("Stoping MongoDB.. ");
	SAFE_DELETE(MongoDB_Game);
	mongo::client::shutdown();

	/*WorldDatabase.Shutdown();
	DestroyDatabaseInterface(Database_World);*/
}

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

#ifdef _WIN64
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#else
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#endif
bool _LoadConfig()
{
	if(Config.GlobalConfig.SetSource(global_config_file))
	{
		cLog.Success("Config", ">> ../arpg2016_config/mainconfig.conf");
	}
	else
	{
		cLog.Error("Config", ">> ../arpg2016_config/mainconfig.conf");
		return false;
	}
	sDBConfLoader.SetStartParm("playerlimit",Config.GlobalConfig.GetStringDefault("CenterServerSetting","PlayerLimit","3000"));
	sDBConfLoader.SetStartParm("playertimelimit",Config.GlobalConfig.GetStringDefault("CenterServerSetting","PlayTimeLimit","0"));
	sDBConfLoader.SetStartParm("adjustpriority","0");
	sDBConfLoader.SetStartParm("floodlines",Config.GlobalConfig.GetStringDefault("FloodProtection","Lines","4"));
	sDBConfLoader.SetStartParm("floodseconds",Config.GlobalConfig.GetStringDefault("FloodProtection","Seconds","10"));
	sDBConfLoader.SetStartParm("sendmessage",Config.GlobalConfig.GetStringDefault("FloodProtection","SendMessage","1"));
	sDBConfLoader.SetStartParm("screenloglv",Config.GlobalConfig.GetStringDefault("MasterLogLevel","Screen","3"));
	sDBConfLoader.SetStartParm("fileloglv",Config.GlobalConfig.GetStringDefault("MasterLogLevel","File","1"));
	sDBConfLoader.SetStartParm("port",Config.GlobalConfig.GetStringDefault("MasterServer","ListenPort","6500"));
	sDBConfLoader.SetStartParm("listenhost",Config.GlobalConfig.GetStringDefault("MasterServer","ListenHost","0.0.0.0"));
	sDBConfLoader.SetStartParm("consolename",Config.GlobalConfig.GetStringDefault("MasterServer","Name","中央服务器"));

	return true;
}

int RunCenter(int argc, char* argv[])
{
#ifndef _WIN64
	SetEXEPathAsWorkDir();
#endif

	CheckTimeZone();
	//取配置文件
	if(!_LoadConfig())
		return 0;

	if(!sDBConfLoader.LoadStartParam(argc,argv))
	{
		return 0;
	}

#ifdef _WIN64
	::SetConsoleTitle(sDBConfLoader.GetStartParm("consolename"));
#endif // _WIN64

	UNIXTIME = time(NULL);
	UNIXMSTIME = getMSTime64();

	srand(time(NULL));
	//文本记录的线程启动
	launch_thread(new TextLoggerThread);

	cLog.Notice("Startup", "==========================================================");
	cLog.Notice("Startup", "The Master Server");
	cLog.Notice("Startup", "==========================================================");
	cLog.Line();

	//程序崩溃的log创建,遇到无法修复的问题用此log记录
	cLog.Notice("Hint", "The key combination <Ctrl-C> will safely shut down the server at any time.");
	cLog.Line();
	
	sLog.Init(sDBConfLoader.GetStartParmInt("fileloglv"), sDBConfLoader.GetStartParmInt("screenloglv"),"CenterSLog");

	cLog.Notice("Log", "Initializing File Loggers...");
	InitRandomNumberGenerators();
	cLog.Success( "Rnd", "Initialized Random Number Generators." );

#ifdef _WIN64
	Crash_Log = new TextLogger("logs", "CenterCrashLog", false);
#endif

#ifdef _WIN64
	// 用作lua调试用
	sLog.outDebug("Press any key to continue or open the LuaDebugger to attach this process..");
	getchar();
#endif

	//启动数据库
	if(!_StartDB())
	{
		cLog.Error("Database", "Start DB Error");
		return 0;
	}
	cLog.Notice("MasterServer", "Init Success");
	
	g_worldPacketBufferPool.Init();
	g_sqlExecuteBufferPool.Init();

	// 初始park
	cLog.Notice("MasterServer", "Initial Park\n");
	sPark.Rehash();
	sPark.Initialize();

	// 初始化脚本环境
	g_pMasterScript = new MasterScript();
	int32 loadRet = g_pMasterScript->LoadScript("MasterScript/master_main.lua");
	ASSERT(loadRet == 0 && "LuaScript LoadFile failed");

	// sPark.SetStartTime(uint32(time(NULL)));//目前没用到
	// 初始化PlayerServant的消息处理包
	// 以上
	_HookSignals();

	// 创建控制台线程
	cLog.Notice("MasterServer", "Control threading..\n");
	CUpdateThread *update = new CUpdateThread(sThreadMgr.GenerateThreadId());
	UserThread::manual_start_thread(update);

	// 创建网络线程，IOPC创建处理个数相关个线程来管理自己的MessageLoop
	CreateSocketEngine();
	SocketEngine::getSingleton().SpawnThreads();
	cLog.Success("Net", "Thread Spawns\n");

	WorldRunnable *logicThread = new WorldRunnable(sThreadMgr.GenerateThreadId());
	launch_thread(logicThread);

	// 设置网关连接密码
	string gateserver_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "GameServer", "r3m0t3b4d");
	char szPass[20] = { 0 };
	snprintf(szPass, 20, "%s", gateserver_pass.c_str());
	sInfoCore.m_phpLoginKey = Config.GlobalConfig.GetStringDefault("Global", "PHPKey", "请设置密钥");

	SHA_1 sha1;
	sha1.SHA1Reset(&(sInfoCore.m_gatewayAuthSha1Data));
	sha1.SHA1Input(&(sInfoCore.m_gatewayAuthSha1Data), (uint8*)szPass, 20);

	// 监听本地端口
	uint32 sport =sDBConfLoader.GetStartParmInt("port");
	string shost = sDBConfLoader.GetStartParm("listenhost");
	CMasterCommServer::InitPacketHandlerTable();

	//CMasterCommServer的个数完全由与之通信的网关数量决定
	ListenSocket<CMasterCommServer> * s = new ListenSocket<CMasterCommServer>();
	bool listnersockcreate = s->Open(shost.c_str(),sport);
	cLog.Success("MMORPG", "中央服务器 正在监听%s: %d端口..\n", shost.c_str(), sport);

	//update
	uint64 start;
	uint64 diff;
	uint64 last_time = getMSTime64();
	uint64 etime;
	uint64 next_printout = getMSTime64(), next_send = getMSTime64();

	while(!m_stopEvent&&listnersockcreate)
	{
		/* Update global UnixTime variable */
		UNIXTIME = time(NULL);
		UNIXMSTIME = getMSTime64();

		start = getMSTime64();
		diff = start - last_time;

		/* UPDATE */
		last_time = getMSTime64();
		etime = last_time - start;
		if(m_ShutdownEvent)
		{
			if(getMSTime64() >= next_printout)
			{
				if(m_ShutdownTimer > 60000.0f)
				{
					if(!((int)(m_ShutdownTimer)%60000))
						sLog.outString("Server shutdown in %i minutes.", (int)(m_ShutdownTimer / 60000.0f));
				}
				else
					sLog.outString("Server shutdown in %i seconds.", (int)(m_ShutdownTimer / 1000.0f));

				next_printout = getMSTime64() + 2000;
			}
			if(diff >= m_ShutdownTimer)
				m_stopEvent = true;
			else
				m_ShutdownTimer -= diff;
		}

		// Database_Game->CheckConnections();
		Database_Log->CheckConnections();

		if(5 > etime)
			Sleep(5 - etime);
	}

	_UnhookSignals();

	update->SetThreadState(THREADSTATE_TERMINATE);
	ThreadMgr::GetSingleton().RemoveThread(update);
	// 10、关闭中央服务器
	s->Disconnect();
	cLog.Notice("Close", "服务器已关闭..\n");
	Sleep(1000);
	// 12、关闭网络引擎
	SocketEngine::getSingleton().Shutdown();
	Sleep(1500);
	//开始关闭服务器
	time_t st = time(NULL);
	sLog.outString("Server shutdown initiated at %s", ctime(&st));

	// 脚本退出
	g_pMasterScript->LoadScript(NULL);
	SAFE_DELETE(g_pMasterScript);

	//等待逻辑线程处理结束
	sThreadMgr.RemoveThread(logicThread);
	Sleep(100);//等待worldthread进入session处理
	logicThread->SetThreadState(THREADSTATE_TERMINATE);
	while(logicThread->ThreadRunning)
	{
		Sleep(100);
	}
	delete logicThread;

	//删除游戏内容,保存数据
	sPark.SaveAllPlayers();//在数据库线程之前保存所以玩家，理论上这里不会有玩家保存

	//关闭数据库线程
	// kill the database thread first so we don't lose any queries/data
	//((MySQLDatabase*)Database_Game)->SetThreadState(THREADSTATE_TERMINATE);
	//Database_Game->Execute("UPDATE character_dynamic_info SET online_flag = 0");
	//// wait for it to finish its work
	//while(((MySQLDatabase*)Database_Game)->ThreadRunning )
	//{
	//	Sleep(100);
	//}
	//sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Game));

	// 关闭world数据库
//#ifdef _DEBUG
//	for(HM_NAMESPACE::hash_set<std::string>::iterator itr = sLang.m_missingkeys.begin();itr!=sLang.m_missingkeys.end();++itr)
//	{
//		Database_World->Execute("INSERT INTO language VALUES(0,'%s','%s(未设置)','未设置');",(*itr).c_str(),(*itr).c_str());
//	};
//	sLang.m_missingkeys.clear();
//#endif
//
//	((MySQLDatabase*)Database_World)->SetThreadState(THREADSTATE_TERMINATE);
//	Database_World->Execute("UPDATE items SET guid = 1 WHERE guid=0");
//	while(((MySQLDatabase*)Database_World)->ThreadRunning )
//	{
//		Sleep(100);
//	}
//	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_World));

	// 关闭LogDatabase数据库线程
	((MySQLDatabase*)Database_Log)->SetThreadState(THREADSTATE_TERMINATE);
	Database_Log->Execute("UPDATE game_server_cmd SET status = 2 WHERE status = 0");//唤醒线程池

	while(((MySQLDatabase*)Database_Log)->ThreadRunning )
	{
		Sleep(100);
	}
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Log));

	cLog.Notice("MasterServer", "Shutting down random generator.");

	CleanupRandomNumberGenerators();

	sPark.ShutdownClasses();
	delete CCentreCommHandler::GetSingletonPtr();
	sLog.outString("\nDeleting World...");
	delete Park::GetSingletonPtr();
	Sleep(100);
	//在shutdown里面已经删除
	//delete SocketEngine::getSingletonPtr();
	_StopDB();

	//一些单体的删除,不处理也没关系,程序马上结束啦
	printf("delete ThreadMgr::GetSingletonPtr();\n");
	delete ThreadMgr::GetSingletonPtr();
	printf("delete CLog::GetSingletonPtr();\n");
	delete CLog::GetSingletonPtr();
	//这里老是会卡死
	printf("TextLogger::Thread->Die();\n");
	Sleep(1000);
	TextLogger::Thread->Die();//TextLogger的实例会在这里delete
	Sleep(1000);//等待文本写完

	printf("delete oLog;\n");
	delete oLog::GetSingletonPtr();

#ifdef _WIN64
	WSACleanup();
#endif
	
	printf("g_worldPacketBufferPool.Stats()\n");
	g_worldPacketBufferPool.Stats();
	g_worldPacketBufferPool.Destroy();

	printf("g_sqlExecuteBufferPool.Stats()\n");
	g_sqlExecuteBufferPool.Stats();
	g_sqlExecuteBufferPool.Destroy();

	printf("程序安全退出，按任意键结束进程\n");
	return 0;
}

int main(int argc, char** argv)
{
	THREAD_TRY_EXECUTION
	{
		RunCenter(argc, argv);
	}
	THREAD_HANDLE_CRASH
}

