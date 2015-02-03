#ifndef FULLSCREENRESOLUTIONS_H
#define FULLSCREENRESOLUTIONS_H

#include "ConfigDialog.h"

void fillFullscreenResolutionsList(QStringList & _listResolutions, int & _resolution, QStringList & _listRefreshRates, int & _rate);

void getFullscreenResolutions(int _idx, unsigned int & _width, unsigned int & _height);
void getFullscreenRefreshRate(int _idx, unsigned int & _rate);

#endif // FULLSCREENRESOLUTIONS_H

