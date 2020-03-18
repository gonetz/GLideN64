#pragma once
#include <string>
#include "wtl.h"
#include "resource.h"

class CConfigDlg :
    public CDialogImpl<CConfigDlg>
{
public:
    CConfigDlg();
    ~CConfigDlg();

    enum { IDD = IDD_CONFIG };

    BEGIN_MSG_MAP_EX(CConfigDlg)
        COMMAND_ID_HANDLER(ID_SAVECLOSE, OnSaveClose)
        COMMAND_ID_HANDLER(ID_SAVE, OnSave)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    void setIniPath(const std::string & IniPath);
    void setRomName(const char * RomName);

protected:
    LRESULT OnSaveClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    std::string m_strIniPath;
    const char * m_romName;
};

#ifdef _WIN32
void ConfigInit(void * hinst);
void ConfigCleanup(void);
#endif