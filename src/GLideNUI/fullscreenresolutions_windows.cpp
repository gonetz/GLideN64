#include <windows.h>
#include <QObject>
#include <stdio.h>
#include <math.h>
#include "FullscreenResolutions.h"
#include "../Config.h"

#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
	int count = -1;

	if (size != 0)
		count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
	if (count == -1)
		count = _vscprintf(format, ap);

	return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
	int count;
	va_list ap;

	va_start(ap, format);
	count = c99_vsnprintf(outBuf, size, format, ap);
	va_end(ap);

	return count;
}

#endif

static
struct
{
	struct
	{
		DWORD width, height, refreshRate;
	} selected;

	struct
	{
		DWORD	width, height;
	} resolution[32];

	DWORD refreshRate[32];

	DWORD	numResolutions;
	DWORD	numRefreshRates;
	CHAR 	deviceName[CCHDEVICENAME];
} fullscreen[32];

static
void _fillFullscreenRefreshRateList(int _monitorIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	memset(&fullscreen[_monitorIdx].refreshRate, 0, sizeof(fullscreen[_monitorIdx].refreshRate));
	fullscreen[_monitorIdx].numRefreshRates = 0;
	_rateIdx = 0;

	int i = 0;
	DEVMODEA deviceMode;
	while (EnumDisplaySettingsA(fullscreen[_monitorIdx].deviceName, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen[_monitorIdx].numRefreshRates; ++j)	{
			if ((deviceMode.dmDisplayFrequency == fullscreen[_monitorIdx].refreshRate[j]))
				break;
		}
		if ((deviceMode.dmDisplayFrequency != fullscreen[_monitorIdx].refreshRate[j]) &&
			(deviceMode.dmPelsWidth == fullscreen[_monitorIdx].selected.width) &&
			(deviceMode.dmPelsHeight == fullscreen[_monitorIdx].selected.height)) {

			fullscreen[_monitorIdx].refreshRate[j] = deviceMode.dmDisplayFrequency;
			//: Abbreviation for Hertz; include a leading space if appropriate
			_listRefreshRates.append(QString::number(deviceMode.dmDisplayFrequency) + QObject::tr(" Hz"));

			if (fullscreen[_monitorIdx].selected.refreshRate == deviceMode.dmDisplayFrequency)
				_rateIdx = fullscreen[_monitorIdx].numRefreshRates;

			++fullscreen[_monitorIdx].numRefreshRates;
		}
	}
}

