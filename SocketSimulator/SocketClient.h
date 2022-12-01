#pragma once
#include "afxcmn.h"
#include "afxwin.h"


#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef DEFAULT_PORT
#define DEFAULT_PORT "27015"
#endif

class CSocketSimulatorDlg;
class CSocketClient {
public:	
	CSocketClient(CSocketSimulatorDlg* dlg, CString serverIp = "127.0.0.1", CString port = DEFAULT_PORT);
	~CSocketClient();

	int Connect();
	int Stop();
	int Send(CString sendBuf);
	int Receive();

protected:
	CString m_serverIp;
	CString m_port;

	SOCKET ConnectSocket;

	CSocketSimulatorDlg* m_dlg;

};