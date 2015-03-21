#ifndef SETTINGS_H
#define SETTINGS_H

void loadSettings(const QString & _strFileName);
void writeSettings(const QString & _strFileName);
void loadCustomRomSettings(const QString & _strFileName, const QString & _strRomName);

#endif // SETTINGS_H

