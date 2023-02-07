#pragma once
#include <string>
#include <set>
typedef std::set<std::string> ProfileList;

class Config;

void loadSettings(Config&, const char * _strIniFolder);
void writeSettings(Config&, const char * _strIniFolder);
void loadCustomRomSettings(Config&, const char * _strIniFolder, const char * _strRomName);
void saveCustomRomSettings(Config&, const char * _strIniFolder, const char * _strRomName);
std::string getTranslationFile(Config&);
ProfileList getProfiles(const char * _strIniFolder);
std::string getCurrentProfile(const char * _strIniFolder);
void changeProfile(Config&, const char * _strIniFolder, const char * _strProfile);
void addProfile(Config&, const char * _strIniFolder, const char * _strProfile);
void removeProfile(const char * _strIniFolder, const char * _strProfile);
