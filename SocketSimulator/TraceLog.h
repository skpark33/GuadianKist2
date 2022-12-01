#pragma once

#ifdef _RELEASE_LOG
	#define TraceLog(_f_) \
		{ \
			CString szTraceLogBuf; \
			szTraceLogBuf.Format(_T("%s(%d) : "), __FILE__, __LINE__); \
			szTraceLogBuf.AppendFormat _f_; \
			OutputDebugString(szTraceLogBuf); \
		}
#elif _DEBUG
	#define TraceLog(_f_) \
		{ \
			CString szTraceLogBuf; \
			szTraceLogBuf.Format _f_; \
			TRACE("%s(%d) :\n >>> %s\n", __FILE__, __LINE__, szTraceLogBuf); \
		}
#elif _TAO
	#include "..\UBCCopCommon\CopModule.h"
	#define TraceLog(_f_) \
		{ \
			CString szTraceLogBuf; \
			szTraceLogBuf.Format _f_; \
			CCopModule::Trace(__FILE__, __LINE__, szTraceLogBuf); \
		}
#else
	#define TraceLog(_f_) \
	{ \
	CString szTraceLogBuf; \
	szTraceLogBuf.Format(_T("%s(%d) : "), __FILE__, __LINE__); \
	szTraceLogBuf.AppendFormat _f_; \
	OutputDebugString(szTraceLogBuf); \
		}
#endif//_DEBUG