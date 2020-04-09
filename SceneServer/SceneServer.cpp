// HohoGame.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "UpdateThread.h"
#include "WorldRunnable.h"
#include "MersenneTwister.h"

//内存泄露检测工具
#ifdef _WIN64
#include "vld/vld.h"
#endif // _WIN64

#include <signal.h>

// MySQLDatabase* Database_Game = NULL;
// MySQLDatabase* Database_World = NULL;	// 静态数据库

MySQLDatabase* Database_Log = NULL;
SceneScript* g_pSceneScript = NULL;

#ifdef _WIN64
TextLogger * Crash_Log;
#endif // _WIN64

bool m_stopEvent = false;
bool m_ShutdownEvent = false;

const uint64 ILLEGAL_DATA_64 = 0xFFFFFFFFFFFFFFFF;
const uint32 ILLEGAL_DATA_32 = 0xFFFFFFFF;

uint32 m_ShutdownTimer = 30000;

#ifdef _WIN64
Mutex m_crashedMutex;
void OnCrash(bool Terminate)
{
	sLog.outString( "Advanced crash handler initialized." );

	if( !m_crashedMutex.AttemptAcquire() )
		TerminateThread( GetCurrentThread(), 0 );

	try
	{
		if( Park::GetSingletonPtr() != 0 )//判断park是否还存在
		{
			sLog.outString( "Waiting for all database queries to finish..." );
			// GameDatabase.Shutdown();
			sLog.outString( "All pending database operations cleared.\n" );
			sPark.SaveAllPlayers();
			sLog.outString( "Data saved." );
		}
	}
	catch(...)
	{
		sLog.outString( "Threw an exception while attempting to save all data." );
	}

	sLog.outString( "Closingw." );

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

	string hostname, username, password, database,charset;
	int port = 0;
	int type = 1;
	int connectcount = 3;
	bool result = false;

	uint32 server_id = Config.GlobalConfig.GetIntDefault("Global", "ServerID", 0);

	// 玩家数据库
	/*result = Config.GlobalConfig.GetString("GameDatabase", "Username", &username);
	Config.GlobalConfig.GetString("GameDatabase", "Password", &password);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Hostname", &hostname);
	result = !result ? result : Config.GlobalConfig.GetString("GameDatabase", "Name", &database);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Port", &port);
	result = !result ? result : Config.GlobalConfig.GetInt("GameDatabase", "Type", &type);
	connectcount = Config.GlobalConfig.GetIntDefault("GameDatabase", "SceneConnectionCount", 1);
	charset = Config.GlobalConfig.GetStringDefault("GameDatabase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: One or more parameters were missing from WorldDatabase directive.");
		return false;
	}

	Database_Game = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());

	// Initialize it
	if(!GameDatabase.Initialize(hostname.c_str(), (unsigned int)port,username.c_str(),password.c_str(),database.c_str(),connectcount,16384,charset.c_str()))
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
	database =  Config.GlobalConfig.GetStringDefault("WorldDatabase", "Name", "ngameworld");
	port =  Config.GlobalConfig.GetIntDefault("WorldDatabase", "Port", 3306);
	type =  Config.GlobalConfig.GetIntDefault("WorldDatabase", "Type", 1);
	Database_World = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());
	charset = Config.GlobalConfig.GetStringDefault("WorldDatabase", "CharSet", "UTF8");

	if(!Database_World->Initialize(hostname.c_str(), (unsigned int)port,username.c_str(),password.c_str(),database.c_str(),1,16384,charset.c_str()))
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
	connectcount = Config.GlobalConfig.GetIntDefault("LogDataBase", "SceneConnectionCount", 1);
	charset = Config.GlobalConfig.GetStringDefault("LogDataBase", "CharSet", "UTF8");

	if(result == false)
	{
		sLog.outError("sql: One or more parameters were missing from GameLogDB directive.");
		return false;
	}

	Database_Log = (MySQLDatabase*)CreateDatabaseInterface((DatabaseType)type, sThreadMgr.GenerateThreadId());

	if(!LogDatabase.Initialize(hostname.c_str(), (unsigned int)port, username.c_str(),password.c_str(), database.c_str(),connectcount,16384,charset.c_str()))
	{
		sLog.outError("sql: Log database initialization failed. Exiting.");
		DestroyDatabaseInterface(Database_Log);

		/*GameDatabase.Shutdown();
		DestroyDatabaseInterface(Database_Game);
		Database_Game = NULL;*/

		/*Database_World->Shutdown();
		DestroyDatabaseInterface(Database_World);
		Database_World = NULL;*/

		Database_Log = NULL;
		return false;
	}

	/**/
	return true;
}
void _StopDB()
{
	/*GameDatabase.Shutdown();
	DestroyDatabaseInterface(Database_Game);*/

	LogDatabase.Shutdown();
	DestroyDatabaseInterface(Database_Log);

	/*WorldDatabase.Shutdown();
	DestroyDatabaseInterface(Database_World);*/
}

