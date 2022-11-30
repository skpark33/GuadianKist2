// FacingDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "FacingDlg.h"
#include "afxdialogex.h"
#include <TraceLog.h>  

// CFacingDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CFacingDlg, CDialogEx)

CFacingDlg::CFacingDlg(int xpos, int ypos, int width, CWnd* pParent /*=NULL*/)
	: CDialogEx(CFacingDlg::IDD, pParent)
{
	//m_hCursor = AfxGetApp()->LoadCursor(IDC_HAND);
	m_ypos = ypos;
	m_xpos = xpos;
	m_width = width;
}

CFacingDlg::~CFacingDlg()
{
}

void CFacingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FACING_CTRL, m_Picture);

}


BEGIN_MESSAGE_MAP(CFacingDlg, CDialogEx)
	ON_WM_CLOSE()
	//ON_WM_SETCURSOR()
END_MESSAGE_MAP()


// CFacingDlg 메시지 처리기입니다.


BOOL CFacingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	MoveWindow(m_xpos, m_ypos, m_width, 10);

	// TODO:  여기에 추가 초기화 작업을 추가합니다.
	m_Picture.MoveWindow(0, 0, 1080, 10, true);
	m_Picture.ShowWindow(SW_SHOW);
	CString filename;
	filename.Format("%sfacing.gif", UBC_GUARDIAN_PATH);
	bool loaded = m_Picture.Load(filename);
	TraceLog(("InitAnimation(%d)", loaded));
	m_Picture.Draw();
	

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void CFacingDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	m_Picture.UnLoad();
	CDialogEx::OnClose();
}

//BOOL CFacingDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
//{
//	// TODO: Add your message handler code here and/or call default
//
//	CRect rect;
//	m_Picture.GetWindowRect(&rect);
//	ScreenToClient(&rect);
//
//	CPoint point;
//	GetCursorPos(&point);
//	ScreenToClient(&point);
//
//	if (rect.PtInRect(point) && m_hCursor)
//	{
//		SetCursor(m_hCursor);
//		return TRUE;
//	};
//
//	return CDialog::OnSetCursor(pWnd, nHitTest, message);
//}