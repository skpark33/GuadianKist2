// stdafx.cpp: 미리 컴파일된 헤더에 해당하는 소스 파일

#include "stdafx.h"


// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.
CString LoadStringById(UINT nID, CString lang)
{
	CString strValue;
	if (!strValue.LoadString(nID)) return _T("");
	return strValue;

	//	USES_CONVERSION;
	//	return A2W(strValue);
	//	return ("ko" == lang ? A2W("℃") : A2W("?"));
}
