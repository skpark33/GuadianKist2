
// SocketSimulator.h : PROJECT_NAME ���� ���α׷��� ���� �� ��� �����Դϴ�.
//

#pragma once

#ifndef __AFXWIN_H__
	#error "PCH�� ���� �� ������ �����ϱ� ���� 'stdafx.h'�� �����մϴ�."
#endif

#include "resource.h"		// �� ��ȣ�Դϴ�.


// CSocketSimulatorApp:
// �� Ŭ������ ������ ���ؼ��� SocketSimulator.cpp�� �����Ͻʽÿ�.
//

class CSocketSimulatorApp : public CWinApp
{
public:
	CSocketSimulatorApp();

// �������Դϴ�.
public:
	virtual BOOL InitInstance();

// �����Դϴ�.

	DECLARE_MESSAGE_MAP()
};

extern CSocketSimulatorApp theApp;