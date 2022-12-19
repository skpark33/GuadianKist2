
// CGuardianDlg.h: 헤더 파일
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"

#include <queue>
#include <vector>

#include <opencv2/highgui/highgui.hpp>		// OpenCV window I/O
#include <opencv2/imgproc/imgproc.hpp>		// Gaussian Blur
#include <opencv2/videoio.hpp>

#include <map>
#include <list>
//------------------ skpark --------------------//
#include "Statistics.h"
#include <GuardianConfig.h>
#include <Weather.h>

#include "HikvisionCameraView.h"
#include "AlarmInfo.h"
#include "ReinForceFR.h"
#include "skpark/PictureEx.h"
#include "skpark/HoverButton.h"


extern HikvisionCameraView* g_hikvisionCameraView;

#define MAX_HUMAN				10
#define ALARM_CHECK_TIMER		24		// new alarm validate time
#define KIST_CHECK_TIMER		25		// new alarm validate time

std::string UTF82ASCII(const char* cont);
std::string ASCII2UTF8(const char* cont);
class CFacingDlg;
class CTimePlate;
class CSocketServer;
class CSocketClient;

//using namespace std;

// CGuardianDlg 대화 상자
class CGuardianDlg : public CDialogEx
{
	// 생성입니다.
public:
	CGuardianDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CGuardianDlg() 
	{
		if (m_config) delete m_config;
		if (m_stat) delete m_stat;
	}
// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GUARDIAN_DLG };
#endif


protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnDestroy();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnWeatherMessage(WPARAM, LPARAM);
	afx_msg LRESULT OnWeatherUpdateMessage(WPARAM, LPARAM);
	afx_msg LRESULT OnAirMessage(WPARAM, LPARAM);
	afx_msg LRESULT OnEventUpdateMessage(WPARAM, LPARAM);
	afx_msg LRESULT OnOldFileDelete(WPARAM, LPARAM);
	afx_msg LRESULT OnRefreshGuardianSettings(WPARAM, LPARAM);

#if (_MSC_VER >= 1500)	//vs2008
	afx_msg void OnTimer(UINT_PTR nIDEvent);
#else
	afx_msg void OnTimer(UINT nIDEvent);
#endif
	DECLARE_MESSAGE_MAP()
private:
	HICON m_hIcon;

	void InitPosition();
	void MaxCamera2(int maxWidth, int maxHeight, bool readraw = false);

	void InitFont(CDC* pDc);
	void InitFont(CFont& targetFont, CDC* pDc, LPCTSTR fontName, int fontSize, float fontWidth = 1);
	void Clear();


	bool KeyboardSimulator(LPCTSTR command);

//	bool WriteIniDeviceInfo(int index);
//	int  GetDeviceIndex();
//	time_t GetLastIndexTime();
//	time_t GetIpChangeTime();
//	bool	CheckIp();

	CString SQISoftFaceAPI(LPCTSTR filePrefix, LPCTSTR cropFileName);

	bool SQIRetryFaceAPI(LPCTSTR eventId, LPCTSTR filePrefix, CString& strRetData);
	bool GetScreenShot(LPCTSTR filename);
	bool GetThermalScreenShot(LPCTSTR filename);
	bool CaptureTest();
	bool WriteLog(LPCTSTR msg);
	void CloseLog();
	bool ParseResult(bool& isMainAlarm, bool& maskFace, CString& face_api_result, CString& strResultCode);
	CString SQISoftFeverFaceAPI(LPCTSTR cropFileName, float faceTemperature);

//	CImage* CropGdiImage(LPCTSTR filePrefix, BYTE * buffer, DWORD size, CString& strMediaFullPath);
	CImage*	CropGdiImage(LPCTSTR filePrefix, cv::Mat cropFace, CString& strMediaFullPath);
	CImage* Mat2CImage(cv::Mat* mat);

	HUMAN_INFOMAP	m_mainAlarmInfoMap;
	long			m_currentMainAlarmIdx;
	AlarmInfoEle*	m_currentMainAlarmInfo;
	CCriticalSection	m_cs;


	void	PushMainAlarm(AlarmInfoEle* ele);
	//int		EraseOldMainAlarm(time_t now);
	int		EraseAllMainAlarm();
	void	EraseLastMainAlarm();
	void	SetMainAlarmInfo(LPCTSTR szName, LPCTSTR szGrade, LPCTSTR szRoom);
	void	SetMainAlarmCheckMask(int maskLevel);		// 마스크착용여부 등록
	void	SetMainAlarmImage(CImage* pImage);
	bool	GetLastMainAlarmInfo(CString& outTemperature, CString& outName, bool& isFever, int& nMaskLevel);
	bool	ExistFeverAlarm();
	void	AddSample(float temper);

