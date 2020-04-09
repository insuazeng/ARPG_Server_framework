
#include "stdafx.h"
#include "CommonTypes.h"
#include "UpdateThread.h"
#include "network/SocketEngine.h"
#include "BufferPool.h"
#include "threading/UserThreadMgr.h"
#include "WorldRunnable.h"

extern bool m_stopEvent;
extern bool m_ShutdownEvent;
extern bool g_sReloadStaticDataBase;


CUpdateThread::CUpdateThread(const int64 threadID) : UserThread(THREADTYPE_CONSOLEINTERFACE, threadID)
{
	this->delete_after_use =false;
}

CUpdateThread::~CUpdateThread(void)
{
}

void CUpdateThread:: run()
{	
	m_input=0;

	while(m_threadState != THREADSTATE_TERMINATE&&m_stopEvent==false)
	{
		// Provision for pausing this thread.
		if(m_threadState == THREADSTATE_PAUSED)
		{
			while(m_threadState == THREADSTATE_PAUSED)
			{
				Sleep(200);
			}
		}
		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		m_threadState = THREADSTATE_BUSY;
#ifdef _WIN64		
		m_input=getchar();//退出
#endif
		switch(m_input)
		{
		case 'q':
			m_ShutdownEvent=!m_ShutdownEvent;
			break;
		case 's':
			printf(sThreadMgr.ShowStatus().c_str());
			break;
		case 'p':
			{

				WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
				if(prun->totalupdate==0)
					prun->totalupdate=1;
				printf("average %d; min %d ;max %d\n",prun->totalparkdiff/prun->totalupdate,prun->minparkdiff,prun->maxparkdiff);
				//printf("总收到数据 %d bytes,总发送数据 %d bytes\n",sInfoCore.m_totalreceivedata,sInfoCore.m_totalsenddata);
				//printf("数据包发送数:%d， 超时发包数:%d，超缓冲区容量发包数:%d\n", sInfoCore.m_totalSendPckCount, sInfoCore.m_TimeOutSendCount,	sInfoCore.m_SizeOutSendCount);
				//sPark.PrintDebugLog();
				break;
			}
		case 'r':
			{
				WorldRunnable* prun = (WorldRunnable*)sThreadMgr.GetThreadByType(THREADTYPE_WORLDRUNNABLE);
				prun->totalupdate=prun->totalparkdiff=prun->maxparkdiff=0;
				prun->minparkdiff = 100;
			}
			break;
		case 'b':

			break;
		case 'm':
				g_worldPacketBufferPool.Stats();
			break;
        case 'g':
            g_sReloadStaticDataBase = true;
            break;
		case 'e':
			{
				break;
			}
		default:
			break;
		}

		m_input=0;

		if(m_threadState == THREADSTATE_TERMINATE)
			break;

		if(m_ShutdownEvent)
			break;

		m_threadState = THREADSTATE_SLEEPING;
		Sleep(500);
	}
}
