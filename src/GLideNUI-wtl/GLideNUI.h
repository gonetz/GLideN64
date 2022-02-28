#ifndef GLIDENUII_H
#define GLIDENUII_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _WIN32
#define EXPORT	__declspec(dllexport)
#define CALL	  __cdecl
#else
#define EXPORT	__attribute__((visibility("default")))
#define CALL
#endif

EXPORT bool CALL RunConfig(const wchar_t * _strFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy);
EXPORT int CALL RunAbout(const wchar_t * _strFileName);
EXPORT void CALL LoadConfig(const wchar_t * _strFileName);
EXPORT void CALL LoadCustomRomSettings(const wchar_t * _strFileName, const char * _romName);

#if defined(__cplusplus)
}
#endif

#endif // GLIDENUII_H
