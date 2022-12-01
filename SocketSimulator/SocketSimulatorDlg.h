
// SocketSimulatorDlg.h : 헤더 파일
//

#pragma once

class CSocketClient;

// CSocketSimulatorDlg 대화 상자
class CSocketSimulatorDlg : public CDialogEx
{
// 생성입니다.
public:
	CSocketSimulatorDlg(CWnd* pParent = NULL);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SOCKETSIMULATOR_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
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
