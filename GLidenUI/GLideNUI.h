#ifndef GLIDENUII_H
#define GLIDENUII_H

#include <windows.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define EXPORT	__declspec(dllexport)
#define CALL	__cdecl

	/******************************************************************
	Function: DllConfig
	Purpose:  This function is optional function that is provided
	to allow the user to configure the dll
	input:    a handle to the window that calls this function
	output:   none
	*******************************************************************/
	EXPORT int CALL RunConfig(HINSTANCE hInstance /*HWND hParent*/);

#if defined(__cplusplus)
}
#endif

#endif // GLIDENUII_H
