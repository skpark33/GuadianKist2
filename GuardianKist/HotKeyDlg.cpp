// HotKeyDlg.cpp : 구현 파일입니다.
//

#include "stdafx.h"
#include "HotKeyDlg.h"
#include "GuardianDlg.h"
#include "afxdialogex.h"
#include <common.h>
#include <TraceLog.h>  
#include <resource.h>  

// CHotKeyDlg 대화 상자입니다.

IMPLEMENT_DYNAMIC(CHotKeyDlg, CDialogEx)

CHotKeyDlg::CHotKeyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CHotKeyDlg::IDD, pParent)
{
	char czHostName[128] = { 0, };
	gethostname(czHostName, 128);
	m_hostId = czHostName;

	getVersion(m_version);
}

CHotKeyDlg::~CHotKeyDlg()
{
}

void CHotKeyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, ID_BT_F3, m_btF3);
	DDX_Control(pDX, ID_BT_F4_NARROW, m_btF4Narrow);
	DDX_Control(pDX, ID_BT_F4_WIDE, m_btF4Wide);
	DDX_Control(pDX, ID_BT_F5_THERMAL, m_btF5Thermal);
	DDX_Control(pDX, ID_BT_F5_NORMAL, m_btF5Normal);
	DDX_Control(pDX, ID_BT_F7, m_btF7);
	DDX_Control(pDX, ID_BT_F8, m_btF8);
	DDX_Control(pDX, ID_BT_F9, m_btF9);
	DDX_Control(pDX, ID_BT_F12_FULL, m_btF12Full);
	DDX_Control(pDX, ID_BT_F12_SHRINK, m_btF12Shrink);
	DDX_Control(pDX, ID_BT_DEL, m_btDel);
	DDX_Control(pDX, ID_BT_BRIGHT_UP, m_btBrightUp);
	DDX_Control(pDX, ID_BT_BRIGHT_DOWN, m_btBrightDown);
	DDX_Control(pDX, ID_BT_CONTRAST_UP, m_btContrastUp);
	DDX_Control(pDX, ID_BT_CONTRAST_DOWN, m_btContrastDown);
	DDX_Control(pDX, ID_BT_KILL_BROWSER, m_btKillBrowser);
	DDX_Control(pDX, IDOK, m_btOK);
}


BEGIN_MESSAGE_MAP(CHotKeyDlg, CDialogEx)
	ON_WM_PAINT()
	ON_BN_CLICKED(ID_BT_F3, &CHotKeyDlg::OnBnClickedBtF3)
	ON_BN_CLICKED(ID_BT_F4_NARROW, &CHotKeyDlg::OnBnClickedBtF4Narrow)
	ON_BN_CLICKED(ID_BT_F4_WIDE, &CHotKeyDlg::OnBnClickedBtF4Wide)
	ON_BN_CLICKED(ID_BT_F5_THERMAL, &CHotKeyDlg::OnBnClickedBtF5Thermal)
	ON_BN_CLICKED(ID_BT_F5_NORMAL, &CHotKeyDlg::OnBnClickedBtF5Normal)
	ON_BN_CLICKED(ID_BT_F7, &CHotKeyDlg::OnBnClickedBtF7)
	ON_BN_CLICKED(ID_BT_F8, &CHotKeyDlg::OnBnClickedBtF8)
	ON_BN_CLICKED(ID_BT_F9, &CHotKeyDlg::OnBnClickedBtF9)
	ON_BN_CLICKED(ID_BT_F12_FULL, &CHotKeyDlg::OnBnClickedBtF12Full)
	ON_BN_CLICKED(ID_BT_F12_SHRINK, &CHotKeyDlg::OnBnClickedBtF12Shrink)
	ON_BN_CLICKED(ID_BT_DEL, &CHotKeyDlg::OnBnClickedBtDel)
	ON_BN_CLICKED(ID_BT_BRIGHT_UP, &CHotKeyDlg::OnBnClickedBtBrightUp)
	ON_BN_CLICKED(ID_BT_BRIGHT_DOWN, &CHotKeyDlg::OnBnClickedBtBrightDown)
	ON_BN_CLICKED(ID_BT_CONTRAST_UP, &CHotKeyDlg::OnBnClickedBtContrastUp)
	ON_BN_CLICKED(ID_BT_CONTRAST_DOWN, &CHotKeyDlg::OnBnClickedBtContrastDown)
	ON_BN_CLICKED(ID_BT_KILL_BROWSER, &CHotKeyDlg::OnBnClickedBtKillBrowser)
