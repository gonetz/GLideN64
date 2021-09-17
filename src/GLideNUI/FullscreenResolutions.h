#ifndef FULLSCREENRESOLUTIONS_H
#define FULLSCREENRESOLUTIONS_H

#include "ConfigDialog.h"

void fillFullscreenResolutionsList(int _monitorIdx, QStringList & _listResolutions, int & _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx);
void fillFullscreenRefreshRateList(int _monitorIdx, int _resolutionIdx, QStringList & _listRefreshRates, int & _rateIdx);
void fillFullscreenMonitorList(int & _monitorIdx, QStringList & _monitors);

void getFullscreenResolutions(int _monitorIdx, int _idx, unsigned int & _width, unsigned int & _height);
void getFullscreenRefreshRate(int _monitorIdx, int _idx, unsigned int & _rate);

#endif // FULLSCREENRESOLUTIONS_H

