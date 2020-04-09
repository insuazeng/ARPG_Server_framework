#include "stdafx.h"
#include "UpdateThread.h"
#include "threading/UserThread.h"
#include "threading/UserThreadMgr.h"
#include "LogonCommHandler.h"
#include "SceneCommHandler.h"

extern bool m_stopEvent;
extern bool m_ShutdownEvent;
extern bool m_ReloadPackConfigEvent;
extern uint32 m_ShutdownTimer;

CUpdateThread::CUpdateThread(const int64 threadID) : UserThread(THREADTYPE_CONSOLEINTERFACE, threadID)
{
	delete_after_use =false;
}

CUpdateThread::~CUpdateThread(void)
{
	SetThreadState(THREADSTATE_TERMINATE);
	Sleep(100);
}

void CUpdateThread:: run()
{	
	m_input=0;

	while(m_threadState != THREADSTATE_TERMINATE && m_stopEvent==false)
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
		m_input=getchar();//ÍË³ö
#endif
		switch(m_input)
		{
		case 'q':
			{
				m_ShutdownEvent=!m_ShutdownEvent;
				if(!m_ShutdownEvent)
				{
					/*WorldPacket data(SMSG_SERVER_MESSAGE,20);
					data << uint32(SERVER_MSG_SHUTDOWN_CANCELLED);
					sSceneCommHandler.SendGlobaleMsg(&data);*/
				}
			}
			break;
		case 's':
			printf(sThreadMgr.ShowStatus().c_str());
			break;
		case 'm':
			g_worldPacketBufferPool.Stats();
			break;
		case 'o':
			g_worldPacketBufferPool.Optimize();
			break;
		case 'p':
			{
				printf("total player num %d; senddata %d;recvdata %d\n", sSceneCommHandler.GetOnlinePlayersCount(), sSceneCommHandler.g_totalSendBytesToClient, sSceneCommHandler.g_totalRecvBytesFromClient);
				sLog.outString("total transfer packet %d size %d", sSceneCommHandler.g_transpacketnum, sSceneCommHandler.g_transpacketsize);
			}
			break;
		case 'r':
			{
				m_ReloadPackConfigEvent = true;
			}
			break;
		case 'b':
			sSceneCommHandler.OutPutPacketLog();
			break;
		}

		m_input = 0;

		if(m_threadState == THREADSTATE_TERMINATE)
			break;
		if(m_ShutdownEvent)
			break;

		m_threadState = THREADSTATE_SLEEPING;
		Sleep(500);
	}
}

