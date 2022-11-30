#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

class CGuardianDlg;
#ifndef DEFAULT_PORT
#define DEFAULT_PORT "27015"
#endif

class CSocketServer {
public:
	CSocketServer(CGuardianDlg* dlg, CString port = DEFAULT_PORT);
	~CSocketServer();

	int Start();
	int Stop();
	int Receive();
	int Send(CString& sendStr);
	
protected:
	SOCKET ClientSocket;
	CString m_port;
	CGuardianDlg* m_dlg;

};