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

#ifndef LOGONCOMMHANDLER_H
#define LOGONCOMMHANDLER_H

#include "ServerCommonDef.h"
#include "GameSocket.h"
#include "LogonCommClient.h"
#include "auth/Sha1.h"

class LogonCommHandler : public Singleton<LogonCommHandler>
{
private:
	map<uint32, GameSocket*> m_pendingAuthSockets;			// 保存待连（未验证）玩家的套接字映射表
	
	LoginServer				m_LoginServerData;				// 登陆服的配置与记录数据
	LogonCommClient*		m_pLoginServerSocket;			// 与登陆服保持连接的套接字
														
	uint32 next_request;									// 玩家请求登陆时候用到的一个请求编号
	Mutex mapLock;											// 登陆服务器通讯的套接字映射表的锁
	Mutex pendingLock;										// 待连玩家映射表锁

public:	
	SHA1Context m_sha1AuthKey;								// 用于网关服在登陆服处验证注册时用到的sha1哈希值

	LogonCommHandler();
	~LogonCommHandler();

	void Startup();															// 本控制器启动（读取文件配置，连接至登陆服务器）
	void ConnectionDropped();												// 某网关服务器断开连接
	void AdditionAck(uint32 ServID);										// 将一个网关服务器变成已注册
	void UpdateSockets();													// 当某一网关服务器ping出错时便关闭它
	void ConnectLoginServer(LoginServer *server);							// 让网关服务器连接到登陆服务器

	void LoadRealmConfiguration();											// 读取文件配置（创建登陆服务器对象以及网关服务器对象）

	/////////////////////////////
	// Worldsocket stuff
	///////

	uint32 GenerateRequestID();
	bool ClientConnected(string &accountName, GameSocket *clientSocket);	// 客户端套接字连入网关服务器，并将验证请求发送至登陆服务器
	
	void UnauthedSocketClose(uint32 id);		// 一个未通过验证的客户端套接字被关闭
	void RemoveUnauthedSocket(uint32 id);		// 移除一个未通过验证的客户端套接字

	inline GameSocket* GetSocketByRequest(uint32 id)		// 通过请求ID找到一个客户端套接字
	{
		//pendingLock.Acquire();
		GameSocket * sock;
		map<uint32, GameSocket*>::iterator itr = m_pendingAuthSockets.find(id);
		sock = (itr == m_pendingAuthSockets.end()) ? 0 : itr->second;
		//pendingLock.Release();
		return sock;
	}
	inline Mutex & GetPendingLock() { return pendingLock; }
	inline LoginServer* GetLoginServerData() { return &m_LoginServerData; }
};

#define sLogonCommHandler LogonCommHandler::GetSingleton()

#endif

