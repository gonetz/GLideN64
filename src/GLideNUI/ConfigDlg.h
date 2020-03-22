#pragma once
#include <string>
#include "wtl.h"
#include "config-tab.h"
#include "resource.h"
#include <vector>

class CConfigDlg :
    public CDialogImpl<CConfigDlg>
{
public:
    CConfigDlg();
    ~CConfigDlg();

    enum { IDD = IDD_CONFIG };

    BEGIN_MSG_MAP_EX(CConfigDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        NOTIFY_HANDLER_EX(IDC_TABS, TCN_SELCHANGE, OnTabChange)
        COMMAND_ID_HANDLER(ID_RESTORE_DEFAULTS, OnRestoreDefaults)
        COMMAND_ID_HANDLER(ID_SAVECLOSE, OnSaveClose)
        COMMAND_ID_HANDLER(ID_SAVE, OnSave)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
    END_MSG_MAP()

    void setIniPath(const std::string & IniPath);
    void setRomName(const char * RomName);
    bool Saved(void) const { return m_Saved; }

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRestoreDefaults(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSaveClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnSave(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTabChange(NMHDR* pNMHDR);

    void Init(bool reInit = false, bool blockCustomSettings = false);
    void AddTab(const wchar_t * caption, CConfigTab * tab);
    void ShowTab(int nPage);
    CRect GetTabRect();
    void SaveSettings();

    CTabCtrl m_Tabs;
    std::vector<CConfigTab *> m_TabWindows;
    std::string m_strIniPath;
    const char * m_romName;
    bool m_blockReInit;
    bool m_Saved;
};

#ifdef _WIN32
void ConfigInit(void * hinst);
void ConfigCleanup(void);
#endif