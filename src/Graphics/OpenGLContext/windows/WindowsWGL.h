#pragma once
#include <stdio.h>
#include <windows.h>
#include <GL/GL.h>
#include <GL/wglext.h>
#include <windows/GLideN64_Windows.h>

class WindowsWGL
{
public:
	static bool start();
	static void stop();
	static void swapBuffers();
	static unsigned int maxMSAALevel();

private:
	static HGLRC hRC;
	static HDC hDC;
	static unsigned int m_sMaxMsaa;
};