END_MESSAGE_MAP()


// CHotKeyDlg 메시지 처리기입니다.


void CHotKeyDlg::OnPaint()
{
	CPaintDC dc(this);
	CRect rect;
	GetClientRect(&rect);

	dc.FillSolidRect(rect, RGB(0x0b, 0x0c, 0x11));

	DrawUBCLogo(&dc, 0, 0);

	int posX1 = 10;
	int posX2 = 190;
	int posY1 = 190;
	int posY2 = 190;
	int width = 240;
	int height = 24;

	m_parent->WriteSingleLine("Version " + m_version, &dc, m_parent->m_alarmFont5, RGB(0xff, 0xff, 0xff), posY1, height, 1, posX1, width);
	m_parent->WriteSingleLine(m_hostId, &dc, m_parent->m_alarmFont5, RGB(151, 53, 43), posY2, height, 1, posX2, width);

}

void CHotKeyDlg::DrawUBCLogo(CPaintDC* dc, int x, int y)
{

	CDC MemDC;
	BITMAP bmpInfo;

	// 화면 DC와 호환되는 메모리 DC를 생성
	MemDC.CreateCompatibleDC(dc);

	// 비트맵 리소스 로딩
	CBitmap bmp;
	CBitmap* pOldBmp = NULL;
	bmp.LoadBitmap(IDB_BITMAP_BG);

	// 로딩된 비트맵 정보 확인
	bmp.GetBitmap(&bmpInfo);

	// 메모리 DC에 선택
	pOldBmp = MemDC.SelectObject(&bmp);

	// 메모리 DC에 들어 있는 비트맵을 화면 DC로 복사하여 출력
	dc->BitBlt(x, y, bmpInfo.bmWidth, bmpInfo.bmHeight, &MemDC, 0, 0, SRCCOPY);

	MemDC.SelectObject(pOldBmp);
}



BOOL CHotKeyDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	m_btF8.LoadBitmapA(IDB_BITMAP_REGISTRATION, RGB(232, 168, 192));
	m_btOK.LoadBitmapA(IDB_BITMAP_CLOSE, RGB(232, 168, 192));
	m_btF9.LoadBitmapA(IDB_BITMAP_CLOSE_ALL, RGB(232, 168, 192));
	m_btBrightUp.LoadBitmapA(IDB_BITMAP_BRIGHT_UP, RGB(232, 168, 192));
	m_btBrightDown.LoadBitmapA(IDB_BITMAP_BRIGHT_DOWN, RGB(232, 168, 192));
	m_btContrastUp.LoadBitmapA(IDB_BITMAP_CONT_UP, RGB(232, 168, 192));
	m_btContrastDown.LoadBitmapA(IDB_BITMAP_CONT_DOWN, RGB(232, 168, 192));
	m_btDel.LoadBitmapA(IDB_BITMAP_DEL, RGB(232, 168, 192));
	m_btKillBrowser.LoadBitmapA(IDB_BITMAP_CLOSE_VIDEO, RGB(232, 168, 192));
	m_btF12Full.LoadBitmapA(IDB_BITMAP_FULL_CAMERA, RGB(232, 168, 192));
	m_btF12Shrink.LoadBitmapA(IDB_BITMAP_SHRINK_CAMERA, RGB(232, 168, 192));
	m_btF4Wide.LoadBitmapA(IDB_BITMAP_WIDE, RGB(232, 168, 192));
	m_btF4Narrow.LoadBitmapA(IDB_BITMAP_NARROW, RGB(232, 168, 192));
	m_btF5Normal.LoadBitmapA(IDB_BITMAP_NORMAL, RGB(232, 168, 192));
	m_btF5Thermal.LoadBitmapA(IDB_BITMAP_THERMAL, RGB(232, 168, 192));
	m_btF7.LoadBitmapA(IDB_BITMAP_UPLOADER, RGB(232, 168, 192));
	m_btF3.LoadBitmapA(IDB_BITMAP_CAMERA_WEB, RGB(232, 168, 192));

	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;
	char buf[10];
	memset(buf, 0x00, 10);
	GetPrivateProfileString("GUARDIAN", "IS_WIDE", "0", buf, 10, iniPath);
	bool isWide = bool(atoi(buf));
	m_btF4Narrow.ShowWindow(isWide ? SW_SHOW : SW_HIDE);
	m_btF4Wide.ShowWindow(isWide ? SW_HIDE : SW_SHOW);

	memset(buf, 0x00, 10);
	GetPrivateProfileString("GUARDIAN", "CAMERA", "0", buf, 10, iniPath);
	bool  isThermal = bool((atoi(buf) == 1));
	m_btF5Normal.ShowWindow(isThermal ? SW_HIDE : SW_SHOW);
	m_btF5Thermal.ShowWindow(isThermal ? SW_SHOW : SW_HIDE);

	m_btF12Full.ShowWindow(SW_HIDE);
	m_btF12Shrink.ShowWindow(SW_SHOW);

	CRect rect;
	GetClientRect(&rect);

	//MoveWindow((1080 - rect.Width()) / 2, 200, rect.Width(), rect.Height()+30);
	::SetWindowPos(this->m_hWnd,NULL, (1080 - rect.Width()) / 2, 150, 0, 0, SWP_NOSIZE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}
