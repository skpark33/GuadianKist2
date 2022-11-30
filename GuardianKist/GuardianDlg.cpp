
// CGuardianDlg.cpp: 구현 파일
//

#include "stdafx.h"
#include "framework.h"
#include "afxdialogex.h"
#include "GuardianApp.h"
#include "GuardianDlg.h"
#include "MsgPopupDlg.h"

#include <iostream>
#include <algorithm>
#include <vector>
#include <zbar.h>

#include <opencv2/core.hpp>					// Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/highgui/highgui.hpp>		// OpenCV window I/O
#include <opencv2/imgproc/imgproc.hpp>		// Gaussian Blur
#include <opencv2/videoio.hpp>


#include "math.h"
#include "cjson/cJSON.h"

//-------------   skpark  -----------------//
#include <TraceLog.h>  
#include <common.h>
#include <TimePlate.h>
#include <mmsystem.h>
#include "ci/libAes256/ciElCryptoAes256.h"
#include "ci/libBase/ciStringUtil.h"
#include "skpark/EzvizDDNSAPI.h"
#include "skpark/ReinforceFR.h"
#include "skpark/WindowCapture.h"
#include "skpark/Crc32Static.h"
#include "HotkeyDlg.h"
#include "FacingDlg.h"
#include "SocketServer.h"
#include "SocketClient.h"

#include "HCNetSDK.h"


#define	WEATHER_FONT_REGULAR	"Noto Sans KR Regular"
#define	WEATHER_FONT_REGULAR_EN	"Segoe UI Symbol"
#define WEATHER_FONT_SIZE_20	20
#define WEATHER_FONT_SIZE_24	24.5
#define WEATHER_FONT_SIZE_25	25
#define WEATHER_FONT_SIZE_30	30
#define WEATHER_FONT_SIZE_82	82.5


#define WEATHER_FG_COLOR_GOOD	RGB(0x0f,0xc2,0xf6)		// 미세먼지 등급 - 좋음
#define WEATHER_FG_COLOR_NRML	RGB(0x37,0xe9,0x07)		// 미세먼지 등급 - 보통
#define WEATHER_FG_COLOR_BAD	RGB(0xff,0xa5,0x07)		// 미세먼지 등급 - 나쁨
#define WEATHER_FG_COLOR_WORST	RGB(0xff,0x06,0x06)		// 미세먼지 등급 - 매무나쁨

#define TITLE_AREA_HEIGHT		225


using namespace std;
using namespace cv;

//static unsigned long ulAlarmPic = 0;


/*---------------------------- for New VS Build ----------------------------//
// https://stackoverflow.com/questions/30412951/unresolved-external-symbol-imp-fprintf-and-imp-iob-func-sdl2

#pragma comment(lib, "legacy_stdio_definitions.lib")
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void)
{
	return _iob;
}
//--------------------------------------------------------------------------------//
#pragma comment(lib, "nafxcw.lib")

#pragma comment(lib, "jasper.lib")
#pragma comment(lib, "Jpeg.lib")
#pragma comment(lib, "libdcr.lib")
#pragma comment(lib, "mng.lib")
#pragma comment(lib, "png.lib")
#pragma comment(lib, "Tiff.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "cximage.lib")
//--------------------------------------------------------------------------------*/


extern HINSTANCE g_hDllCurlLib;

CRITICAL_SECTION g_cs;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


string UTF82ASCII(const char* cont)
{
	if (NULL == cont)
	{
		return string("");
	}
	int num = MultiByteToWideChar(CP_UTF8, NULL, cont, -1, NULL, NULL);
	wchar_t* buffw = new wchar_t[(unsigned int)num];
	MultiByteToWideChar(CP_UTF8, NULL, cont, -1, buffw, num);
	int len = WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
	char* lpsz = new char[(unsigned int)len + 1];
	WideCharToMultiByte(CP_ACP, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;
	string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

string ASCII2UTF8(const char* cont)
{
	if (NULL == cont)
	{
		return string("");
	}

	int num = MultiByteToWideChar(CP_ACP, NULL, cont, -1, NULL, NULL);
	wchar_t* buffw = new wchar_t[(unsigned int)num];
	MultiByteToWideChar(CP_ACP, NULL, cont, -1, buffw, num);

	int len = WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, NULL, NULL, NULL, NULL);
	char* lpsz = new char[(unsigned int)len + 1];
	WideCharToMultiByte(CP_UTF8, 0, buffw, num - 1, lpsz, len, NULL, NULL);
	lpsz[len] = '\0';
	delete[] buffw;

	string rtn(lpsz);
	delete[] lpsz;
	return rtn;
}

// 대기질등급명 - 한국어에만 필요, 다국어불필요
CString GetAirGradeDesc(CString airGrade)
{
	if (airGrade == "1")
		return _T("좋음");
	else if (airGrade == "2")
		return _T("보통");
	else if (airGrade == "3")
		return _T("나쁨");
	else if (airGrade == "4")
		return _T("매우나쁨");
	else
		return _T("보통");
}

// 대기등급색상
COLORREF GetAirGradeColor(CString airGrade) {
	if (airGrade == "1")
		return WEATHER_FG_COLOR_GOOD;
	else if (airGrade == "2")
		return WEATHER_FG_COLOR_NRML;
	else if (airGrade == "3")
		return WEATHER_FG_COLOR_BAD;
	else if (airGrade == "4")
		return WEATHER_FG_COLOR_WORST;
	else
		return WEATHER_FG_COLOR_NRML;
}

int GetWeatherIconLeftPos(CString weatherIcon)
{
	if (weatherIcon == "clear-day" || weatherIcon == "clear-night")
		return 90;
	else if (weatherIcon == "rain")
		return 50;
	else if (weatherIcon == "snow")
		return 50;
	else if (weatherIcon == "sleet")
		return 170;
	else if (weatherIcon == "wind")
		return 90;
	else if (weatherIcon == "fog")
		return 90;
	else if (weatherIcon == "cloudy")
		return 170;
	else if (weatherIcon == "partly-cloudy-day" || weatherIcon == "partly-cloudy-night")
		return 170;
	else if (weatherIcon == "hail" || weatherIcon == "thunderstorm" || weatherIcon == "tornado")
		return 90;
	else
		return 170;
}


// CGuardianDlg 대화 상자
CGuardianDlg::CGuardianDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_GUARDIAN_DLG, pParent)
	, m_bFullScreen(true)
	, m_videoWidth(0)
	, m_videoHeight(0)
	, m_videoLeft(0)
	, m_videoTop(0)
	, m_currentMainAlarmIdx(-1)
	, m_cameraID(1)
	, m_isMinimize(false)
	, m_frManager(0)
	, m_videoEnd(984)
	, m_titleStart(55)
	, m_letterLeft(83)
	, m_letterWide(915)
	, m_title(0)
	, m_logo(0)
	, m_bgInit1(0)
	, m_bgInit2(0)
	, m_bgFail1(0)
	, m_bgFail2(0)
	, m_bgNext(0)
	, m_bgIdentified(0)
	, m_notice(0)
	, m_logoW(471)
	, m_logoH(112)
	, m_lastFaceLeft(0)
	, m_logFP(0)
	, m_isAniPlay(false)
	, m_FacingDlg(NULL)
	, m_isVoiceRunning(false)
	, m_bgMode(BG_MODE::NONE)
	, m_modeCheckCounter(0)
	, m_socket(0)
	, m_test_client(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

//	InitializeCriticalSection(&m_struLock);  //most thread might use log print function, especially alarm part, needing to add a lock
	InitializeCriticalSection(&g_cs);

	m_config = new CGuardianConfig();
	m_stat = new CStatistics();

	char locale[8];
	int lang_len = GetLocaleInfo(LOCALE_NAME_USER_DEFAULT, LOCALE_SISO639LANGNAME, locale, sizeof(locale));
	m_localeLang = locale;
	TraceLog(("Locale Language [%s]", m_localeLang));
	if (m_localeLang.IsEmpty())
		m_localeLang = "ko";

	CDC* pDC = GetDC();
	InitFont(pDC);
	ReleaseDC(pDC);

	m_bgInitFile1 = "init1_bg.png";
	m_bgInitFile2 = "init2_bg.png";
	m_bgFailFile1 = "fail1_bg.png";
	m_bgFailFile2 = "fail2_bg.png";
	m_bgNextFile = "next_bg.png";
	m_bgIdentifiedFile = "identified_bg.png";

	m_logoFile = "customerLogo.png";
	m_titleFile = "customerTitle.png";
	m_noticeFile = "customerNotice.png";
}


void CGuardianDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAMERA_NORMAL, m_cameraNormal);
	DDX_Control(pDX, IDC_CAMERA_THERMAL, m_cameraThermal);
	//DDX_Control(pDX, IDC_BG_PICTURE_CTRL, m_bgPic);
	DDX_Control(pDX, ID_BN_CLEAR2, m_btNext);
	DDX_Control(pDX, IDOK, m_btNextDisabled);
}

BEGIN_MESSAGE_MAP(CGuardianDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_DESTROY()
	ON_WM_QUERYDRAGICON()
	ON_MESSAGE(UM_WEATHER_MESSAGE, OnWeatherMessage)
	ON_MESSAGE(UM_WEATHER_UPDATE_MESSAGE, OnWeatherUpdateMessage)
	ON_MESSAGE(UM_EVENT_UPDATE_MESSAGE, OnEventUpdateMessage)
	ON_MESSAGE(UM_AIR_MESSAGE, OnAirMessage)
	//ON_MESSAGE(UM_REFRESH_GUARDIAN_SETTINGS, OnRefreshGuardianSettings)
//	ON_MESSAGE(WM_PROC_ALARM, OnWMProcAlarm)
	ON_MESSAGE(WM_OLD_FILE_DELETE, OnOldFileDelete)
	ON_WM_COPYDATA()  
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(ID_BN_CLEAR2, &CGuardianDlg::OnBnClickedBnNext)
	ON_BN_CLICKED(IDOK, &CGuardianDlg::OnBnClickedBnNextDisabled)
	ON_STN_CLICKED(IDC_STATIC_STAT, &CGuardianDlg::OnStnClickedStaticStat)
END_MESSAGE_MAP()


// CGuardianDlg 메시지 처리기
BOOL CGuardianDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;
	
	char buf[512];
	//memset(buf, 0x00, 512);
	//GetPrivateProfileString("UTV_brwClient2", "PosY", "1220", buf, 512, iniPath);
	//int brwY = atoi(buf);
	//memset(buf, 0x00, 512);
	//GetPrivateProfileString("UTV_brwClient2", "Height", "700", buf, 512, iniPath);
	//int brwHeight = atoi(buf);

	//TraceLog(("OnInitDialog Browser isVideoBelow[%d] Ypos[%d] Height[%d]", m_config->m_videoBelow, brwY, brwHeight));

	// 카메라영상이 아래에 위치한 옵션인 경우 UBC Browser는 상단에 위치해야 함
	//if (m_config->m_videoBelow) {
	//	if (brwY > 1000) {
	//		TraceLog(("OnInitDialog Browser Ini Write ---- 111"));

	//		WritePrivateProfileString("UTV_brwClient2", "PosY", "192", iniPath);
	//		WritePrivateProfileString("UTV_brwClient2", "Height", "871", iniPath);

	//		// 재실행을 위해 종료
	//		KillBrowser();
	//	}
	//}
	//else {
	//	if (m_config->m_isWide && m_config->m_is_KIST_type)
	//	{
	//		if (brwY != 995)
	//		{
	//			//// 1220 - 225 = 995
	//			//WritePrivateProfileString("UTV_brwClient2", "PosY", "995", iniPath);
	//			//// 재실행을 위해 종료
	//			//KillBrowser();
	//		}
	//	}
	//	else
	//	{
	//		if (brwY < 500  || brwY == 995) {
	//			TraceLog(("OnInitDialog Browser Ini Write ---- 222"));

	//			WritePrivateProfileString("UTV_brwClient2", "PosY", "1220", iniPath);
	//			WritePrivateProfileString("UTV_brwClient2", "Height", "700", iniPath);

	//			// 재실행을 위해 종료
	//			KillBrowser();
	//		}
	//	}
	//}

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_pictureSavePath = _T("C:\\SQISoft\\Upload");

	char exeFullPath[MAX_PATH] = { 0 }; // Full path
	CString strExePath = "";
	GetModuleFileName(NULL, exeFullPath, MAX_PATH);
	strExePath = exeFullPath;    // Get full path of the file
	int pos = strExePath.ReverseFind('\\');
	CString stringXSDPath = strExePath.Left(pos);  // Return the directory without the file name

	CString stringCurlPath = strExePath.Left(pos);  // Return the directory without the file name
	stringCurlPath = strExePath.Left(pos) + "\\ClientDemoDll\\libcurl.dll";
	g_hDllCurlLib = LoadLibraryEx(stringCurlPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
	if (NULL == g_hDllCurlLib) {
		TraceLog(("LoadLibraryEx 실패 [%s]", stringCurlPath));
	}

	CGuardianApp* app = (CGuardianApp*)AfxGetApp();
	if (!app->UseGuardian())
	{
		m_isMinimize = true;
		AfxBeginThread(CGuardianDlg::WaitForWindowCreated, this);
		return TRUE;
	}
	m_isMinimize = false;

	InitMainWin();

	// Camera Initialize
	memset(buf, 0x00, 512);
	GetPrivateProfileString("GUARDIAN", "CAMERA", "1", buf, 512, iniPath);
	//CString cameraName;
	//cameraName.Format("Camera%s", buf);

	m_cameraID = atoi(buf);
	m_cameraNormal.ShowWindow(SW_HIDE);
	m_cameraThermal.ShowWindow(SW_HIDE);
	if (1 == m_cameraID) {
		m_cameraNormal.ShowWindow(SW_SHOW);
	}
	else {
		m_cameraThermal.ShowWindow(SW_SHOW);
	}

	memset(buf, 0x00, 512);
	GetPrivateProfileString("GUARDIAN", "IP", "", buf, 512, iniPath);
	CString cameraIP;
	cameraIP.Format("%s", buf);

	memset(buf, 0x00, 512);
	GetPrivateProfileString("GUARDIAN", "CAMERA_PW", "#kYRDiPMDYmSxvJ4dLQCcCQ==", buf, 512, iniPath);	// hikvision123
	CString cameraPasswd;
	cameraPasswd.Format("%s", buf);

	if (0 > m_cameraNormal.CameraConfig(1, cameraIP, 8000, "admin", cameraPasswd)) {
		//unsigned long pid = getPid("UTV_brwClient2.exe");
		//if (pid) {
		//	killProcess(pid);
		//}
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
	}

	if (0 > m_cameraThermal.CameraConfig(2, cameraIP, 8000, "admin", cameraPasswd)) {
		//unsigned long pid = getPid("UTV_brwClient2.exe");
		//if (pid) {
		//	killProcess(pid);
		//}
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
	}

//	CheckIp(); 
//	HideEverything(); 
	//LoadVideo("warning.mp4");
	//InitPosition(); 
//	AfxBeginThread(CGuardianDlg::AutoSetupThread, this); 

	m_cameraNormal.GetThermalRegion();
	TraceLog(("m_thermalX,m_thermalY=(%f,%f)", m_cameraNormal.m_thermalX, m_cameraNormal.m_thermalY));
	TraceLog(("m_thermalWidth,m_thermalHeight=(%f,%f)", m_cameraNormal.m_thermalWidth, m_cameraNormal.m_thermalHeight));
	
	
	MaxCamera2(m_width, m_height, true);
	
	m_normalPageKey = m_config->m_is_hiden_camera ? "CTRL+5" : (m_config->m_useQR ? "CTRL+4" : "CTRL+1");
	//GenerateQRCode();

	SetTimer(ALARM_CHECK_TIMER, 1000, NULL);
	SetTimer(KIST_CHECK_TIMER, 100, NULL);
	showTaskbar(false);

	//if (m_config->m_is_hiden_camera)
	//{
	//	TraceLog(("hidden=%d", m_config->m_is_hiden_camera));
	//	ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX, NULL);

	//	CWnd* statPlate = GetDlgItem(IDC_STATIC_STAT);
	//	statPlate->ShowWindow(SW_HIDE);

	//	float wh_ratio = (STREAM_HEIGHT / STREAM_WIDTH);  // 가로에 대한 세로의 비율 = 0.56547619f

	//	float w = static_cast<float>(m_width) / 2.4f;
	//	float h = w * wh_ratio; // = 1080 x  0.56547619 = 610.7

	//	int offset = 50;
	//	int x = m_width - w - offset;
	//	int y = offset;

	//	m_width = static_cast<int>(w);
	//	m_height = static_cast<int>(h);

	//	MaxCamera2(m_width, m_height);
	//	MoveWindow(x, y, m_width, m_height, TRUE);
	//	SetTopMost(true);
	//	return TRUE;
	//}


	//if (!LoadTitle())
	//{
	LoadBGInit1();
	LoadBGInit2();
	LoadBGFail1();
	LoadBGFail2();
	LoadBGNext();
	LoadBGIdentified();
	//LoadLogo();
	InitWatch();
	//}
	//LoadNotice();

	m_statBrush.CreateSolidBrush(NORMAL_BG_COLOR); // 평균값 배경 색 변경
	CWnd* statPlate = GetDlgItem(IDC_STATIC_STAT);
	if (m_stat->IsUsed() && m_stat->IsValid()) {
		CString msg = m_stat->ToString(m_localeLang);
		statPlate->SetWindowText(msg);
		statPlate->MoveWindow(0, m_height - 42, m_width, 42);
		statPlate->ShowWindow(SW_SHOW);
		statPlate->SetForegroundWindow();
	}
	else
	{
		statPlate->ShowWindow(SW_HIDE);
	}

	SetWindowText(_T("Guardian2.0"));
	CString msg;
	CTime now = CTime::GetCurrentTime();
	msg.Format("StartTime:%04d-%02d-%02d %02d:%02d:%02d",
		now.GetYear(), now.GetMonth(), now.GetDay(),
		now.GetHour(), now.GetMinute(), now.GetSecond());
	WriteLog((msg));


	m_btNext.LoadBitmapA(IDB_BITMAP_NEXT_ENABLED, RGB(232, 168, 192));
	m_btNextDisabled.LoadBitmapA(IDB_BITMAP_NEXT_DISABLED, RGB(232, 168, 192));

	CRect nextRect;
	m_btNext.GetWindowRect(&nextRect);
	m_btNext.MoveWindow((m_width - nextRect.Width()) / 2, m_videoEnd + 130, nextRect.Width(), nextRect.Height());
	m_btNext.ShowWindow(SW_HIDE);

	m_btNextDisabled.MoveWindow((m_width - nextRect.Width()) / 2, m_videoEnd + 130, nextRect.Width(), nextRect.Height());
	m_btNextDisabled.ShowWindow(SW_SHOW);


	//m_btNext.EnableWindow(FALSE);

	AfxBeginThread(CGuardianDlg::StartSocketServer, this);
	AfxBeginThread(CGuardianDlg::TestClient, this);
	return TRUE;
}

void CGuardianDlg::InitPosition()
{
	//if (m_config->m_is_hiden_camera)
	//{
	//	m_videoWidth = m_width;
	//	m_videoHeight = m_height;
	//	m_videoLeft = 0;
	//	m_videoTop = 0;
	//	return;
	//}

	if (m_config->m_isWide)
	{
		this->m_titleStart = 35;
	}
	else
	{
		m_titleStart = 55;
	}
	m_letterLeft = 83;
	m_letterWide = 915;
}

void CGuardianDlg::InitMainWin()
{
	m_width = WIN_WIDTH;
	m_height = WIN_HEIGHT;   
	MoveWindow(0, 0, m_width, m_height, TRUE);

	int widthGap = 55 + (WIN_WIDTH - DISPLAY_FULL_WIDTH) / 2;
	int iBottomGap = TITLE_AREA_HEIGHT;
	//int iBottomGap =  TITLE_AREA_HEIGHT;

	//CRect m_rectPreviewBG;
	//m_rectPreviewBG.left = widthGap;
	//m_rectPreviewBG.top = iBottomGap;
	//m_rectPreviewBG.right = WIN_WIDTH - widthGap;
	//m_rectPreviewBG.bottom = m_rectPreviewBG.Width() * 3 / 4 + iBottomGap + 4;
	//

	//m_cameraNormal.MoveWindow(&m_rectPreviewBG, TRUE);
	//m_cameraThermal.MoveWindow(&m_rectPreviewBG, TRUE);

	//	TraceLog(("InitMainWin >>>  left:%d top:%d right:%d bottom:%d", m_rectPreviewBG.left, m_rectPreviewBG.top, m_rectPreviewBG.right, m_rectPreviewBG.bottom));
}

void CGuardianDlg::InitFont(CDC* pDc)
{
	InitFont(m_alarmFont1, pDc, ALARM_FONT_1, ALARM_FONT_SIZE_1);
	InitFont(m_alarmFont2, pDc, ALARM_FONT_2, ALARM_FONT_SIZE_2);
	InitFont(m_alarmFont3, pDc, ALARM_FONT_3, ALARM_FONT_SIZE_3);
	InitFont(m_alarmFont4, pDc, ALARM_FONT_4, ALARM_FONT_SIZE_4);
	InitFont(m_alarmFont5, pDc, WEATHER_FONT_REGULAR_EN, ALARM_FONT_SIZE_4);
	InitFont(m_alarmFont6, pDc, WEATHER_FONT_REGULAR_EN, ALARM_FONT_SIZE_2);

	InitFont(m_normalFont1, pDc, NORMAL_FONT_1, NORMAL_FONT_SIZE_1);
	InitFont(m_normalFont2, pDc, NORMAL_FONT_2, NORMAL_FONT_SIZE_2);
	InitFont(m_normalFont3, pDc, NORMAL_FONT_3, NORMAL_FONT_SIZE_3);

	InitFont(m_weatherFontTitle, pDc, WEATHER_FONT_REGULAR, WEATHER_FONT_SIZE_20);
	InitFont(m_weatherFontValue, pDc, NORMAL_FONT_3, WEATHER_FONT_SIZE_24);
	InitFont(m_weatherFontTemp, pDc, WEATHER_FONT_REGULAR, WEATHER_FONT_SIZE_82);
	InitFont(m_weatherFontUnit, pDc, ("ko" == m_localeLang ? WEATHER_FONT_REGULAR : WEATHER_FONT_REGULAR_EN), WEATHER_FONT_SIZE_30);
	InitFont(m_weatherFontDesc, pDc, WEATHER_FONT_REGULAR, WEATHER_FONT_SIZE_25);
}

void CGuardianDlg::Clear()
{
	DeleteObject(m_alarmFont1);
	DeleteObject(m_alarmFont2);
	DeleteObject(m_alarmFont3);
	DeleteObject(m_alarmFont4);
	DeleteObject(m_alarmFont5);
	DeleteObject(m_alarmFont6);

	DeleteObject(m_normalFont1);
	DeleteObject(m_normalFont2);
	DeleteObject(m_normalFont3);

	DeleteObject(m_weatherFontTitle);
	DeleteObject(m_weatherFontValue);
	DeleteObject(m_weatherFontTemp);
	DeleteObject(m_weatherFontUnit);
	DeleteObject(m_weatherFontDesc);

	EraseAll();
	ClearWeaherMap();

	if (m_frManager)
	{
		delete this->m_frManager;
	}

	if (m_logo)
	{
		m_logo->Destroy();
		delete m_logo;
	}
	if (m_bgInit1)
	{
		m_bgInit1->Destroy();
		delete m_bgInit1;
	}
	if (m_bgInit2)
	{
		m_bgInit2->Destroy();
		delete m_bgInit2;
	}
	if (m_bgFail1)
	{
		m_bgFail1->Destroy();
		delete m_bgFail1;
	}
	if (m_bgFail2)
	{
		m_bgFail2->Destroy();
		delete m_bgFail2;
	}
	if (m_bgNext)
	{
		m_bgNext->Destroy();
		delete m_bgNext;
	}
	if (m_bgIdentified)
	{
		m_bgIdentified->Destroy();
		delete m_bgIdentified;
	}



	if (m_notice)
	{
		m_notice->Destroy();
		delete m_notice;
	}
	if (m_title)
	{
		m_title->Destroy();
		delete m_title;
	}
}

void CGuardianDlg::ClearWeaherMap()
{
	for (ImageMap::iterator itr = m_bgWeatherMap.begin(); itr != m_bgWeatherMap.end(); itr++) {
		if (itr->second) {
			delete itr->second;
		}
	}
	m_bgWeatherMap.clear();
}

void CGuardianDlg::InitFont(CFont& targetFont, CDC* pDc, LPCTSTR fontName, int fontSize, float fontWidth)
{
	//fontSize = static_cast<int>(static_cast<float>(fontSize)*0.6666666f);
	int lfHeight = -MulDiv(fontSize, pDc->GetDeviceCaps(LOGPIXELSY), 72);

	// TraceLog(("skpark font=%s(%d), fontSize=%d, lfHeight=%d", fontName, strlen(fontName), fontSize, lfHeight))

	LOGFONT* LogFont = new LOGFONT;
	memset(LogFont, 0x00, sizeof(LOGFONT));
	memset(LogFont->lfFaceName, 0x00, 32);
	//_tcsncpy_s(LogFont->lfFaceName, LF_FACESIZE, fontName, strlen(fontName));
	strncpy(LogFont->lfFaceName, fontName, strlen(fontName));
	LogFont->lfHeight = lfHeight;
	LogFont->lfQuality = ANTIALIASED_QUALITY;
	LogFont->lfOrientation = 0;
	LogFont->lfEscapement = 0;
	LogFont->lfItalic = 0;
	LogFont->lfUnderline = 0;
	LogFont->lfStrikeOut = 0;
	LogFont->lfWidth = 0;// lfHeight*fontWidth; //85% 수준의 장평을 준다.

	targetFont.CreateFontIndirect(LogFont);
}

void CGuardianDlg::WriteSingleLine(LPCTSTR text,
	CDC* pDc,
	CFont& targetFont,
	COLORREF color,
	int posY,
	int height,
	int align, /*=DT_CENTER*/
	int posX, /*=0*/
	int width /*=0*/
	)
{
	HDC hDc = pDc->m_hDC;

	HGDIOBJ pOld = SelectObject(hDc, targetFont);
	SetTextColor(pDc->m_hDC, color);

	posY -= 20;  // 조금 올려준다.

	if (width == 0)
		width = m_width;

	CRect rc(posX, posY, posX + width, posY + height);
	//pDc->Draw3dRect(&rc, ALARM_FG_COLOR_2, ALARM_FG_COLOR_2);
	pDc->DrawText(text, &rc, align | DT_VCENTER | DT_SINGLELINE);
	//DT_VCENTER | DT_SINGLELINE | DT_CENTER);

	//	SelectObject(hDc, pOld);
}

void CGuardianDlg::WriteMultiLine(LPCTSTR text,
	CDC* pDc,
	CFont& targetFont,
	COLORREF color,
	int posY,
	int height
	)
{
	HDC hDc = pDc->m_hDC;

	HGDIOBJ pOld = SelectObject(hDc, targetFont);
	SetTextColor(pDc->m_hDC, color);

	posY -= 20;  // 조금 올려준다.

	CRect rc(0, posY, 0 + m_width, posY + height);
	pDc->Draw3dRect(&rc, ALARM_FG_COLOR_2, ALARM_FG_COLOR_2);
	pDc->DrawText(text, &rc, DT_CENTER | DT_VCENTER);
	//DT_VCENTER | DT_SINGLELINE | DT_CENTER);

	//SelectObject(hDc, pOld);
}

bool CGuardianDlg::CheckMode()
{
	CString iniPath;
	iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;

	char buf[4096];
	memset(buf, 0x00, 4096);
	GetPrivateProfileString("UTV_brwClient2", "UsePosition", "1", buf, 4096, iniPath);

	return bool(atoi(buf));
}

UINT CGuardianDlg::WaitForWindowCreated(LPVOID pParam)
{
	CGuardianDlg* pWnd = (CGuardianDlg*)pParam;
	HWND hWnd = pWnd->GetSafeHwnd();
	if (hWnd) {
		while (::WaitForSingleObject(hWnd, 0) == WAIT_TIMEOUT) {
			TraceLog(("WaitForWindowCreated timeout"));
		}//if
		TraceLog(("WaitForWindowCreated passed"));

		::Sleep(1000);
		if (pWnd->m_isMinimize)
		{
			pWnd->ShowWindow(SW_MINIMIZE);
		}
	}
	return 0;
}

#if (_MSC_VER >= 1500)	//vs2008
void CGuardianDlg::OnTimer(UINT_PTR nIDEvent)
#else
void CGuardianDlg::OnTimer(UINT nIDEvent)
#endif
{
	time_t now = time(NULL);
	static unsigned long counter = 0;
	switch (nIDEvent)
	{
	case KIST_CHECK_TIMER:
		if ((m_bgMode == BG_MODE::NONE || m_bgMode == BG_MODE::INIT1) && AreYouReady())
		{
			TraceLog(("AreYouReady ?  Yes I'm Ready"));
			GotoPage(BG_MODE::INIT2);
		}
		break;
	case ALARM_CHECK_TIMER: 	// check alarm time
		if (m_bgMode == BG_MODE::IDENTIFIED) {
			if (m_modeCheckCounter >= 3) {
				m_modeCheckCounter = 0;
				GotoPage(BG_MODE::NEXT);
			}
			else{
				TraceLog(("skpark timer %d   ", m_modeCheckCounter));
				m_modeCheckCounter++;
			}
		}
		else if(m_bgMode == BG_MODE::FAIL1) {
			if (m_modeCheckCounter >= 3) {
				m_modeCheckCounter = 0;
				GotoPage(BG_MODE::NONE);
			}
			else{
				TraceLog(("skpark timer %d   ", m_modeCheckCounter));
				m_modeCheckCounter++;
			}
		}
		else if (m_bgMode == BG_MODE::FAIL2) {
			if (m_modeCheckCounter >= 3) {
				m_modeCheckCounter = 0;
				GotoPage(BG_MODE::NONE);
			}
			else{
				TraceLog(("skpark timer %d   ", m_modeCheckCounter));
				m_modeCheckCounter++;
			}
		}

		if (FRRetry::getInstance()->IsKISTType())
		{
			if (FRRetry::getInstance()->IsRunning())
			{
				StartAnimation();
			}
			else
			{
				StopAnimation();
				// 여기서 WAIT 모드를 해제하고 IDENTIFIDED 로 가거나 FAIL 로 가야한다.
				if (m_bgMode == BG_MODE::INIT2) {
					GotoPage(BG_MODE::FAIL2);
				}
			}
			
		}
		//if (m_config->m_is_hiden_camera)
		//{
		//	TraceLog(("hidden=%d", m_config->m_is_hiden_camera));
		//	SetTopMost(true);
		//}
		if (m_currentMainAlarmIdx >= 0)
		{
			int deleted = EraseOldMainAlarm(now);
			deleted += EraseOldRetryFaces(now);
			if (deleted > 0)
			{
				Invalidate();
			}
		}
		counter++;
		if (counter % 60 == 1) {
			// 매 1분마다  시계를 다시 그린다.
			DrawWatch();
		}


		if (counter % (5 * 60) == 5) {
			// 매 5분마다
			PostMessage(WM_OLD_FILE_DELETE, 0, 0);
		}

		//if (this->m_config->m_use_weather) {
		//	if (counter % (120 * 60) == 5) {
		//		// 매 2시간마다
		//		PostMessage(UM_WEATHER_MESSAGE, 0, 0);
		//		PostMessage(UM_AIR_MESSAGE, 0, 0);
		//	}
		//}


		//if (counter % (60) == 57)  // 매 1분 마다 CapturePicture test 를 한다.
		//{
		//	CaptureTest();
		//}
		break;
	default:
		break;
	}

	CDialog::OnTimer(nIDEvent);
}

LRESULT CGuardianDlg::OnWeatherMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_weatherProvider.GetWeatherInfo(m_hWnd);

	return 0;
}

