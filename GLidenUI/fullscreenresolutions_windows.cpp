#include "FullscreenResolutions.h"

struct
{
	struct
	{
		unsigned int width, height, bitDepth, refreshRate;
	} selected;

	unsigned int bitDepth[4];

	struct
	{
		unsigned int	width, height;
	} resolution[32];

	unsigned int refreshRate[32];

	unsigned int	numBitDepths;
	unsigned int	numResolutions;
	unsigned int	numRefreshRates;
} fullscreen;


void fillFullscreenResolutionsList(QStringList & _list)
{

}