void CHotKeyDlg::OnBnClickedBtF3()
{
	OpenHikvisionCameraPage();
}
void CHotKeyDlg::OnBnClickedBtF4Narrow()
{
	ToggleWideNarrow(m_parent /* m_config->m_is_KIST_type */);
	::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
}
void CHotKeyDlg::OnBnClickedBtF4Wide()
{
	ToggleWideNarrow(m_parent /*, m_config->m_is_KIST_type*/);
	::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
}
void CHotKeyDlg::OnBnClickedBtF5Thermal()
{
	bool isThermal = m_parent->ToggleCamera();
	m_btF5Normal.ShowWindow(isThermal ? SW_SHOW : SW_HIDE);
	m_btF5Thermal.ShowWindow(isThermal ? SW_HIDE : SW_SHOW);
}
void CHotKeyDlg::OnBnClickedBtF5Normal()
{
	bool isThermal = m_parent->ToggleCamera();
	m_btF5Normal.ShowWindow(isThermal ? SW_SHOW : SW_HIDE);
	m_btF5Thermal.ShowWindow(isThermal ? SW_HIDE : SW_SHOW);
}
void CHotKeyDlg::OnBnClickedBtF7()
{
	m_parent->OpenUploader();
}
void CHotKeyDlg::OnBnClickedBtF8()
{
	m_parent->OpenAuthorizer();
}
void CHotKeyDlg::OnBnClickedBtF9()
{
	m_parent->KillEverything(false);
}
void CHotKeyDlg::OnBnClickedBtF12Full()
{
	showTaskbar(true);
	bool fullScreen = m_parent->ToggleFullScreen();
	this->m_btF12Full.ShowWindow(fullScreen ? SW_HIDE : SW_SHOW);
	this->m_btF12Shrink.ShowWindow(fullScreen ? SW_SHOW : SW_HIDE);
}
void CHotKeyDlg::OnBnClickedBtF12Shrink()
{
	showTaskbar(true);
	bool fullScreen = m_parent->ToggleFullScreen();
	this->m_btF12Full.ShowWindow(fullScreen ? SW_HIDE : SW_SHOW);
	this->m_btF12Shrink.ShowWindow(fullScreen ? SW_SHOW : SW_HIDE);
}
void CHotKeyDlg::OnBnClickedBtDel()
{
	m_parent->KillEverything(true);
}
void CHotKeyDlg::OnBnClickedBtBrightUp()
{
	m_parent->m_cameraNormal.BrightnessUp();
}
void CHotKeyDlg::OnBnClickedBtBrightDown()
{
	m_parent->m_cameraNormal.BrightnessDown();
}
void CHotKeyDlg::OnBnClickedBtContrastUp()
{
	m_parent->m_cameraNormal.ContrastUp();
}
void CHotKeyDlg::OnBnClickedBtContrastDown()
{
	m_parent->m_cameraNormal.ContrastDown();
}
void CHotKeyDlg::OnBnClickedBtKillBrowser()
{
	KillBrowserOnly();
}