LRESULT CGuardianDlg::OnAirMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_localeLang == "ko") {
		m_weatherProvider.GetAirInfo(m_hWnd, m_config->m_airMeasurStation);
	}

	return 0;
}

LRESULT CGuardianDlg::OnWeatherUpdateMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	Invalidate();
	return 0;
}

LRESULT CGuardianDlg::OnRefreshGuardianSettings(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	//TraceLog(("OnRefreshGuardianSettings >>> Reload Settings"));

	m_cameraThermal.SetFaceThermometry(0,0);
	m_cameraThermal.SetTemperatureCompensation(NULL,0);
	return 0;
}

LRESULT CGuardianDlg::OnEventUpdateMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	int qSize = m_eventQ.size();
	if (qSize < 1) {
		return -1;
	}

	ST_EVENT evt = m_eventQ.front();

	TraceLog(("OnEventUpdateMessage >>> Queue size[%d] eventType[%d]", qSize, evt.eventType));

	if (0 == evt.eventType) {
		ProThermometryAlarm(evt.temperature, evt.cropFace, evt.maskLevel, evt.alarmLevel);
	}
	else if (1 == evt.eventType) {
		SaveQRLog(evt.detectedType, evt.detectedCode);
	}
	else if (2 == evt.eventType) {
		ProcFaceSnapAlarm(evt.cropFace, evt.maskLevel);
	}

	m_eventQ.pop();
	return 0;
}



void CGuardianDlg::ProThermometryAlarm(float temperature, Mat cropFace, int maskLevel, int alarmLevel)
{
	TraceLog(("~~~~~~~~~~~~~~~~~~~~~~~~~~~ [ ProThermometryAlarm ] ~~~~~~~~~~~~~~~~~~~~~~~~~~~"));
	CString szPointInfo = "[ROOT]";
	char szInfoBuf[2048] = { 0 };
	//		int alarmLevel = (temperature < m_feverTmpr ? 0 : 1);
	bool savePicture = false;
	bool isMainAlarm = false;
	bool maskFace = false;
	CString strResultCode("");
	int avgAlarmLevel = alarmLevel;

	bool isFever = false;
	float faceTemperature = temperature;

	if (temperature <= 0.0 || temperature > m_config->m_max_temperature)
	{
		TraceLog(("ProThermometryAlarm >>> Camera Exception ( temperature <= 0.0 || temperature > %f )", m_config->m_max_temperature));
		return;
	}

	if (m_config->m_is_hiden_camera)
	{
		BrowserShouldShrink();
	}
	
	sprintf(szInfoBuf, "%.1f", temperature);
	szPointInfo.Format("[ROOT]\r\nTemp=%s\r\nX=%f\r\nY=%f\r\nWidth=%f\r\nHeight=%f\r\n", szInfoBuf,	0, 0, 0, 0);

	SYSTEMTIME t;
	GetLocalTime(&t);
	char chTime[128];
	memset(chTime, 0x00, 128);
	sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d_%3.3d",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	CString eventId;
	eventId.Format("%s_0", chTime);
	AlarmInfoEle* aInfo = new AlarmInfoEle(eventId);
	aInfo->Init(szInfoBuf, alarmLevel);
	aInfo->m_maskLevel = m_config->m_use_mask_check ? maskLevel : 1;

	if (m_config->m_record_all || alarmLevel == 1) {
		savePicture = true;
	}

	if ((m_config->m_use_mask_check && m_config->m_use_face) && alarmLevel == 0) {
		savePicture = true;
	}

	if (alarmLevel == 0 && m_config->m_useVoice)
	{
		TraceLog(("ProThermometryAlarm >>> voice alarm"));
		if (this->m_isVoiceRunning == false)
		{
			RunAlarmVoice(NULL);
		}
	}

	bool pushed = false;
	if (m_stat->IsUsed()) { // skpark sub alarm !!!!
		float temper = atof(aInfo->m_currentTemp);
		TraceLog(("평균값 비교방식 샘플온도 추가[%.2f]", temper));
		AddSample(temper);

		if (m_stat->IsAlarmCase(temper)) {
			TraceLog(("skpark Case 1"));
			isMainAlarm = true;
			aInfo->m_isFever = true;
			isFever = true;
			savePicture = true;
			avgAlarmLevel = 1;
			PushMainAlarm(aInfo);
			pushed = true;

			faceTemperature = temper;
		}
		else {
			if ((m_config->m_use_mask_check && m_config->m_use_face) && alarmLevel == 0) {
				TraceLog(("skpark Case 2"));
				isMainAlarm = true;
				PushMainAlarm(aInfo);
				pushed = true;
			}
		}
	}
	else {
		if (alarmLevel == 1) {
			TraceLog(("skpark Case 3"));
			isMainAlarm = true;
			aInfo->m_isFever = true;
			isFever = true;
			PushMainAlarm(aInfo);
			pushed = true;
		}
		else if ((m_config->m_use_mask_check && m_config->m_use_face) && alarmLevel == 0) {
			TraceLog(("skpark Case 4"));
			isMainAlarm = true;
			PushMainAlarm(aInfo);
			pushed = true;
		}
	}


	isFever = aInfo->m_isFever;

	if (m_config->m_record_all && pushed == false)
	{
		TraceLog(("skpark Case 5"));
		isMainAlarm = false;
		PushMainAlarm(aInfo);
	}

	// New Kist
	if (IsFRRetryType() || IsFRWatchType())
	{
		PushReservedMap(eventId);
	}


	szPointInfo.Format("%sLevel=%d\r\n", szPointInfo, avgAlarmLevel);

	TraceLog(("ProThermometryAlarm >>> alarmLevel[%d] use_mask_check[%d] isMainAlarm[%d] savePicture[%d]",
		alarmLevel, m_config->m_use_mask_check, isMainAlarm, savePicture));

	if (savePicture) { // save picture at alarm case only
		if (GetFileAttributes(m_pictureSavePath) != FILE_ATTRIBUTE_DIRECTORY) {
			CreateDirectory(m_pictureSavePath, NULL);
		}

		CString filePrefix;
		filePrefix.Format("%s\\[%s_%d]", m_pictureSavePath, chTime, 0);
		TraceLog(("ProThermometryAlarm >>> filePrefix[%s]", filePrefix));
		
		char* targetText = NULL;
		int targetLen = 0;
		CString cropFileName = "";
		if (!cropFace.empty()) {
			CImage* aImage = CropGdiImage(filePrefix, cropFace, cropFileName);

			TraceLog(("use face_api=(%d)", m_config->m_use_face));

			bool imageShouldDelete = true;
			if (!aImage->IsNull()) {
				if (isMainAlarm || m_config->m_record_all) {
					TraceLog(("skpark face MAIN Alarm !!!"));
					SetMainAlarmImage(aImage);
					imageShouldDelete = false;
				}
			}
			if (imageShouldDelete)
			{
				delete aImage;
			}
			// TraceLog(("Face API >>> cropFileName[%s] m_use_face[%d] m_faceApiType[%d] m_faceApi[%s]",	cropFileName, m_config->m_use_face, m_faceApiType, m_config->m_faceApi));

			if (!cropFileName.IsEmpty() && m_config->m_use_face && !m_config->m_faceApi.IsEmpty())
			{
				CString face_api_result = SQISoftFaceAPI(filePrefix, cropFileName);
				if (face_api_result.GetLength() > 256)
				{
					TraceLog(("skpark face_api_result (%s)", face_api_result.Mid(0, 255)));
				}
				else
				{
					TraceLog(("skpark face_api_result (%s)", face_api_result));
				}

				bool isFaceIdentified = ParseResult(isMainAlarm, maskFace, face_api_result, strResultCode);
				TraceLog(("skpark isFaceIdentified=%d, FRRetry type=%d, maskFace=%d, use_face=%d", 
					isFaceIdentified, m_config->m_faceApiType, maskFace , m_config->m_use_face));
				//if (!isFaceIdentified && IsFRRetryType())
				//{
				//	FRRetry::getInstance()->Start(this);
				//	CString eventId;
				//	eventId.Format("[%s_x]", chTime);
				//	FRRetry::getInstance()->PushEventQ(eventId, (alarmLevel == 1), faceTemperature);
				//}
			

				// kist 요구사항
				TraceLog(("IsFRWatchType()=%d", IsFRWatchType()));

				if ((isFaceIdentified == false || (m_config->m_use_mask_check && maskFace == true) ) && IsFRWatchType())
				{
					CString eventId;
					eventId.Format("[%s_x]", chTime);
					TraceLog(("StartWatch Case (%s)", eventId));

					FRRetry::getInstance()->StartWatchInstance(eventId);
					FRRetry::getInstance()->Start(this);
					FRRetry::getInstance()->PushEventQ(eventId, (alarmLevel == 1), faceTemperature);
				}


				szPointInfo += "faceResult=";
				szPointInfo += face_api_result;
				szPointInfo += "\r\n";


				//-------------------------- [add_fever] API --------------------------------//
#ifdef _FEVER_FACE_API_
				if (isFever) {
					CString fever_api_result = SQISoftFeverFaceAPI(cropFileName, faceTemperature);
					if (fever_api_result.GetLength() > 256)
					{
						TraceLog(("fever_api_result (%s)", fever_api_result.Mid(0, 255)));
					}
					else
					{
						TraceLog(("fever_api_result (%s)", fever_api_result));
					}
				}
				else {
					TraceLog(("SQISoftFeverFaceAPI skip!!! - NOT FEVER"));
				}
#endif
			} // if face recog

			TraceLog(("byAlarmLevel[%d] use_mask_check[%d] maskFace[%d]", alarmLevel, m_config->m_use_mask_check, maskFace));

			if (!this->m_config->m_not_use_encrypt ||
				(alarmLevel == 0 && m_config->m_use_mask_check && (maskFace || strResultCode != "S"))
				) {
				TraceLog(("TRY : (%s) delete file", cropFileName));
				if (!::DeleteFile(cropFileName)) {
					TraceLog(("WARN : (%s) delete failed", cropFileName));
				}
			}
		}
		else {
			TraceLog(("ProThermometryAlarm >>> cropFace empty"));
		}

		if (avgAlarmLevel < 1 && alarmLevel == 0)
		{
			if (!m_config->m_use_mask_check || maskFace)
			{
				isMainAlarm = false;
			}
		}

		if (!szPointInfo.IsEmpty() &&
			(
			m_config->m_record_all || alarmLevel == 1 ||
			(m_stat->IsUsed() && isMainAlarm) ||  //skpark 2020.07.17
			(strResultCode == "S" && alarmLevel == 0 && m_config->m_use_mask_check && !maskFace)
			)
			) {
			CString fullpath;
			fullpath.Format("%sINFO.ini", filePrefix);

			szPointInfo += m_config->ToIniString();
			szPointInfo += m_stat->ToIniString();

			TraceLog(("SaveFile=%s", szPointInfo));
			SaveFile(fullpath, (void*)(LPCTSTR)szPointInfo, szPointInfo.GetLength(), 0);
		}
		else {
			//		EraseLastMainAlarm();
		}
	}

	if (m_stat->IsUsed() || isMainAlarm || m_config->m_record_all || (strResultCode == "S" && m_config->m_use_mask_check && !maskFace))
	{
		m_stat->IsChanged();
		CWnd* statPlate = GetDlgItem(IDC_STATIC_STAT);
		CString msg = m_stat->ToString(m_localeLang);
		statPlate->SetWindowText(msg);
		//statPlate->Invalidate();
		if (isMainAlarm || m_config->m_record_all || (strResultCode == "S" && m_config->m_use_mask_check && !maskFace))
		{
			RunAlarmSound(NULL);
			Invalidate();
		}
	}
	//skpark modify end
}

