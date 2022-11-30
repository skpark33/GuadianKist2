#pragma once
#include <afxwin.h>
#include "XmlBase.h"

#define CAMERA_VIEW_CLASSNAME _T("HikvisionCameraView")

//Get Osd time macro
#define GET_YEAR(_time_)      (((_time_)>>26) + 2000) 
#define GET_MONTH(_time_)     (((_time_)>>22) & 15)
#define GET_DAY(_time_)       (((_time_)>>17) & 31)
#define GET_HOUR(_time_)      (((_time_)>>12) & 31) 
#define GET_MINUTE(_time_)    (((_time_)>>6)  & 63)
#define GET_SECOND(_time_)    (((_time_)>>0)  & 63)


//Alarm host
typedef struct tagLOCAL_ALARM_INFO
{
    int iDeviceIndex;
    LONG lCommand;
    char sDeviceIP[128];
    DWORD dwBufLen;
    WORD wLinkPort;
    tagLOCAL_ALARM_INFO()
    {
        iDeviceIndex = -1;
        lCommand = -1;
        memset(&sDeviceIP, 0, 128);
        dwBufLen = 0;
        wLinkPort = 0;
    }
}LOCAL_ALARM_INFO, * LPLOCAL_ALARM_INFO;


class HikvisionCameraView : public CWnd
{
    DECLARE_DYNAMIC(HikvisionCameraView)

public:
    HikvisionCameraView();
    virtual ~HikvisionCameraView();

    static BOOL RegisterWindowClass();
    int CameraConfig(int channel, const char* ip = "192.168.11.96", int port = 8000, const char* userid = "admin", const char* password = "hikvision123");
    int CleanUp();

private:
	LONG m_lRealPlayHandle;		// play handle
	LONG m_lPort;				// play port


protected:
    DECLARE_MESSAGE_MAP()
public:
    static	HikvisionCameraView* getInstance();

    static int m_eventPeriod;    // = 50;
    //static int m_eventCnt;     // = 3;

	float	m_thermalX;
	float	m_thermalY;
	float	m_thermalWidth;
	float	m_thermalHeight;

	bool	Ping();
	bool	GetThermalRegion();
	bool	SetFaceThermometry(float alarmTemperature, float alertMinTemperature);
	bool	SetTemperatureCompensation(LPCTSTR compensationType, float manualCalibration);

	BOOL BrightnessDown();
	BOOL BrightnessUp();
	BOOL ContrastDown();
	BOOL ContrastUp();

	// for QR-Code
	clock_t	m_clkStart, m_clkCur;

	// for FRRetry
	clock_t	m_clkFRRetryStart, m_clkFRRetryCur;  //skpark

	bool CapturePicture(CString& captureFile);
	bool TestPicture(CString& captureFile);

private:
    static HikvisionCameraView* g_instance;
	LONG	m_lUserID;

	DWORD	m_contrast;		// camera contrast  (alpha) [0-10]
	DWORD	m_brightness;	// camera brightness(beta)  [0-10]
	DWORD	m_saturation;
	DWORD	m_hue;
	void	RefreshVideoEffect();

	bool	ReadFaceThermometry(LONG lUserID);
	bool	ReadTemperatureCompensation(LONG lUserID);

	CXmlBase m_faceThermometryXml;
};