void fillFullscreenResolutionsList(int _monitorIdx, QStringList & _listResolutions, int & _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen[_monitorIdx].selected.width = config.video.fullscreenWidth;
	fullscreen[_monitorIdx].selected.height = config.video.fullscreenHeight;
	fullscreen[_monitorIdx].selected.refreshRate = config.video.fullscreenRefresh;

	memset(&fullscreen[_monitorIdx].resolution, 0, sizeof(fullscreen[_monitorIdx].resolution));
	memset(&fullscreen[_monitorIdx].refreshRate, 0, sizeof(fullscreen[_monitorIdx].refreshRate));
	fullscreen[_monitorIdx].numResolutions = 0;
	fullscreen[_monitorIdx].numRefreshRates = 0;
	_resolutionIdx = 0;

	static
	struct
	{
		unsigned short x, y;
		const char *description;
	} ratios[] = {
		{ 3,  2, "3:2" },
		{ 4,  3, "4:3" },
		{ 5,  4, "5:4" },
		{ 16, 9, "16:9" },
		{ 8,  5, "16:10" },
		{ 21, 9, "21:9" }
	};
	const int numRatios = sizeof(ratios);

	int i = 0;
	char text[128];
	DEVMODEA deviceMode;
	while (EnumDisplaySettingsA(fullscreen[_monitorIdx].deviceName, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen[_monitorIdx].numResolutions; ++j) {
			if ((deviceMode.dmPelsWidth == fullscreen[_monitorIdx].resolution[j].width) &&
				(deviceMode.dmPelsHeight == fullscreen[_monitorIdx].resolution[j].height)) {
				break;
			}
		}
		if ((deviceMode.dmPelsWidth != fullscreen[_monitorIdx].resolution[j].width) ||
			(deviceMode.dmPelsHeight != fullscreen[_monitorIdx].resolution[j].height)) {

			fullscreen[_monitorIdx].resolution[fullscreen[_monitorIdx].numResolutions].width = deviceMode.dmPelsWidth;
			fullscreen[_monitorIdx].resolution[fullscreen[_monitorIdx].numResolutions].height = deviceMode.dmPelsHeight;
			snprintf(text, 128, "%i x %i", deviceMode.dmPelsWidth, deviceMode.dmPelsHeight);

			for (int j = 0; j < numRatios; ++j)
				if (fabs((float)deviceMode.dmPelsWidth / (float)deviceMode.dmPelsHeight
					- (float)ratios[j].x / (float)ratios[j].y) < 0.005f) {
					snprintf(text, 128, "%s (%s)", text, ratios[j].description);
					break;
				}

			_listResolutions.append(text);

			if ((fullscreen[_monitorIdx].selected.width == deviceMode.dmPelsWidth) &&
				(fullscreen[_monitorIdx].selected.height == deviceMode.dmPelsHeight))
				_resolutionIdx = fullscreen[_monitorIdx].numResolutions;

			++fullscreen[_monitorIdx].numResolutions;
		}
	}

	_fillFullscreenRefreshRateList(_monitorIdx, _listRefreshRates, _rateIdx);
}

void fillFullscreenRefreshRateList(int _monitorIdx, int _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen[_monitorIdx].selected.width = fullscreen[_monitorIdx].resolution[_resolutionIdx].width;
	fullscreen[_monitorIdx].selected.height = fullscreen[_monitorIdx].resolution[_resolutionIdx].height;
	_fillFullscreenRefreshRateList(_monitorIdx, _listRefreshRates, _rateIdx);
	_rateIdx = fullscreen[_monitorIdx].numRefreshRates - 1;
}

void getFullscreenResolutions(int _monitorIdx, int _idx, unsigned int & _width, unsigned int & _height)
{
	_width = fullscreen[_monitorIdx].resolution[_idx].width;
	_height = fullscreen[_monitorIdx].resolution[_idx].height;
}

static BOOL CALLBACK Monitorenumproc(HMONITOR monitorHandle, HDC deviceHandle, LPRECT monitorRect, LPARAM data)
{
	UNREFERENCED_PARAMETER(deviceHandle);
	UNREFERENCED_PARAMETER(monitorRect);

	int* currentMonitor = (int*)data;

	memset(&fullscreen[*currentMonitor].deviceName, 0, sizeof(fullscreen[*currentMonitor].deviceName));

	MONITORINFOEXA monitorInfo;
	monitorInfo.cbSize = sizeof(MONITORINFOEXA);
	if (GetMonitorInfoA(monitorHandle, &monitorInfo)) {
		strcpy(fullscreen[*currentMonitor].deviceName, monitorInfo.szDevice);
	}

	*currentMonitor = *currentMonitor + 1;
	return true;
}

void fillFullscreenMonitorList(int & _monitorIdx, QStringList & _monitors)
{
	_monitorIdx = config.video.fullscreenMonitor;

	int monitorCount = 0;
	EnumDisplayMonitors(NULL, NULL, Monitorenumproc, (LPARAM)&monitorCount);

	for (int i = 0; (i < monitorCount && i < 32); i++) {
		_monitors.append(QString::number(i + 1));
	}
}

void getFullscreenRefreshRate(int _monitorIdx, int _idx, unsigned int & _rate)
{
	_rate = fullscreen[_monitorIdx].refreshRate[_idx];
}
