#include "stdafx.h"
#include "string.h"
#include <fstream>
#include <ctime>

#include <windows.h>
#include <list>
#include <map>
#include <Gdiplus.h>
#include <tlhelp32.h>
#include <shlwapi.h>
#include <winternl.h>
#include <afxinet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>

#include "common.h"
#include "ci/libAes256/ciElCryptoAes256.h"

//------------- Hikvision Camera SDK ------------//
#include <stdio.h>
#include <iostream>
#include "Windows.h"
#include "HCNetSDK.h"
#include "plaympeg4.h"

#include "GuardianApp.h"
#include "GuardianDlg.h"
#include <TraceLog.h>

#include <opencv2/core.hpp>					// Basic OpenCV structures (cv::Mat, Scalar)

#ifdef _QR_CODE_
#include <opencv2/imgproc/imgproc.hpp>		// Gaussian Blur
#include <opencv2/imgcodecs.hpp>
#include <zbar.h>
#endif


using namespace std;

IMPLEMENT_DYNAMIC(HikvisionCameraView, CWnd)

// Class varibale
HikvisionCameraView* HikvisionCameraView::g_instance;

int HikvisionCameraView::m_eventPeriod(10);	// per 50 frame
// int HikvisionCameraView::m_eventCnt(3);		// count per Period

static unsigned int g_AnalyzerDataCBCnt = 0;
static unsigned long frameCnt = 0;
static int g_realDataRequestCnt = 0;
static int g_realDataLastFrameCnt = 0;

static std::fstream g_realdata_fs;


static LONG lUserID(0);
static LONG lPort(-1);			// Global Player Port No.
static LONG lHandle(-1);		// Alarm Handle
static int nDetectNum(0);

//Gdiplus init
//ULONG_PTR gdiplusToken;

#ifdef _QR_CODE_
//----------------- QR Code Reader ---------------//
typedef struct
{
	string type;
	string data;
	std::vector <cv::Point> location;
} decodedQR;


// Find and decode barcodes and QR codes
void decodeQR(cv::Mat& im, std::vector<decodedQR>& decodedObjects)
{
	// Create zbar scanner
	zbar::ImageScanner scanner;

	// Configure scanner (enblae QR)
	scanner.set_config(zbar::ZBAR_QRCODE, zbar::ZBAR_CFG_ENABLE, 1);

	// Convert image to grayscale
	cv::Mat imGray;
	cv::cvtColor(im, imGray, 6); // CV_BGR2GRAY);

	// Wrap image data in a zbar image
	zbar::Image image(im.cols, im.rows, "Y800", (uchar*)imGray.data, im.cols * im.rows);

	// Scan the image for barcodes and QRCodes
	int n = scanner.scan(image);

	// Print results
	for (zbar::Image::SymbolIterator symbol = image.symbol_begin(); symbol != image.symbol_end(); ++symbol) {
		decodedQR obj;
		obj.type = symbol->get_type_name();
		obj.data = symbol->get_data();

		// Obtain location
		for (int i = 0; i < symbol->get_location_size(); i++) {
			obj.location.push_back(cv::Point(symbol->get_location_x(i), symbol->get_location_y(i)));
		}
		decodedObjects.push_back(obj);
	}
}
#endif
//-------------------------------------------------------------------------//


void FRRetryProcess(U8* pBuf, long nSize, FRAME_INFO * pFrameInfo)
{
	// FRS Timer  0.33 sec
	//TraceLog(("skpark FRRetryProcess1"));
	if (((float)g_hikvisionCameraView->m_clkFRRetryCur - (float)g_hikvisionCameraView->m_clkFRRetryStart) < 333.0) {
		return;
	}
	//TraceLog(("skpark FRRetryProcess2"));

	static int s_counter = 0;
	static CString s_jpgArr[3];
	static FRAME_SHOT s_shot;

	static int retryPictureCount = -1;
	if (FRRetry::getInstance()->IsKISTType())
	{
		//TraceLog(("FRRetryProcess %d", s_counter));
		if (retryPictureCount < 0){
			CString iniPath = UBC_CONFIG_PATH;
			iniPath += UBCBRW_INI;
			char buf[10];
			memset(buf, 0x00, 10);
			GetPrivateProfileString("GUARDIAN", "FAIL_RETRY_PICTURE", "1", buf, 10, iniPath);
			retryPictureCount = atof(buf);
		}
		if (retryPictureCount > 3) retryPictureCount = 3;
	}
	else
	{
		retryPictureCount = 1;
	}

	if (s_counter == 0)
	{
		s_shot.Clear();
		//TraceLog(("PopEventQ"));
		if (!FRRetry::getInstance()->PopEventQ(s_shot))
		{
			return;
		}
	}
	if (s_shot.eventId.IsEmpty())
	{
		return;
	}

	if (s_counter < retryPictureCount)
	{
		CString fname;
		fname.Format("C:\\SQISOFT\\Face\\%s[%d]frame.jpg", s_shot.eventId, s_counter);

		TraceLog(("skpark FRRetry %s file create (%ld, %d - %d)", fname, nSize, pFrameInfo->nWidth, pFrameInfo->nHeight));
		if (yuv2jpgfile(YUV_YV12, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, fname) > 0)
		{
			TraceLog(("skpark convert images succeed"));
			s_jpgArr[s_counter] = fname;
		}
		else
		{
			TraceLog(("skpark create images fail"));
		}
		g_hikvisionCameraView->m_clkFRRetryStart = g_hikvisionCameraView->m_clkFRRetryCur;
		TraceLog(("skpark convert images counter = %d", s_counter));
		s_counter++;
	}
	if (s_counter == retryPictureCount)
	{
		FRRetry::getInstance()->PushFrameQ(s_shot.eventId, s_jpgArr[0], s_jpgArr[1], s_jpgArr[2], s_shot.isMain, s_shot.temperature);
		//::PostMessage(g_pMainDlg->m_hWnd, WM_FRRETRY_PUSH_EVENT, WPARAM((LPCTSTR)s_shot.eventId), NULL);
		for (int j = 0; j < retryPictureCount; j++)
		{
			s_jpgArr[j] = "";
		}
		s_counter = 0;
		s_shot.Clear();
	}
}

void FRRetryProcessWatch(U8* pBuf, long nSize, FRAME_INFO * pFrameInfo)
{

	// FRS Timer  1.0 sec
	if (((float)g_hikvisionCameraView->m_clkFRRetryCur - (float)g_hikvisionCameraView->m_clkFRRetryStart) < 1000.0) {
		return;
	}

	FRAME_SHOT s_shot;
	if (!FRRetry::getInstance()->PopEventQ(s_shot))
	{
		return;
	}
	if (s_shot.eventId.IsEmpty())
	{
		s_shot.Clear();
		return;
	}
	int counter = FRRetry::getInstance()->getWatchInstance(s_shot.eventId);
	if (counter == 0)
	{
		s_shot.Clear();
		return;
	}

	TraceLog(("FRRetryProcessWatch"));

	CString fname;
	fname.Format("C:\\SQISOFT\\Face\\%s[%d]frame.jpg", s_shot.eventId, counter);

	TraceLog(("skpark FRRetry %s file create (%ld, %d - %d)", fname, nSize, pFrameInfo->nWidth, pFrameInfo->nHeight));
	if (yuv2jpgfile(YUV_YV12, pBuf, pFrameInfo->nWidth, pFrameInfo->nHeight, fname) > 0)
	{
		TraceLog(("skpark convert images succeed"));
	}
	else
	{
		TraceLog(("skpark create images fail"));
		return;
	}
	g_hikvisionCameraView->m_clkFRRetryStart = g_hikvisionCameraView->m_clkFRRetryCur;
	TraceLog(("skpark convert images counter = %d", counter));

	FRRetry::getInstance()->PushFrameQ(s_shot.eventId, fname, "", "", s_shot.isMain, s_shot.temperature);
	//::PostMessage(g_pMainDlg->m_hWnd, WM_FRRETRY_PUSH_EVENT, WPARAM((LPCTSTR)s_shot.eventId), NULL);
}