void _OnSignal(int s)
{
	switch (s)
	{
#ifndef _WIN64
	case SIGHUP:
		//sWorld.Rehash(true);
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

/*
main执行流程
1.处理启动参数
2.启动log线程
launch_thread(new TextLoggerThread);
sLog.Init(-1, 3);
3.Crash_Log创建
4.随机数种子
5.读取配置文件
6.启动数据库
7.游戏内容初始化
8._HookSignals();
9.启动控制台线程
10.启动socket线程
11.连接logon服务器
12.监听端口
13.while循环更新
14._UnhookSignals();
15.结束控制台线程
16.结束socket线程
17.结束数据库线程
18.删除各种单体
19.关闭数据库
*/

#ifdef _WIN64
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#else
static const char * global_config_file = "../arpg2016_config/mainconfig.conf";
#endif


int g_serverDaylightBias;
int g_serverTimeOffset;//时差

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
	cLog.Notice("Time", "TimeOffset = %d min...\n",g_serverTimeOffset);
}
void _PrintRunStatus()
{

	WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
	if (prun == NULL)
		return ;
	if(prun->totalupdate==0)
		prun->totalupdate=1;
	sLog.outString("average %d; min %d ;max %d\n",prun->totalparkdiff/prun->totalupdate,prun->minparkdiff,prun->maxparkdiff);
	sLog.outString("总收到数据 %d bytes,总发送数据 %d bytes\n",sGateCommHandler.m_totalreceivedata,sGateCommHandler.m_totalsenddata);
	sLog.outString("数据包发送数:%d， 超时发包数:%d，超缓冲区容量发包数:%d\n", sGateCommHandler.m_totalSendPckCount, sGateCommHandler.m_TimeOutSendCount,	sGateCommHandler.m_SizeOutSendCount);
	sPark.PrintDebugLog();

	prun->PrintMemoryUseStatus();
}

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

	sDBConfLoader.SetStartParm("playerlimit",Config.GlobalConfig.GetStringDefault("SceneServerSetting","PlayerLimit","1000"));
	sDBConfLoader.SetStartParm("logcheaters","1");
	sDBConfLoader.SetStartParm("adjustpriority","0");
	sDBConfLoader.SetStartParm("floodlines",Config.GlobalConfig.GetStringDefault("FloodProtection","Lines","4"));
	sDBConfLoader.SetStartParm("floodseconds",Config.GlobalConfig.GetStringDefault("FloodProtection","Seconds","10"));
	sDBConfLoader.SetStartParm("sendmessage",Config.GlobalConfig.GetStringDefault("FloodProtection","SendMessage","1"));
	sDBConfLoader.SetStartParm("screenloglv",Config.GlobalConfig.GetStringDefault("SceneLogLevel","Screen","3"));
	sDBConfLoader.SetStartParm("fileloglv",Config.GlobalConfig.GetStringDefault("SceneLogLevel","File","1"));

	sDBConfLoader.SetStartParm("sid","1");
	char szSceneServer[32] = { 0 };
	snprintf(szSceneServer, 32, "SceneServer%d", sDBConfLoader.GetStartParmInt("sid"));
	sDBConfLoader.SetStartParm("host",Config.GlobalConfig.GetStringDefault(szSceneServer, "Host", "0.0.0.0"));
	sDBConfLoader.SetStartParm("port",Config.GlobalConfig.GetStringDefault(szSceneServer, "ListenPort", "14001"));
	sDBConfLoader.SetStartParm("consolename",Config.GlobalConfig.GetStringDefault(szSceneServer, "Name", "场景s1_14001"));

	return true;
}

