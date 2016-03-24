#ifndef __LOG_H__
#define __LOG_H__

#define LOG_NONE	0
#define LOG_ERROR   1
#define LOG_MINIMAL	2
#define LOG_WARNING 3
#define LOG_VERBOSE 4
#define LOG_APIFUNC 5

#define LOG_LEVEL LOG_WARNING

#if LOG_LEVEL>0
#ifdef ANDROID
#include <android/log.h>

#define LOG(A, ...) \
    if (A <= LOG_LEVEL) \
    { \
		__android_log_print(ANDROID_LOG_DEBUG, "GLideN64", __VA_ARGS__); \
    }

#else // ANDROID
#include <stdio.h>
#include <stdarg.h>
inline void LOG( u16 type, const char * format, ... ) {
	if (type > LOG_LEVEL)
		return;
	FILE *dumpFile = fopen( "gliden64.log", "a+" );
	if (dumpFile == NULL)
		return;
	va_list va;
	va_start( va, format );
	vfprintf( dumpFile, format, va );
	fclose( dumpFile );
	va_end( va );
}
#endif // ANDROID
#else

#define LOG(A, ...)

#endif

#ifdef OS_WINDOWS
#include "windows/GLideN64_Windows.h"
#include <stdio.h>

inline void debugPrint(const char * format, ...) {
	char text[256];
	wchar_t wtext[256];
	va_list va;
	va_start(va, format);
	vsprintf(text, format, va);
	mbstowcs(wtext, text, 256);
	OutputDebugString(wtext);
	va_end(va);
}
#else
#define debugPrint(A, ...)
#endif

#endif
