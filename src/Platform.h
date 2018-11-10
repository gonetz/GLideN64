#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include "winlnxdefs.h"
#endif

#ifdef PANDORA
typedef char GLchar;
#endif

#endif // PLATFORM_H
