#pragma once
#include "afxwin.h"
#include "resource.h"
#include "Statistics.h"
#include <GuardianConfig.h>
#include "skpark/HoverButton.h"

class CGuardianDlg;

// CHotKeyDlg 대화 상자입니다.

class CHotKeyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHotKeyDlg)

public:
	CHotKeyDlg(CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CHotKeyDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_DLG_HOTKEY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CStatistics*				m_stat;
	CGuardianConfig*	m_config;
	CGuardianDlg*				m_parent;

	void SetParentInfo(CGuardianDlg* p, CStatistics* s, CGuardianConfig* c) { m_parent = p; m_config = c; m_stat = s; }

	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	CHoverButton m_btF3;
	CHoverButton m_btF4Narrow;
	CHoverButton m_btF4Wide;
	CHoverButton m_btF5Thermal;
	CHoverButton m_btF5Normal;
	CHoverButton m_btF7;
	CHoverButton m_btF8;
	CHoverButton m_btF9;
	CHoverButton m_btF12Full;
	CHoverButton m_btF12Shrink;
	CHoverButton m_btDel;
	CHoverButton m_btBrightUp;
	CHoverButton m_btBrightDown;
	CHoverButton m_btContrastUp;
	CHoverButton m_btContrastDown;
	afx_msg void OnBnClickedBtF3();
	afx_msg void OnBnClickedBtF4Narrow();
	afx_msg void OnBnClickedBtF4Wide();
	afx_msg void OnBnClickedBtF5Thermal();
	afx_msg void OnBnClickedBtF5Normal();
	afx_msg void OnBnClickedBtF7();
	afx_msg void OnBnClickedBtF8();
	afx_msg void OnBnClickedBtF9();
	afx_msg void OnBnClickedBtF12Full();
	afx_msg void OnBnClickedBtF12Shrink();
	afx_msg void OnBnClickedBtDel();
	afx_msg void OnBnClickedBtBrightUp();
	afx_msg void OnBnClickedBtBrightDown();
	afx_msg void OnBnClickedBtContrastUp();
	afx_msg void OnBnClickedBtContrastDown();

	void DrawUBCLogo(CPaintDC* dc, int x, int y);

	CString m_hostId;
	CString m_version;

	CHoverButton m_btKillBrowser;
	afx_msg void OnBnClickedBtKillBrowser();
	CHoverButton m_btOK;

};
