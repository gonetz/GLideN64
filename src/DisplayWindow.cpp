#include <assert.h>
#include "Config.h"
#include "VI.h"
#include "Graphics/Context.h"
#include "DisplayWindow.h"

void DisplayWindow::start()
{
	_start(); // TODO: process initialization error
	gfxContext.init();
	m_drawer._initData();
	m_buffersSwapCount = 0;
}

void DisplayWindow::stop()
{
	m_drawer._destroyData();
	gfxContext.destroy();
	_stop();
}

void DisplayWindow::restart()
{
	m_bResizeWindow = true;
}

void DisplayWindow::swapBuffers()
{
	m_drawer.drawOSD();
	_swapBuffers();
	gDP.otherMode.l = 0;
	if ((config.generalEmulation.hacks & hack_doNotResetTLUTmode) == 0)
		gDPSetTextureLUT(G_TT_NONE);
	++m_buffersSwapCount;
}

void DisplayWindow::setCaptureScreen(const char * const _strDirectory)
{
	::mbstowcs(m_strScreenDirectory, _strDirectory, PLUGIN_PATH_SIZE - 1);
	m_bCaptureScreen = true;
}

void DisplayWindow::saveScreenshot()
{
	if (!m_bCaptureScreen)
		return;
	_saveScreenshot();
	m_bCaptureScreen = false;
}

bool DisplayWindow::changeWindow()
{
	if (!m_bToggleFullscreen)
		return false;
	m_drawer._destroyData();
	_changeWindow();
	updateScale();
	m_drawer._initData();
	m_bToggleFullscreen = false;
	return true;
}

void DisplayWindow::setWindowSize(u32 _width, u32 _height)
{
	if (m_width != _width || m_height != _height) {
		m_resizeWidth = _width;
		m_resizeHeight = _height;
		m_bResizeWindow = true;
	}
}

bool DisplayWindow::resizeWindow()
{
	if (!m_bResizeWindow)
		return false;
	m_drawer._destroyData();
	if (!_resizeWindow())
		_start();
	updateScale();
	m_drawer._initData();
	m_bResizeWindow = false;
	return true;
}

void DisplayWindow::updateScale()
{
	if (VI.width == 0 || VI.height == 0)
		return;
	m_scaleX = m_width / (float)VI.width;
	m_scaleY = m_height / (float)VI.height;
}

void DisplayWindow::_setBufferSize()
{
	m_bAdjustScreen = false;
	if (config.frameBufferEmulation.enable) {
		switch (config.frameBufferEmulation.aspect) {
		case Config::aStretch: // stretch
			m_width = m_screenWidth;
			m_height = m_screenHeight;
			break;
		case Config::a43: // force 4/3
			if (m_screenWidth * 3 / 4 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 4 / 3;
			}
			else if (m_screenHeight * 4 / 3 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 3 / 4;
			}
			else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
		case Config::a169: // force 16/9
			if (m_screenWidth * 9 / 16 > m_screenHeight) {
				m_height = m_screenHeight;
				m_width = m_screenHeight * 16 / 9;
			}
			else if (m_screenHeight * 16 / 9 > m_screenWidth) {
				m_width = m_screenWidth;
				m_height = m_screenWidth * 9 / 16;
			}
			else {
				m_width = m_screenWidth;
				m_height = m_screenHeight;
			}
			break;
		case Config::aAdjust: // adjust
			m_width = m_screenWidth;
			m_height = m_screenHeight;
			if (m_screenWidth * 3 / 4 > m_screenHeight) {
				f32 width43 = m_screenHeight * 4.0f / 3.0f;
				m_adjustScale = width43 / m_screenWidth;
				m_bAdjustScreen = true;
			}
			break;
		default:
			assert(false && "Unknown aspect ratio");
			m_width = m_screenWidth;
			m_height = m_screenHeight;
		}
	}
	else {
		m_width = m_screenWidth;
		m_height = m_screenHeight;
		if (config.frameBufferEmulation.aspect == Config::aAdjust && (m_screenWidth * 3 / 4 > m_screenHeight)) {
			f32 width43 = m_screenHeight * 4.0f / 3.0f;
			m_adjustScale = width43 / m_screenWidth;
			m_bAdjustScreen = true;
		}
	}
}

void DisplayWindow::readScreen(void **_pDest, long *_pWidth, long *_pHeight)
{
	_readScreen(_pDest, _pWidth, _pHeight);
}

void DisplayWindow::readScreen2(void * _dest, int * _width, int * _height, int _front)
{
	_readScreen2(_dest, _width, _height, _front);
}
