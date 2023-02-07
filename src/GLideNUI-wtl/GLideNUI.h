#ifndef GLIDENUII_H
#define GLIDENUII_H

class Config;

bool RunConfig(const wchar_t * _strFileName, const char * _romName, unsigned int _maxMSAALevel, unsigned int _maxAnisotropy);
int RunAbout(const wchar_t * _strFileName);
void LoadConfig(Config* cfg, const wchar_t * _strFileName);
void LoadCustomRomSettings(Config* cfg, const wchar_t * _strFileName, const char * _romName);

#endif // GLIDENUII_H
