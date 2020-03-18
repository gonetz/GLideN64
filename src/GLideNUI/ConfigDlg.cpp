#pragma once
#include "wtl.h"
#include "ConfigDlg.h"

CConfigDlg::CConfigDlg() 
{
}

CConfigDlg::~CConfigDlg()
{
}

void CConfigDlg::setIniPath(const std::string & IniPath)
{
    m_strIniPath = IniPath;
}

void CConfigDlg::setRomName(const char * RomName)
{
    m_romName = RomName == NULL || strlen(RomName) == 0 ? NULL : RomName;
}

LRESULT CConfigDlg::OnSaveClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CConfigDlg::OnSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    return 0;
}

LRESULT CConfigDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

class GlideN64WtlModule :
    public CAppModule
{
public:
    GlideN64WtlModule(HINSTANCE hinst)
    {
        Init(NULL, hinst);
    }
    virtual ~GlideN64WtlModule(void)
    {
        Term();
    }
};

GlideN64WtlModule * WtlModule = NULL;

void ConfigInit(void * hinst)
{
    WtlModule = new GlideN64WtlModule((HINSTANCE)hinst);
}

void ConfigCleanup(void)
{
    if (WtlModule)
    {
        delete WtlModule;
        WtlModule = NULL;
    }
}