int win32_main(int argc, char* argv[])
{
#ifndef _WIN64
	SetEXEPathAsWorkDir();
#endif

	CheckTimeZone();
	/*
	这里处理启动参数一些参数既可以从mainconfig.conf中读取，又可以在启动参数中覆盖。
	使用sDBConfLoader.SetStartParm保存，然后统一用sDBConfLoader的接口获取即可
	*/
	if(!_LoadConfig())
		return 0;

	uint32 ServerIndex = 0;
	uint32 sport = 0;
	string shost;

	if(sDBConfLoader.LoadStartParam(argc,argv))
	{
		ServerIndex = sDBConfLoader.GetStartParmInt("sid");
		char szSceneServer[32] = { 0 };
		snprintf(szSceneServer, 32, "SceneServer%d", sDBConfLoader.GetStartParmInt("sid"));
		sDBConfLoader.SetStartParm("host",Config.GlobalConfig.GetStringDefault(szSceneServer, "Host", "0.0.0.0"));
		sDBConfLoader.SetStartParm("port",Config.GlobalConfig.GetStringDefault(szSceneServer, "ListenPort", "14001"));
		sDBConfLoader.SetStartParm("consolename",Config.GlobalConfig.GetStringDefault(szSceneServer, "Name", "场景服"));

		shost = sDBConfLoader.GetStartParm("host");
		sport = sDBConfLoader.GetStartParmInt("port");
	}
	else
	{
		return 0;
	}	

	if(ServerIndex==0||sport==0||shost.length()<7)
	{
		cLog.Notice("Startup", "Start Param Missing,Process Will Exit!(try start with -sid [id] -host [listenip] -port [listenport])");
		return 0;
	}
	else
	{
		cLog.Notice("Startup", "SceneServerID:%d  ListenIP:%s  Port:%d", ServerIndex, shost.c_str(), sport);
	}

#ifdef _WIN64
	::SetConsoleTitle(sDBConfLoader.GetStartParm("consolename"));
#endif

	UNIXTIME = time(NULL);
	//文本记录的线程启动
	launch_thread(new TextLoggerThread);
	char szLogname[128] = { 0 };
	snprintf(szLogname, 128, "SceneSLog_ID_%d", ServerIndex);

	sLog.Init(sDBConfLoader.GetStartParmInt("fileloglv"), sDBConfLoader.GetStartParmInt("screenloglv"), szLogname);

	cLog.Notice("Startup", "==========================================================");
	cLog.Notice("Startup", "   the Scene Server  ");
	cLog.Notice("Startup", "==========================================================");
	cLog.Line();

	InitRandomNumberGenerators();
	cLog.Success( "Rnd", "Initialized Random Number Generators." );

	//程序崩溃的log创建,遇到无法修复的问题用此log记录
	cLog.Notice("Hint", "The key combination <Ctrl-C> will safely shut down the server at any time.");
	cLog.Line();
	cLog.Notice("Log", "Initializing File Loggers...");
#ifdef _WIN64
	Crash_Log = new TextLogger("logs", "GameCrashLog", false);

	// 用作lua调试用
	sLog.outDebug("Press any key to continue or open the LuaDebugger to attach this process..");
	getchar();
#endif // _WIN64

	//随机数种子
	uint32 seed = time(NULL);

	//启动数据库
	if(!_StartDB())
	{
		return 0;
	}

	//worldbuff池分配
	g_worldPacketBufferPool.Init();
	g_sqlExecuteBufferPool.Init();

	sGateCommHandler.SetSelfSceneServerID(ServerIndex);

	cLog.Notice("Game", "Initial Park\n");
	//游戏内容创建和初始化
	/* load the config file */
	sPark.Rehash(false);

	sPark.SetInitialWorldSettings();
	
	// 初始化脚本
	g_pSceneScript = new SceneScript();
	int loadRet = g_pSceneScript->LoadScript("SceneScript/scene_main.lua");
	ASSERT(loadRet == 0 && "LuaScript LoadFile failed");

	// hooksignals
	_HookSignals();
	
	//创建控制台线程
	cLog.Notice("Game", "Consol threading\n");
	CUpdateThread* update = new CUpdateThread(sThreadMgr.GenerateThreadId());
	UserThread::manual_start_thread(update);

	//创建网络线程
	CreateSocketEngine();
	SocketEngine::getSingleton().SpawnThreads();
	cLog.Success("Net", "thread spawned\n");

	WorldRunnable* worldthread = new WorldRunnable(sThreadMgr.GenerateThreadId());
	launch_thread(worldthread);

	//接收网关服务器连接时的密码
	string gateserver_pass = Config.GlobalConfig.GetStringDefault("RemotePassword", "GameServer", "r3m0t3b4d");
	char szPass[20] = { 0 };
	snprintf(szPass, 20, "%s", gateserver_pass.c_str());
	SHA_1 sha1;
	sha1.SHA1Reset(&(sGateCommHandler.m_gatewayAuthSha1Data));
	sha1.SHA1Input(&(sGateCommHandler.m_gatewayAuthSha1Data), (const uint8*)szPass, 20);

	CSceneCommServer::InitPacketHandlerTable();
	ListenSocket<CSceneCommServer> * s = new ListenSocket<CSceneCommServer>();
	bool listnersockcreate = s->Open(shost.c_str(),sport);
	cLog.Success("网关模式", "场景服务器（索引 %u）正在监听%s:%d端口..\n", sGateCommHandler.m_sceneServerID, shost.c_str(),sport);

	//update
	uint64 start = 0;
	uint32 diff = 0, etime = 0;
	uint64 last_time = getMSTime64();
	uint64 next_printout = last_time, next_send = last_time;

	while(!m_stopEvent&&listnersockcreate)
	{
		/* Update global UnixTime variable */
		UNIXTIME = time(NULL);
		UNIXMSTIME = getMSTime64();

		start = UNIXMSTIME;
		diff = start - last_time;

		sSocketDeleter.Update();

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

		// 每秒update 20帧
		if(50 > etime)
			Sleep(50 - etime);
	}
	_UnhookSignals();

	//输出运行状态
	_PrintRunStatus();
	//结束update控制台
	update->SetThreadState(THREADSTATE_TERMINATE);
	sThreadMgr.RemoveThread(update);//update无法delete
	//结束socket引擎.
	s->Disconnect();
	SocketEngine::getSingleton().Shutdown();
	//开始关闭服务器
	time_t st = time(NULL);
	sLog.outString("Server shutdown initiated at %s", ctime(&st));

	//等待逻辑线程处理结束
	sThreadMgr.RemoveThread(worldthread);
	Sleep(100);//等待worldthread进入session处理
	worldthread->SetThreadState(THREADSTATE_TERMINATE);
	while(worldthread->ThreadRunning)
	{
		Sleep(100);
	}
	delete worldthread;

	//删除游戏内容,保存数据
	sPark.SaveAllPlayers();//在数据库线程之前保存所以玩家，理论上这里不会有玩家保存
	sPark.PrintAllSessionStatusOnProgramExit();

	//在关闭前做最后的统计
	sStatisticsSystem.OnShutDownServer();

//#ifdef _DEBUG
//	for(HM_NAMESPACE::hash_set<std::string>::iterator itr = sLang.m_missingkeys.begin();itr!=sLang.m_missingkeys.end();++itr)
//	{
//		Database_World->Execute("INSERT INTO language VALUES(0,'%s','%s(未设置)','未设置');",(*itr).c_str(),(*itr).c_str());
//	};
//	sLang.m_missingkeys.clear();
//#endif

	//关闭数据库线程,关闭之后所有的执行都是同步的
	//静态数据库
	/*((MySQLDatabase*)Database_World)->SetThreadState(THREADSTATE_TERMINATE);
	Database_World->Execute("UPDATE items SET guid = 1 WHERE guid=0");
	while(((MySQLDatabase*)Database_World)->ThreadRunning )
	{
		Sleep(100);
	}
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_World));*/

	//游戏数据库
	/*((MySQLDatabase*)Database_Game)->SetThreadState(THREADSTATE_TERMINATE);
	Database_Game->Execute("UPDATE character_dynamic_info SET online_flag = 0 WHERE player_guid=0");//只是为了触发线程运行，实际设置不在线，应该在中央服务器关闭的时候
	while(((MySQLDatabase*)Database_Game)->ThreadRunning )
	{
		Sleep(100);
	}
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Game));*/

	//关闭LogDatabase数据库线程
	((MySQLDatabase*)Database_Log)->SetThreadState(THREADSTATE_TERMINATE);
	Database_Log->Execute("SELECT * from game_pay limit 1");//唤醒线程池
	while(((MySQLDatabase*)Database_Log)->ThreadRunning )
	{
		Sleep(100);
	}
	sThreadMgr.RemoveThread(((MySQLDatabase*)Database_Log));

	//线程池测试中,必须在scenemapmgr结束前结束
	ThreadPool.Shutdown();

	cLog.Notice("Server", "Shutting down random generator.");
	CleanupRandomNumberGenerators();

	sPark.ShutdownClasses();
	
	// 脚本退出
	g_pSceneScript->LoadScript(NULL);
	SAFE_DELETE(g_pSceneScript);

	delete SceneCommHandler::GetSingletonPtr();
	sLog.outString("Deleting Park World...");
	delete Park::GetSingletonPtr();

	cLog.Notice( "EventMgr", "~EventMgr()" );
	delete EventMgr::GetSingletonPtr();

	Sleep(100);
	_StopDB();

	//一些单体的删除,不处理也没关系,程序马上结束啦
	delete ThreadMgr::GetSingletonPtr();
	delete CLog::GetSingletonPtr();
	printf("TextLogger::Thread->Die()\n");

	Sleep(1000);//等待文本写完
	TextLogger::Thread->Die();//TextLogger的实例会在这里delete
	Sleep(1000);//等待文本写完
	delete oLog::GetSingletonPtr();

#ifdef _WIN64
	WSACleanup();
#endif
	g_worldPacketBufferPool.Stats();
	g_worldPacketBufferPool.Destroy();

	g_sqlExecuteBufferPool.Stats();
	g_sqlExecuteBufferPool.Destroy();

	printf("程序安全退出，按任意键结束进程\n");
	return 0;
}

int main(int argc, char* argv[])
{
#ifndef _DEBUG
	THREAD_TRY_EXECUTION
	{
#endif		
		win32_main(argc,argv);
#ifndef _DEBUG
	}
	THREAD_HANDLE_CRASH
#endif	

}