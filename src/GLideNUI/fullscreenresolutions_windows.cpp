#include <windows.h>
#include "FullscreenResolutions.h"
#include "../Config.h"

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
} fullscreen;

static
void _fillFullscreenRefreshRateList(QStringList & _listRefreshRates, int & _rateIdx)
{
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numRefreshRates = 0;
	_rateIdx = 0;

	int i = 0;
	char text[128];
	DEVMODE deviceMode;
	while (EnumDisplaySettings(NULL, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen.numRefreshRates; ++j)	{
			if ((deviceMode.dmDisplayFrequency == fullscreen.refreshRate[j]))
				break;
		}
		if ((deviceMode.dmDisplayFrequency != fullscreen.refreshRate[j]) &&
			(deviceMode.dmPelsWidth == fullscreen.selected.width) &&
			(deviceMode.dmPelsHeight == fullscreen.selected.height)) {

			fullscreen.refreshRate[j] = deviceMode.dmDisplayFrequency;
			sprintf(text, "%i Hz", deviceMode.dmDisplayFrequency);
			_listRefreshRates.append(text);

			if (fullscreen.selected.refreshRate == deviceMode.dmDisplayFrequency)
				_rateIdx = fullscreen.numRefreshRates;

			++fullscreen.numRefreshRates;
		}
	}
}

void fillFullscreenResolutionsList(QStringList & _listResolutions, int & _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen.selected.width = config.video.fullscreenWidth;
	fullscreen.selected.height = config.video.fullscreenHeight;
	fullscreen.selected.refreshRate = config.video.fullscreenRefresh;

	memset(&fullscreen.resolution, 0, sizeof(fullscreen.resolution));
	memset(&fullscreen.refreshRate, 0, sizeof(fullscreen.refreshRate));
	fullscreen.numResolutions = 0;
	fullscreen.numRefreshRates = 0;
	_resolutionIdx = 0;

	int i = 0;
	char text[128];
	DEVMODE deviceMode;
	while (EnumDisplaySettings(NULL, i++, &deviceMode) != 0)
	{
		if (deviceMode.dmBitsPerPel != 32)
			continue;

		DWORD j = 0;
		for (; j < fullscreen.numResolutions; ++j) {
			if ((deviceMode.dmPelsWidth == fullscreen.resolution[j].width) &&
				(deviceMode.dmPelsHeight == fullscreen.resolution[j].height)) {
				break;
			}
		}
		if ((deviceMode.dmPelsWidth != fullscreen.resolution[j].width) ||
			(deviceMode.dmPelsHeight != fullscreen.resolution[j].height)) {

			fullscreen.resolution[fullscreen.numResolutions].width = deviceMode.dmPelsWidth;
			fullscreen.resolution[fullscreen.numResolutions].height = deviceMode.dmPelsHeight;
			sprintf(text, "%i x %i", deviceMode.dmPelsWidth, deviceMode.dmPelsHeight);
			_listResolutions.append(text);

			if ((fullscreen.selected.width == deviceMode.dmPelsWidth) &&
				(fullscreen.selected.height == deviceMode.dmPelsHeight))
				_resolutionIdx = fullscreen.numResolutions;

			++fullscreen.numResolutions;
		}
	}

	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);
}

void fillFullscreenRefreshRateList(int _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx)
{
	fullscreen.selected.width = fullscreen.resolution[_resolutionIdx].width;
	fullscreen.selected.height = fullscreen.resolution[_resolutionIdx].height;
	_fillFullscreenRefreshRateList(_listRefreshRates, _rateIdx);
	_rateIdx = fullscreen.numRefreshRates - 1;
}

void getFullscreenResolutions(int _idx, unsigned int & _width, unsigned int & _height)
{
	_width = fullscreen.resolution[_idx].width;
	_height = fullscreen.resolution[_idx].height;
}

void getFullscreenRefreshRate(int _idx, unsigned int & _rate)
{
	_rate = fullscreen.refreshRate[_idx];
}
