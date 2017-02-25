#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef OS_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include "winlnxdefs.h"
#endif

#ifdef __LIBRETRO__
#include <glsm/glsmsym.h>
#include <GLideN64_libretro.h>
#endif

#ifdef PANDORA
typedef char GLchar;
#endif

#endif // PLATFORM_H