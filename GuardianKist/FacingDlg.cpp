// FacingDlg.cpp : ���� �����Դϴ�.
//

#include "stdafx.h"
#include "FacingDlg.h"
#include "afxdialogex.h"
#include <TraceLog.h>  

// CFacingDlg ��ȭ �����Դϴ�.

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


// CFacingDlg �޽��� ó�����Դϴ�.


BOOL CFacingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	MoveWindow(m_xpos, m_ypos, m_width, 10);

	// TODO:  ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	m_Picture.MoveWindow(0, 0, 1080, 10, true);
	m_Picture.ShowWindow(SW_SHOW);
	CString filename;
	filename.Format("%sfacing.gif", UBC_GUARDIAN_PATH);
	bool loaded = m_Picture.Load(filename);
	TraceLog(("InitAnimation(%d)", loaded));
	m_Picture.Draw();
	

	return TRUE;  // return TRUE unless you set the focus to a control
	// ����: OCX �Ӽ� �������� FALSE�� ��ȯ�ؾ� �մϴ�.
}


void CFacingDlg::OnClose()
{
	// TODO: ���⿡ �޽��� ó���� �ڵ带 �߰� ��/�Ǵ� �⺻���� ȣ���մϴ�.
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