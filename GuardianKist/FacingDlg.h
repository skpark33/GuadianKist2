#pragma once
#include "skpark/PictureEx.h"
#include "resource.h"

// CFacingDlg ��ȭ �����Դϴ�.

class CFacingDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CFacingDlg)

public:
	CFacingDlg(int xpos,int ypos, int width, CWnd* pParent = NULL);   // ǥ�� �������Դϴ�.
	virtual ~CFacingDlg();

// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_FACING_DLG };

	CPictureEx m_Picture;
	int  m_ypos;
	int  m_xpos;
	int  m_width;
protected:
	HCURSOR m_hCursor;

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.
	//afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
};
