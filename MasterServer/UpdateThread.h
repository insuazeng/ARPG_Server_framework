#pragma once

#include "threading/UserThread.h"


class CUpdateThread : public UserThread
{
public:
	CUpdateThread(const int64 threadID);
	~CUpdateThread(void);
	void run();
	int m_input;
};
