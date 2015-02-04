#ifndef GLIDENUII_H
#define GLIDENUII_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef OS_WINDOWS
#define EXPORT	__declspec(dllexport)
#define CALL		__cdecl
#else
#define EXPORT 	__attribute__((visibility("default")))
#define CALL          _cdecl
#endif

EXPORT int CALL RunConfig();
EXPORT void CALL LoadConfig();

#if defined(__cplusplus)
}
#endif

#endif // GLIDENUII_H
