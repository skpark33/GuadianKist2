#pragma once
#include "skpark/PictureEx.h"
#include "resource.h"

// CFacingDlg 대화 상자입니다.

class CFacingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFacingDlg)

public:
	CFacingDlg(int xpos,int ypos, int width, CWnd* pParent = NULL);   // 표준 생성자입니다.
	virtual ~CFacingDlg();

// 대화 상자 데이터입니다.
	enum { IDD = IDD_FACING_DLG };

	CPictureEx m_Picture;
	int  m_ypos;
	int  m_xpos;
	int  m_width;
protected:
	HCURSOR m_hCursor;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.
	//afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
};
