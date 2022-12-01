
// SocketSimulatorDlg.h : ��� ����
//

#pragma once

class CSocketClient;

// CSocketSimulatorDlg ��ȭ ����
class CSocketSimulatorDlg : public CDialogEx
{
// �����Դϴ�.
public:
	CSocketSimulatorDlg(CWnd* pParent = NULL);	// ǥ�� �������Դϴ�.

// ��ȭ ���� �������Դϴ�.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SOCKETSIMULATOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �����Դϴ�.


// �����Դϴ�.
protected:
	HICON m_hIcon;

	// ������ �޽��� �� �Լ�
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	static UINT TestClient(LPVOID pParam);
	CSocketClient* m_test_client;

public:
	afx_msg void OnClose();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();

	void Received(CString rvalue);
};