void CGuardianDlg::ProcFaceSnapAlarm(Mat cropFace, int maskLevel)
{
	if (m_config->m_use_face || !m_config->m_use_mask_check || cropFace.empty()) {
		TraceLog(("ProcFaceSnapAlarm >>> return : m_config->m_use_face[%d] m_config->m_use_mask_check[%d] IsCropFace[%d]", 
			m_config->m_use_face, m_config->m_use_mask_check, cropFace.empty()));
		return;
	}

	SYSTEMTIME t;
	GetLocalTime(&t);
	char chTime[128];
	memset(chTime, 0x00, 128);
	sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d_%6.6d",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	CString filePrefix;
	filePrefix.Format("%s\\[%s_%d]", m_pictureSavePath, chTime, 0);
	if (GetFileAttributes(m_pictureSavePath) != FILE_ATTRIBUTE_DIRECTORY)	{
		CreateDirectory(m_pictureSavePath, NULL);
	}

	TraceLog(("ProcFaceSnapAlarm >>> CropGdiImage filePrefix[%s]", filePrefix));

	CString cropFileName("");
	CImage* aImage = CropGdiImage(filePrefix, cropFace, cropFileName);
	TraceLog(("TRY : (%s) delete file", cropFileName));
	if (!::DeleteFile(cropFileName)) {
		TraceLog(("WARN : (%s) delete failed", cropFileName));
	}

	//----------------------------------- ini write --------------------------------//
	CString szPointInfo = "[ROOT]";
	szPointInfo.Format("%sLevel=%d\r\n", szPointInfo, 0);

	CString fullpath;
	fullpath.Format("%sINFO.ini", filePrefix);
	szPointInfo += m_config->ToIniString();
	szPointInfo += m_stat->ToIniString();
	TraceLog(("SaveFile=%s", szPointInfo));
	SaveFile(fullpath, (void*)(LPCTSTR)szPointInfo, szPointInfo.GetLength(), 0);
	//-----------------------------------------------------------------------------//

	CString eventId;
	eventId.Format("%s_%d", chTime, 0);
	AlarmInfoEle* aInfo = new AlarmInfoEle(eventId);
	aInfo->Init("", -1);
	aInfo->m_cxImage = aImage;
	aInfo->m_maskLevel = maskLevel;
	PushMainAlarm(aInfo);

	TraceLog(("ProcFaceSnapAlarm >>> No Mask Sound!!!"));
	Invalidate();
}

void CGuardianDlg::SaveQRLog(CString detectedType, CString detectedCode)
{
	char czHostName[128] = { 0, };
	gethostname(czHostName, 128);

	CHttpClient  http;
	std::string strResponse;
	std::string apiUrl = "http://b-49.co.kr/qrcode/log";
	apiUrl += "?data_type=QRCODE";
	apiUrl += ("&code=" + detectedCode);
	apiUrl += ("&kiosk_no=" + (CString)czHostName);
	TraceLog(("QR >>> Request [%s]", apiUrl));

	int retCode = http.Get(AnsiToUTF8(apiUrl.c_str()), strResponse);
	if (CURLE_FAILED_INIT == retCode) {
		TraceLog(("QR Request Fail >>> Code[%d] Response[%s]", retCode, strResponse));
	}
	else {
		strResponse = UTF8ToANSIString(strResponse);
		TraceLog(("QR >>> Response [%s]", strResponse));

		cJSON* json = cJSON_Parse(strResponse.c_str());
		if (json != NULL) {
			CString result, resultMsg, userName, userPhone;

			cJSON* jsonResult = cJSON_GetObjectItem(json, "result");
			if (jsonResult != NULL) {
				result = jsonResult->valuestring;
				if (result == "N") {
					cJSON* jsonMsg = cJSON_GetObjectItem(json, "resultMsg");
					if (jsonMsg != NULL) {
						resultMsg = jsonMsg->valuestring;

						CMsgPopupDlg* pDlg = new CMsgPopupDlg;
						pDlg->Create(IDD_POP_MSG);
						pDlg->GetDlgItem(IDC_STATIC_MSG)->SetWindowText(resultMsg);
						pDlg->ShowWindow(SW_SHOW);
					}
				}
				else {
					cJSON* jsonName = cJSON_GetObjectItem(json, "userName");
					if (jsonName != NULL) {
						userName = jsonName->valuestring;

						cJSON* jsonPhone = cJSON_GetObjectItem(json, "userPhone");
						if (jsonPhone != NULL) {
							userPhone = jsonPhone->valuestring;
							//	TraceLog(("QR >>> Name[%s] Phone[%s]", userName, userPhone));
						}

						CMsgPopupDlg* pDlg = new CMsgPopupDlg;
						pDlg->Create(IDD_POP_MSG);
						pDlg->GetDlgItem(IDC_STATIC_MSG)->SetWindowText(userName + "(" + userPhone + ")님이 확인되었습니다.");
						pDlg->ShowWindow(SW_SHOW);
					}
				}
			}
		}
		else {
			TraceLog(("QR >>> HTML이 리턴된 경우"));
		}
		cJSON_Delete(json);
	}
}


//----------------------   skpark add start ----------------------------//
CString CGuardianDlg::SQISoftFaceAPI(LPCTSTR filePrefix, LPCTSTR cropFileName)
{
	//CHttpClient httpClient;
	//string strRetData;
	//string params = "image=@/";
	//params += filepath;
	//TraceLog(("skpark face params=%s", params.c_str()))

	//httpClient.Posts("http://54.180.40.71:8080/face/REST/b49/identify_bigFace",  params, strRetData);
	//strRetData = UTF8ToANSIString(strRetData);
	//TraceLog(("skpark face api=%s", strRetData.c_str()));
	//return strRetData.c_str();

	CString params = "--location --request POST ";
	params += m_config->m_faceApi;
	params += " --form image=@";
	params += cropFileName;

	TraceLog(("skpark face curl %s", params));

	std::string strRetData = RunCLI(NULL, "curl.exe", params);
	strRetData = UTF8ToANSIString(strRetData);
	if (strRetData.size() > 256)
	{
		TraceLog(("skpark face api=%s", strRetData.substr(0, 255).c_str()));
	}
	else
	{
		TraceLog(("skpark face api=%s", strRetData.c_str()));
	}

	return strRetData.c_str();
}

bool CGuardianDlg::SQIRetryFaceAPI(LPCTSTR eventId, LPCTSTR filePrefix, CString& strRetData)
{
	TraceLog(("SQIRetryFaceAPI(%s)", filePrefix));
	CString dumpFileName[3];

	for (int i = 0; i < 3; i++)
	{
		dumpFileName[i].Format("%sdump[%d].jpg", filePrefix, i);
		if (!GetScreenShot(dumpFileName[i]))
		{
			TraceLog(("GetScreenShot(%s) failed", dumpFileName[i]));
			return false;
		}
		::Sleep(300);
	}
	CString params = "--location --request POST ";
	params += m_config->m_faceApi;
	params += "_allfaces";
	params += " --form event_id=";
	params += eventId;
	params += " --form kiosk_type=";
	params += (m_config->m_isWide ? "B" : "A");
	params += " --form image_1=@";
	params += dumpFileName[0];
	params += " --form image_2=@";
	params += dumpFileName[1];
	params += " --form image_3=@";
	params += dumpFileName[2];

	TraceLog(("skpark retry curl %s", params));

	strRetData = RunCLI(NULL, "curl.exe", params);
	strRetData = UTF8ToANSIString(strRetData).c_str();
	TraceLog(("skpark face retry api=%s", strRetData));

	return true;
}

// 발열자등록
CString CGuardianDlg::SQISoftFeverFaceAPI(LPCTSTR cropFileName, float faceTemperature)
{
	// http://192.168.11.63:10049/face/REST/b49/add_fever

	CString api = m_config->m_faceApi;
	int n = api.Find("b49");
	api = (api.Mid(0, n) + "b49/add_fever");

	CString temperature;
	temperature.Format("%.1f", faceTemperature);

	TraceLog(("SQISoftFeverFaceAPI[%s] >>> file[%s] temperature[%s]", api, cropFileName, temperature));

	CString params = "--location --request POST ";
	params += api;
	params += " --form image=@";
	params += cropFileName;
	params += " --form temperature=";
	params += temperature;

	TraceLog(("SQISoftFeverFaceAPI curl %s", params));

	std::string strRetData = RunCLI(NULL, "curl.exe", params);
	strRetData = UTF8ToANSIString(strRetData);
	if (strRetData.size() > 256) {
		TraceLog(("SQISoftFeverFaceAPI result[%s]", strRetData.substr(0, 255).c_str()));
	}
	else {
		TraceLog(("SQISoftFeverFaceAPI result[%s]", strRetData.c_str()));
	}

	return strRetData.c_str();
}

bool CGuardianDlg::GetScreenShot(LPCTSTR filename)
{
	CWindowCapture winCapture;
	try {
		int left = (m_videoLeft < 0 ? 0 : m_videoLeft);
		int top = m_videoTop;
		int right = left + (m_videoWidth > m_width ? m_width : m_videoWidth);
		int bottom = top + m_videoHeight;

		CRect rect(left, top, right, bottom);
		winCapture.OnCaptureRect(CWindowCapture::CAP_JPG, filename, rect);
		TraceLog(("skpark (%s, %d,%d,%d,%d", filename, left, top, right, bottom));
	}
	catch (...) {
		return false;
	}
	return true;
}

bool CGuardianDlg::GetThermalScreenShot(LPCTSTR filename)
{
	CWindowCapture winCapture;
	try {
		//int left = this->m_cameraNormal.m_thermalX;
		//int top = this->m_cameraNormal.m_thermalY;
		//int right = this->m_cameraNormal.m_thermalX + this->m_cameraNormal.m_thermalWidth;
		//int bottom = this->m_cameraNormal.m_thermalY + this->m_cameraNormal.m_thermalHeight;
		int left = 0;
		int top = TITLE_AREA_HEIGHT;
		int right = 1080;
		int bottom = m_videoEnd - 100;

		CRect rect(left, top, right, bottom);
		winCapture.OnCaptureRect(CWindowCapture::CAP_JPG, filename, rect);
		TraceLog(("skpark (%s, %d,%d,%d,%d", filename, left, top, right, bottom));
	}
	catch (...) {
		return false;
	}
	return true;
}

CString CGuardianDlg::RunCLI(LPCTSTR path, LPCTSTR command, LPCTSTR parameter)
{
	CString strPath;
	if (path == NULL)
	{
		TCHAR szWin32Path[MAX_PATH] = { 0, };
		::SHGetSpecialFolderPath(NULL, szWin32Path, CSIDL_SYSTEM, FALSE);
		strPath = szWin32Path;
	}
	else
	{
		strPath = path;
	}
	strPath += "\\";
	strPath += command;

	TCHAR szParam[MAX_PATH * 4];
	sprintf(szParam, _T("%s %s"), strPath, parameter);

	TraceLog(("skpark face %s", szParam));

	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;
	TCHAR szOutputFilePath[MAX_PATH];
	lstrcpy(szOutputFilePath, TEXT("stdout.txt"));

	::DeleteFileA(szOutputFilePath);
	HANDLE hFile = CreateFile(szOutputFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = stdin;
	si.hStdOutput = hFile;

	PROCESS_INFORMATION pi;

	if (!CreateProcess((LPSTR)(LPCTSTR)strPath
		, (LPSTR)(LPCTSTR)szParam
		, NULL
		, NULL
		, TRUE
		, CREATE_NO_WINDOW
		, NULL
		, NULL
		, &si
		, &pi
	))
	{
		DWORD dwErrNo = GetLastError();

		LPVOID lpMsgBuf = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
			| FORMAT_MESSAGE_IGNORE_INSERTS
			| FORMAT_MESSAGE_FROM_SYSTEM
			, NULL
			, dwErrNo
			, 0
			, (LPTSTR)&lpMsgBuf
			, 0
			, NULL);

		CString strErrMsg;
		if (!lpMsgBuf)
		{
			strErrMsg.Format(_T("Unknow Error - CreateProcess [%d]"), dwErrNo);
		}
		else
		{
			strErrMsg.Format(_T("%s [%d]"), (LPCTSTR)lpMsgBuf, dwErrNo);
		}

		LocalFree(lpMsgBuf);
		TraceLog(("skpark face error : %s", strErrMsg));
		return "0 ";
	}

	int counter = 0;
	while (::WaitForSingleObject(pi.hProcess, 0))
	{
		//if (!m_bRunning)
		if (counter > 5)
		{
			TraceLog(("Createprocess break!!!"));
			TerminateProcess(pi.hProcess, 0);
			::WaitForSingleObject(pi.hProcess, 2000);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			return "0 ";
		}
		counter++;
		ProcessWindowMessage();
		Sleep(500);
	}
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	CloseHandle(hFile);

	CString msg = "";
	FILE* fp = fopen(szOutputFilePath, "r");
	if (fp != NULL)
	{
		char buf[1024 + 1];
		memset(buf, 0x00, sizeof(buf));
		while (fgets(buf, 1024, fp) != 0)
		{
			if (strlen(buf) > 0)
			{
				msg += buf;
				//TraceLog(("skpark face buf=%s", buf))
			}
			memset(buf, 0x00, sizeof(buf));
		}
		fclose(fp);
	}
	else
	{
		TraceLog(("skpark face error : %s file read error", szOutputFilePath));
	}
	return msg;
}

void CGuardianDlg::AddSample(float temper)
{
	m_cs.Lock();
	TraceLog(("AddSample(%f)", temper));
	m_stat->AddSample(temper);
	m_cs.Unlock();
}

CImage* CGuardianDlg::Mat2CImage(cv::Mat* mat)
{
	if (!mat || mat->empty())
		return NULL;

	int nBPP = mat->channels() * 8;
	CImage* pImage = new CImage();
	pImage->Destroy();
	pImage->Create(mat->cols, mat->rows, nBPP);
	if (nBPP == 8) {
		static RGBQUAD pRGB[256];
		for (int i = 0; i < 256; i++) {
			pRGB[i].rgbBlue = pRGB[i].rgbGreen = pRGB[i].rgbRed = i;
		}
		pImage->SetColorTable(0, 256, pRGB);
	}

	uchar* psrc = mat->data;
	uchar* pdst = (uchar*)pImage->GetBits();
	int imgPitch = pImage->GetPitch();
	for (int y = 0; y < mat->rows; y++) {
		memcpy(pdst, psrc, mat->cols * mat->channels());	// mat->step is incorrect for those images created by roi (sub-images!)
		psrc += mat->step;
		pdst += imgPitch;
	}
	return pImage;
}

CImage* CGuardianDlg::CropGdiImage(LPCTSTR filePrefix, Mat cropFace, CString& strMediaFullPath)
{
	strMediaFullPath = "";
	TraceLog(("CropGdiImage : Prefix[%s] MediaFullPath[%s]", filePrefix, strMediaFullPath));

	CImage* pImage = Mat2CImage(&cropFace);
	if (NULL == pImage)
		return pImage;

	SaveImage(pImage, filePrefix, strMediaFullPath);
	return pImage;

	/*
	TraceLog(("m_not_use_encrypt=%d", this->m_config->m_not_use_encrypt));

	strMediaFullPath.Format(_T("%sPICT.jpg"), filePrefix);
	if (pImage->Save(strMediaFullPath) < 0) {
		TraceLog(("Save %s file failed", strMediaFullPath));
		return pImage;
	}

	if (!m_config->m_not_use_encrypt) {
		//--- TEST  -----------------------------------------------------------
		//CString strTestFile;
		//strTestFile.Format(_T("%sPICT_TEST.jpg"), filePrefix);
		//::CopyFile(strMediaFullPath, strTestFile, true);
		//--------------------------------------------------------------------

		CString strJpg = strMediaFullPath;
		strMediaFullPath.Format("%sPICT.tmp", filePrefix);
		::MoveFile(strJpg, strMediaFullPath);

		CString decryptedPath;
		decryptedPath.Format("%sPICT.jpg", filePrefix);
		if (!ciAes256Util::EncryptFile(strMediaFullPath, decryptedPath, false)) {
			TraceLog(("WARN : Encrypt Failed"));
		}
	}
	TraceLog(("Save %s file succeed", strMediaFullPath));
	return pImage;
	*/
}

/*
CImage* CGuardianDlg::CropGdiImage(LPCTSTR filePrefix, BYTE * buffer, DWORD size, CString& strMediaFullPath)
{
	strMediaFullPath = "";
	TraceLog(("skpark face : m_faceX,Y=%f", m_faceX, m_faceY));

	//int left	=	static_cast<int>(m_faceX*m_thermalWidth + m_thermalX - 50.0f);
	//int top		=	static_cast<int>(m_faceY*m_thermalHeight + m_thermalY - 50.0f);
	//int width = static_cast<int>(m_faceWidth*m_thermalWidth + 100.0f);
	//int height	=	static_cast<int>(m_faceHeight*m_thermalHeight + 100.0f);

	//TraceLog(("skpark face video : %d,%d,%d,%d", m_videoWidth, m_videoHeight, m_letterLeft, m_videoTop));

	int left = static_cast<int>(m_faceX*STREAM_WIDTH - 80.0f);
	int top = static_cast<int>(m_faceY*STREAM_HEIGHT - 80.0f);
	int width = static_cast<int>(m_faceWidth*STREAM_WIDTH + 160.0f);
	int height = static_cast<int>(m_faceHeight*STREAM_HEIGHT + 160.0f);

	TraceLog(("skpark face : %d,%d,%d,%d", left, top, width, height));

	// int nImgType = CxImage::GetTypeIdFromName("jpg");
	CImage* aImage = new CImage();
	if (!Bytes2Image(buffer, size, *aImage)) {
		TraceLog(("CropGdiImage >>> Bytes2Image Fail"));
	}

	TraceLog(("skpark face CropGdiImage(%s, rc(%d,%d,%d,%d))", filePrefix, left, top, width, height));

	if (!aImage->Crop(left, top, width + left, height + top))
	{
		TraceLog(("%s crop failed(%d,%d,%d,%d)", filePrefix, left, top, width, height));
		return aImage;
	}

	SaveImage(aImage, filePrefix, strMediaFullPath);
	return aImage;
}
*/

bool CGuardianDlg::SaveImage(CImage* pImage, LPCTSTR filePrefix, CString& strMediaFullPath)
{
	TraceLog(("skpark m_not_use_encrypt=%d", this->m_config->m_not_use_encrypt));

	strMediaFullPath.Format(_T("%sPICT.jpg"), filePrefix);
	if (pImage->Save(strMediaFullPath) < 0) {
		TraceLog(("Save %s file failed", strMediaFullPath));
		return pImage;
	}

	if (!m_config->m_not_use_encrypt) {
		/*--- TEST  -----------------------------------------------------------
		CString strTestFile;
		strTestFile.Format(_T("%sPICT_TEST.jpg"), filePrefix);
		::CopyFile(strMediaFullPath, strTestFile, true);
		//--------------------------------------------------------------------*/

		CString strJpg = strMediaFullPath;
		strMediaFullPath.Format("%sPICT.tmp", filePrefix);
		::MoveFile(strJpg, strMediaFullPath);

		CString decryptedPath;
		decryptedPath.Format("%sPICT.jpg", filePrefix);
		if (!ciAes256Util::EncryptFile(strMediaFullPath, decryptedPath, false)) {
			TraceLog(("WARN : Encrypt Failed"));
		}
	}
	TraceLog(("Save %s file succeed", strMediaFullPath));
	return true;

	/*
	if (this->m_config->m_not_use_encrypt)
	{
		strMediaFullPath.Format("%sPICT.jpg", filePrefix);
		if (!aImage->Save(strMediaFullPath))
		{
			TraceLog(("Save %s file failed", strMediaFullPath));
			return false;
		}
	}
	else
	{
		strMediaFullPath.Format("%sPICT.tmp", filePrefix);
		if (!aImage->Save(strMediaFullPath))
		{
			TraceLog(("Save %s file failed", strMediaFullPath));
			return false;
		}

		CString decryptedPath;
		decryptedPath.Format("%sPICT.jpg", filePrefix);

		if (!ciAes256Util::EncryptFile(strMediaFullPath, decryptedPath, false))
		{
			TraceLog(("WARN : Encrypt Failed"));
		}
	}
	TraceLog(("Save %s file succeed", strMediaFullPath));
	return true;
	*/
}

void CGuardianDlg::PushMainAlarm(AlarmInfoEle* ele)
{
	m_cs.Lock();
	++m_currentMainAlarmIdx;
	m_mainAlarmInfoMap[m_currentMainAlarmIdx] = ele;
	TraceLog(("skparkAPI PushMainAlarm(%s, %d, %s,%lu)", ele->m_eventId, m_currentMainAlarmIdx, ele->m_currentTemp, ele->m_currentTempTime));
	m_cs.Unlock();
}

int CGuardianDlg::EraseOldMainAlarm(time_t now)
{
	//	TraceLog((" ================================== EraseOldMainAlarm =================================="));
	int retval = 0;
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr;
	for (itr = m_mainAlarmInfoMap.begin(); itr != m_mainAlarmInfoMap.end();) {
		AlarmInfoEle* aInfo = itr->second;
		if (aInfo->m_currentTempTime > 0 && now - aInfo->m_currentTempTime > m_config->m_alarmValidSec) {
			delete aInfo;
			TraceLog(("skparkAPI EraseOldMainAlarm[%d] erased >>> m_currentTempTime[%d] m_alarmValidSec[%d]",
				itr->first, now - aInfo->m_currentTempTime, m_config->m_alarmValidSec))
				m_mainAlarmInfoMap.erase(itr++);
			retval++;
		}
		else {
			++itr;
		}
	}
	m_cs.Unlock();
	return retval;
}