	void FRRetryFileDownloadResponse(CString& errMsg);

	int	 LoadRetryImage();
	//int	 EraseOldRetryFaces(time_t now);
	int	 EraseAllRetryFaces();
	void DrawRetryFaces(CDC& dc);
	bool DrawRetryBackground(CDC& dc);

	DRAW_FACE_MAP		m_retrylFaces;
	CCriticalSection	m_retry_cs;
	
	void	EraseAll();
	//	bool	GetThermalRegion(); -> HikvisionCameraView


	CString m_currentCtrlKey;
	CString m_normalPageKey;
	int		m_cameraID;		
/*
	float	m_faceX;		
	float	m_faceY;		 
	float	m_faceWidth;	
	float	m_faceHeight;	 

	float	m_thermalX; 
	float	m_thermalY; 
	float	m_thermalWidth; 
	float	m_thermalHeight; 
	*/
public:
	int	m_width; 
	int m_height; 

	int	m_videoWidth; 
	int m_videoHeight; 
	int	m_videoLeft; 
	int m_videoTop; 
	int m_videoEnd; 
	int m_titleStart; 
	int m_letterLeft; 
	int m_letterWide; 

	int m_lastFaceLeft; 

	bool m_bFullScreen; 


	CFont	m_normalFont1;
	CFont	m_normalFont2;
	CFont	m_normalFont3;

	CFont	m_weatherFontTitle;		// 20
	CFont	m_weatherFontValue;		// 24.5
	CFont	m_weatherFontTemp;		// 82.5
	CFont	m_weatherFontDesc;		// 25
	CFont	m_weatherFontUnit;		// 30
	
	CString m_localeLang;
	CString m_pictureSavePath;
	
	CImage*		m_logo;
	CImage*		m_title;
	CImage*		m_notice;

	int			m_logoW;
	int			m_logoH;
	CString		m_logoFile;
	CString		m_titleFile;
	CString		m_noticeFile;
	CBrush		m_statBrush;
	//CString		m_currentShortcut;

	bool m_isMinimize;
	void InitMainWin();

	bool CheckMode();
	
	typedef  map<CString, CImage*>		ImageMap;
	ImageMap	m_bgWeatherMap;
	CImage*		LoadBackgroundWeather(CString weatherIcon);
	void		ClearWeaherMap();

	//ReinforceFRManager*  m_frManager;
	bool IsFRRetryType() { return  (m_config->m_use_face && m_config->m_faceApiType == CGuardianConfig::FACE_API_REINFORCE); }
	bool IsFRWatchType() { return  (m_config->m_use_face && m_config->m_faceApiType == CGuardianConfig::FACE_API_WATCH); }

	void DrawTemperature(CDC& dc, LPCTSTR temperature, COLORREF foreColorTemp, LPCTSTR currentTime, COLORREF foreColorTime, int align, int posX, int posY);
	bool DrawLogo(CDC& dc);
	bool DrawTitle(CDC& dc, int x, int y);
	bool DrawNotice(CDC& dc, int x, int y);

	bool LoadLogo();
	bool LoadTitle();
	bool LoadNotice();

	UINT RunAlarmSound(LPVOID param);
	UINT RunAlarmVoice(LPVOID param);
	bool m_isVoiceRunning;
	void SaveQRLog(CString detectedType, CString detectedCode);
	void GenerateQRCode();

	//-----------------------------------

//	enum { FACE_API_NONE, FACE_API_SQISOFT, FACE_API_SAFR };
//	int		m_faceApiType;
	

	CString RunCLI(LPCTSTR path, LPCTSTR command, LPCTSTR parameter);


//	CRect m_rectPreviewBG;
	void ProThermometryAlarm(float temperature, cv::Mat cropFace, int maskLevel, int alarmLevel);
	void ProcFaceSnapAlarm(cv::Mat cropFace, int maskLevel);
/*	
	LRESULT OnWMProcAlarm(WPARAM wParam, LPARAM lParam);
	void ProcISAPIAlarm(LPLOCAL_ALARM_INFO pAlarmDev, char* pAlarmInfo);
	void ProThermometryAlarm(WPARAM wParam, LPARAM lParam);
	void ProcFaceSnapAlarm(WPARAM wParam, LPARAM lParam);
*/

	

	void SetLeftZero() { m_lastFaceLeft = 0;}
	int		GetMainAlarmImageInfoList(DRAW_FACE_MAP& outMap);
	bool DrawNormal(CDC& dc);
	bool DrawAbnormal(CDC& dc, /*DRAW_FACE_MAP& humanMap, */ int imgaeCount, bool isFever, int nMaskLevel, CString& temperature);
	int  DrawFaces(CDC& dc, DRAW_FACE_MAP& humanMap, COLORREF rectColor, COLORREF foreColor, int leftOffset = -1);

