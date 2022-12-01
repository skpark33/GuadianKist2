// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#ifndef PCH_H
#define PCH_H

// 여기에 미리 컴파일하려는 헤더 추가
#include "framework.h"
#include <ostream>	// for vs2019
#include <string>

//skpark define start
// Original Source Resolution is 347:262
#define	WIN_WIDTH	1080
#define	WIN_HEIGHT	1920
#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

#define DISPLAY_FULL_WIDTH 1024		//default display width
#define DISPLAY_FULL_HEIGHT 768		//default display height


#define UBC_GUARDIAN_PATH		_T("C:\\SQISoft\\UTV1.0\\Guardian\\")
#define UBC_WM_KEYBOARD_EVENT	1000
#define UBCVARS_INI				_T("UBCVariables.ini")
#define UBCBRW_INI				_T("UBCBrowser.ini")
#define UBC_CONFIG_PATH			_T("C:\\SQISoft\\UTV1.0\\execute\\data\\")
#define UBC_EXE_PATH			_T("C:\\SQISoft\\UTV1.0\\execute\\")
#define UBC_CONTENTS_PATH		_T("C:\\SQISoft\\Contents\\Enc\\")
#define FACE_PATH				_T("C:\\SQISoft\\FACE\\")
#define UPLOAD_PATH				_T("C:\\SQISoft\\UPLOAD\\")
#define	PROPERTY_DELI1			_T("###")
#define	PROPERTY_DELI2			_T("+++")

#define	STREAM_WIDTH		2688.0f
#define STREAM_HEIGHT		1520.0f

#define ALARM_GO_ON_SEC		30

#define WM_UPDATEDATA					(WM_USER + 1)
#define WM_PROC_ALARM					(WM_USER + 5)		// alarm handle
#define	UM_WEATHER_MESSAGE				(WM_USER+201)		// weather, micro dust search
#define	UM_WEATHER_UPDATE_MESSAGE		(WM_USER+202)		// weather, micro dust updated
#define	UM_EVENT_UPDATE_MESSAGE			(WM_USER+203)		// Event Queue Update
#define	UM_AIR_MESSAGE					(WM_USER+204)		// 미세먼지정보 조회
#define UM_REFRESH_GUARDIAN_SETTINGS	(WM_USER+301)		// 설정변경 후 INI Reload요청

#define WM_FRRETRY_PUSH_EVENT			WM_USER + 110   // skpark  FR RETRY EVENT
#define WM_FRRETRY_FILE_DOWN_REQUEST	WM_USER + 111   // skpark  FR RETRY FILE DOWNLOAD
#define WM_FRRETRY_FILE_DOWN_RESPONSE	WM_USER + 112   // skpark  FR RETRY FILE DOWNLOAD_RES
#define WM_OLD_FILE_DELETE				WM_USER + 113   // skpark  OLD FILE DELETE
#define WM_BACK_TO_YOUR_POSTION   3000  //skpark browser should shrink
#define WM_BACK_TO_YOUR_POSTION_RESPONSE				3001   // skpark  OLD FILE DELETE
#define FG_COLOR_WHITE			RGB(0xff,0xff,0xff)

#define	ALARM_BG_COLOR			RGB(0x93, 0x1f, 0x1f)
#define	ALARM_BG_COLOR_MASK		RGB(0xff, 0xd9, 0x28)

#define ALARM_FG_COLOR_1		RGB(0xff,0xd9,0x28)
#define	ALARM_FONT_1			"Pretendard-Bold" 
#define ALARM_FONT_SIZE_1		96

#define ALARM_FG_COLOR_2		RGB(0xff,0xd9,0x28)
#define	ALARM_FONT_2			"Pretendard" 
#define ALARM_FONT_SIZE_2		80

#define ALARM_FG_COLOR_3		RGB(0xe4,0xe5,0xeb)
#define	ALARM_FONT_3			"Pretendard-Bold" 
#define ALARM_FONT_SIZE_3		42

#define ALARM_FG_COLOR_4		RGB(0xe4,0xe5,0xeb)
#define	ALARM_FONT_4			"Pretendard-Bold"
#define ALARM_FONT_SIZE_4		26

//#define	NORMAL_BG_COLOR			RGB(0x21,0x29,0x42)
#define	NORMAL_BG_COLOR			RGB(0xff,0xff,0xff)
#define NORMAL_FG_COLOR			RGB(0xff,0xff,0xff)

#define NORMAL_FG_COLOR_1		RGB(0xd6,0xd8,0xe4)
#define	NORMAL_FONT_1			"Pretendard"
#define NORMAL_FONT_SIZE_1		66

#define NORMAL_FG_COLOR_2		RGB(0xd6,0xd8,0xe4)
#define	NORMAL_FONT_2			"Pretendard"
#define NORMAL_FONT_SIZE_2		40

#define NORMAL_FG_COLOR_3		RGB(0xd4,0xd5,0xdb)
#define	NORMAL_FONT_3			"Pretendard"
#define NORMAL_FONT_SIZE_3		40



CString LoadStringById(UINT nID, CString lang);

#include <opencv2/core.hpp>		// Basic OpenCV structures (cv::Mat, Scalar)
#include <afxcontrolbars.h>

typedef struct _ST_EVENT {
	int		eventType;			// 0:Thermometry, 1:QRCode, 2:MaskDetection 
	CString detectedType;		// Detected Code Type(QR-Code, Bar-Code, Car-No)
	CString detectedCode;		// Detected Code
	cv::Mat	cropFace;			// Face Crop (with temperature)
	float	temperature;		// Face temperature
	int		maskLevel;			// Mask Detection Level(미검증:-1, 미착용:0, 착용:1)
	int		alarmLevel;			// (0:체온정상, 1:발열)
} ST_EVENT;


enum BG_MODE
{
	NONE,  // 버튼이 활성화 되기전
	INIT1,		// 버튼이 활성화 되기전 == 사실상 NONE 과 다를것이 없다.
	INIT2,  // 버튼이 활성화 된 상태
	WAIT,  // 버튼을 누른 상태
	FAIL1,  // 발열  fault
	FAIL2,  // 신원인식  fault
	IDENTIFIED,  // 확인이 완료된 상태
	NEXT,  // 확인이 완료된 후 3초 후,  다음단계로 넘어가라
};


/* State machine 

 init1(init 1 page) -->  얼굴을 디민다 --> init2 (int2 page 버튼 활성화) --> 버튼을 누른다 
 1> 발열자의 경우
 --> fail1 --> 3초후 --> init1

 2> 발열자가 아닌 경우
  --> waiting (버튼 비활성화, Waiting 사인) - 신원확인이 끝남
 

 1> 성공으로 끝난경우
	--> identified(identified page) --> 3초후 --> Next
 2)  실패로 끝난 경우
   --> fail2(fail2 page) --> 3초후 init1




*/



using namespace std;

#endif //PCH_H