void CALLBACK DecCBFun(long nPort, char* pBuf, long nSize, FRAME_INFO* pFrameInfo, long nReserved1, long nReserved2)
{
	//TraceLog((_T("DecCBFun  !!!!!!!")));
	g_hikvisionCameraView->m_clkFRRetryCur = g_hikvisionCameraView->m_clkCur = clock();

#ifdef _QR_CODE_
	frameCnt++;

	if (pFrameInfo->nType != T_YV12) {
		TraceLog(( _T("[QR/Bar Code detection] >>> Not T_YV12") ));
		return;
	}

	if ((frameCnt - g_realDataLastFrameCnt) < HikvisionCameraView::m_eventPeriod) {
		return;
	}
	// TraceLog((_T("[QR/Bar Code detection] %d, %d, %d"), frameCnt, g_realDataLastFrameCnt, HikvisionCameraView::m_eventPeriod));

	g_realDataLastFrameCnt = frameCnt;
	g_realDataRequestCnt--;

	cv::Mat pImg(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
	cv::Mat pImg_YCrCb(pFrameInfo->nHeight, pFrameInfo->nWidth, CV_8UC3);
	cv::Mat pImg_YUV(pFrameInfo->nHeight + pFrameInfo->nHeight / 2, pFrameInfo->nWidth, CV_8UC1, pBuf);
	cv::cvtColor(pImg_YUV, pImg, 99);		// CV_YUV2BGR_YV12
//	cv::cvtColor(pImg, pImg_YCrCb, 36);		// CV_BGR2YCrCb

//	cv::Mat frame;
//	cv::resize(pImg, frame, cv::Size(pImg.cols / 4, pImg.rows / 4));	// down size image
	/*
	string fileNameQR;
	fileNameQR = cv::format("QRCode_%d.jpg", frameCnt);
	cv::imwrite(fileNameQR, pImg);  //only for test
	*/

	if (!pImg.empty()) {
		CString codeType, code;
		std::vector<decodedQR> detectedQRs;
		decodeQR(pImg, detectedQRs);
		//	TraceLog((_T("[QR/Bar Code detection] detectedQRs.size(): %d"), detectedQRs.size()));
		// vector<cv::Point> points;
		// vector<cv::Point> hull;

		CString detectedType, detectedCode;
		for (int i = 0; i < detectedQRs.size(); i++) {
			detectedType = detectedQRs[i].type.c_str();
			detectedCode = detectedQRs[i].data.c_str();
			//	TraceLog(( _T("[QR/Bar Code detection] type:%s Serial No:%s  -- [%d/%d]frams"), 
			//		detectedType, detectedCode, (frameCnt - g_realDataLastFrameCnt), DahuaCameraView::m_eventPeriod ));

			if ("QR-Code" == detectedType) {
				TraceLog((_T("[QR Code detection] Serial No:%s"), detectedCode));

				ST_EVENT evt;
				evt.eventType = 1;
				evt.detectedType = detectedType;
				evt.detectedCode = detectedCode;
				((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->m_eventQ.push(evt);
				((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->PostMessage(UM_EVENT_UPDATE_MESSAGE, 0, 0);
			}
			else {
				// BAR-Code : EAN, CODE39, ITF, CODABAR, CODE128
				TraceLog((_T("[BAR Code detection] Serial No:%s"), detectedQRs[i].data.c_str()));
			}

			/*
			points = detectedQRs[i].location;

			// If the points do not form a quad, find convex hull
			if (points.size() > 4) {
				convexHull(points, hull);
			}
			else {
				hull = points;
			}

			// Number of points in the convex hull
			int n = hull.size();
			for (int j = 0; j < n; j++) {
				line(pImg, hull[j], hull[(j + 1) % n], cv::Scalar(255, 0, 0), 3);
			}
			*/
		}
	}
	else {
		TraceLog((_T("[QR Code detection] mat(frame) is NULL")));
	}
#endif

	if (FRRetry::getInstance()->IsFRRetryType())
	{
		FRRetryProcess(reinterpret_cast<U8*>(pBuf), nSize, pFrameInfo);
	}else
	if (FRRetry::getInstance()->IsFRWatchType())
	{
		FRRetryProcessWatch(reinterpret_cast<U8*>(pBuf), nSize, pFrameInfo);
	}
}


void CALLBACK Hikvision_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, void* pUser)
{
	//TraceLog((_T("~~~~~~~~~~[Hikvision_RealDataCallBack_V30]~~~~~~~~~~(%lu)", dwDataType)));

	switch (dwDataType)
	{
	case NET_DVR_SYSHEAD: //System Header
		if (lPort < 0) {
			//PlayM4_FreePort(lPort);
			if (!PlayM4_GetPort(&lPort)) {
				TraceLog(("Hikvision_RealDataCallBack_V30 >>> PlayM4_GetPort Fail"));
				break;
			} else {
				TraceLog(("PlayM4_GetPort Success --- port[%d]", lPort));
			}
		}
		TraceLog((_T("~~~~~~~~~~[Hikvision_RealDataCallBack_V30]~ (%lu)~~~~~~~", dwDataType)));
		//m_iPort = lPort; //The first callback data is system header. Assign obtained player port No. to global port, and it will be used in next callback.
		if (dwBufSize > 0) {
			
			if (!PlayM4_SetStreamOpenMode(lPort, STREAME_REALTIME)) {			// Set Real-time Stream Playing Mode
				break;
			}

			if (!PlayM4_OpenStream(lPort, pBuffer, dwBufSize, 2*1024*1024)) {	// Open stream API
				break;
			}
			

			// PlayM4_SetDecCBStream(lPort, 1);
			if (!PlayM4_SetDecCallBack(lPort, DecCBFun)) {
				TraceLog(("PlayM4_SetDecCallBack err[%d]", PlayM4_GetLastError(lPort)));
			}

			/*
			// set frame buffer number
			if (!PlayM4_SetDisplayBuf(lPort, 1)) {
				TraceLog(("PlayM4_SetDisplayBuf [%d] err[%d]", 1, PlayM4_GetLastError(lPort)));
			}
			*/

			// Start playing
			if (!PlayM4_Play(lPort, NULL)) {
				TraceLog(("PlayM4_Play err[%d]", PlayM4_GetLastError(lPort)));
				break;
			}

			/*
			//set display mode
			if (!PlayM4_SetOverlayMode(lPort, FALSE, COLORREF(0)))//play off screen
			{
				TraceLog(("PlayM4_SetOverlayMode err[%d]", PlayM4_GetLastError(lPort)));
			}
			*/
		}
		break;

	case NET_DVR_STD_VIDEODATA:
	case NET_DVR_STD_AUDIODATA:
	case NET_DVR_STREAMDATA:
		if (dwBufSize > 0 && lPort != -1) {
			if (!PlayM4_InputData(lPort, pBuffer, dwBufSize)) {
				break;
			}
		}
		break;
	default: //Other data
		if (dwBufSize > 0 && lPort != -1) {
			if (!PlayM4_InputData(lPort, pBuffer, dwBufSize)) {
				break;
			}
		}
		break;
	}
}

HikvisionCameraView::HikvisionCameraView()
{
	m_thermalX = 0;
	m_thermalY = 0;
	m_thermalWidth = 0;
	m_thermalHeight = 0;

	m_clkFRRetryStart = 0;
}

HikvisionCameraView::~HikvisionCameraView()
{
	CleanUp();
}

BEGIN_MESSAGE_MAP(HikvisionCameraView, CWnd)
	//ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL HikvisionCameraView::RegisterWindowClass()
{
	WNDCLASS    sttClass;
	HINSTANCE   hInstance = AfxGetInstanceHandle();

	if (GetClassInfo(hInstance, CAMERA_VIEW_CLASSNAME, &sttClass) == FALSE) {
		sttClass.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
		sttClass.lpfnWndProc = ::DefWindowProc;
		sttClass.cbClsExtra = 0;
		sttClass.cbWndExtra = 0;
		sttClass.hInstance = hInstance;
		sttClass.hIcon = NULL;
		sttClass.hCursor = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		sttClass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
		sttClass.lpszMenuName = NULL;
		sttClass.lpszClassName = CAMERA_VIEW_CLASSNAME;

		if (!AfxRegisterClass(&sttClass)) {
			AfxThrowResourceException();
			return FALSE;
		}
	}

	return TRUE;
}

void CALLBACK Hikvision_ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void* pUser)
{
	TraceLog(("!!!!!     ExceptionCallBack 발생     !!!!!"));

	char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RELOGIN:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_RELOGIN[%d]\n", time(NULL)));
		break;
	case RELOGIN_SUCCESS:
		TraceLog(("ExceptionCallBack >>> RELOGIN_SUCCESS[%d]\n", time(NULL)));
		break;
	case EXCEPTION_EXCHANGE://the device is off line
		TraceLog(("ExceptionCallBack >>> EXCEPTION_EXCHANGE[%d]\n", time(NULL)));
		break;
	case RESUME_EXCHANGE://the device is on line
		TraceLog(("ExceptionCallBack >>> RESUME_EXCHANGE[%d]\n", time(NULL)));
		break;
	case EXCEPTION_AUDIOEXCHANGE:	//network exception while voice talk
		TraceLog(("ExceptionCallBack >>> EXCEPTION_AUDIOEXCHANGE[%d]\n", time(NULL)));
		break;
	case EXCEPTION_ALARM:			//network exception while uploading alarm
		TraceLog(("ExceptionCallBack >>> EXCEPTION_ALARM[%d]\n", time(NULL)));
		break;
	case EXCEPTION_PREVIEW:			// exception while preview
		TraceLog(("ExceptionCallBack >>> EXCEPTION_PREVIEW[%d]\n", time(NULL)));
		break;
	case EXCEPTION_SERIAL:			//exception while connecting in a transparent channel mode
		TraceLog(("ExceptionCallBack >>> EXCEPTION_SERIAL[%d]\n", time(NULL)));
		break;
	case EXCEPTION_RECONNECT:		//reconnect while preview	
		TraceLog(("ExceptionCallBack >>> EXCEPTION_RECONNECT[%d]\n", time(NULL)));
		break;
	case PREVIEW_RECONNECTSUCCESS:		//reconnect successfully while preview				
		TraceLog(("ExceptionCallBack >>> PREVIEW_RECONNECTSUCCESS[%d]\n", time(NULL)));
		break;
	case EXCEPTION_ALARMRECONNECT://reconnect alarm guard channel
		TraceLog(("ExceptionCallBack >>> EXCEPTION_ALARMRECONNECT[%d]\n", time(NULL)));
		break;
	case ALARM_RECONNECTSUCCESS://reconnect alarm guard channel successfully
		TraceLog(("ExceptionCallBack >>> ALARM_RECONNECTSUCCESS[%d]\n", time(NULL)));
		break;
	case EXCEPTION_SERIALRECONNECT://reconnect transparent channel
		TraceLog(("ExceptionCallBack >>> EXCEPTION_SERIALRECONNECT[%d]\n", time(NULL)));
		break;
	case EXCEPTION_PLAYBACK:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_PLAYBACK[%d]\n", time(NULL)));
		break;
	case EXCEPTION_DISKFMT:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_DISKFMT[%d]\n", time(NULL)));
		break;
	case EXCEPTION_PASSIVEDECODE:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_PASSIVEDECODE[%d]\n", time(NULL)));
		break;
	case SERIAL_RECONNECTSUCCESS:
		TraceLog(("ExceptionCallBack >>> SERIAL_RECONNECTSUCCESS[%d]\n", time(NULL)));
		break;
	case EXCEPTION_LOST_ALARM:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_LOST_ALARM[%d]\n", time(NULL)));
		break;
	case EXCEPTION_MAX_ALARM_INFO:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_MAX_ALARM_INFO[%d]\n", time(NULL)));
		break;
	case EXCEPTION_PASSIVEDECODE_RECONNNECT:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_PASSIVEDECODE_RECONNNECT[%d]\n", time(NULL)));
		break;
	case EXCEPTION_VIDEO_DOWNLOAD:
		TraceLog(("ExceptionCallBack >>> EXCEPTION_VIDEO_DOWNLOAD[%d]\n", time(NULL)));
		break;
	default:
		TraceLog(("ExceptionCallBack >>> default[%d]\n", time(NULL)));
		break;
	}
}

void AlarmMessage(LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser)
{
//    TraceLog(("AlarmMessage(0x%x)", lCommand))

    UINT iDeviceIndex = 0xffff;
    UINT i = 0;

    try {
        char* pAlarmMsg = NULL;
		pAlarmMsg = new char[dwBufLen];
		if (pAlarmMsg == NULL) {
			return;
		}
		memset(pAlarmMsg, 0, dwBufLen);
		memcpy(pAlarmMsg, pAlarmInfo, dwBufLen);

        if (lCommand == COMM_ISAPI_ALARM) {
			TraceLog(("COMM_ISAPI_ALARM"));			

			((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData = new char[((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->dwAlarmDataLen + 1];
            memset(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData, 0, ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->dwAlarmDataLen + 1);
            memcpy(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData, ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->pAlarmData, ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->dwAlarmDataLen);
			if (((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byDataType == 1) {	// 0:invalid, 1:xml, 2:json
				CXmlBase xmlBase;
				xmlBase.Parse(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData);
				TraceLog(("COMM_ISAPI_ALARM JSON[%s]", xmlBase.GetData().c_str()));

			} else if (((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byDataType == 2) {	// 0:invalid, 1:xml, 2:json
			//	CXmlBase xmlBase;
			//	xmlBase.Parse(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData);
				TraceLog(("COMM_ISAPI_ALARM JSON[%s]", ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pAlarmData));
			}
			
            if (((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byPicturesNumber != 0) {
                ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData = (NET_DVR_ALARM_ISAPI_PICDATA*)new BYTE[sizeof(NET_DVR_ALARM_ISAPI_PICDATA) * ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byPicturesNumber];
                memset(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData, 0, sizeof(NET_DVR_ALARM_ISAPI_PICDATA) * ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byPicturesNumber);
                memcpy(((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData, ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->pPicPackData, sizeof(NET_DVR_ALARM_ISAPI_PICDATA) * ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byPicturesNumber);
                for (int i = 0; i < ((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->byPicturesNumber; i++)
                {
                    ((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].pPicData = new BYTE[((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].dwPicLen];
                    memset(((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].pPicData, 0, ((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].dwPicLen);
                    memcpy(((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].pPicData, ((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmInfo)->pPicPackData)[i].pPicData, ((NET_DVR_ALARM_ISAPI_PICDATA*)((LPNET_DVR_ALARM_ISAPI_INFO)pAlarmMsg)->pPicPackData)[i].dwPicLen);
                }
            }
        }
        else if (COMM_UPLOAD_FACESNAP_RESULT == lCommand)
        {
            DWORD dwPrePicLen = 0;
            if (((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->dwFacePicLen > 0)
            {
                ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->pBuffer1 = (BYTE*)pAlarmMsg + sizeof(NET_VCA_FACESNAP_RESULT) + dwPrePicLen;
                dwPrePicLen += ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->dwFacePicLen;
            }

            if (((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->dwBackgroundPicLen > 0)
            {
                ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->pBuffer2 = (BYTE*)pAlarmMsg + sizeof(NET_VCA_FACESNAP_RESULT) + dwPrePicLen;
                dwPrePicLen += ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->dwBackgroundPicLen;
            }

            if (((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->byUIDLen > 0)
            {
                ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->pUIDBuffer = (BYTE*)pAlarmMsg + sizeof(NET_VCA_FACESNAP_RESULT) + dwPrePicLen;
                dwPrePicLen += ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->byUIDLen;
            }

            if (((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->byAddInfo > 0)
            {
                ((LPNET_VCA_FACESNAP_RESULT)pAlarmMsg)->pAddInfoBuffer = (BYTE*)pAlarmMsg + sizeof(NET_VCA_FACESNAP_RESULT) + dwPrePicLen;
                dwPrePicLen += sizeof(NET_VCA_FACESNAP_ADDINFO);
            }

			///--------------------------- ----------------------- ----------------------- 
			//char szInfoBuf[1024 * 5] = { 0 };
			NET_VCA_FACESNAP_RESULT struFaceSnapAlarm = { 0 };
			memcpy(&struFaceSnapAlarm, pAlarmMsg, sizeof(struFaceSnapAlarm));

			// 없으면:1, 있으면:2, 모르겠으면:0 
			TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> FaceScore[%d] Mask[%d] EyeGlass[%d] Hat[%d] Beard[%d] Sex[%d] Age[%d] FaceExpression[%d]",
				struFaceSnapAlarm.dwFaceScore,
				struFaceSnapAlarm.struFeature.byMask,
				struFaceSnapAlarm.struFeature.byEyeGlass,
				struFaceSnapAlarm.struFeature.byHat,
				struFaceSnapAlarm.struFeature.byBeard,
				struFaceSnapAlarm.struFeature.bySex,
				struFaceSnapAlarm.struFeature.byAge,
				struFaceSnapAlarm.struFeature.byFaceExpression      // FACE_EXPRESSION_GROUP_ENUM
				));

			if ( struFaceSnapAlarm.struFeature.byMask != 1) {
				TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> Mask face or Unknown"));

				delete[] pAlarmMsg;
				pAlarmMsg = NULL;
				return;
			}

			CImage aImage;
			if (!Bytes2Image((BYTE*)struFaceSnapAlarm.pBuffer1, struFaceSnapAlarm.dwFacePicLen, aImage)) {
				TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> Bytes2Image Fail"));
			}
			else {
				TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> Bytes2Image Success"));
			}

			cv::Mat cropFace;
			CImage2Mat(aImage, cropFace);
			TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> CImage2Mat Success", cropFace));

			ST_EVENT evt;
			evt.eventType = 2;
			evt.cropFace = cropFace;
//			evt.temperature = temperature;
			evt.maskLevel = (int)struFaceSnapAlarm.struFeature.byMask - 1;

			TraceLog(("COMM_UPLOAD_FACESNAP_RESULT >>> UM_EVENT_UPDATE_MESSAGE"));
			((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->m_eventQ.push(evt);
			((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->PostMessage(UM_EVENT_UPDATE_MESSAGE, 0, 0);

			delete[] pAlarmMsg;
			pAlarmMsg = NULL;
			return;
        }
        else if (COMM_THERMOMETRY_ALARM == lCommand)
        {
			TraceLog(("COMM_THERMOMETRY_ALARM"));

            ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pPicBuff = new char[((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwPicLen];
            memset(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pPicBuff, 0, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwPicLen);
            memcpy(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pPicBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmInfo)->pPicBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwPicLen);

			((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalPicBuff = new char[((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalPicLen];
            memset(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalPicBuff, 0, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalPicLen);
            memcpy(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalPicBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmInfo)->pThermalPicBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalPicLen);

			((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalInfoBuff = new char[((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalInfoLen];
            memset(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalInfoBuff, 0, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalInfoLen);
            memcpy(((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->pThermalInfoBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmInfo)->pThermalInfoBuff, ((LPNET_DVR_THERMOMETRY_ALARM)pAlarmMsg)->dwThermalInfoLen);

			///--------------------------- ----------------------- ----------------------- 

			NET_DVR_THERMOMETRY_ALARM  struThermometryAlarm;
			memset(&struThermometryAlarm, 0, sizeof(struThermometryAlarm));
			memcpy(&struThermometryAlarm, pAlarmMsg, sizeof(struThermometryAlarm));

			// Regular calibration type 0-Point, 1-Region, 2-Line
			if (1 <= struThermometryAlarm.byRuleCalibType) {
				cv::Mat pic;
				CImage aImage;

				if (struThermometryAlarm.dwPicLen > 0 && struThermometryAlarm.pPicBuff != NULL)	{

					/*------------------------  TEST --------------------------------
					char cFilename[256] = { 0 };
					HANDLE hFile;
					DWORD dwReturn;

					char chTime[128];
					sprintf(cFilename, "C:\\SQISoft\\Upload\\CropFace_Bytes.jpg");
					hFile = CreateFile(cFilename, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
					if (hFile != INVALID_HANDLE_VALUE)
					{
						WriteFile(hFile, struThermometryAlarm.pPicBuff, struThermometryAlarm.dwPicLen, &dwReturn, NULL);
						CloseHandle(hFile);
					}
					-------------------------------------------------------------------*/

					if (Bytes2Image((BYTE*)struThermometryAlarm.pPicBuff, struThermometryAlarm.dwPicLen, aImage)) {
						TraceLog(("COMM_THERMOMETRY_ALARM >>> Bytes2Image Success"));
						CImage2Mat(aImage, pic);

						// aImage.Save("C:\\SQISoft\\Upload\\Full_Image.jpg");
					}
					else {
						TraceLog(("COMM_THERMOMETRY_ALARM >>> Bytes2Image Fail"));
					}
				}

				float	faceX;
				float	faceY;
				float	faceWidth;
				float	faceHeight;

				int iPointNum = struThermometryAlarm.struRegion.dwPointNum;
				for (int i = 0; i < iPointNum; i++)	{
					float fX = struThermometryAlarm.struRegion.struPos[i].fX;
					float fY = struThermometryAlarm.struRegion.struPos[i].fY;
					if (i == 0)	{
						faceX = fX;
						faceY = fY;
					}

					if (i == 2)	{
						faceWidth = fX - faceX;
						faceHeight = fY - faceY;
					}
					// TraceLog(("face crop[%d] x:%f,y:%f", i, fX, fY));
				}

				float streamHeight = aImage.GetHeight();
				float streamWidth = aImage.GetWidth();

				// TraceLog(("face width x height = : %f x %f", streamWidth, streamHeight));

				float minusMargin = 80.0f * (streamHeight / STREAM_HEIGHT);
				float plusMargin = minusMargin * 2.0f;

				HikvisionCameraView* pView = (HikvisionCameraView*) pUser;
				int left = static_cast<int>(faceX*streamWidth - minusMargin);
				int top = static_cast<int>(faceY*streamHeight - minusMargin);
				int width = static_cast<int>(faceWidth*streamWidth + plusMargin);
				int height = static_cast<int>(faceHeight*streamHeight + plusMargin);
				TraceLog(("face : %d,%d,%d,%d", left, top, width, height));
				
				cv::Rect bounds(0, 0, streamWidth, streamHeight);
				cv::Rect r(left, top, width, height);	// partly outside 
				cv::Mat cropFace = pic(r & bounds);		// cropped to fit image
				// cv::imwrite("C:\\SQISoft\\Upload\\CropFace_MAT.jpg", cropFace);

				ST_EVENT evt;
				evt.eventType = 0;
				evt.cropFace = cropFace;
				evt.temperature = struThermometryAlarm.fCurrTemperature;
				evt.maskLevel = -1;
				evt.alarmLevel = struThermometryAlarm.byAlarmLevel;

				TraceLog(("COMM_THERMOMETRY_ALARM >>> UM_EVENT_UPDATE_MESSAGE"));
				((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->m_eventQ.push(evt);
				((CGuardianDlg*)AfxGetApp()->m_pMainWnd)->PostMessage(UM_EVENT_UPDATE_MESSAGE, 0, 0);
			}

			if (struThermometryAlarm.pPicBuff != NULL) {
				delete[](struThermometryAlarm.pPicBuff);
				struThermometryAlarm.pPicBuff = NULL;
			}

			if (struThermometryAlarm.pThermalPicBuff != NULL) {
				delete[](struThermometryAlarm.pThermalPicBuff);
				struThermometryAlarm.pThermalPicBuff = NULL;
			}

			if (struThermometryAlarm.pThermalInfoBuff != NULL) {
				delete[](struThermometryAlarm.pThermalInfoBuff);
				struThermometryAlarm.pThermalInfoBuff = NULL;
			}
		}

		delete[] pAlarmMsg;
		pAlarmMsg = NULL;
	}
    catch (...)
    {
        TraceLog(("New Alarm Exception!"));
    }
}

void CALLBACK Hikvision_MessageCallback(LONG lCommand, NET_DVR_ALARMER* pAlarmer, char* pAlarmInfo, DWORD dwBufLen, void* pUser)
{
//	TraceLog(("Hikvision_MessageCallback"));
	UNREFERENCED_PARAMETER(pUser);
	AlarmMessage(lCommand, pAlarmer, pAlarmInfo, dwBufLen, pUser);
}

void SavePictureInfo(const char* pFileName, char* pBinBuf, UINT nBufLength)
{
	if (nBufLength <= 0) {
		return;
	}

	FILE* fp = NULL;
	fopen_s(&fp, pFileName, "wb+");
	if (fp) {
		fwrite(pBinBuf, 1, nBufLength, fp);
		fclose(fp);
	}
}

int HikvisionCameraView::CameraConfig(int channel, const char* ip, int port, const char* userid, const char* password)
{
	// encPasswd[#kYRDiPMDYmSxvJ4dLQCcCQ == ] decPasswd[hikvision123]
	// encPasswd[#XSMIkkLGMWOvu0aG9kcl6Q == ] decPasswd[b49eoqkr]
	// string encPasswd = ciAes256Util::Encrypt("b49eoqkr", false);
	// string decPasswd = ciAes256Util::Decrypt(password);
	//TraceLog(("CameraConfig >>> encPasswd[%s] decPasswd[%s]", encPasswd.c_str(), decPasswd.c_str()));
	if (password == NULL)
	{
		TraceLog(("password is null"));
		return -99;
	}

	CString decrypted = password;
	if (password[0] == '#')
	{
		decrypted = ciAes256Util::Decrypt(password).c_str();
	}

	TraceLog(("CameraConfig >>> IP[%s] CAMERA_PW[%s]", ip, decrypted));
	//TraceLog(("CameraConfig >>> IP[%s] CAMERA_PW[%s]", ip, ciAes256Util::Decrypt(password).c_str()));
	
	//---------------------------------------
	// Initialize
	NET_DVR_Init();
	//Set connected time and reconnected time
	NET_DVR_SetConnectTime(2000, 1);
	NET_DVR_SetReconnect(5000, true);

	//---------------------------------------
	// Set callback function of exception message
	NET_DVR_SetExceptionCallBack_V30(0, NULL, Hikvision_ExceptionCallBack, NULL);

	//---------------------------------------
	// Register device
	//Login parameters, including IP address, user name, and password, etc.
	NET_DVR_USER_LOGIN_INFO struLoginInfo = { 0 };
	struLoginInfo.bUseAsynLogin = 0; //synchronous login mode
	strcpy(struLoginInfo.sDeviceAddress, ip);
	struLoginInfo.wPort = port;
	strcpy(struLoginInfo.sUserName, userid);
	strcpy(struLoginInfo.sPassword, decrypted);

	//Device information, output parameters
	NET_DVR_DEVICEINFO_V40 struDeviceInfoV40 = { 0 };
	lUserID = NET_DVR_Login_V40(&struLoginInfo, &struDeviceInfoV40);
	if (lUserID < 0) {
		TraceLog(("Login failed, error code: %d", NET_DVR_GetLastError()));
		NET_DVR_Cleanup();
		return -1;
	}
	else {
		TraceLog(("Login success: lUserID[%d]", lUserID));

		//---------------------------------------
		// Start live view and set callback data stream
		NET_DVR_PREVIEWINFO struPlayInfo = { 0 };
		struPlayInfo.hPlayWnd = GetSafeHwnd();	// Set the handle as valid when the stream should be decoded by SDK, while set the handle as null when only streaming.
		struPlayInfo.lChannel = channel;		// Live view channel No.
		struPlayInfo.dwStreamType = (channel == 1 ? 0 : 1);		// 0 - Main Stream, 1 - Sub - Stream, 2 - Stream 3, 3 - Stream 4, and so on.
		struPlayInfo.dwLinkMode = 0;			// 0-TCP Mode, 1-UDP Mode, 2-Multi-play mode, 3-RTP Mode, 4-RTP/RTSP, 5-RSTP/HTTP
		struPlayInfo.bBlocked = 0;				// 0-Non-Blocking Streaming, 1-Blocking Streaming
		m_lRealPlayHandle = NET_DVR_RealPlay_V40(lUserID, &struPlayInfo, (channel == 1 ? Hikvision_RealDataCallBack_V30 : NULL), this);
		if (m_lRealPlayHandle < 0) {
			TraceLog(("NET_DVR_RealPlay_V40 error"));
			NET_DVR_Logout(lUserID);
			NET_DVR_Cleanup();
			return -1;
		}
		else {
			TraceLog(("NET_DVR_RealPlay_V40 Success --- channel[%d] lRealPlayHandle[%d]", channel, m_lRealPlayHandle));
		}

//		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		// Initialize GDI+.
//		Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		if (channel != 1) {
			bool retval1 =  ReadFaceThermometry(lUserID);
			bool retval2 = ReadTemperatureCompensation(lUserID);
			if (retval1 && retval2)
			{
				time_t now = time(NULL);
				CString nowStr;
				nowStr.Format("%lu", now);
				CString iniPath = UBC_CONFIG_PATH;
				iniPath += UBCBRW_INI;
				WritePrivateProfileString("GUARDIAN_STAT", "LAST_API_INVOKE_TIME", nowStr, iniPath);
			}

			GetThermalRegion();
			return 0;
		}
		else {
			g_hikvisionCameraView = this;
		}
		
		// SDK uploads the info such as alarm and log sent from DVR by calling callback function 
		NET_DVR_SetDVRMessageCallBack_V50(0, Hikvision_MessageCallback, this);

		//-----------------------------------------
		// Enable arming
		NET_DVR_SETUPALARM_PARAM_V50 struSetupParamV50 = { 0 };
		struSetupParamV50.dwSize = sizeof(NET_DVR_SETUPALARM_PARAM_V50);
		struSetupParamV50.byAlarmInfoType = 1;			// 0- History alarm information(NET_DVR_PLATE_RESULT), 1- New alarm information(NET_ITS_PLATE_RESULT)
		struSetupParamV50.byFaceAlarmDetection = 1;		// Face capture alarm, upload alarm information in the type of COMM_UPLOAD_FACESNAP_RESULT
		struSetupParamV50.byRetAlarmTypeV40 = TRUE;
		struSetupParamV50.byRetDevInfoVersion = TRUE;	// CVR눈괩쒸斤口잚謹(쏭뚤쌈CVR珂唐槻)
		struSetupParamV50.byRetVQDAlarmType = TRUE;		// Prefer VQD Alarm type of NET_DVR_VQD_ALARM
		struSetupParamV50.bySupport = 0;
		struSetupParamV50.bySupport |= (1 << 2);

		LONG lHandle(-1);
		char szSubscribe[1024] = { 0 };
		// The following code is for alarm subscribe all
		memcpy(szSubscribe, "<SubscribeEvent version=\"2.0\" xmlns=\"http://www.isapi.org/ver20/XMLSchema\">\r\n<eventMode>all</eventMode>\r\n", 1024);
		if (0 == strlen(szSubscribe)) {
			//Arm
			lHandle = NET_DVR_SetupAlarmChan_V50(lUserID, &struSetupParamV50, NULL, 0);
			TraceLog(("NET_DVR_SetupAlarmChan_V50 Arming Success"));
		}
		else {
			//Subscribe
			lHandle = NET_DVR_SetupAlarmChan_V50(lUserID, &struSetupParamV50, szSubscribe, strlen(szSubscribe));
			TraceLog(("NET_DVR_SetupAlarmChan_V50 Subscribe Success"));
		}

		if (lHandle < 0) {
			TraceLog(("NET_DVR_SetupAlarmChan_V50 error[%d]", NET_DVR_GetLastError()));
			return -1;
		}

		if (NET_DVR_ClientGetVideoEffect(m_lRealPlayHandle, &m_brightness, &m_contrast, &m_saturation, &m_hue)) {
			TraceLog(("Camera contrast changed >>> brightness[%d], contrast[%d], saturation[%d], hue[%d]", m_brightness, m_contrast, m_saturation, m_hue));
		} else {
			TraceLog(("NET_DVR_ClientGetVideoEffect error[%d]", NET_DVR_GetLastError()));
		}
	}

    return 0;
}

bool HikvisionCameraView::SetFaceThermometry(float alarmTemperature, float alertMinTemperature)
{
	bool retval = false;
	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;
//	TraceLog(("SetFaceThermometry() >>> INI[%s]", iniPath));

	char buf[1024];
	//----------------------------- change xml ----------------------------------//
	m_faceThermometryXml.SetRoot();
	if (m_faceThermometryXml.FindElem("FaceThermometry") && m_faceThermometryXml.IntoElem()) {
		if (m_faceThermometryXml.FindElem("FaceThermometryRegionList") && m_faceThermometryXml.IntoElem()) {
			if (m_faceThermometryXml.FindElem("ThermometryRegion") && m_faceThermometryXml.IntoElem()) {
				if (m_faceThermometryXml.FindElem("alarmTemperature"))  {

					CString modi;
					if (alarmTemperature == 0.0f)
					{
						memset(buf, 0x00, 1024);
						GetPrivateProfileString("GUARDIAN_STAT", "ALARM_TEMPERATURE", "37.5", buf, 1024, iniPath);
						modi.Format(_T("%.1f"), atof(buf));
					}
					else
					{
						modi.Format(_T("%.1f"), alarmTemperature);
					}
					TraceLog(("SetFaceThermometry() >>> ALARM_TEMPERATURE[%s]", modi));
					if (!m_faceThermometryXml.ModifyData("alarmTemperature", modi)) {
						TraceLog(("SetFaceThermometry() >>> [alarmTemperature] ModifyData fail"));
					}
				}
				else {
					TraceLog(("SetFaceThermometry() >>> [alarmTemperature] Element not found"));
					return retval;
				}

				if (m_faceThermometryXml.FindElem("alert"))  {
					CString modi;

					if (alertMinTemperature == 0.0f)
					{
						memset(buf, 0x00, 1024);
						GetPrivateProfileString("GUARDIAN_STAT", "ALERT_MIN_TEMPERATURE", "30.0", buf, 1024, iniPath);
						modi.Format(_T("%.1f"), atof(buf));
					}
					else
					{
						modi.Format(_T("%.1f"), alertMinTemperature);
					}
					TraceLog(("SetFaceThermometry() >>> ALERT_MIN_TEMPERATURE[%s]", modi));
					if (!m_faceThermometryXml.ModifyData("alert", modi)) {
						TraceLog(("SetFaceThermometry() >>> [alert] ModifyData fail"));
					}
				}
				else {
					TraceLog(("SetFaceThermometry() >>> [alert] Element not found"));
					return retval;
				}
			}
			else {
				TraceLog(("SetFaceThermometry() >>> [ThermometryRegion] Element not found"));
				return retval;
			}
		}
		else {
			TraceLog(("SetFaceThermometry() >>> [FaceThermometryRegionList] Element not found"));
			return retval;
		}
	}
	else {
		TraceLog(("SetFaceThermometry() >>> [FaceThermometry] Element not found"));
		return retval;
	}
	
	//------------------------------------- put xml -----------------------------------------------//
	char szInput[10240] {0};
	int dwReturn;
	m_faceThermometryXml.WriteToBuf(szInput, sizeof(szInput), dwReturn);
	// TraceLog(("SetFaceThermometry() >>> WriteToBuf[%d][%s]", dwReturn, szInput));
	
	char szUrl[1024] = { 0 };
	sprintf(szUrl, "PUT /ISAPI/Thermal/channels/1/faceThermometry");
	NET_DVR_XML_CONFIG_INPUT struXmlCfgIn = { 0 };
	struXmlCfgIn.dwSize = sizeof(struXmlCfgIn);
	struXmlCfgIn.lpRequestUrl = szUrl;
	struXmlCfgIn.dwRequestUrlLen = strlen(szUrl);
	struXmlCfgIn.lpInBuffer = szInput;
	struXmlCfgIn.dwInBufferSize = 10240;
//	struXmlCfgIn.dwRecvTimeOut = 30000;

	NET_DVR_XML_CONFIG_OUTPUT struXmlCfgOut = { 0 };
	DWORD dwOutputLen = 10240;
	char *pOutBuf = new char[dwOutputLen];
	memset(pOutBuf, 0, dwOutputLen);
	struXmlCfgOut.dwSize = sizeof(struXmlCfgOut);
	struXmlCfgOut.lpOutBuffer = pOutBuf;
	struXmlCfgOut.dwOutBufferSize = dwOutputLen;

	if (!NET_DVR_STDXMLConfig(m_lUserID, &struXmlCfgIn, &struXmlCfgOut)) {
		TraceLog(("SetFaceThermometry() >>> NET_DVR_STDXMLConfig error[%d] m_lUserID[%d]", NET_DVR_GetLastError(), m_lUserID));
		// #define NET_DVR_NETWORK_ERRORDATA		11	//Transferred data has error
		// #define NET_DVR_PARAMETER_ERROR 			17  //Parameters error
		AfxMessageBox("[사전알람온도], 고정값 비교방식의 [측정온도]가 \r\n설정되지 않았습니다. \r\n다시 시도해 주세요.");
	}
	else {
		TraceLog(("SetFaceThermometry() >>> faceThermometry Settings changed!!!"));
		retval = true;
	}

	//TraceLog(("SetFaceThermometry() >>> result[%d][%s]", struXmlCfgOut.dwSize, pOutBuf));

	if (pOutBuf != NULL) {
		delete[] pOutBuf;
		pOutBuf = NULL;
	}
	return retval;
}

bool HikvisionCameraView::ReadFaceThermometry(LONG lUserID)
{
	bool retval = false;
	m_lUserID = lUserID;

	char szUrl[512] = "";
	sprintf(szUrl, "GET /ISAPI/Thermal/channels/1/faceThermometry");
	DWORD dwOutputLen = 10240;
	char *pOutBuf = new char[dwOutputLen];
	if (NULL == pOutBuf) {
		TraceLog(("ReadFaceThermometry() >>> Apply Memory Failed!"));
		return retval;
	}
	memset(pOutBuf, 0, dwOutputLen);
	NET_DVR_XML_CONFIG_INPUT struXmlCfgIn = { 0 };
	struXmlCfgIn.dwSize = sizeof(struXmlCfgIn);
	struXmlCfgIn.lpRequestUrl = szUrl;
	struXmlCfgIn.dwRequestUrlLen = strlen(szUrl);

	NET_DVR_XML_CONFIG_OUTPUT struXmlCfgOut = { 0 };
	struXmlCfgOut.dwSize = sizeof(struXmlCfgOut);
	struXmlCfgOut.lpOutBuffer = pOutBuf;
	struXmlCfgOut.dwOutBufferSize = dwOutputLen;

	if (NET_DVR_STDXMLConfig(lUserID, &struXmlCfgIn, &struXmlCfgOut)) {
		
//		TraceLog(("ReadFaceThermometry() >>> OutBuf[%s]", pOutBuf));

		CString iniPath = UBC_CONFIG_PATH;
		iniPath += UBCBRW_INI;
		char buf[512];
		memset(buf, 0x00, 512);

		CXmlBase xmlAlarm;
		xmlAlarm.Parse(pOutBuf);
		m_faceThermometryXml = CXmlBase(xmlAlarm);
		if (xmlAlarm.FindElem("FaceThermometry") && xmlAlarm.IntoElem()) {
			if (xmlAlarm.FindElem("FaceThermometryRegionList") && xmlAlarm.IntoElem()) {
				if (xmlAlarm.FindElem("ThermometryRegion") && xmlAlarm.IntoElem()) {
					if (xmlAlarm.FindElem("alarmTemperature"))  {
						WritePrivateProfileString("GUARDIAN_STAT", "ALARM_TEMPERATURE", xmlAlarm.GetData().c_str(), iniPath);
						retval = true;
						TraceLog(("ReadFaceThermometry() >>> ALARM_TEMPERATURE[%.1f]", atof(xmlAlarm.GetData().c_str())));
					}
					else {
						TraceLog(("ReadFaceThermometry() >>> [alarmTemperature] Element not found"));
					}

					if (xmlAlarm.FindElem("alert"))  {
						WritePrivateProfileString("GUARDIAN_STAT", "ALERT_MIN_TEMPERATURE", xmlAlarm.GetData().c_str(), iniPath);
						TraceLog(("ReadFaceThermometry() >>> ALERT_MIN_TEMPERATURE[%.1f]", atof(xmlAlarm.GetData().c_str())));
					}
					else {
						TraceLog(("ReadFaceThermometry() >>> [alert] Element not found"));
					}
				}
			}
		}
		else {
			TraceLog(("ReadFaceThermometry() >>> [FaceThermometry] Element not found"));
		}
	}

	if (pOutBuf != NULL) {
		delete[] pOutBuf;
		pOutBuf = NULL;
	}
	return retval;
	/*
	char szInput[10240] {0};
	int dwReturn;
	m_faceThermometryXml.WriteToBuf(szInput, sizeof(szInput), dwReturn);
	TraceLog(("SetFaceThermometry() >>> FaceThermometry[%d][%s]", dwReturn, szInput));
	*/
}

bool HikvisionCameraView::SetTemperatureCompensation(LPCTSTR compensationType, float manualCalibration)
{
	bool retval = false;

	CString iniPath = UBC_CONFIG_PATH;
	iniPath += UBCBRW_INI;
	TraceLog(("SetTemperatureCompensation() >>> INI[%s]", iniPath));

	char buf[1024];

	CString compType;
	if (compensationType == NULL)
	{
		memset(buf, 0x00, 1024);
		GetPrivateProfileString("GUARDIAN_STAT", "COMPENSATION_TYPE", "auto", buf, 1024, iniPath);
		compType.Format(_T("%s"), buf);
	}
	else
	{
		compType = compensationType;
	}

	CString calibration;
	calibration.Format(_T("%.1f"), manualCalibration);
	/*
	if (manualCalibration == 0.0f)
	{
		memset(buf, 0x00, 1024);
		GetPrivateProfileString("GUARDIAN_STAT", "MANUAL_CALIBRATION", "0.4", buf, 1024, iniPath);
		calibration.Format(_T("%.1f"), atof(buf));
	}
	else
	{
		calibration.Format(_T("%.1f"), manualCalibration);
	}
	*/
	TraceLog(("SetTemperatureCompensation() >>> MANUAL_CALIBRATION[%s]", calibration));

	//----------------------------- make xml ----------------------------------//
	CXmlBase xmlBase;
	xmlBase.CreateRoot("BodyTemperatureCompensationCap");
	xmlBase.SetAttribute("version", "2.0");
	xmlBase.SetAttribute("xmlns", "http://www.hikvision.com/ver20/XMLSchema");
	xmlBase.AddNode("type", compType.GetBuffer(0));
	xmlBase.OutOfElem();

	CString param = (compType == _T("auto") ? _T("AutoParam") : _T("ManualParam"));
	xmlBase.AddNode(param.GetBuffer(0));
	xmlBase.AddNode("smartCorrection", calibration.GetBuffer(0));
	xmlBase.OutOfElem();
	xmlBase.OutOfElem();

	//--------------------------------- put xml -------------------------------//
	char szInput[10240] {0};
	int dwReturn;
	xmlBase.WriteToBuf(szInput, sizeof(szInput), dwReturn);
	TraceLog(("SetTemperatureCompensation() >>> WriteToBuf[%d][%s]", dwReturn, szInput));

	char szUrl[1024] = { 0 };
	sprintf(szUrl, "PUT /ISAPI/Thermal/channels/2/bodyTemperatureCompensation");
	NET_DVR_XML_CONFIG_INPUT struXmlCfgIn = { 0 };
	struXmlCfgIn.dwSize = sizeof(struXmlCfgIn);
	struXmlCfgIn.lpRequestUrl = szUrl;
	struXmlCfgIn.dwRequestUrlLen = strlen(szUrl);
	struXmlCfgIn.lpInBuffer = szInput;
	struXmlCfgIn.dwInBufferSize = 10240;

	NET_DVR_XML_CONFIG_OUTPUT struXmlCfgOut = { 0 };
	DWORD dwOutputLen = 10240;
	char *pOutBuf = new char[dwOutputLen];
	memset(pOutBuf, 0, dwOutputLen);
	struXmlCfgOut.dwSize = sizeof(struXmlCfgOut);
	struXmlCfgOut.lpOutBuffer = pOutBuf;
	struXmlCfgOut.dwOutBufferSize = dwOutputLen;

	if (!NET_DVR_STDXMLConfig(m_lUserID, &struXmlCfgIn, &struXmlCfgOut)) {
		TraceLog(("SetTemperatureCompensation() >>> NET_DVR_STDXMLConfig error[%d] m_lUserID[%d]", NET_DVR_GetLastError(), m_lUserID));
		// #define NET_DVR_NETWORK_ERRORDATA		11	//Transferred data has error
		// #define NET_DVR_PARAMETER_ERROR 			17  //Parameters error
		AfxMessageBox("[보상유형], [수동교정]값이 설정되지 않았습니다. \r\n다시 시도해 주세요.");
	}
	else {
		TraceLog(("SetTemperatureCompensation() >>> TemperatureCompensation Settings changed!!!"));
		retval = true;
	}

	//TraceLog(("SetTemperatureCompensation() >>> result[%d][%s]", struXmlCfgOut.dwSize, pOutBuf));

	if (pOutBuf != NULL) {
		delete[] pOutBuf;
		pOutBuf = NULL;
	}

	return retval;
}

bool HikvisionCameraView::ReadTemperatureCompensation(LONG lUserID)
{
	bool retval = false;
	m_lUserID = lUserID;

	char szUrl[512] = "";
	sprintf(szUrl, "GET /ISAPI/Thermal/channels/2/bodyTemperatureCompensation");
	DWORD dwOutputLen = 10240;
	char *pOutBuf = new char[dwOutputLen];
	if (NULL == pOutBuf) {
		TraceLog(("ReadTemperatureCompensation() >>> Apply Memory Failed!"));
		return retval;
	}
	memset(pOutBuf, 0, dwOutputLen);
	NET_DVR_XML_CONFIG_INPUT struXmlCfgIn = { 0 };
	struXmlCfgIn.dwSize = sizeof(struXmlCfgIn);
	struXmlCfgIn.lpRequestUrl = szUrl;
	struXmlCfgIn.dwRequestUrlLen = strlen(szUrl);

	NET_DVR_XML_CONFIG_OUTPUT struXmlCfgOut = { 0 };
	struXmlCfgOut.dwSize = sizeof(struXmlCfgOut);
	struXmlCfgOut.lpOutBuffer = pOutBuf;
	struXmlCfgOut.dwOutBufferSize = dwOutputLen;

	if (NET_DVR_STDXMLConfig(lUserID, &struXmlCfgIn, &struXmlCfgOut)) {
		// TraceLog(("ReadTemperatureCompensation() >>> OutBuf[%s]", pOutBuf));

		CString iniPath = UBC_CONFIG_PATH;
		iniPath += UBCBRW_INI;
		char buf[512];
		memset(buf, 0x00, 512);

		CXmlBase xmlAlarm;
		xmlAlarm.Parse(pOutBuf);
		if (xmlAlarm.FindElem("BodyTemperatureCompensationCap") && xmlAlarm.IntoElem()) {
			if (xmlAlarm.FindElem("type"))  {
				WritePrivateProfileString("GUARDIAN_STAT", "COMPENSATION_TYPE", xmlAlarm.GetData().c_str(), iniPath);
				TraceLog(("ReadTemperatureCompensation() >>> COMPENSATION_TYPE[%s]", xmlAlarm.GetData().c_str()));
			}
			else {
				TraceLog(("ReadTemperatureCompensation() >>> [type] Element not found"));
			}
			
			CString compParam = xmlAlarm.GetData().c_str();
			compParam = (compParam == _T("auto") ? _T("AutoParam") : _T("ManualParam"));
			
			if (xmlAlarm.FindElem(compParam) && xmlAlarm.IntoElem()) {
				if (xmlAlarm.FindElem("smartCorrection"))  {
					WritePrivateProfileString("GUARDIAN_STAT", "MANUAL_CALIBRATION", xmlAlarm.GetData().c_str(), iniPath);
					retval = true;
					TraceLog(("ReadTemperatureCompensation() >>> MANUAL_CALIBRATION[%.1f]", atof(xmlAlarm.GetData().c_str())));
				}
				else {
					TraceLog(("ReadTemperatureCompensation() >>> [smartCorrection] Element not found"));
				}
			}
			else {
				TraceLog(("ReadTemperatureCompensation() >>> [AutoParam] Element not found"));
			}
		}
		else {
			TraceLog(("ReadTemperatureCompensation() >>> [BodyTemperatureCompensationCap] Element not found"));
		}
	}

	if (pOutBuf != NULL) {
		delete[] pOutBuf;
		pOutBuf = NULL;
	}
	return retval;
}

int HikvisionCameraView::CleanUp()
{
	//Stop live view
	NET_DVR_StopRealPlay(m_lRealPlayHandle);

	// Release player resource
	PlayM4_Stop(lPort);
	PlayM4_CloseStream(lPort);
	PlayM4_FreePort(lPort);

	//User logout
	NET_DVR_Logout(lUserID);
	//Release SDK resource
	NET_DVR_Cleanup();

//	Gdiplus::GdiplusShutdown(gdiplusToken);

	return 0;
}
bool HikvisionCameraView::Ping()
{
	NET_DVR_XML_CONFIG_INPUT  xmlConfigInput = { 0 };
	NET_DVR_XML_CONFIG_OUTPUT  xmlCongfigOutput = { 0 };
	xmlConfigInput.dwSize = sizeof(xmlConfigInput);
	xmlCongfigOutput.dwSize = sizeof(xmlCongfigOutput);

	char szUrl[256] = "";
	DWORD dwOutputLen = 1024 * 1024;
	char *pOutBuf = new char[dwOutputLen];
	memset(pOutBuf, 0x00, dwOutputLen);

	//http://192.168.11.64/ISAPI/Thermal/channels/1/faceThermometry/capabilities

	sprintf(szUrl, "GET /ISAPI/Thermal/channels/%d/faceThermometry/capabilities", 1);
	TraceLog(("skpark region url=%s", szUrl));
	TraceLog(("skpark region loginID[0]=%ld", lUserID));
	TraceLog(("skpark region loginID[1]=%ld", lUserID));
	xmlConfigInput.lpRequestUrl = szUrl;
	xmlConfigInput.dwRequestUrlLen = strlen(szUrl);

	memset(pOutBuf, 0, dwOutputLen);
	xmlCongfigOutput.dwOutBufferSize = dwOutputLen;
	xmlCongfigOutput.lpOutBuffer = pOutBuf;

	if (NET_DVR_STDXMLConfig(lUserID, &xmlConfigInput, &xmlCongfigOutput))
	{
		delete[]pOutBuf;
		pOutBuf = NULL;
		return true;
	}
	delete[]pOutBuf;
	pOutBuf = NULL;
	return false;
}

bool HikvisionCameraView::GetThermalRegion()
{
	NET_DVR_XML_CONFIG_INPUT  xmlConfigInput = { 0 };
	NET_DVR_XML_CONFIG_OUTPUT  xmlCongfigOutput = { 0 };
	xmlConfigInput.dwSize = sizeof(xmlConfigInput);
	xmlCongfigOutput.dwSize = sizeof(xmlCongfigOutput);

	char szUrl[256] = "";
	DWORD dwOutputLen = 1024 * 1024;
	char *pOutBuf = new char[dwOutputLen];
	memset(pOutBuf, 0x00, dwOutputLen);

	BOOL succeed = FALSE;

	//http://192.168.11.64/ISAPI/Thermal/channels/1/faceThermometry/capabilities

	sprintf(szUrl, "GET /ISAPI/Thermal/channels/%d/faceThermometry/capabilities", 1);
	TraceLog(("skpark region url=%s", szUrl));
	TraceLog(("skpark region loginID[0]=%ld", lUserID));
	TraceLog(("skpark region loginID[1]=%ld", lUserID));
	xmlConfigInput.lpRequestUrl = szUrl;
	xmlConfigInput.dwRequestUrlLen = strlen(szUrl);

	memset(pOutBuf, 0, dwOutputLen);
	xmlCongfigOutput.dwOutBufferSize = dwOutputLen;
	xmlCongfigOutput.lpOutBuffer = pOutBuf;

	if (NET_DVR_STDXMLConfig(lUserID, &xmlConfigInput, &xmlCongfigOutput))
	{
		succeed = TRUE;
		//CXmlBase xmlBase;
		//xmlBase.Parse(pOutBuf);

		//if ((xmlBase.FindElem("RegionBoundary") && xmlBase.IntoElem() && xmlBase.FindElem("RegionCoordinatesList")))
		//{
		//	TraceLog(("parsing succeed"));
		//}


		CString resultStr = pOutBuf;
		resultStr.Remove(' ');
		resultStr.Remove('\t');
		resultStr.Remove('\n');
		resultStr.Remove('\r');
		TraceLog(("skpark region result succeed=%s", resultStr));
		float posX1 = -1.0f;
		float posY1 = -1.0f;
		float posX2 = -1.0f;
		float posY2 = -1.0f;
		CString keyVal = "<RegionBoundary><RegionCoordinatesListsize=\"4\"><RegionCoordinates><positionX>";
		int pos = resultStr.Find(keyVal);
		if (pos > 0)
		{
			pos += keyVal.GetLength();
			resultStr = resultStr.Mid(pos);

			keyVal = "</positionX><positionY>";
			pos = resultStr.Find(keyVal);
			if (pos > 0)
			{
				CString posXStr = resultStr.Mid(0, pos);
				posX1 = atof(posXStr);
				pos += keyVal.GetLength();
				resultStr = resultStr.Mid(pos);
				keyVal = "</positionY>";
				pos = resultStr.Find(keyVal);
				if (pos > 0)
				{
					CString posYStr = resultStr.Mid(0, pos);
					posY1 = atof(posYStr);
					keyVal = "</positionY></RegionCoordinates><RegionCoordinates><positionX>";
					pos = resultStr.Find(keyVal);
					if (pos > 0)
					{
						pos += keyVal.GetLength();
						resultStr = resultStr.Mid(pos);
						pos = resultStr.Find(keyVal);
						if (pos > 0)
						{
							pos += keyVal.GetLength();
							resultStr = resultStr.Mid(pos);
							keyVal = "</positionX><positionY>";
							pos = resultStr.Find(keyVal);
							if (pos > 0)
							{
								CString posXStr = resultStr.Mid(0, pos);
								posX2 = atof(posXStr);
								pos += keyVal.GetLength();
								resultStr = resultStr.Mid(pos);
								keyVal = "</positionY>";
								pos = resultStr.Find(keyVal);
								if (pos > 0)
								{
									CString posYStr = resultStr.Mid(0, pos);
									posY2 = atof(posYStr);
								}
							}
						}
					}
				}
			}
		}
		TraceLog(("posX1,Y1=(%f,%f)", posX1, posY1));
		TraceLog(("posX2,Y2=(%f,%f)", posX2, posY2));

		if (posX1 >= 0 && posY1 >= 0 && posX2 >= 0 && posY2 >= 0)
		{
			this->m_thermalX = STREAM_WIDTH *(posX1 / 1000.0f);
			this->m_thermalY = STREAM_HEIGHT *((1000.0f - posY1) / 1000.0f);

			TraceLog(("m_thermalX,m_thermalY=(%f,%f)", m_thermalX, m_thermalY));

			float thermalX2 = STREAM_WIDTH *(posX2 / 1000.0f);
			float thermalY2 = STREAM_HEIGHT *((1000.0f - posY2) / 1000.0f);

			TraceLog(("thermalX2,thermalY2=(%f,%f)", thermalX2, thermalY2));

			m_thermalWidth = thermalX2 - m_thermalX;
			m_thermalHeight = thermalY2 - m_thermalY;

			// real entire picture size = 2688x1520
			// 2688x(219/1000) = 588.672
			// 1520x(1000-913)/1000) = 1520x0.087 = 132.24
			// m_thermalWidth = 2228.0f - m_thermalX;
			// m_thermalHeight = 1365.0f - m_thermalY;
		}
	}
	else
	{
		TraceLog(("skpark region result failed=%s", pOutBuf));
	}


	delete[]pOutBuf;
	pOutBuf = NULL;

	/*
	CString vendorApi1;
	vendorApi1.Format("http://%s/ISAPI/Thermal/channels/%d/faceThermometry/capabilities", m_ip, m_cameraID);

	m_thermalX = 588.0f;
	m_thermalY = 133.0f;

	CHttpClient httpClient;
	string strRetData;
	string params = "";

	httpClient.Posts(string(vendorApi1), params, strRetData);
	strRetData = UTF8ToANSIString(strRetData);
	TraceLog(("skpark region api =%s", vendorApi1));
	TraceLog(("skpark region api result=%s", strRetData.c_str()));
	*/

	//real entire picture size = 2688x1520
	// 2688x(219/1000) = 588.672
	// 1520x(1000-913)/1000) = 1520x0.087 = 132.24
	
	return succeed;
}

void HikvisionCameraView::RefreshVideoEffect()
{
	TraceLog(("RefreshVideoEffect >>> lRealPlayHandle[%d] brightness[%d] contrast[%d] saturation[%d] hue[%d]", m_lRealPlayHandle, m_brightness, m_contrast, m_saturation, m_hue));

	if (!NET_DVR_ClientSetVideoEffect(m_lRealPlayHandle, m_brightness, m_contrast, m_saturation, m_hue)) {
		TraceLog(("NET_DVR_ClientSetVideoEffect error[%d]", NET_DVR_GetLastError()));
	}
}

BOOL HikvisionCameraView::BrightnessDown()
{
	if (0 == m_brightness)
		return FALSE;

	m_brightness--;
	RefreshVideoEffect();
	return TRUE;
}

BOOL HikvisionCameraView::BrightnessUp()
{
	if (10 == m_brightness)
		return FALSE;

	m_brightness++;
	RefreshVideoEffect();
	return TRUE;
}
	
BOOL HikvisionCameraView::ContrastDown()
{
	if (0 == m_contrast)
		return FALSE;

	m_contrast--;
	RefreshVideoEffect();
	return TRUE;
}

BOOL HikvisionCameraView::ContrastUp()
{
	if (10 == m_contrast)
		return FALSE;

	m_contrast++;
	RefreshVideoEffect();
	return TRUE;
}


bool HikvisionCameraView::CapturePicture(CString& captureFile)
{
	if (m_lRealPlayHandle < 0)
	{
		captureFile = "";
		return false;
	}
	CString sTemp;
	CTime time = CTime::GetCurrentTime();
	sTemp.Format("%sPicture\\", UBC_GUARDIAN_PATH);
	DWORD dwRet = GetFileAttributes(sTemp);
	if ((dwRet == -1) || !(dwRet & FILE_ATTRIBUTE_DIRECTORY))
	{
		CreateDirectory(sTemp, NULL);
	}
	captureFile.Format("%sCapture_%04d%02d%02d_%02d%02d%02d.bmp", sTemp,
		time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute(), time.GetSecond());

	//captureFile.Format("%sCapture.bmp", sTemp	);

	if (NET_DVR_CapturePicture(m_lRealPlayHandle, captureFile.GetBuffer()))
	{
		TraceLog(("NET_DVR_CapturePicture[%s] succeed", captureFile));
		return true;
	}
	
	if (NET_DVR_GetLastError() == NET_DVR_NONBLOCKING_CAPTURE_NOTSUPPORT)
	{
		if (NET_DVR_CapturePictureBlock(m_lRealPlayHandle, captureFile, 500))
		{
			TraceLog(("NET_DVR_CapturePictureBlock[%s] failed", captureFile));
		}
		else
		{
			TraceLog(("NET_DVR_CapturePictureBlock failed[%s]", captureFile));
		}
	}
	else
	{
		TraceLog(("NET_DVR_CapturePicture[%s] failed ", captureFile));
	}
	captureFile = "";
	return false;
}

bool HikvisionCameraView::TestPicture(CString& captureFile)
{
	return true;
}