	void DrawStat(CDC& dc, int posY, int height);
	bool SaveImage(CImage* pImage, LPCTSTR filePrefix, CString& strMediaFullPath);
	bool SaveFile(LPCTSTR fullpath, void* targetText, int targetLen, int iDeviceIndex);
	int	 PutRetryResult(FACE_INFO_LIST* resultList, float temperature);

	// 영상표시
	HikvisionCameraView m_cameraNormal;
	HikvisionCameraView m_cameraThermal;

	HWND GetNormalCameraHwnd();

	CStatistics*		m_stat;
	CGuardianConfig* m_config;
	WeatherProvider m_weatherProvider;	// 날씨, 미세먼지 정보

	static UINT WaitForWindowCreated(LPVOID pParam);
//	static UINT AutoSetupThread(LPVOID pParam);  -> HikvisionCameraView config
		
	std::queue<ST_EVENT> m_eventQ;

	void KillEverything(bool killFirmwareView);
	void	SetTopMost(bool bTopMost);
	bool BrowserShouldShrink();
	void KillBrowser();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	bool ToggleCamera();
	void OpenUploader();
	void OpenAuthorizer();
	bool ToggleFullScreen();

	void WriteSingleLine(LPCTSTR text,
		CDC* pDc,
		CFont& targetFont,
		COLORREF color,
		int posY,
		int height,
		int align = DT_CENTER,
		int posX = 0,
		int width = 0
		);
	void WriteMultiLine(LPCTSTR text,
		CDC* pDc,
		CFont& targetFont,
		COLORREF color,
		int posY,
		int height
		);

	CFont	 m_alarmFont1;
	CFont	 m_alarmFont2;
	CFont	 m_alarmFont3;
	CFont m_alarmFont4;
	CFont	 m_alarmFont5;
	CFont	 m_alarmFont6;

	FILE*	 m_logFP;
	
	CFacingDlg* m_FacingDlg;
	bool m_isAniPlay;
	bool StopAnimation();
	void StartAnimation();


	// New Kist

	void DrawWatch();
	void InitWatch();
	CTimePlate*  m_timePlate;

	//CPictureEx m_bgPic;
	//void bg4Init();

	CImage*		m_bgInit1;
	CImage*		m_bgInit2;
	CImage*		m_bgWait;
	CImage*		m_bgFail1;
	CImage*		m_bgFail2;
	CImage*		m_bgIdentified;
	//CImage*		m_bgNext;

	CString		m_bgInitFile1;
	CString		m_bgInitFile2;
	CString		m_bgWaitFile;
	CString		m_bgFailFile1;
	CString		m_bgFailFile2;
	CString		m_bgIdentifiedFile;
	//CString		m_bgNextFile;

	bool _DrawBG(CDC& dc, CImage* image);
	bool _LoadBG(CString bgFile, CImage*& image);

	bool LoadBGInit1();
	bool LoadBGInit2();
	bool LoadBGWait();
	bool LoadBGFail1();
	bool LoadBGFail2();
	bool LoadBGIdentified();
	//bool LoadBGNext();

	bool DrawBGInit1(CDC& dc);
	bool DrawBGInit2(CDC& dc);
	bool DrawBGWait(CDC& dc);
	bool DrawBGFail1(CDC& dc);
	bool DrawBGFail2(CDC& dc);
	bool DrawBGIdentified(CDC& dc, CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade);
	bool DrawBGNext(CDC& dc);

	RESERVED_MAP  m_reservedMap;  // new KIST 
	CCriticalSection m_reserved_cs; // new KIST

	void PushReservedMap(CString chTime);

	BG_MODE  m_bgMode;

	bool DrawBG(CDC& dc, CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade);

	bool AreYouReady();

	bool GetNames(CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade);
	bool HasNames();
	bool _GetNames(DRAW_FACE_MAP& humanMap, CString& pCurrentTemp, CString& pMainAlarmName, CString& pMainAlarmGrade);

	int m_modeCheckCounter;

	void GotoPage(BG_MODE mode, bool redraw = true);
	bool IsBgMode(BG_MODE mode);
	
	static UINT StartSocketServer(LPVOID pParam);
	CString SocketReceived(CString received);
	CSocketServer* m_socket;
	CHoverButton m_btNextDisabled;
	afx_msg void OnBnClickedBnNextDisabled();
	CHoverButton m_btNext;
	afx_msg void OnBnClickedBnNext();

	afx_msg void OnStnClickedStaticStat();
	void EraseAllReservedMap();

	bool SendGrade(LPCTSTR grade);
	void CGuardianDlg::SetChromeTopMost(bool val);
	
	CString m_grade;
};