bool CGuardianDlg::ExistFeverAlarm()
{
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr;
	for (itr = m_mainAlarmInfoMap.begin(); itr != m_mainAlarmInfoMap.end();)
	{
		AlarmInfoEle* aInfo = itr->second;
		if (aInfo->m_isFever)	{
			m_cs.Unlock();
			TraceLog(("skparkAPI ExistFeverAlarm(%s, %s,%d,%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel));
			return TRUE;
		}
		else {
			++itr;
		}
	}
	m_cs.Unlock();
	return FALSE;
}

int CGuardianDlg::EraseOldRetryFaces(time_t now)
{
	int retval = 0;
	m_retry_cs.Lock();
	DRAW_FACE_MAP::iterator itr;
	for (itr = m_retrylFaces.begin(); itr != m_retrylFaces.end();)
	{
		AlarmInfoEle* aInfo = itr->second;
		if (aInfo->m_currentTempTime > 0 && now - aInfo->m_currentTempTime > m_config->m_alarmValidSec)
		{
			delete aInfo;
			m_retrylFaces.erase(itr++);
			retval++;
		}
		else{
			++itr;
		}
	}
	m_retry_cs.Unlock();
	return retval;
}

void CGuardianDlg::EraseAll()
{
	TraceLog(("EraseAll"));
	int retval = 0;
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr;
	for (itr = m_mainAlarmInfoMap.begin(); itr != m_mainAlarmInfoMap.end(); itr++) {
		AlarmInfoEle* aInfo = itr->second;
		TraceLog(("skparkAPI EraseAlarm(%s, %s, mask=%d, alarm=%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel))

		if (aInfo) {
			delete aInfo;
		}
	}
	m_mainAlarmInfoMap.clear();
	m_cs.Unlock();
}

void CGuardianDlg::SetMainAlarmInfo(LPCTSTR szName, LPCTSTR szGrade, LPCTSTR szRoom)
{
	//	TraceLog(("SetMainAlarmInfo(%s)=%d  시작", szName, m_currentMainAlarmIdx));
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr = m_mainAlarmInfoMap.find(m_currentMainAlarmIdx);
	if (itr != m_mainAlarmInfoMap.end()) {
		AlarmInfoEle* aInfo = itr->second;
		aInfo->m_humanName = szName;
		aInfo->m_grade = szGrade;
		aInfo->m_room = szRoom;
		TraceLog(("skparkAPI SetMainAlarmInfo(%s, %s, mask=%d, alarm=%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel))
	}
	m_cs.Unlock();
	//	TraceLog(("SetMainAlarmInfo(%s)=%d 종료", szName, m_currentMainAlarmIdx));
}

void CGuardianDlg::SetMainAlarmImage(CImage* pImage)
{
	//	TraceLog(("SetMainAlarmImage(%d)", m_currentMainAlarmIdx));
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr = m_mainAlarmInfoMap.find(m_currentMainAlarmIdx);
	if (itr != m_mainAlarmInfoMap.end()) {
		AlarmInfoEle* aInfo = itr->second;
		if (aInfo->m_cxImage) {
			TraceLog(("%d has old image it's wierd", itr->first))
			delete aInfo->m_cxImage;
		}
		aInfo->m_cxImage = pImage;
		TraceLog(("skparkAPI SetMainAlarmImage(%s, %s, mask=%d, alarm=%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel))

	}
	m_cs.Unlock();
}

void CGuardianDlg::SetMainAlarmCheckMask(int maskLevel)
{
	//	TraceLog(("SetMainAlarmCheckMask(%d)=%d 시작", maskLevel, m_currentMainAlarmIdx));
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr = m_mainAlarmInfoMap.find(m_currentMainAlarmIdx);
	if (itr != m_mainAlarmInfoMap.end()) {
		AlarmInfoEle* aInfo = itr->second;
		aInfo->m_maskLevel = maskLevel;
		TraceLog(("skparkAPI SetMainAlarmCheckMask(%s, %s,%d,%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel));
	}
	m_cs.Unlock();
	//	TraceLog(("SetMainAlarmCheckMask(%d)=%d 종료", maskLevel, m_currentMainAlarmIdx));
}

void CGuardianDlg::EraseLastMainAlarm()
{
	m_cs.Lock();

	HUMAN_INFOMAP::iterator itr = m_mainAlarmInfoMap.find(m_currentMainAlarmIdx);
	if (itr != m_mainAlarmInfoMap.end()) {
		AlarmInfoEle* aInfo = itr->second;
		delete aInfo;
		m_mainAlarmInfoMap.erase(itr);
		TraceLog(("skparkAPI EraseLastMainAlarm(%s,%s,%d,%d)", aInfo->m_eventId, aInfo->m_humanName, aInfo->m_maskLevel, aInfo->m_alarmLevel));

		m_currentMainAlarmIdx--;
	}
	m_cs.Unlock();
}

bool CGuardianDlg::GetLastMainAlarmInfo(CString& outTemperature, CString& outName, bool& isFever, int& maskLevel)
{
	isFever = false;
	maskLevel = -1;
	m_cs.Lock();
	TraceLog(("skparkAPI GetLastMainAlarmInfo(%d)", m_currentMainAlarmIdx));

	HUMAN_INFOMAP::iterator itr;
	for (int i = m_currentMainAlarmIdx; i >= 0; i--) {
		itr = m_mainAlarmInfoMap.find(i);
		if (itr != m_mainAlarmInfoMap.end()) {
			AlarmInfoEle* aInfo = itr->second;
			outName = aInfo->m_humanName;
			outTemperature = aInfo->m_currentTemp;
			isFever = aInfo->m_isFever;
			maskLevel = aInfo->m_maskLevel;

			TraceLog(("skparkAPI GetLastMainAlarmInfo(%d) >>> humanName[%s] isFever[%d] MaskLevel[%d]",
				m_currentMainAlarmIdx, aInfo->m_humanName, aInfo->m_isFever, aInfo->m_maskLevel));

			if (!isFever && m_config->m_use_mask_check && maskLevel == -1) continue;	// 마스크 미확인인 경우

			if (!isFever && maskLevel != 0) {
				m_cs.Unlock();
				return false;
			}
			m_cs.Unlock();
			return true;
		}
	}

	m_cs.Unlock();
	return false;
}

int CGuardianDlg::GetMainAlarmImageInfoList(DRAW_FACE_MAP& outMap)
{
	int retval = 0;
	m_cs.Lock();
	HUMAN_INFOMAP::iterator itr;
	for (itr = m_mainAlarmInfoMap.begin(); itr != m_mainAlarmInfoMap.end(); itr++)
	{
		AlarmInfoEle* aInfo = itr->second;
		//if (aInfo->m_cxImage && aInfo->m_cxImage->IsValid())
		if (aInfo->m_cxImage)
		{
			if (!aInfo->m_cxImage->IsNull())
			{
				TraceLog(("WARN : GetMainAlarmImageInfoList is invalid(%d)", itr->first));
			}
			else
			{
				TraceLog(("GetMainAlarmImageInfoList is valid(%d)", itr->first));
			}
			TraceLog(("skparkAPI GetMainAlarmImageInfoList outMap[%s]", aInfo->m_eventId));
			outMap[aInfo->m_eventId] = aInfo;
			//outList.push_back(aInfo);
			retval++;
		}
		else
		{
			TraceLog(("skparkAPI GetMainAlarmImageInfoList is null(%d)", itr->first));
		}
	}
	TraceLog(("skparkAPI GetMainAlarmImageInfoList(%d)", retval));
	m_cs.Unlock();
	return retval;
}

bool CGuardianDlg::KeyboardSimulator(LPCTSTR command)
{
	if (m_currentCtrlKey == command) {
		return false;
	}
	m_currentCtrlKey = command;
	TraceLog(("skpark keyboardSimulator(%s)", command));

	//static HWND brwHwnd = 0;
	//if (brwHwnd==0) brwHwnd = getWHandle("UTV_brwClient2.exe");
	HWND brwHwnd = getWHandle("UTV_brwClient2.exe");


	if (!brwHwnd) {
		TraceLog(("skpark process not found(%s)", "UTV_brwClient2.exe"));
		return true;
	}

	COPYDATASTRUCT appInfo;
	appInfo.dwData = UBC_WM_KEYBOARD_EVENT;  // 1000
	appInfo.lpData = (char*)command;
	appInfo.cbData = strlen(command) + 1;
	//::PostMessage(brwHwnd, WM_COPYDATA, NULL, (LPARAM)&appInfo);
	TraceLog(("keyboardSimulator(%s)", command));
	::SendMessage(brwHwnd, WM_COPYDATA, NULL, (LPARAM)&appInfo);

	return true;
}

bool CGuardianDlg::ToggleFullScreen()
{
	TraceLog(("ToggleFullScreen"));

	m_bFullScreen = !m_bFullScreen;

	if (m_bFullScreen)
	{
		ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX, NULL);
		MaxCamera2(m_width, m_height);
		MoveWindow(0, 0, m_width, m_height, TRUE);
	}
	else
	{
		ModifyStyle(NULL, WS_CAPTION | WS_SYSMENU | WS_MAXIMIZEBOX | WS_SIZEBOX);
		MaxCamera2(m_width / 8, m_height / 8);
		MoveWindow(m_width / 4, m_height / 8, m_width / 4, m_height / 8, TRUE);
	}
	return m_bFullScreen;
}

void CGuardianDlg::MaxCamera2(int maxWidth, int maxHeight, bool redraw /*=false*/)
{
	InitPosition();

	/*
	if (g_dlgOutput[m_iCurWndIndex].m_lPlayHandle < 0)
	{
		return;
	}
	*/

	//if (m_config->m_is_hiden_camera)
	//{
	//}
	//else
	{
		float f_maxW = static_cast<float>(maxWidth);
		float f_maxH = static_cast<float>(maxHeight);

		if (m_config->m_isWide) {
			// video size is STREAM_WIDTH x STREAM_HEIGHT
			// thermal size is 1280 x 720

			float wh_ratio = (STREAM_HEIGHT / STREAM_WIDTH);  // 가로에 대한 세로의 비율 = 0.56547619f

			float displayWidth = f_maxW;  // full!!! = 1080
			float displayHeight = displayWidth * wh_ratio; // = 1080 x  0.56547619 = 610.7

			float  w_margin = 24.0f;  // 파란선으로 부터 벽면까지의 마진

			float w_shrinkRatio = (f_maxW / STREAM_WIDTH);  // 가로크기가 원본에 비해 줄어든 비율 =  0.40178571
			//float leftOffset = ((2688.0f - 1280.0f) / 2.0f) * w_shrinkRatio;  // 왼쪽으로 이동해야할 양
			float leftOffset = (m_cameraNormal.m_thermalX)* w_shrinkRatio;  // 왼쪽으로 이동해야할 양
			float rightOffset = (STREAM_WIDTH - m_cameraNormal.m_thermalWidth - m_cameraNormal.m_thermalX) * w_shrinkRatio;  // 오른쪽 남는 영역의 길이
			float realWidth = displayWidth + leftOffset + rightOffset;  // 왼쪽으로 이동한 만큼 커져야 함.
			//float realHeight = realWidth * wh_ratio * 1.05f;  // 5% 정도 늘려서 보정
			float realHeight = realWidth * wh_ratio;  // 5% 정도 늘려서 보정

			m_videoWidth = static_cast<int>(realWidth);
			m_videoHeight = static_cast<int>(realHeight);
			m_videoTop = static_cast<int>(f_maxH * 0.1f);  // 위에서 10% 지점에서 시작
			m_videoLeft = (-1) * static_cast<int>(leftOffset);  // 왼쪽으로 left offset 만큼 이동 +  약간의 편차 조정
			m_videoEnd = m_videoTop + m_videoHeight; // -(m_config->m_is_hiden_camera ? TITLE_AREA_HEIGHT : 0);
			TraceLog(("m_videoEnd=%d", m_videoEnd));
		}
		else
		{
			float percent = 0.94;
			float ratio = STREAM_HEIGHT / STREAM_WIDTH;  // 1520/2688 = 0.5654761904761905  가로세로비
			//m_videoWidth = static_cast<int>(f_maxW * 0.84722222f);
			//m_videoHeight = static_cast<int>(f_maxH * 0.35989583f);
			//m_videoTop = static_cast<int>(f_maxH * 0.11927083f);
			//m_videoLeft = static_cast<int>((f_maxW - m_videoWidth) / 2.0f);

			m_videoWidth = static_cast<int>(f_maxW * percent);  // 1080 * 0.94 = 1,015.2 = 1015
			m_videoHeight = static_cast<int>(f_maxW * percent * ratio * 1.1);   // 1080 * 0.94 * 0.5654761904761905 * 1.1 = 602.775 = 602
			m_videoTop = static_cast<int>(f_maxH * 0.1);  // 1920 * 0.1 = 192
			m_videoLeft = static_cast<int>((f_maxW * (1-percent)) / 2.0f);  // (1080*.0.06)/2 = 32.4 = 32
			m_videoEnd = m_videoTop + m_videoHeight; // -(m_config->m_is_hiden_camera ? TITLE_AREA_HEIGHT : 0);

			TraceLog(("skpark videoSize=(%d,%d,%d,%d)", m_videoLeft, m_videoTop, m_videoWidth, m_videoHeight));

			m_letterLeft = m_videoLeft;
			m_letterWide = m_videoWidth;
		}
	}

//	TraceLog(("--------------------  m_videoTop[%d] m_videoHeight[%d]  ----------------------", m_videoTop, m_videoHeight));
	if (m_config->m_videoBelow) {
		m_videoTop = 1220;
		m_videoHeight = 700;
	}

	//if (m_config->m_is_KIST_type)
	//{
	//	TraceLog(("KIST_TYPE=%d", m_config->m_is_KIST_type));
	//	m_videoTop -= TITLE_AREA_HEIGHT;
	//}

	m_cameraNormal.MoveWindow(m_videoLeft, m_videoTop, m_videoWidth, m_videoHeight);
	m_cameraThermal.MoveWindow(m_videoLeft, m_videoTop, m_videoWidth, m_videoHeight);

	if (redraw)
	{
		Invalidate();
	}

	CRect thisRect;
	m_cameraNormal.GetWindowRect(thisRect);
	TraceLog(("Window Size=%d,%d,%d,%d", thisRect.left, thisRect.top, thisRect.right, thisRect.bottom));
}

void CGuardianDlg::KillEverything(bool killFirmwareView)
{
	TraceLog(("skpark KillEverything()"));
	if (killFirmwareView) {
		RunCLI(UBC_EXE_PATH, "kill.exe", "UBCFirmwareView");
	}
	RunCLI(UBC_EXE_PATH, "ubckill.bat", "");
}

CImage* CGuardianDlg::LoadBackgroundWeather(CString weatherIcon)
{
	ImageMap::iterator itr = m_bgWeatherMap.find(weatherIcon);
	if (itr != m_bgWeatherMap.end()) {
		return itr->second;
	}

	CString postfix = "";
	if (m_config->m_is_KIST_type)
	{
		postfix = "_upsidedown";
	}

	CString weatherFile = "bg_cloudy_partly";
	if (weatherIcon == "clear-day" || weatherIcon == "clear-night")
		weatherFile = "bg_sun";
	else if (weatherIcon == "rain")
		weatherFile = "bg_rain";
	else if (weatherIcon == "snow")
		weatherFile = "bg_snow";
	else if (weatherIcon == "sleet")
		weatherFile = "bg_sleet";
	else if (weatherIcon == "wind")
		weatherFile = "bg_wind";
	else if (weatherIcon == "fog")
		weatherFile = "bg_fog";
	else if (weatherIcon == "cloudy")
		weatherFile = "bg_cloudy";
	else if (weatherIcon == "partly-cloudy-day" || weatherIcon == "partly-cloudy-night")
		weatherFile = "bg_cloudy_partly";
	else if (weatherIcon == "hail" || weatherIcon == "thunderstorm" || weatherIcon == "tornado")
		weatherFile = "bg_thunder";
	else
		weatherFile = "bg_cloudy_partly";

	weatherFile += postfix;
	weatherFile += ".jpg";

	CString path("");
	path.Format("C:\\SQISoft\\UTV1.0\\Guardian\\icon\\%s", weatherFile);
	CImage* aImage = new CImage();
	if (E_FAIL == aImage->Load(path)) {
		TraceLog(("바탕 이미지 로딩 실패!!!"));
		delete aImage;
		return NULL;
	}

	m_bgWeatherMap[weatherIcon] = aImage;
	return aImage;
}

BOOL CGuardianDlg::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

HBRUSH CGuardianDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_STAT)  //stat plate
	{
		if (m_stat->IsUsed() && m_stat->IsValid())
		{
			pDC->SetBkColor(NORMAL_BG_COLOR);
			pDC->SetTextColor(NORMAL_FG_COLOR_2);
			pDC->SetBkMode(TRANSPARENT);
			CFont* statFont = &m_alarmFont4;
			if ("ko" != m_localeLang)
			{
				statFont = &m_alarmFont5;
			}
			CFont* oldFont = pDC->SelectObject(statFont);
			return (HBRUSH)m_statBrush;
		}
	}
	return hbr;
}

void CGuardianDlg::OnPaint()
{
	TraceLog(("DrawOnPaint()"))
	CPaintDC dc(this); // device context for painting
	
	//if (m_config->m_is_hiden_camera)
	//{
	//	CString temperature;
	//	CString humanName;
	//	bool isFever(false);
	//	int nMaskLevel(-1);

	//	bool abnormalHumanFounded = GetLastMainAlarmInfo(temperature, humanName, isFever, nMaskLevel);
	//	bool existFeverAlarm = ExistFeverAlarm();
	//	if (abnormalHumanFounded || existFeverAlarm)
	//	{
	//		DRAW_FACE_MAP humanList;
	//		int imageCount = GetMainAlarmImageInfoList(humanList);
	//		//DrawAbnormal(dcMem, humanList, imageCount, existFeverAlarm, nMaskLevel, temperature);

	//		if (existFeverAlarm) {
	//			this->KeyboardSimulator("CTRL+2");
	//		}
	//		else {
	//			if (nMaskLevel == 0) {
	//				this->KeyboardSimulator("CTRL+3");
	//			}
	//			else {
	//				TraceLog(("OnPaint >>> SKIP!!! Something wrong"));
	//				return;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		this->m_lastFaceLeft = 0;
	//		this->KeyboardSimulator(m_config->m_useQR ? "CTRL+4" : "CTRL+1");
	//		//DrawNormal(dcMem);
	//	}
	//	if (IsFRRetryType())
	//	{
	//		//DrawRetryFaces(dcMem);
	//	}
	//	return;
	//}

	// 깜빡임 방지 코드 start
	CRect rect;
	this->GetClientRect(&rect);
	CDC* pDC = (CDC*)&dc;
	CDC dcMem;
	CBitmap bitmap;
	dcMem.CreateCompatibleDC(pDC);
	bitmap.CreateCompatibleBitmap(pDC, rect.Width(), rect.Height());
	CBitmap* pOldBitmap = dcMem.SelectObject(&bitmap);
	// 깜박임 방지 코드 end

	

	//if (this->m_config->m_use_weather) {
	//	CImage* aImage = LoadBackgroundWeather(m_weatherProvider.m_weatherIcon);
	//	if (aImage == NULL || !aImage->BitBlt(dcMem.m_hDC, 0, 0)) 
	//	{
	//		TraceLog(("OnEraseBkgnd >>> 바탕 이미지 그리기 실패!!!"));
	//	}
	//}

	CString temperature;
	CString humanName;
	bool isFever(false);
	int nMaskLevel(-1);

	DRAW_FACE_MAP humanMap;
	int imageCount = GetMainAlarmImageInfoList(humanMap);
	COLORREF foreColor	= NORMAL_FG_COLOR;
	COLORREF rectColor		= NORMAL_BG_COLOR;
	bool isDrawFaceCase = false;

	bool abnormalHumanFounded = GetLastMainAlarmInfo(temperature, humanName, isFever, nMaskLevel);
	bool existFeverAlarm = ExistFeverAlarm();
	if (abnormalHumanFounded || existFeverAlarm)
	{
		//if (DrawAbnormal(dcMem, imageCount, existFeverAlarm, nMaskLevel, temperature))
		//{
		//	foreColor = ALARM_FG_COLOR_4;
		//	rectColor = ALARM_FG_COLOR_2;
		//	isDrawFaceCase = true;
		//}
	}
	else
	{


		this->m_lastFaceLeft = 0;
		//DrawNormal(dcMem);
		if (m_config->m_record_all)
		{
			TraceLog(("skpark record_all Case(%d)", imageCount));
			isDrawFaceCase = true;
		}
	}


	//if (IsFRRetryType() || IsFRWatchType())
	//{
	//	DrawRetryBackground(dcMem);
	//}

	bool hasName = false;
	CString aCurrentTemp, aMainAlarmName, aMainAlarmGrade;
	// 먼저 background 를 다 그리고 나서, face를 그려야 한다.
	if (isDrawFaceCase)
	{
		//if (DrawFaces(dcMem, humanMap, rectColor, foreColor) == 0)
		//{
		//	m_lastFaceLeft = 0;
		//}
		hasName = GetNames(humanMap, aCurrentTemp, aMainAlarmName, aMainAlarmGrade);

	}

	if (!hasName)
	{
		//DrawRetryFaces(dcMem);
		hasName = GetNames(m_retrylFaces, aCurrentTemp, aMainAlarmName, aMainAlarmGrade);
	}

	if (hasName) {
		TraceLog(("skpark hasName %s", aMainAlarmName));
		GotoPage(BG_MODE::IDENTIFIED, false);
	}

	DrawBG(dcMem, aCurrentTemp, aMainAlarmName, aMainAlarmGrade);
	//DrawLogo(dcMem);

	//깜박임 방지 코드 start
	pDC->BitBlt(rect.left, rect.top, rect.Width(), rect.Height(), &dcMem, 0, 0, SRCCOPY);
	dcMem.SelectObject(pOldBitmap);
	//깜박임 방지 코드 end
}

bool CGuardianDlg::DrawAbnormal(CDC& dc, /*DRAW_FACE_MAP& humanMap, */int imageCount, bool isFever, int nMaskLevel, CString& temperature)
{
	TraceLog(("DrawAbNormal(%d,%d,%d,%s,%s)", imageCount, isFever, nMaskLevel, temperature, m_currentCtrlKey));
	COLORREF foreColorTitle = ALARM_FG_COLOR_1;
	COLORREF foreColor = ALARM_FG_COLOR_4;
	COLORREF foreColorTemp = ALARM_FG_COLOR_2;
	COLORREF foreColorTime = ALARM_FG_COLOR_3;
	COLORREF rectColor = ALARM_FG_COLOR_2;

	BOOL	existFeverAlarm = FALSE;
	if (isFever) {
		this->KeyboardSimulator("CTRL+2");
		dc.FillSolidRect(0, 0, m_width, m_height, ALARM_BG_COLOR);
	}
	else {
		if (nMaskLevel == 0) {
			this->KeyboardSimulator("CTRL+3");
			dc.FillSolidRect(0, 0, m_width, m_height, ALARM_BG_COLOR_MASK);
			foreColor = NORMAL_BG_COLOR;
			foreColorTitle = ALARM_BG_COLOR;

			foreColorTemp = ALARM_BG_COLOR;
			foreColorTime = ALARM_BG_COLOR;
			rectColor = NORMAL_BG_COLOR;
		}
		else {
			TraceLog(("OnPaint >>> SKIP!!! Something wrong"));
			return false;
		}
	}

	SetBkMode(dc.m_hDC, TRANSPARENT);

	WriteSingleLine(m_config->m_alarmTitle, &dc, m_alarmFont1, foreColorTitle, m_titleStart + 12, 150);

	CString currentTime(_T(""));
	if (temperature != _T("")) {
		temperature += LoadStringById(m_localeLang == "ko" ? IDS_TEMPERATURE_KO : IDS_TEMPERATURE_EN, m_localeLang);
		CTime aNow = CTime::GetCurrentTime();
		currentTime.Format("%02d:%02d:%02d", aNow.GetHour(), aNow.GetMinute(), aNow.GetSecond());
	}

	if (imageCount == 0) {
		if (isFever) {
			DrawTemperature(dc, temperature, foreColorTemp, currentTime, foreColorTime, 1, 0, 0);
		}
		return false;
	}

	if (isFever) {
		DrawTemperature(dc, temperature, foreColorTemp, currentTime, foreColorTime, DT_LEFT, m_letterLeft, m_letterWide);
	}

	// DrawImages ...
	//if (DrawFaces(dc, humanMap, rectColor, foreColor) == 0)
	//{
	//	m_lastFaceLeft = 0;
	//}
	return true;
}




bool CGuardianDlg::DrawNormal(CDC& dc)
{
	TraceLog(("DrawNormal(%s-->%s)", m_currentCtrlKey, m_normalPageKey));
	this->KeyboardSimulator(m_normalPageKey);
	SetBkMode(dc.m_hDC, TRANSPARENT);

	//if (this->m_config->m_use_weather) {
	//	if (!DrawTitle(dc, 0, 0))
	//	{
	//		int weatherLeft = (int)(m_width * 0.60) + 30;
	//		if (!DrawLogo(dc))
	//		{
	//			WriteSingleLine(m_config->m_title, &dc, m_normalFont1, NORMAL_FG_COLOR_1, m_titleStart, 96, DT_LEFT, 75);
	//		}
	//		WriteSingleLine(m_config->m_subTitle, &dc, m_normalFont2, NORMAL_FG_COLOR_2, m_titleStart +  75 , 72, DT_LEFT, 75);

	//		// Weather

	//		if (m_localeLang == "ko") {
	//			WriteSingleLine("미세먼지", &dc, m_weatherFontTitle, FG_COLOR_WHITE, m_titleStart, 30, DT_CENTER, weatherLeft, 120);
	//			WriteSingleLine((LPCTSTR)GetAirGradeDesc(m_weatherProvider.m_weatherPM10Grade1h), &dc, m_weatherFontValue, GetAirGradeColor(m_weatherProvider.m_weatherPM10Grade1h), m_titleStart + 30, 35, DT_CENTER, weatherLeft, 120);
	//			WriteSingleLine("초미세먼지", &dc, m_weatherFontTitle, FG_COLOR_WHITE, m_titleStart + 80, 30, DT_CENTER, weatherLeft, 120);
	//			WriteSingleLine((LPCTSTR)GetAirGradeDesc(m_weatherProvider.m_weatherPM25Grade1h), &dc, m_weatherFontValue, GetAirGradeColor(m_weatherProvider.m_weatherPM25Grade1h), m_titleStart + 110, 35, DT_CENTER, weatherLeft, 120);
	//		}

	//		CString temper = LoadStringById(m_localeLang == "ko" ? IDS_TEMPERATURE_KO : IDS_TEMPERATURE_EN, m_localeLang);	// "°C"
	//		WriteSingleLine((LPCTSTR)temper, &dc, m_weatherFontUnit, FG_COLOR_WHITE, m_titleStart + 30, 35, DT_RIGHT, weatherLeft + 270, 50);
	//		WriteSingleLine((LPCTSTR)(m_weatherProvider.m_weatherTempr), &dc, m_weatherFontTemp, FG_COLOR_WHITE, m_titleStart + 35, 100, DT_RIGHT, weatherLeft + 150, 140);
	//		//		WriteSingleLine((LPCTSTR)GetWeatherDesc(m_weatherProvider.m_weatherIcon), &dc, m_weatherFontDesc, FG_COLOR_WHITE, 160 + g, 35, DT_CENTER, weatherLeft + 180, 110);
	//	}
	//	
	//}
	//else {
		dc.FillSolidRect(0, 0, m_width, m_height, NORMAL_BG_COLOR);
		if (!DrawTitle(dc, 0, 0))
		{
			if (!DrawLogo(dc))
			{
				WriteSingleLine(m_config->m_title, &dc, m_normalFont1, NORMAL_FG_COLOR_1, m_titleStart, 96);
			}
			WriteSingleLine(m_config->m_subTitle, &dc, m_normalFont2, NORMAL_FG_COLOR_2, m_titleStart + 89, 72);
		}
	//}

	int yOffset = 6;
	if (m_stat->IsUsed() && m_stat->IsValid())
	{
		//DrawStat(dc, 1850,120);
		yOffset = 16;
	}
	if (m_config->m_isWide)
	{
		//if (m_config->m_is_KIST_type)
		//{
		//	yOffset = TITLE_AREA_HEIGHT*(-1);
		//}
		//else
		{
			yOffset = (-22);
		}
		TraceLog(("m_videoEnd+yOffset=%d+%d", m_videoEnd, yOffset));
	}

	if (!DrawNotice(dc, 0, m_videoEnd - (m_config->m_isWide ? 60 : 35)))
	{
		TraceLog(("m_videoEnd+yOffset=%d+%d", m_videoEnd, yOffset));
		WriteSingleLine(m_config->m_noticeTitle, &dc, m_normalFont3, NORMAL_FG_COLOR_3, m_videoEnd + yOffset, 110);
	}
	TraceLog(("STAT End"));
	return true;

	//if (m_config->m_record_all)
	//{
	//	TraceLog(("skpark record_all Case"));
	//	DRAW_FACE_MAP humanMap;
	//	int imageCount = GetMainAlarmImageInfoList(humanMap);
	//	TraceLog(("skpark record_all Case(%d)", imageCount));
	//	DrawFaces(dc, humanMap, NORMAL_BG_COLOR, RGB(0xff, 0xff, 0xff));
	//}
	//PreviewPlayer* preview 
	//	= PreviewFactory::Create(CONTENTS_IMAGE, "sample.png","sample.png");
	//if (preview)
	//{
	//	CRect contents_rect;
	//	contents_rect.SetRect(83, 1106, 83 + 915, 1106 + 740);
	//	preview->Show(&dc, contents_rect, contents_rect);
	//}
	//m_wndVideo.Stop(false);
	//m_wndVideo.ShowWindow(SW_HIDE);
}

void CGuardianDlg::DrawStat(CDC& dc, int posY, int height)
{
	CString msg = m_stat->ToString(m_localeLang);
	TraceLog(("STAT=%s", msg));
	if ("ko" == m_localeLang)
		WriteSingleLine(msg, &dc, m_alarmFont4, NORMAL_FG_COLOR_2, posY, height);
	else
		WriteSingleLine(msg, &dc, m_alarmFont5, NORMAL_FG_COLOR_2, posY, height);
}

void CGuardianDlg::DrawTemperature(CDC& dc, LPCTSTR temperature, COLORREF foreColorTemp, LPCTSTR currentTime, COLORREF foreColorTime, int align, int posX, int posY)
{
	int isWideOffset = 0;
	if (!m_config->m_isWide)
	{
		WriteSingleLine(currentTime, &dc, m_alarmFont3, foreColorTime, m_videoEnd + 100, 48, DT_LEFT, m_letterLeft, m_letterWide);
	}
	else
	{
		isWideOffset = -20;
	}
	if ("ko" == m_localeLang) {
		WriteSingleLine(temperature, &dc, m_alarmFont2, foreColorTemp, m_videoEnd + isWideOffset, 96, DT_LEFT, m_letterLeft, m_letterWide);
	}
	else {
		WriteSingleLine(temperature, &dc, m_alarmFont6, foreColorTemp, m_videoEnd + isWideOffset, 96, DT_LEFT, m_letterLeft, m_letterWide);
	}
}

int CGuardianDlg::DrawFaces(CDC& dc, DRAW_FACE_MAP& humanMap,
	COLORREF rectColor, COLORREF foreColor, int leftOffset/* = 0*/)
{
	int gap = 20;
	int h = 3 * 36;
	int w = 2 * 36;
	//int top = m_videoEnd + (96 / 2) - (h / 2) - 20 - (m_config->m_is_KIST_type ? TITLE_AREA_HEIGHT : 0);
	int top = m_videoEnd + (96 / 2) - (h / 2) - 20;
	int counter = 0;
	DRAW_FACE_MAP::iterator itr;

	int len = humanMap.size();
	TraceLog(("DrawFaces=%d(Top=%d)", len, top));

	//			int persons = humanList.size();
	//			TraceLog(("감지된 얼굴수=%d", persons));
	int idx = 0;
	for (itr = humanMap.begin(); itr != humanMap.end(); itr++)
	{
		idx++;
		TraceLog(("DrawFaces=counter=%d", counter));
		AlarmInfoEle* ele = itr->second;
		if (!m_config->m_record_all && !ele->m_isFever && ele->m_maskLevel != 0) {
			TraceLog(("DrawFaces >>> not draw"));
			continue;
		}
		if (FRRetry::getInstance()->IsKISTType())
		{
			if (ele->m_score > 0.0f  && ele->m_score < m_config->m_face_score_limit)
			{
				// KIST 의 경우 Retry 결과도 점수가 높지 않으면 안나와야 한다.
				continue;
			}

		
			if (ele->m_humanName.IsEmpty()) {
				continue;
			}

			// 이름 가운데를  * 로 치환한다.
			TraceLog(("human = %s, %d grade=%s,", ele->m_humanName, ele->m_humanName.GetLength(), ele->m_grade));

			if (ele->m_humanName.GetLength() >= 6) {
				TraceLog(("human = %s*%s", ele->m_humanName.Mid(0, 2), ele->m_humanName.Mid(4)));
				CString temp = ele->m_humanName.Mid(0, 2);
				temp += "*";
				temp += ele->m_humanName.Mid(4);
				ele->m_humanName = temp;
			}
			else if (ele->m_humanName.GetLength() >= 4) {
				TraceLog(("human = %s*", ele->m_humanName.Mid(0, 2)));
				CString temp = ele->m_humanName.Mid(0, 2);
				temp += "*";
				ele->m_humanName = temp;

			}
		}




		int leftStart = static_cast<int>(m_width * 0.84722222f);
		int left = leftStart - counter*(w + gap) - leftOffset;
		if ((left) <= (m_width - leftStart - w - gap))
		{
			counter = 0;
			leftOffset = -1;
			left = leftStart - counter*(w + gap) - leftOffset;
		}
		if (leftOffset < 0)
		{
			m_lastFaceLeft = (counter + 1)*(w + gap);
		}

		CRect rc(left, top, left + w, top + h);

		TraceLog(("DrawFaces >>> left=%d idx[%d / %d]", left, idx, len));

		CRect fillRect(rc.left - 5, rc.top - 36, rc.right + 5, rc.bottom + 36);
		COLORREF bgColor = RGB(0x00, 0x00, 0x00);
		if (ele->m_isFever)
		{
			bgColor = RGB(0xCC, 0x1A, 0x1A);
		}
		else if (idx == len)
		{
			bgColor = RGB(0x1A, 0xCC, 0x37);
		}
		dc.FillSolidRect(fillRect, bgColor);

		CImage* aImage = ele->m_cxImage;
		if (!aImage)
		{
			TraceLog(("CImage is null"));
			continue;
		}

		// 이미지 보정
		Mat src, dst;
		CImage2Mat(*aImage, src);
		resize(src, dst, cv::Size(rc.Width(), rc.Height()), rc.left, rc.top, cv::INTER_LINEAR);
		CImage* pImage = Mat2CImage(&dst);
		if (pImage->Draw(dc.m_hDC, rc))	{
			TraceLog(("Image Draw succeed"));
		} else {
			TraceLog(("Image Draw fail"));
		}

		CString aCurrentTemp = ele->m_currentTemp;
		CString aMainAlarmName = ele->m_humanName;
		CString aMainAlarmGrade = ele->m_grade;
		CString aMainAlarmRoom = ele->m_room;

		CString state;
		if (aCurrentTemp != "0.0")
		{
			state.Format("%s", aCurrentTemp);
		}
		if (m_config->m_use_mask_check && !ele->m_isFever && ele->m_maskLevel == 0)
		{
			state = (m_localeLang == "ko" ? "마스크" : "Mask");
		}
		WriteSingleLine(state, &dc, m_weatherFontTitle, foreColor, top - 10, 24, DT_CENTER, left - 40, w + 80);

		TraceLog(("aMainAlarmName = %s %d", aMainAlarmName, m_config->m_show_name));
		if (!aMainAlarmName.IsEmpty())
		{
			if (m_config->m_show_name)
			{
				state.Format("%s", aMainAlarmName);
			}
			else
			{
				state = (m_localeLang == "ko" ? "확인됨" : "Identified");
			}
			WriteSingleLine(state, &dc, m_weatherFontTitle, foreColor, top + h + 20, 28, DT_CENTER, left - 40, w + 80);
			TraceLog(("show name (%s)", aMainAlarmName));
		}
		counter++;
	}
	return counter;
}

bool CGuardianDlg::DrawRetryBackground(CDC& dc)
{
	bool retval = false;
	m_retry_cs.Lock();
	TraceLog(("DrawRetryBackground"));
	DRAW_FACE_MAP::iterator itr;

	bool existFeverAlarm = false;
	int nMaskLevel = 1;
	CString temperature = "0.0";
	float fTemperature = atof(temperature);
	for (itr = m_retrylFaces.begin(); itr != m_retrylFaces.end(); itr++)
	{
		AlarmInfoEle* aEle = itr->second;
		if (aEle->m_alarmLevel > 0 && aEle->m_isFever )
		{
			existFeverAlarm = true;
			if (atof(aEle->m_currentTemp) > fTemperature)
			{
				temperature = aEle->m_currentTemp;
				fTemperature = atof(temperature);
			}
		}
		if (aEle->m_maskLevel == 0)
		{
			nMaskLevel = 0;
		}
	}
	if (existFeverAlarm || nMaskLevel == 0)
	{
		DrawAbnormal(dc, 1, existFeverAlarm, nMaskLevel, temperature);
		retval =  true;
	}
	//else
	//{
	//	DrawNormal(dc);
	//}
	m_retry_cs.Unlock();
	return retval;
}


void CGuardianDlg::DrawRetryFaces(CDC& dc)
{
	m_retry_cs.Lock();
	TraceLog(("DrawRetryFaces"));
	DRAW_FACE_MAP::iterator itr;
	DrawFaces(dc, m_retrylFaces, NORMAL_BG_COLOR, RGB(0xff, 0xff, 0xff), m_lastFaceLeft);
	for (itr = m_retrylFaces.begin(); itr != m_retrylFaces.end(); itr++)
	{
		// 여기서, PICT.jpg 로 복사하고, INFO.ini 를 제작해야 한다.
		//  INFO.ini 를 제작하고
		//  PICT.jpg 를 move하고
		//  더이상 필요없어진 frame 파일을 삭제한다.

		//1 INFO.ini 제작
		AlarmInfoEle* aEle = itr->second;
		TraceLog(("MaskLevel=%d", aEle->m_maskLevel));

		char drive[_MAX_PATH], path[_MAX_PATH], filename[_MAX_PATH], ext[_MAX_PATH];
		_splitpath(aEle->m_imgUrl, drive, path, filename, ext);

		CString fileBody = filename;
		CString fullpath;
		fullpath.Format("%s%sINFO.ini", UPLOAD_PATH, fileBody);

		CString szPointInfo;
		aEle->ToJson(szPointInfo);
		szPointInfo += m_config->ToIniString();
		szPointInfo += m_stat->ToIniString();

		TraceLog(("SaveFile(%s)=%s", fullpath, szPointInfo));
		if (!SaveFile(fullpath, (void*)(LPCTSTR)szPointInfo, szPointInfo.GetLength(), 0))
		{
			TraceLog(("SaveFile(%s) failed", fullpath));
			continue;
		}

		//2. MOVE PICT.jpg 파일
		CString src = "C:\\SQISOFT\\" + aEle->m_imgUrl;
		CString target = UPLOAD_PATH + fileBody + "PICT.jpg";

		if (m_config->m_not_use_encrypt)
		{
			TraceLog(("Move %s --> %s", src, target));
			if (::MoveFileExA(src, target, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
			{
				TraceLog(("Move %s --> %s succeed", src, target));
			}
		}
		else
		{
			TraceLog(("EncryptFile %s --> %s", src, target));
			if (!ciAes256Util::EncryptFile(src, target, false))
			{
				TraceLog(("WARN : Encrypt Failed"));
			}
			else
			{
				// 단, FACE_PATH 파일은 Thread 로 처리되므로, 여기서 지우면, 문제가 될 수 있으므로 여기서 지우지 
				// 않고 별도의 5분 타이머가 지우게 된다.
				int len = strlen(FACE_PATH);
				if (src.GetLength() > len && src.Mid(0, len) == FACE_PATH)
				{
					TraceLog(("FACE_PATH source file(%s) will not deleted at this point", src));
				}
				else
				{
					TraceLog(("DeleteFileA %s", src));
					::DeleteFileA(src);
				}
			}
		}

		//for (int i = 0; i < 3; i++)
		//{
		//	CString  name;
		//	name.Format("%s%s[i]frame.jpg", , aEle->m_eventId, i);
		//	::DeleteFileA(name);
		//}
	}
	m_retry_cs.Unlock();
	return ;
}


// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CGuardianDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

bool CGuardianDlg::ParseResult(bool& isMainAlarm, bool& maskFace, CString& face_api_result, CString& strResultCode)
{
	// api result json format
	//{
	//	"matches":
	//	[
	//		{
	//		"bounds":"954,746,210,280",
	//			"name" : "홍길동",
	//			"score" : 0.7493957,
	//			"grade" : "5",
	//			"room" : "2" 
	//		}
	//	],
	//	"mask":false,
	//	"occlusion" : 0.0374024361,
	//	"result_code":"S",
	//	"result_msg" : null
	//}

	cJSON *root = cJSON_Parse(face_api_result);
	if (root == NULL) {
		TraceLog(("json format error", face_api_result));
		maskFace = true;
		return false;
	}

	//if (!GetValueFromJson(face_api_result, "\"result_code\":", ",", true, strResultCode)) {
	//	maskFace = true;
	//	return false;
	//}

	cJSON *result_code = cJSON_GetObjectItem(root, "result_code");
	if (result_code == NULL) {
		TraceLog(("json format error 1"));
		maskFace = true;
		cJSON_Delete(root);

		return false;
	}
	strResultCode = result_code->valuestring;
	if (strResultCode != "S") {
		TraceLog(("result fail"));
		maskFace = true;
		cJSON_Delete(root);

		return false;
	}

	if (m_config->m_use_mask_check) {
		cJSON *mask = cJSON_GetObjectItem(root, "mask");
		if (mask != NULL)
		{
			bool boolMask = mask->valueint;
			cJSON *occlusion = cJSON_GetObjectItem(root, "occlusion");
			if (occlusion != NULL)
			{
				float focclusion = occlusion->valuedouble;
				if (boolMask || focclusion > 0.98)
				{
					SetMainAlarmCheckMask(1);
					TraceLog(("wear mask case"));
					maskFace = true;
				}
				else
				{
					SetMainAlarmCheckMask(0);
				}
			}
			else
			{
				TraceLog(("occlusion null case"));
				maskFace = true;
			}
		}
		else
		{
			TraceLog(("mask null case"));
			maskFace = true;
		}
		//if (GetValueFromJson(face_api_result, "\"mask\":", ",", false, strMask))
		//{
		//	if (GetValueFromJson(face_api_result, "\"occlusion\":", ",", true, strOcclusion))
		//	{
		//		TraceLog(("SetMainAlarmCheckMask >>> Mask(%s)  Occlusion(%s)", strMask, strOcclusion));
		//		if (strMask == "true" || atof(strOcclusion) > 0.98) {
		//			SetMainAlarmCheckMask(1);
		//			maskFace = true;
		//		}
		//		else {
		//			SetMainAlarmCheckMask(0);
		//		}
		//	}
		//	else
		//	{
		//		maskFace = true;
		//	}
		//}
		//else
		//{
		//	maskFace = true;
		//}
	}
	else
	{
		maskFace = true;
	}

	cJSON *jsonList = cJSON_GetObjectItem(root, "matches");
	if (jsonList == NULL) {
		TraceLog(("json format error 3"));
		cJSON_Delete(root);

		return false;
	}

	bool retval = false;
	CString strName, strScore, strGrade, strRoom;

	int dataLength = cJSON_GetArraySize(jsonList);
	for (int i = 0; i < dataLength; i++)
	{
		cJSON * data = cJSON_GetArrayItem(jsonList, i);
		if (data == NULL) {
			TraceLog(("json format error 4"));
			continue;
		}
		cJSON *name = cJSON_GetObjectItem(data, "name");
		if (name == NULL)
		{
			TraceLog(("json format error 5 name"));
			continue;
		}
		strName = name->valuestring;

		cJSON *score = cJSON_GetObjectItem(data, "score");
		if (score == NULL)
		{
			TraceLog(("json format error 6 score"));
			continue;
		}
		float fscore = score->valuedouble;
		if (fscore < m_config->m_face_score_limit)
		{
			continue;
		}
		retval = true;
		if (m_currentMainAlarmIdx < 0)
		{
			continue;
		}
		if (!isMainAlarm && !m_config->m_record_all)
		{
			continue;
		}
		cJSON *grade = cJSON_GetObjectItem(data, "grade");
		if (score != NULL)
		{
			strGrade = grade->valuestring;
		}
		cJSON *room = cJSON_GetObjectItem(data, "room");
		if (room != NULL)
		{
			strRoom = room->valuestring;
		}
		SetMainAlarmInfo(strName, strGrade, strRoom);
		TraceLog(("face_api_result >>> name(%s) grade(%s)  room(%s)", strName, strGrade, strRoom));
	}
	cJSON_Delete(root);
	return retval;
}

bool CGuardianDlg::SaveFile(LPCTSTR fullpath, void* targetText, int targetLen, int iDeviceIndex)
{
	TraceLog(("SaveFile(%s)", fullpath));
	DWORD dwWrittenBytes = 0;
	HANDLE hFile = CreateFile(fullpath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		return false;
	}

	DWORD dwRet = WriteFile(hFile, targetText, targetLen, &dwWrittenBytes, NULL);
	if (dwRet == 0 || dwWrittenBytes < targetLen) {
		DWORD dwError = GetLastError();
		return false;
	}

	CloseHandle(hFile);
	TraceLog(("SaveFile end"));
	return true;
}

BOOL CGuardianDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_F12)
		{
			TraceLog(("skpark VK_F12..."));
			showTaskbar(true);
			ToggleFullScreen();
		}
		else if (pMsg->wParam == VK_F11)
		{
			TraceLog(("skpark VK_F11..."));
			OpenIExplorer("http://192.168.11.1");
		}
		else if (pMsg->wParam == VK_DELETE)
		{
			TraceLog(("skpark VK_DELETE..."));
			KillEverything(true);
		}
		else if (pMsg->wParam == VK_F9)
		{
			TraceLog(("skpark VK_F9..."));
			KillEverything(false);
		}
		else if (pMsg->wParam == VK_F8)
		{
			OpenAuthorizer();
		}
		else if (pMsg->wParam == VK_F7)
		{
			OpenUploader();
		}
		else if (pMsg->wParam == VK_F5)
		{
			ToggleCamera();
		}
		else if (pMsg->wParam == VK_F4)
		{
			ToggleWideNarrow(this /*, m_config->m_is_KIST_type */);
		}
		else if (pMsg->wParam == VK_F3)
		{
			OpenHikvisionCameraPage();
		}
		else if (pMsg->wParam == VK_F2)
		{
			TraceLog(("skpark VK_F2..."));

			CHotKeyDlg aDlg(this);
			aDlg.SetParentInfo(this, m_stat, m_config);
			aDlg.DoModal();

			//CString helpExe = "HelpDialog.exe";
			//if (m_localeLang == "en")
			//	helpExe = "HelpDialog_en.exe";
			//else if (m_localeLang == "jp")
			//	helpExe = "HelpDialog_jp.exe";

			//unsigned long pid = getPid(helpExe);
			//if (pid)
			//{
			//	killProcess(pid);
			//}
			//createProcess(helpExe, " ", UBC_GUARDIAN_PATH);
			return FALSE;
		}

		else if (GetKeyState(VK_CONTROL) < 0 && pMsg->wParam == VK_LEFT)
		{
			return m_cameraNormal.BrightnessDown();
		}
		else if (GetKeyState(VK_CONTROL) < 0 && pMsg->wParam == VK_RIGHT)
		{
			return m_cameraNormal.BrightnessUp();
		}
		else if (GetKeyState(VK_SHIFT) < 0 && pMsg->wParam == VK_LEFT)
		{
			return m_cameraNormal.ContrastDown();
		}
		else if (GetKeyState(VK_SHIFT) < 0 && pMsg->wParam == VK_RIGHT)
		{
			return m_cameraNormal.ContrastUp();
		}
		else if (GetKeyState(VK_CONTROL) < 0 && pMsg->wParam == VK_UP)
		{
		//	m_feverTmpr += 0.1;
		//	TraceLog(("Alarm Temperature Changed >>> %f", m_feverTmpr));
		}
		else if (GetKeyState(VK_CONTROL) < 0 && pMsg->wParam == VK_DOWN)
		{
		//	m_feverTmpr -= 0.1;
		//	TraceLog(("Alarm Temperature Changed >>> %f", m_feverTmpr));
		}
		else if (pMsg->wParam == 70) // F
		{
		//	m_cameraThermal.DisplayFPS();
		//	m_cameraNormal.DisplayFPS();
			return TRUE;
		}
	}
	//if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
	//{
	//	if (g_struLocalParam.bFullScreen)
	//	{
	//		g_struLocalParam.bEnlarged    = FALSE;
	//		g_struLocalParam.bMultiScreen = FALSE;
	//		g_struLocalParam.bFullScreen  = FALSE;
	//		g_pMainDlg->GetDlgItem(IDC_COMBO_WNDNUM)->EnableWindow(TRUE);
	//		g_pMainDlg->FullScreen(g_struLocalParam.bFullScreen);
	//		if (g_struLocalParam.bFullScreen)
	//		{
	//			g_pMainDlg->ArrangeOutputs(g_pMainDlg->m_iCurWndNum);
	//		}
	//		else//multi-pic zoom
	//		{
	//			g_pMainDlg->ArrangeOutputs(g_pMainDlg->m_iCurWndNum);
	//			
	//			g_pMainDlg->GetDlgItem(IDC_STATIC_PREVIEWBG)->Invalidate(TRUE);
	//		}
	//	}
	//	else
	//	{
	//		GetDlgItem(IDC_BTN_TEST)->ShowWindow(SW_HIDE);
	//	}
	//	return TRUE;
	//}
	//if (pMsg->message==WM_KEYDOWN && pMsg->wParam==VK_RETURN)
	//{
	//	GetDlgItem(IDC_BTN_TEST)->ShowWindow(SW_SHOW);
	//	return TRUE; 
	//}
	//skpark end

	/*
	if (pMsg->message == WM_SYSKEYDOWN && pMsg->wParam == VK_F4) {	// alt + f4 : quit program
		if (m_pNormalThrd) {
			m_bStopRecv = true;
			WaitForSingleObject(m_pNormalThrd->m_hThread, 1000);
			m_pNormalThrd = NULL;
		}

		if (m_pThermalThrd) {
			m_bStopRecv = true;
			WaitForSingleObject(m_pThermalThrd->m_hThread, 1000);
			m_pThermalThrd = NULL;
		}
		if (m_pTEA) {
			m_pTEA->CloseTE();
			m_pTEA = nullptr;
		}

		if (m_pImgBuf)
			delete m_pImgBuf;
		m_pImgBuf = nullptr;

		PostQuitMessage(0);
		return TRUE;
	}
	else if (pMsg->message == WM_KEYDOWN) {
		switch (pMsg->wParam) {
		case VK_RETURN:
		case VK_ESCAPE:
			return TRUE;
		}

	}

	*/

	return CDialog::PreTranslateMessage(pMsg);
}

HWND CGuardianDlg::GetNormalCameraHwnd()
{
	return m_cameraNormal.GetSafeHwnd();
}

/*
LRESULT CGuardianDlg::OnWMProcAlarm(WPARAM wParam, LPARAM lParam)
{
	LPLOCAL_ALARM_INFO pAlarmDev = (LPLOCAL_ALARM_INFO)(wParam);
	char* pAlarmInfo = (char*)(lParam);
	int iDeviceIndex = pAlarmDev->iDeviceIndex;
	char szLan[128] = { 0 };
	char szInfoBuf[1024] = { 0 };

	//TraceLog(("############################ OnWMProcAlarm(%d,0x%x) ############################", pAlarmDev->lCommand, pAlarmDev->lCommand))

	switch (pAlarmDev->lCommand)
	{
	case COMM_UPLOAD_FACESNAP_RESULT:
	//	ProcFaceSnapAlarm(wParam, lParam);
		break;
	case COMM_THERMOMETRY_ALARM:
	//	ProThermometryAlarm(wParam, lParam);
		break;

//	case COMM_ISAPI_ALARM:
//		ProcISAPIAlarm(pAlarmDev, pAlarmInfo);
//		break;

	default:
		break;
	}


	if (pAlarmInfo != NULL)
	{
		delete[] pAlarmInfo;
		pAlarmInfo = NULL;
	}
	if (pAlarmDev != NULL)
	{
		delete pAlarmDev;
		pAlarmDev = NULL;
	}
	return NULL;
}


void CGuardianDlg::ProThermometryAlarm(WPARAM wParam, LPARAM lParam)
{
	TraceLog(("ProThermometryAlarm"));
	char szInfoBuf[2048] = { 0 };
	LPLOCAL_ALARM_INFO pAlarmDev = (LPLOCAL_ALARM_INFO)(wParam);
	char *pAlarmInfo = (char *)(lParam);
	int iDeviceIndex = pAlarmDev->iDeviceIndex;
	char szLan[128] = { 0 };
//	g_StringLanType(szLan, "侊똑괩쒸눈", "Thermometry Alarm");
	NET_DVR_THERMOMETRY_ALARM  struThermometryAlarm;
	memset(&struThermometryAlarm, 0, sizeof(struThermometryAlarm));
	memcpy(&struThermometryAlarm, pAlarmInfo, sizeof(struThermometryAlarm));
	CString szPointInfo = "[ROOT]";  //skpark add
	bool savePicture = false; //skpark add
	bool isMainAlarm = false; //skpark add
	bool maskFace = false; //skpark add
	CString strResultCode(""); //skpark add
	int avgAlarmLevel = 0;

	SYSTEMTIME t;
	GetLocalTime(&t);
	char chTime[128];
	memset(chTime, 0x00, 128);
	sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d_%3.3d",
		t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);  //skpark

	TraceLog(("%s: Channel:%d, RuleID:%d,TemperatureSuddenChangeCycle:%d; TemperatureSuddenChangeValue:%f;  ToleranceTemperature:%f, AlertFilteringTime:%d, AlarmFilteringTime:%d,ThermometryUnit:%d, PresetNo:%d, RuleTemperature:%.1f, CurrTemperature:%.1f, PTZ Info[Pan:%f, Tilt:%f, Zoom:%f], AlarmLevel:%d, \
			  				AlarmType:%d, AlarmRule:%d, RuleCalibType:%d,  Point[x:%f, y:%f], PicLen:%d, ThermalPicLen:%d, ThermalInfoLen:%d",
							szLan, struThermometryAlarm.dwChannel, struThermometryAlarm.byRuleID, struThermometryAlarm.dwTemperatureSuddenChangeCycle, struThermometryAlarm.fTemperatureSuddenChangeValue, struThermometryAlarm.fToleranceTemperature, struThermometryAlarm.dwAlertFilteringTime, struThermometryAlarm.dwAlarmFilteringTime, struThermometryAlarm.byThermometryUnit, struThermometryAlarm.wPresetNo, \
							struThermometryAlarm.fRuleTemperature, struThermometryAlarm.fCurrTemperature, \
							struThermometryAlarm.struPtzInfo.fPan, struThermometryAlarm.struPtzInfo.fTilt, struThermometryAlarm.struPtzInfo.fZoom, \
							struThermometryAlarm.byAlarmLevel, struThermometryAlarm.byAlarmType, struThermometryAlarm.byAlarmRule, struThermometryAlarm.byRuleCalibType, \
							struThermometryAlarm.struPoint.fX, struThermometryAlarm.struPoint.fY, struThermometryAlarm.dwPicLen, struThermometryAlarm.dwThermalPicLen, struThermometryAlarm.dwThermalInfoLen));

	if (0 == struThermometryAlarm.byRuleCalibType)
	{
	}
	else if (1 == struThermometryAlarm.byRuleCalibType || 2 == struThermometryAlarm.byRuleCalibType)
	{
		char szRegionInfo[512] = { 0 };
		int iPointNum = struThermometryAlarm.struRegion.dwPointNum;
		for (int i = 0; i < iPointNum; i++)
		{
			float fX = struThermometryAlarm.struRegion.struPos[i].fX;
			float fY = struThermometryAlarm.struRegion.struPos[i].fY;
			// skpark start
			if (i == 0)
			{
				m_faceX = fX;
				m_faceY = fY;
			}
			if (i == 2)
			{
				m_faceWidth = fX - m_faceX;
				m_faceHeight = fY - m_faceY;
			}
			TraceLog(("skpark face : %d:%f,%f", i, fX, fY));
			//skpark end
			sprintf(szRegionInfo, "%sX%d:%f,Y%d:%f;", szRegionInfo, i + 1, fX, i + 1, fY);
		}

		//skpark start

		sprintf(szInfoBuf, "%.1f", struThermometryAlarm.fCurrTemperature);

		//szPointInfo.Format("[ROOT]\r\nTemp=%s\r\nX=%f\r\nY=%f\r\nWidth=%f\r\nHeight=%f\r\nLevel=%d\r\n", szInfoBuf,
		//	m_faceX, m_faceY, m_faceWidth, m_faceHeight, struThermometryAlarm.byAlarmLevel);
		szPointInfo.Format("[ROOT]\r\nTemp=%s\r\nX=%f\r\nY=%f\r\nWidth=%f\r\nHeight=%f\r\n", szInfoBuf,
			m_faceX, m_faceY, m_faceWidth, m_faceHeight);

		avgAlarmLevel = struThermometryAlarm.byAlarmLevel;

		TraceLog(("skpark face %s  m_record_all=%d", szPointInfo, m_config->m_record_all));

		CString eventId;
		eventId.Format("%s_%d", chTime, iDeviceIndex);
		AlarmInfoEle* aInfo = new AlarmInfoEle(eventId);

		aInfo->Init(szInfoBuf, struThermometryAlarm.byAlarmLevel);

		if (m_config->m_record_all || struThermometryAlarm.byAlarmLevel == 1)
		{
			savePicture = true;
		}

		if (m_config->m_use_mask_check && struThermometryAlarm.byAlarmLevel == 0) {
			savePicture = true;
		}

		bool pushed = false;
		if (m_stat->IsUsed()) {  // skpark sub alarm !!!!
			float temper = atof(aInfo->m_currentTemp);
			TraceLog(("평균값 비교방식 샘플온도 추가[%.2f]", temper));
			AddSample(temper);

			if (m_stat->IsAlarmCase(temper)) {
				TraceLog(("skpark Case 1"));
				isMainAlarm = true;
				aInfo->m_isFever = true;
				savePicture = true;
				avgAlarmLevel = 1;
				PushMainAlarm(aInfo);
				pushed = true;

			}
			else {
				if (m_config->m_use_mask_check && struThermometryAlarm.byAlarmLevel == 0) {
					TraceLog(("skpark Case 2"));
					isMainAlarm = true;
					PushMainAlarm(aInfo);
					pushed = true;
				}
			}
		}
		else {
			if (struThermometryAlarm.byAlarmLevel == 1)	{
				TraceLog(("skpark Case 3"));
				isMainAlarm = true;
				aInfo->m_isFever = true;
				PushMainAlarm(aInfo);
				pushed = true;
			}
			else if (m_config->m_use_mask_check && struThermometryAlarm.byAlarmLevel == 0) {
				TraceLog(("skpark Case 4"));
				isMainAlarm = true;
				PushMainAlarm(aInfo);
				pushed = true;
			}
		}
		if (m_config->m_record_all && pushed == false)
		{
			TraceLog(("skpark Case 5"));
			isMainAlarm = false;
			PushMainAlarm(aInfo);
		}

		szPointInfo.Format("%sLevel=%d\r\n", szPointInfo, avgAlarmLevel);
	}

	TraceLog(("ProThermometryAlarm >>> byAlarmLevel[%d] use_mask_check[%d] isMainAlarm[%d] savePicture[%d]",
		struThermometryAlarm.byAlarmLevel, m_config->m_use_mask_check, isMainAlarm, savePicture));

	if (savePicture)  //skpark save picture at alarm case only
	{
		if (GetFileAttributes(m_pictureSavePath) != FILE_ATTRIBUTE_DIRECTORY)
		{
			CreateDirectory(m_pictureSavePath, NULL);
		}

		CString filePrefix;
		filePrefix.Format("%s\\[%s_%d]", m_pictureSavePath, chTime, iDeviceIndex);

		char* targetText = NULL;
		int targetLen = 0;
		CString cropFileName = "";

		if (struThermometryAlarm.dwPicLen > 0 && struThermometryAlarm.pPicBuff != NULL)
		{
			CImage* aImage = this->CropGdiImage(filePrefix,
				(BYTE*)struThermometryAlarm.pPicBuff,
				struThermometryAlarm.dwPicLen, cropFileName);

			TraceLog(("skpark use face_api=(%d)", m_config->m_use_face));

			if (aImage)
			{
				if (isMainAlarm || m_config->m_record_all)
				{
					TraceLog(("skpark face MAIN Alarm !!!"));
					SetMainAlarmImage(aImage);
				}
				//else if (struThermometryAlarm.byAlarmLevel == 0)
				//{
				//	TraceLog(("skpark face SUB Alarm !!!"));
				//	SetSubAlarmImage(aImage);
				//}
			}

			//if (!cropFileName.IsEmpty() && m_config->m_use_face && m_config->m_faceApiType == CGuardianConfig::FACE_API_SQISOFT && !m_config->m_faceApi.IsEmpty())
			if (!cropFileName.IsEmpty() && m_config->m_use_face && !m_config->m_faceApi.IsEmpty())
			{
				CString face_api_result = SQISoftFaceAPI(filePrefix, cropFileName);
				if (face_api_result.GetLength() > 256)
				{
					TraceLog(("skpark face_api_result (%s)", face_api_result.Mid(0, 255)));
				}
				else
				{
					TraceLog(("skpark face_api_result (%s)", face_api_result));
				}

				bool isFaceIdentified = ParseResult(isMainAlarm, maskFace, face_api_result, strResultCode);
				TraceLog(("skpark isFaceIdentified=%d, FRRetry type=%d", isFaceIdentified, IsFRRetryType()));
				if (!isFaceIdentified && IsFRRetryType())
					//if (IsFRRetryType())
				{
					FRRetry::getInstance()->Start(this);
					CString eventId;
					eventId.Format("[%s_x]", chTime);
					FRRetry::getInstance()->PushEventQ(eventId, (struThermometryAlarm.byAlarmLevel == 1));
				}
				szPointInfo += "faceResult=";
				szPointInfo += face_api_result;
				szPointInfo += "\r\n";

			} // if face recog

			TraceLog(("byAlarmLevel[%d] use_mask_check[%d] maskFace[%d]", struThermometryAlarm.byAlarmLevel, m_config->m_use_mask_check, maskFace))

				if (!this->m_config->m_not_use_encrypt ||
					(struThermometryAlarm.byAlarmLevel == 0 && m_config->m_use_mask_check && (maskFace || strResultCode != "S"))
					) {
					TraceLog(("TRY : (%s) delete file", cropFileName));
					if (::DeleteFile(cropFileName))
					{

					}
					else {
						TraceLog(("WARN : (%s) delete failed", cropFileName));
					}
				}
		}
		if (avgAlarmLevel < 1 && struThermometryAlarm.byAlarmLevel == 0 && m_config->m_use_mask_check && maskFace)
		{
			isMainAlarm = false;
		}

		if (!szPointInfo.IsEmpty() &&
			(
			m_config->m_record_all || struThermometryAlarm.byAlarmLevel == 1 ||
			(m_stat->IsUsed() && isMainAlarm) ||  //skpark 2020.07.17
			(strResultCode == "S" && struThermometryAlarm.byAlarmLevel == 0 && m_config->m_use_mask_check && !maskFace)
			)
			) {
			CString fullpath;
			fullpath.Format("%sINFO.ini", filePrefix);

			szPointInfo += m_config->ToIniString();
			szPointInfo += m_stat->ToIniString();

			TraceLog(("SaveFile=%s", szPointInfo))

				SaveFile(fullpath, (void*)(LPCTSTR)szPointInfo, szPointInfo.GetLength(), iDeviceIndex);
		}
		else {
			//		EraseLastMainAlarm();
		}
	}

	if (m_stat->IsUsed() || isMainAlarm || m_config->m_record_all || (strResultCode == "S" && m_config->m_use_mask_check && !maskFace))
	{
		m_stat->IsChanged();
		CWnd* statPlate = GetDlgItem(IDC_STATIC_STAT);
		CString msg = m_stat->ToString(m_localeLang);
		statPlate->SetWindowText(msg);
		//statPlate->Invalidate();
		if (isMainAlarm || m_config->m_record_all || (strResultCode == "S" && m_config->m_use_mask_check && !maskFace))
		{
			RunAlarmSound(NULL);
			Invalidate();
		}
	}
	//skpark modify end

	if (struThermometryAlarm.pPicBuff != NULL)
	{
		delete[](struThermometryAlarm.pPicBuff);
		struThermometryAlarm.pPicBuff = NULL;
	}

	if (struThermometryAlarm.pThermalPicBuff != NULL)
	{
		delete[](struThermometryAlarm.pThermalPicBuff);
		struThermometryAlarm.pThermalPicBuff = NULL;
	}

	if (struThermometryAlarm.pThermalInfoBuff != NULL)
	{
		delete[](struThermometryAlarm.pThermalInfoBuff);
		struThermometryAlarm.pThermalInfoBuff = NULL;
	}
}

void CGuardianDlg::ProcFaceSnapAlarm(WPARAM wParam, LPARAM lParam)
{
	//	TraceLog(("ProcFaceSnapAlarm"));
	char szInfoBuf[1024 * 5] = { 0 };
	LPLOCAL_ALARM_INFO pAlarmDev = (LPLOCAL_ALARM_INFO)(wParam);
	char *pAlarmInfo = (char *)(lParam);
	int iDeviceIndex = pAlarmDev->iDeviceIndex;

	NET_VCA_FACESNAP_RESULT struFaceSnapAlarm = { 0 };
	memcpy(&struFaceSnapAlarm, pAlarmInfo, sizeof(struFaceSnapAlarm));

	// 없으면:1, 있으면:2, 모르겠으면:0 
	TraceLog(("ProcFaceSnapAlarm >>> FaceScore[%d] Mask[%d] EyeGlass[%d] Hat[%d] Beard[%d] Sex[%d] Age[%d] FaceExpression[%d]",
		struFaceSnapAlarm.dwFaceScore,
		struFaceSnapAlarm.struFeature.byMask,
		struFaceSnapAlarm.struFeature.byEyeGlass,
		struFaceSnapAlarm.struFeature.byHat,
		struFaceSnapAlarm.struFeature.byBeard,
		struFaceSnapAlarm.struFeature.bySex,
		struFaceSnapAlarm.struFeature.byAge,
		struFaceSnapAlarm.struFeature.byFaceExpression      // FACE_EXPRESSION_GROUP_ENUM
		));

	if (m_config->m_use_face || (!m_config->m_use_mask_check || struFaceSnapAlarm.struFeature.byMask != 1)) {
		return;
	}

	if (struFaceSnapAlarm.dwFacePicLen > 0 && struFaceSnapAlarm.pBuffer1 != NULL && struFaceSnapAlarm.byUploadEventDataType == 0) {
		SYSTEMTIME t;
		GetLocalTime(&t);
		char chTime[128];
		memset(chTime, 0x00, 128);
		sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d_%6.6d",
			t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

		CString filePrefix;
		filePrefix.Format("%s\\[%s_%d]", m_pictureSavePath, chTime, iDeviceIndex);

		CString cropFileName = "";
//		CImage* aImage = this->CropGdiImage(filePrefix, (BYTE*)struFaceSnapAlarm.pBuffer1, struFaceSnapAlarm.dwFacePicLen, cropFileName);

		CImage* aImage = new CImage();
		if (Bytes2Image((BYTE*)struFaceSnapAlarm.pBuffer1, struFaceSnapAlarm.dwFacePicLen, *aImage)) {
			aImage->Save(filePrefix);
		}
		else {
			TraceLog(("ProcFaceSnapAlarm >>> Bytes2Image Fail"));
		}

		if (1 == struFaceSnapAlarm.struFeature.byMask) {

			if (GetFileAttributes(m_pictureSavePath) != FILE_ATTRIBUTE_DIRECTORY)	{
				CreateDirectory(m_pictureSavePath, NULL);
			}

			CString eventId;
			eventId.Format("%s_%d", chTime, iDeviceIndex);
			AlarmInfoEle* aInfo = new AlarmInfoEle(eventId);
			aInfo->Init("", -1);
			aInfo->m_cxImage = aImage;
			aInfo->m_maskLevel = (int)struFaceSnapAlarm.struFeature.byMask - 1;
			PushMainAlarm(aInfo);

			TraceLog(("ProcFaceSnapAlarm >>> No Mask Sound!!!"));
			Invalidate();
		}
	}
}

void CGuardianDlg::ProcISAPIAlarm(LPLOCAL_ALARM_INFO pAlarmDev, char* pAlarmInfo)
{
	char szLan[128] = { 0 };
//	LPLOCAL_ALARM_INFO pAlarmDev = (LPLOCAL_ALARM_INFO)(wParam);
//	char* pAlarmInfo = (char*)(lParam);
	int iDeviceIndex = pAlarmDev->iDeviceIndex;
	int iPort = pAlarmDev->wLinkPort;

	NET_DVR_ALARM_ISAPI_INFO struISAPIAlarm = { 0 };
	memcpy(&struISAPIAlarm, pAlarmInfo, sizeof(NET_DVR_ALARM_ISAPI_INFO));

	SYSTEMTIME t;
	GetLocalTime(&t);
	char chTime[128];
//	sprintf(chTime, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d%3.3d", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
	sprintf(chTime, "%2.2d%2.2d%2.2d%3.3d", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);

	if (struISAPIAlarm.pAlarmData != NULL) {
		char cFilename[256] = { 0 };
		char cExt[12] = { 0 };
		char cSubFilename[256] = { 0 };
		DWORD dwWrittenBytes = 0;

		sprintf(cFilename, "D:\\ISAPIAlarmData\\%s.txt", chTime);
		//	TraceLog(("ProcISAPIAlarm   SaveFileName[%s]", cFilename));

		if (GetFileAttributes(cFilename) != FILE_ATTRIBUTE_DIRECTORY)
		{
			CreateDirectory(cFilename, NULL);
		}

		try
		{
			CFile file;
			if (!file.Open(cFilename, CFile::modeNoTruncate | CFile::modeCreate | CFile::modeReadWrite)) {
				return;
			}
//			file.SeekToEnd();
			file.Write(struISAPIAlarm.pAlarmData, struISAPIAlarm.dwAlarmDataLen);
//			char szEnd[3] = { 0 };
//			szEnd[0] = '\r';
//			szEnd[1] = '\n';
//			file.Write(szEnd, strlen(szEnd));
			file.Close();
		}
		catch (Exception e)
		{
			TraceLog(("ISAPI Alarm Write Exception! [%s]", e.msg));
			
			char xmlText[99999] = { 0 };
			memcpy(xmlText, struISAPIAlarm.pAlarmData, struISAPIAlarm.dwAlarmDataLen);
			TraceLog(("ProcISAPIAlarm [%s]", xmlText));
		}

		// delete struISAPIAlarm.pAlarmData;
	}
}
*/

void CGuardianDlg::OnDestroy()
{
	m_hIcon = NULL;
	DeleteCriticalSection(&g_cs);
	Clear();
	showTaskbar(true);

	CloseLog();
	if (m_test_client) {
		delete m_test_client;
	}

	if (m_socket) {
		delete m_socket;
	}

	TraceLog(("OnDestroy() --- bye!!!"));
	CDialog::OnDestroy();
}


bool CGuardianDlg::LoadLogo()
{
	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;

	char buf[128];
	memset(buf, 0x00, 128);
	GetPrivateProfileString("LOGO", "width", "291", buf, 128, iniPath);
	m_logoW = atoi(buf);

	memset(buf, 0x00, 128);
	GetPrivateProfileString("LOGO", "height", "166", buf, 128, iniPath);
	m_logoH = atoi(buf);

	memset(buf, 0x00, 128);
	GetPrivateProfileString("LOGO", "logoFile", "CustomerLogo.png", buf, 128, iniPath);
	m_logoFile = buf;


	CString fullpath = UBC_GUARDIAN_PATH;
	fullpath += m_logoFile;

	if (!IsLocalExist(fullpath))
	{
		TraceLog(("LOGO file does not exist [%s]", fullpath));
		return false;
	}

	m_logo = new CImage();
	m_logo->Load(fullpath);

	return true;
}

bool CGuardianDlg::LoadNotice()
{
	CString fullpath = UBC_GUARDIAN_PATH;
	fullpath += m_noticeFile;

	if (!IsLocalExist(fullpath))
	{
		TraceLog(("Title %s file does not exist", fullpath));
		return false;
	}

	m_notice = new CImage();
	m_notice->Load(fullpath);
	return true;
}

bool CGuardianDlg::LoadTitle()
{
	CString fullpath = UBC_GUARDIAN_PATH;
	fullpath += m_titleFile;

	if (!IsLocalExist(fullpath))
	{
		TraceLog(("Title %s file does not exist", fullpath));
		return false;
	}

	m_title = new CImage();
	m_title->Load(fullpath);
	return true;
}

bool  CGuardianDlg::DrawLogo(CDC& dc)
{
	if (!m_logo || m_logo->IsNull())
	{
		TraceLog(("LOGO does not set"))
		return false;
	}

	float w = static_cast<float>(m_logo->GetWidth());
	float h = static_cast<float>(m_logo->GetHeight());
	if (w == 0 || h == 0)
	{
		TraceLog(("Invalid LOGO size width[%d] height[%d]", w, h));
		return false;
	}

	int display_w = w;
	int display_h = h;

	//int display_w = 240;
	//int display_h = static_cast<int>((h / w) * 240.0);

	int x = m_width - display_w - m_letterLeft;
	int y = m_videoTop - display_h -10;

	CRect rc(x, y, display_w + x, display_h + y);

	// 투명처리
	unsigned char * pCol = 0;
	long lw = m_logo->GetWidth();
	long lh = m_logo->GetHeight();
	for (long y = 0; y < lh; y++) {
		for (long x = 0; x < lw; x++) {
			pCol = (unsigned char *)m_logo->GetPixelAddress(x, y);
			unsigned char alpha = pCol[3];
			if (alpha != 255) {
				pCol[0] = ((pCol[0] * alpha) + 128) >> 8;
				pCol[1] = ((pCol[1] * alpha) + 128) >> 8;
				pCol[2] = ((pCol[2] * alpha) + 128) >> 8;
			}
		}
	}
	m_logo->SetHasAlphaChannel(true);

	// 출처: https://2016amie.tistory.com/entry/CImgae-png-투명값-static-Picture-MFC [AMIE]

	long ret = m_logo->Draw(dc.m_hDC, rc);
	TraceLog(("DrawLogo=%d", ret));
	return true;
}

bool CGuardianDlg::DrawTitle(CDC& dc, int x, int y)
{
	if (!m_title || m_title->IsNull())
	{
		TraceLog(("TITLE does not set"))
			return false;
	}
	int w = m_title->GetWidth();
	int h = m_title->GetHeight();
	if (w == 0 || h == 0)
	{
		TraceLog(("Invalid TITLE size"));
		return false;
	}

	CRect rc(x, y, w + x, h + y);

	// 투명처리
	//RGBQUAD  transColor = m_title->GetTransColor();
	//if (transColor.rgbBlue >= 0 && transColor.rgbGreen >= 0 && transColor.rgbRed >= 0)
	//{
	//	m_title->SetTransIndex(0);
	//	m_title->SetTransColor(transColor);
	//	m_title->AlphaStrip();
	//}
	long ret = m_title->Draw(dc.m_hDC, rc);
	TraceLog(("TITLE %d,%d,%d,%d Draw=%ld", x, y, w, h, ret));
	return true;
}

bool CGuardianDlg::DrawNotice(CDC& dc, int x, int y)
{
	if (!m_notice || m_notice->IsNull())
	{
		TraceLog(("NOTICE does not set"))
			return false;
	}
	int w = m_notice->GetWidth();
	int h = m_notice->GetHeight();
	if (w == 0 || h == 0)
	{
		TraceLog(("Invalid NOTICE size"));
		return false;
	}

	CRect rc(x, y, w + x, h + y);

	// 투명처리
	//RGBQUAD  transColor = m_notice->GetTransColor();
	//if (transColor.rgbBlue >= 0 && transColor.rgbGreen >= 0 && transColor.rgbRed >= 0)
	//{
	//	m_notice->SetTransIndex(0);
	//	m_notice->SetTransColor(transColor);
	//	m_notice->AlphaStrip();
	//}
	long ret = m_notice->Draw(dc.m_hDC, rc);
	TraceLog(("NOTICE %d,%d,%d,%d Draw=%ld", x, y, w, h, ret));
	return true;
}


void CGuardianDlg::FRRetryFileDownloadResponse(CString& errMsg)
{
	TraceLog(("FRRetryFileDownloadResponse(%s)", errMsg));
	if (errMsg == "SUCCEED")
	{
		int count = this->LoadRetryImage();
		TraceLog(("%d LoadRetryImaged", count));
		if (count > 0)
		{
			RunAlarmSound(NULL);
			this->Invalidate();
		}
	}
	return;
}

BOOL CGuardianDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	switch (pCopyDataStruct->dwData)
	{
		case WM_FRRETRY_FILE_DOWN_RESPONSE:
		{
			char* pszArray = NULL;
			int nSize = (pCopyDataStruct->cbData*sizeof(char));
			pszArray = (char*)malloc(nSize);
			memset(pszArray, 0x00, nSize);
			memcpy(pszArray, pCopyDataStruct->lpData, nSize);

			CString strArray;
			strArray.Format("%s", pszArray);
			free(pszArray);

			FRRetryFileDownloadResponse(strArray);
			break;
		}
		case UM_REFRESH_GUARDIAN_SETTINGS:
		{
			char* pszArray = NULL;
			int nSize = (pCopyDataStruct->cbData*sizeof(char));
			pszArray = (char*)malloc(nSize);
			memset(pszArray, 0x00, nSize);
			memcpy(pszArray, pCopyDataStruct->lpData, nSize);
			CString strArray;
			strArray.Format("%s", pszArray);
			free(pszArray);

			float alarmTemperature = 0.0f;
			float alertMinTemperature = 0.0f;
			CString compensationType;
			float manualCalibration = 0.0f;

			int pos = 0;
			CString token = strArray.Tokenize(",", pos);
			if (!token.IsEmpty()) 			
			{ 			
				alarmTemperature = atof(token);
			}
			token = strArray.Tokenize(",", pos);
			if (!token.IsEmpty())
			{
				alertMinTemperature = atof(token);
			}
			token = strArray.Tokenize(",", pos);
			if (!token.IsEmpty())
			{
				compensationType = token;
			}
			token = strArray.Tokenize(",", pos);
			if (!token.IsEmpty())
			{
				manualCalibration = atof(token);
			}

			TraceLog(("alarmTemperature = %.1f", alarmTemperature));
			TraceLog(("alertMinTemperature = %.1f", alertMinTemperature));
			TraceLog(("compensationType = %s", compensationType));
			TraceLog(("manualCalibration = %.1f", manualCalibration));

			bool retval1 = m_cameraThermal.SetFaceThermometry(alarmTemperature, alertMinTemperature);
			bool retval2 = m_cameraThermal.SetTemperatureCompensation(compensationType, manualCalibration);
			if (retval1 && retval2)
			{
				m_stat->WriteISAPIInfo(alarmTemperature, alertMinTemperature, compensationType, manualCalibration);
			}
			break;
		}
		case WM_BACK_TO_YOUR_POSTION_RESPONSE:
		{
				if (this->m_config->m_is_hiden_camera)  {
					m_normalPageKey = "CTRL+5";
					m_currentCtrlKey = m_normalPageKey;
				}
				break;
		}
	}//switch
	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

int CGuardianDlg::PutRetryResult(FACE_INFO_LIST* resultList, float temperature)
{
	TraceLog(("PutRetryResult()"));
	int counter = 0;
	FACE_INFO_LIST::iterator mtr;
	for (mtr = resultList->begin(); mtr != resultList->end(); mtr++)
	{
		FaceInfo* aInfo = *mtr;
		if (aInfo->isDisplayed)
		{
			continue;
		}
		if (FRRetry::getInstance()->IsKISTType())
		{
			if (aInfo->score < m_config->m_face_score_limit)
			{
				TraceLog(("founded but score(%f) is too low", aInfo->score));
				continue;
			}
		}
		aInfo->isDisplayed = true;
		AlarmInfoEle* aEle = new AlarmInfoEle(aInfo->event_id);

		int pos = aInfo->image_url.Find("C:\\Project\\ubc\\");
		if (pos >= 0)
		{
			aInfo->image_url = aInfo->image_url.Mid(pos + strlen("C:\\Project\\ubc\\"));
		}
		TraceLog(("PutRetryResult(%s)", aInfo->image_url));

		aEle->m_imgUrl = aInfo->image_url;
		aEle->m_cxImage = 0;
		//std::string imageString = ciStringUtil::base64_decode(std::string(aInfo->image_url));
		//int nImgType = CxImage::GetTypeIdFromName("jpg");
		//BYTE*		buffer = (BYTE*)imageString.c_str();
		//CxImage* aImage = new CxImage(buffer, imageString.size(), nImgType);
		//TraceLog(("Create Image Suceed()"));

		aEle->m_alarmLevel = aInfo->isFever;
		aEle->m_currentTemp.Format("%.1f", aInfo->temperature > 0.0f ? aInfo->temperature : temperature);
		aEle->m_currentTempTime = time(NULL);
		//aEle->m_cxImage = aImage;
		aEle->m_grade = aInfo->grade;
		aEle->m_humanName = aInfo->name;
		aEle->m_isFever = aInfo->isFever;
		aEle->m_maskLevel = m_config->m_use_mask_check ? aInfo->wear_mask : 1;
		TraceLog(("WearMask = %d", aEle->m_maskLevel));
		aEle->m_room = aInfo->room;
		aEle->m_score = aInfo->score;
		counter++;
		m_retry_cs.Lock();

		bool skip = false;
		if (IsFRWatchType())  // 만약 WATCHman 상황이면
		{
			DRAW_FACE_MAP::iterator itr = m_retrylFaces.find(aEle->m_eventId);
			if (itr != m_retrylFaces.end())
			{
				AlarmInfoEle* old = itr->second;
				if (aEle->SameAs(old))
				{
					TraceLog(("Same person(%s) skipped", aEle->m_humanName));
					skip = true;
				}
			}
		}
		if (!skip)
		{
			m_retrylFaces[aEle->m_eventId] = aEle;
		}
		m_retry_cs.Unlock();
	}
	return counter;
}

int CGuardianDlg::LoadRetryImage()
{
	TraceLog(("LoadRetryImage()"));
	int counter = 0;
	m_retry_cs.Lock();

	DRAW_FACE_MAP::iterator mtr;
	for (mtr = m_retrylFaces.begin(); mtr != m_retrylFaces.end(); mtr++)
	{
		AlarmInfoEle* aEle = mtr->second;
		if (FRRetry::getInstance()->IsKISTType())
		{
			if (aEle->m_score > 0.0f  && aEle->m_score < m_config->m_face_score_limit)
			{
				// KIST 의 경우 Retry 결과도 점수가 높지 않으면 안나와야 한다.
				continue;
			}
		}

		if (aEle->m_cxImage) continue;
		//int nImgType = CxImage::GetTypeIdFromName("jpg");
		CImage* aImage = new CImage();
		CString fullpath = "C:\\SQISOFT\\" + aEle->m_imgUrl;
		//if (aImage->Load(fullpath, nImgType))
		if (aImage->Load(fullpath) != E_FAIL)
		{
			TraceLog(("%s file uploaded", fullpath));
			aEle->m_cxImage = aImage;
			counter++;
		}
		else
		{
			TraceLog(("%s file uploaded fail", fullpath));
		}
	}
	m_retry_cs.Unlock();
	return counter;
}

UINT  CGuardianDlg::RunAlarmSound(LPVOID param)
{
	CString szSoundPath;
	szSoundPath.Format("%s%s", UBC_EXE_PATH, "count.wav");

	if (IsLocalExist(szSoundPath))
	{
		TraceLog(("RunAlarmSound(%s)", szSoundPath));

		//PlaySound(szSoundPath, AfxGetInstanceHandle(),  SND_ASYNC | SND_LOOP); // 무한
		//if(PlaySound(szSoundPath, AfxGetInstanceHandle(),  SND_ASYNC| 
		if (PlaySound(szSoundPath, AfxGetInstanceHandle(), SND_FILENAME | SND_ASYNC))
		{// 1회 재생
			TraceLog(("PlaySound(%s)", szSoundPath));
			//PlaySound(NULL, AfxGetInstanceHandle(),  NULL); // 정지
		}
	}
	else
	{
		TraceLog(("RunAlarmSound(%s) file not exist", szSoundPath));
	}
	return 0;
}

UINT  CGuardianDlg::RunAlarmVoice(LPVOID param)
{
	m_isVoiceRunning = true;
	CString szSoundPath;
	szSoundPath.Format("%s%s", UBC_EXE_PATH, "voice1.wav");

	if (IsLocalExist(szSoundPath))
	{
		TraceLog(("RunAlarmVoice(%s)", szSoundPath));

		//PlaySound(szSoundPath, AfxGetInstanceHandle(),  SND_ASYNC | SND_LOOP); // 무한
		//if(PlaySound(szSoundPath, AfxGetInstanceHandle(),  SND_ASYNC| 
		if (PlaySound(szSoundPath, AfxGetInstanceHandle(), SND_FILENAME | SND_ASYNC))
		{// 1회 재생
			TraceLog(("PlaySound(%s)", szSoundPath));
			//PlaySound(NULL, AfxGetInstanceHandle(),  NULL); // 정지
		}
	}
	else
	{
		TraceLog(("RunAlarmVoice(%s) file not exist", szSoundPath));
	}
	m_isVoiceRunning = false;
	return 0;
}

LRESULT CGuardianDlg::OnOldFileDelete(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	deleteOldFile(FACE_PATH,0,0,5, "*.jpg");  // 최근 5분 분량만 놔두고 다 지운다.
	return 0;
}

void CGuardianDlg::GenerateQRCode()
{
	TraceLog(("GenerateQRCode()"));
	this->KeyboardSimulator(m_normalPageKey);
}

void CGuardianDlg::SetTopMost(bool bTopMost)
{
	if (bTopMost)
	{
		TraceLog(("Player set Topmost mode"));
		BringWindowToTop();
		SetForegroundWindow();
		SetFocus();
		::SetWindowPos(this->m_hWnd, HWND_TOPMOST, -1, -1, -1, -1, SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}
	else
	{
		TraceLog(("Player set noTopmost mode"));
		::SetWindowPos(this->m_hWnd, HWND_NOTOPMOST, -1, -1, -1, -1, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW);
	}
}

bool CGuardianDlg::BrowserShouldShrink()
{
	HWND brwHwnd = getWHandle("UTV_brwClient2.exe");
	if (!brwHwnd) {
		TraceLog(("skpark process not found(%s)", "UTV_brwClient2.exe"));
		return false;
	}
	m_normalPageKey = m_config->m_useQR ? "CTRL+4" : "CTRL+1";
	TraceLog(("BrowserShouldShrink(%s)", m_normalPageKey));

	CString param;
	param.Format("%d,%s", m_config->m_alarmValidSec, m_normalPageKey);

	COPYDATASTRUCT appInfo;
	appInfo.dwData = WM_BACK_TO_YOUR_POSTION;  // 3000
	appInfo.lpData = (char*)(LPCTSTR)param;  // m_alarmValidSec 초간 shrink 상태를 유지한다.
	appInfo.cbData = param.GetLength() + 1;
	::SendMessage(brwHwnd, WM_COPYDATA, NULL, (LPARAM)&appInfo);
	return true;
}

void CGuardianDlg::KillBrowser()
{
	unsigned long pid = getPid("UTV_brwClient2.exe");
	if (pid) {
		killProcess(pid);
	}
	::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
}

void CGuardianDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.

	static time_t pre_time = 0;
	static int aCount = 0;

	if (aCount == 0){
		pre_time = time(NULL);
		aCount++;

		CDialog::OnLButtonUp(nFlags, point);
		return;
	}

	time_t now = time(NULL);
	if (now - pre_time > 1){
		aCount = 0;

		CDialog::OnLButtonUp(nFlags, point);
		return;
	}

	if (aCount >= 5){
		//ToggleFullScreen();
		aCount = 0;

		CHotKeyDlg aDlg(this);
		aDlg.SetParentInfo(this, m_stat, m_config);
		aDlg.DoModal();

		CDialog::OnLButtonUp(nFlags, point);
		return;
	}

	aCount++;
	CDialog::OnLButtonUp(nFlags, point);
}

bool CGuardianDlg::ToggleCamera()
{
	static int counter = 1;
	TraceLog(("skpark VK_F5..."));
	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;
	char buf[10];
	memset(buf, 0x00, 10);
	GetPrivateProfileString("GUARDIAN", "CAMERA", "1", buf, 10, iniPath);

	m_cameraNormal.ShowWindow(SW_HIDE);
	m_cameraThermal.ShowWindow(SW_HIDE);

	//	TraceLog((_T("[PretranslateMsg] >>> CAMERA[%d]"), atoi(buf)));

	bool isThermal = false;
	CString newId("1");
	if (atoi(buf) == 1)	{
		newId = "2";
		m_cameraID = 2;
		m_cameraThermal.ShowWindow(SW_SHOW);
		isThermal = true;
	}
	else {
		newId = "1";
		m_cameraID = 1;
		m_cameraNormal.ShowWindow(SW_SHOW);
	}
	//CString cameraName = "Camera";
	//cameraName += newId;
	counter++;

	WritePrivateProfileStringA("GUARDIAN", "CAMERA", newId, iniPath);
	//::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL);
	//AutoDblclkTreeDeviceList(cameraName);

	return isThermal;
}

void CGuardianDlg::OpenUploader()
{
	TraceLog(("skpark VK_F7..."));
	CString uploader = "UBCUploader.exe";
	if (m_localeLang == "en")
		uploader = "UBCUploader_en.exe";
	else if (m_localeLang == "jp")
		uploader = "UBCUploader_jp.exe";

	unsigned long pid = getPid(uploader);
	if (pid)
	{
		killProcess(pid);
	}
	createProcess(uploader, " +ENT +serverOnly", UBC_EXE_PATH);
}

void CGuardianDlg::OpenAuthorizer()
{
	showTaskbar(false);

	TraceLog(("skpark VK_F8..."));
	CString authorizer = "UBCHostAuthorizer.exe";
	//CString param = ("ko" == m_localeLang ? " +ENT +authOnly " : " +ENT  +authOnly  +global");
	CString param = ("ko" == m_localeLang ? " +ENT +authOnly " : " +ENT  +authOnly  +global");
	unsigned long pid = getPid(authorizer);
	if (pid)
	{
		killProcess(pid);
	}
	createProcess(authorizer, param, UBC_EXE_PATH);
}


bool CGuardianDlg::CaptureTest()
{
	TraceLog(("CaptureTest"));
	static CString prev_captureFile = "";
	static DWORD prev_crcCode = 0;

	CString sTemp;
	sTemp.Format("%sPicture\\", UBC_GUARDIAN_PATH);
	DWORD dwRet = GetFileAttributes(sTemp);
	if ((dwRet == -1) || !(dwRet & FILE_ATTRIBUTE_DIRECTORY))
	{
		CreateDirectory(sTemp, NULL);
	}
	CTime now = CTime::GetCurrentTime();
	CString captureFile = "";
	captureFile.Format("%sCapture_%04d%02d%02d_%02d%02d%02d.jpg", sTemp,
		now.GetYear(), now.GetMonth(), now.GetDay(), now.GetHour(), now.GetMinute(), now.GetSecond());

	if (!GetThermalScreenShot(captureFile))
	{
		TraceLog(("CapturePicture failed() !!!!"));
		CString msg;
		msg.Format("(%s) Capture fail !!!!", captureFile);
		WriteLog(msg);
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL); // Capture 에 실패하면 재기동한다.
		return false;
	}
	// 캡춰에 성공하면, CRC 값을 구한다.
	DWORD crcCode=0, dwErrorCode = NO_ERROR;
	dwErrorCode = CCrc32Static::FileCrc32Win32(captureFile, crcCode);
	if (dwErrorCode != NO_ERROR)
	{
		CString msg;
		msg.Format("(%s) CRC fail !!!!(%d)", captureFile, dwErrorCode);
		WriteLog(msg);
		//::DeleteFileA(captureFile);  // CRC fail 의 경우, 파일을 지우지 말아보자...
		::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL); // CRC 에 실패하면 재기동한다.
		return false;
	}

	if (prev_crcCode != 0 && crcCode != 0)
	{
		if (prev_crcCode == crcCode)
		{
			// 두 CRC Code 는 같을수 없다.  같다는 것은 화면이 멈췄음을 뜻한다.
			CString msg;
			msg.Format("(%s) CRC same !!!!(%ld==%ld)", captureFile, prev_crcCode, crcCode);
			WriteLog(msg);
			::DeleteFileA(captureFile);
			::SendMessage(GetSafeHwnd(), WM_CLOSE, NULL, NULL); // CRC 에 실패하면 재기동한다.
			return false;
		}
	}
	::DeleteFileA(captureFile);  
	prev_crcCode = crcCode;
	return true;
}

bool CGuardianDlg::WriteLog(LPCTSTR msg)
{
	TraceLog((msg));

	CString sTemp;
	if (m_logFP == 0)
	{
		sTemp.Format("%sGuardian.log", UBC_GUARDIAN_PATH);
		if (::PathFileExistsA(sTemp))
		{
			::MoveFileExA(sTemp, sTemp + ".bak", MOVEFILE_REPLACE_EXISTING);
		}

		m_logFP = fopen(sTemp, "w");
		if (m_logFP == NULL)
		{
			return false;
		}
	}
	sTemp.Format("%s\n", msg);
	fputs(sTemp, m_logFP);
	fflush(m_logFP);
	return true;
}

void CGuardianDlg::CloseLog()
{
	if (m_logFP)
	{
		fclose(m_logFP);
		m_logFP = 0;
	}
}

void CGuardianDlg::StopAnimation()
{
	if (m_isAniPlay)
	{
		TraceLog(("StopAnimation"));
		if (m_FacingDlg)
		{
			//m_FacingDlg->DestroyWindow(); 
			//delete m_FacingDlg;
			//m_FacingDlg = NULL;
			m_FacingDlg->ShowWindow(SW_HIDE);
			SetFocus();
		}
	}
	m_isAniPlay = false;
}
void CGuardianDlg::StartAnimation()
{
	if (!m_isAniPlay)
	{
		TraceLog(("StartAnimation(%d)", m_videoHeight+m_videoTop));
		if (m_FacingDlg == NULL)
		{
			m_FacingDlg = new CFacingDlg(m_videoLeft, m_videoHeight + m_videoTop, m_videoWidth, this);
			m_FacingDlg->Create(IDD_FACING_DLG);
		}
		m_isAniPlay = true;
		m_FacingDlg->ShowWindow(SW_SHOW);
	}
}


void CGuardianDlg::InitWatch()
{
	int w = 400;
	int h = 150;
	int x = m_letterLeft;
	int y = m_videoTop - h;

	CRect rect_contents(x,y,x+w,y+h);

	m_timePlate = new CTimePlate();
	m_timePlate->CreateEx(0, NULL, "WATCH", WS_CHILD | WS_VISIBLE, rect_contents, this, 0xfefe);
	m_timePlate->SetParent(this);
	m_timePlate->SetParentDlg(this);
	m_timePlate->MoveWindow(x,y,w,h);
	m_timePlate->ShowWindow(SW_SHOW);
	m_timePlate->SetForegroundWindow();
}

void CGuardianDlg::DrawWatch()
{
	m_timePlate->Invalidate();
}

//void CGuardianDlg::bg4Init()
//{
//	m_bgPic.MoveWindow(0, 0, 1080, 1920, true);
//	m_bgPic.ShowWindow(SW_SHOW);
//	CString filename;
//	filename.Format("init_bg.gif", UBC_GUARDIAN_PATH);
//	bool loaded = m_bgPic.Load(filename);
//	TraceLog(("bg4Init(%d)", loaded));
//	if (loaded) {
//		TraceLog(("load %s filed loaded", filename));
//		m_bgPic.Draw();
//	}
//	else {
//		TraceLog(("load %s filed failed", filename));
//	}
//
//}

bool CGuardianDlg::_LoadBG(CString bgFile, CImage*& image)
{
	TraceLog(("_LoadBG(%s)", bgFile));
	CString fullpath = UBC_GUARDIAN_PATH;
	fullpath += bgFile;

	if (!IsLocalExist(fullpath))
	{
		TraceLog(("file does not exist [%s]", fullpath));
		return false;
	}

	image = new CImage();
	image->Load(fullpath);
	return true;
}

bool  CGuardianDlg::_DrawBG(CDC& dc, CImage* image)
{
	//SetBkMode(dc.m_hDC, TRANSPARENT);
	//dc.FillSolidRect(0, 0, m_width, m_height, NORMAL_BG_COLOR);
	//return true;

	TraceLog(("DrawBG"));

	if (!image || image->IsNull())
	{
		TraceLog(("image does not set"))
			return false;
	}

	float w = static_cast<float>(image->GetWidth());
	float h = static_cast<float>(image->GetHeight());
	if (w == 0 || h == 0)
	{
		TraceLog(("Invalid BG size width[%d] height[%d]", w, h));
		return false;
	}


	int display_w = 1080;
	int display_h = 1920;



	CRect rc(0, 0, display_w, display_h);

	// 투명처리
	//unsigned char * pCol = 0;
	//long lw = m_bgInit->GetWidth();
	//long lh = m_bgInit->GetHeight();
	//for (long y = 0; y < lh; y++) {
	//	for (long x = 0; x < lw; x++) {
	//		pCol = (unsigned char *)m_bgInit->GetPixelAddress(x, y);
	//		unsigned char alpha = pCol[3];
	//		if (alpha != 255) {
	//			pCol[0] = ((pCol[0] * alpha) + 128) >> 8;
	//			pCol[1] = ((pCol[1] * alpha) + 128) >> 8;
	//			pCol[2] = ((pCol[2] * alpha) + 128) >> 8;
	//		}
	//	}
	//}
	//m_bgInit->SetHasAlphaChannel(true);

	// 출처: https://2016amie.tistory.com/entry/CImgae-png-투명값-static-Picture-MFC [AMIE]

	long ret = image->Draw(dc.m_hDC, rc);
	TraceLog(("_DrawBG retval=%d", ret));

	return true;
}

bool CGuardianDlg::LoadBGInit1() { return _LoadBG(m_bgInitFile1, m_bgInit1); }
bool CGuardianDlg::LoadBGInit2() { return _LoadBG(m_bgInitFile2, m_bgInit2); }
bool CGuardianDlg::LoadBGFail1() { return _LoadBG(m_bgFailFile1, m_bgFail1); }
bool CGuardianDlg::LoadBGFail2() { return _LoadBG(m_bgFailFile2, m_bgFail2); }
bool CGuardianDlg::LoadBGNext() { return _LoadBG(m_bgNextFile, m_bgNext);  }
bool CGuardianDlg::LoadBGIdentified() { return _LoadBG(m_bgIdentifiedFile, m_bgIdentified); }

bool CGuardianDlg::DrawBGInit1(CDC& dc) { return _DrawBG(dc, m_bgInit1); }
bool CGuardianDlg::DrawBGInit2(CDC& dc) { return _DrawBG(dc, m_bgInit2); }
bool CGuardianDlg::DrawBGFail1(CDC& dc) { return _DrawBG(dc, m_bgFail1); }
bool CGuardianDlg::DrawBGFail2(CDC& dc) { return _DrawBG(dc, m_bgFail2); }
bool CGuardianDlg::DrawBGNext(CDC& dc) { return _DrawBG(dc, m_bgNext); }
bool CGuardianDlg::DrawBGIdentified(CDC& dc, CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade)
{ 
	bool retval = _DrawBG(dc, m_bgIdentified); 
	TraceLog(("DrawBGIdentified(%s, %d)", pMainAlarmName, m_videoTop + m_videoHeight + 400));
	WriteSingleLine(pMainAlarmName, &dc, m_alarmFont3, RGB(0x00, 0x00, 0x00), m_videoTop + m_videoHeight + 400, 60, DT_CENTER, m_videoLeft, m_videoWidth);
	WriteSingleLine(pMainAlarmGrade, &dc, m_alarmFont3, RGB(0x00, 0x00, 0x00), m_videoTop + m_videoHeight + 460, 60, DT_CENTER, m_videoLeft, m_videoWidth);

	return retval;
}


void CGuardianDlg::PushReservedMap(CString eventId)
{
	m_reserved_cs.Lock();
	TraceLog(("PushReservedMap(%d,%s)", m_currentMainAlarmIdx, eventId));
	m_reservedMap[m_currentMainAlarmIdx] = eventId;
	m_reserved_cs.Unlock();
}


bool CGuardianDlg::DrawBG(CDC& dc, CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade)
{

	switch (m_bgMode) {
	case BG_MODE::NONE:    DrawBGInit1(dc);  break;
	case BG_MODE::INIT1:    DrawBGInit1(dc); break;
	case BG_MODE::INIT2:    DrawBGInit2(dc);	 break;
	case BG_MODE::FAIL1:   DrawBGFail1(dc); break;
	case BG_MODE::FAIL2:   DrawBGFail2(dc); break;
	case BG_MODE::NEXT:   DrawBGNext(dc);  break;
	case BG_MODE::IDENTIFIED:   DrawBGIdentified(dc, pCurrentTemp, pMainAlarmName, pMainAlarmGrade); break;
	default: DrawBGInit1(dc);
	}

	if (m_bgMode == BG_MODE::NONE || m_bgMode == BG_MODE::INIT1) {
		m_btNext.ShowWindow(SW_HIDE);
		m_btNextDisabled.ShowWindow(SW_SHOW);
	}
	else if (m_bgMode == BG_MODE::INIT2) {
		m_btNext.ShowWindow(SW_SHOW);
		m_btNextDisabled.ShowWindow(SW_HIDE);
	}
	else {
		m_btNext.ShowWindow(SW_HIDE);
		m_btNextDisabled.ShowWindow(SW_HIDE);
	}



	return true;
}

bool CGuardianDlg::AreYouReady()
{
	switch (m_bgMode) {
	case BG_MODE::FAIL1:
	case BG_MODE::FAIL2:
	case BG_MODE::IDENTIFIED:
		return false;
	}

	m_reserved_cs.Lock();
	if (m_reservedMap.size() > 0) {
		m_reserved_cs.Unlock();
		return true;
	}
	m_reserved_cs.Unlock();
	return false;
}

void CGuardianDlg::OnBnClickedBnNext()
{
	m_reserved_cs.Lock();
	RESERVED_MAP::iterator itr;
	for (itr = m_reservedMap.begin(); itr != m_reservedMap.end(); itr++) {
		TraceLog(("m_reservedMap key = %d", itr->first));
		m_cs.Lock();
		HUMAN_INFOMAP::iterator jtr = m_mainAlarmInfoMap.find(itr->first);
		if (jtr == m_mainAlarmInfoMap.end()) {
			TraceLog(("m_reservedMap (%d) no key founded", itr->first));
			m_cs.Unlock();
			continue;
		}
		AlarmInfoEle* ele = jtr->second;

		if (ele->m_alarmLevel == 1) {
			m_cs.Unlock();
			GotoPage(BG_MODE::FAIL1);
			
			break;
		}
		if (!ele->m_humanName.IsEmpty()) {
			m_cs.Unlock();
			GotoPage(BG_MODE::IDENTIFIED);
			break;
		}
		TraceLog(("m_reservedMap (%d) key founded", itr->first));
		FRRetry::getInstance()->Start(this);
		CString eventId;
		eventId.Format("[%s_x]", ele->m_eventId.Mid(0, ele->m_eventId.GetLength() - 2));
		FRRetry::getInstance()->PushEventQ(eventId, (ele->m_alarmLevel == 1), atof(ele->m_currentTemp));
		m_cs.Unlock();
	}
	m_reservedMap.clear();
	m_reserved_cs.Unlock();

	GotoPage(BG_MODE::INIT2);
}

bool CGuardianDlg::GetNames(DRAW_FACE_MAP& humanMap, CString& pCurrentTemp,CString& pMainAlarmName,CString& pMainAlarmGrade)
{
	DRAW_FACE_MAP::iterator itr;
	int len = humanMap.size();

	int idx = 0;
	for (itr = humanMap.begin(); itr != humanMap.end(); itr++)
	{
		idx++;
		AlarmInfoEle* ele = itr->second;
	
		if (ele->m_score > 0.0f  && ele->m_score < m_config->m_face_score_limit)
		{
			// KIST 의 경우 Retry 결과도 점수가 높지 않으면 안나와야 한다.
			continue;
		}

		// KIST 의 경우,  이름이 없으면 표시하지 않는다.
		// 근데, 지금은 이름이 안나오므로, 일단, 무조건 있는것으로 하기 위해 가라 이름을 넣는다.
		/*if (idx % 4 == 0) {
			ele->m_humanName = "김지수";
			ele->m_grade = "실험실A";
		}
		else if (idx % 4 == 1) {
			ele->m_humanName = "김제니";
			ele->m_grade = "실험실B";
		}
		else if (idx % 4 == 2)  {
			ele->m_humanName = "박채영";
			ele->m_grade = "실험실A";
		}
		else if (idx % 4 == 3)  {
			ele->m_humanName = "라리사";
			ele->m_grade = "실험실B";
		}*/

		if (ele->m_humanName.IsEmpty()) {
			continue;
		}

		// 이름 가운데를  * 로 치환한다.
		TraceLog(("human = %s, %d grade=%s,", ele->m_humanName, ele->m_humanName.GetLength(), ele->m_grade));

		if (ele->m_humanName.GetLength() >= 6) {
			TraceLog(("human = %s*%s", ele->m_humanName.Mid(0, 2), ele->m_humanName.Mid(4)));
			CString temp = ele->m_humanName.Mid(0, 2);
			temp += "*";
			temp += ele->m_humanName.Mid(4);
			ele->m_humanName = temp;
		}
		else if (ele->m_humanName.GetLength() >= 4) {
			TraceLog(("human = %s*", ele->m_humanName.Mid(0, 2)));
			CString temp = ele->m_humanName.Mid(0, 2);
			temp += "*";
			ele->m_humanName = temp;

		}
		

		pCurrentTemp = ele->m_currentTemp;
		pMainAlarmName = ele->m_humanName;
		pMainAlarmGrade = ele->m_grade ;
		TraceLog(("show name (%s)", pMainAlarmName));

		return true;
	}
	return false;
}

void CGuardianDlg::GotoPage(BG_MODE mode, bool redraw) {
	m_bgMode = mode;
	if (redraw) Invalidate();

	if (mode == BG_MODE::NEXT) {
		// Send here
		if (m_socket) {
			m_socket->Send(CString("come-on"));
			SetTopMost(false);
		}
	}
}

UINT CGuardianDlg::StartSocketServer(LPVOID pParam)
{
	TraceLog(("StartSocketServer()"));

	CGuardianDlg* thisDlg = (CGuardianDlg*)pParam;

	thisDlg->m_socket = new CSocketServer(thisDlg);
	if (thisDlg->m_socket->Start() == 0){
		thisDlg->m_socket->Receive();
	}
	return 0;
}

CString CGuardianDlg::SocketReceived(CString received)
{
	TraceLog(("SocketReceived(%s)", received));

	if (received == "come-on") {
		SetTopMost(true);
	}
	CString retval = "OK";
	return retval;
}

UINT CGuardianDlg::TestClient(LPVOID pParam)
{
	TraceLog(("TestClient()"));

	CGuardianDlg* thisDlg = (CGuardianDlg*)pParam;

	thisDlg->m_test_client = new CSocketClient();
	while (1) {
		if (thisDlg->m_test_client->Connect() == 0){
			TraceLog(("Connected..."));
			break;
		}
		::Sleep(1000);
	}
	thisDlg->m_test_client->Send("come-on");
	thisDlg->m_test_client->Receive();

	return 0;
}


void CGuardianDlg::OnBnClickedBnNextDisabled()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}


void CGuardianDlg::OnStnClickedStaticStat()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}
