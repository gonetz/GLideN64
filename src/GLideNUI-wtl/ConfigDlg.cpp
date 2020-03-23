#pragma once
#include "wtl.h"
#include "ConfigDlg.h"
#include "../Config.h"
#include "Settings.h"
#include "config-video.h"
#include "config-emulation.h"
#include "config-framebuffer.h"
#include "config-texture.h"
#include "config-osd.h"
#include "config-debug.h"

CConfigDlg::CConfigDlg() :
    m_blockReInit(false),
    m_Saved(false)
{
}

CConfigDlg::~CConfigDlg()
{
    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        delete m_TabWindows[i];
    }
    m_TabWindows.clear();
}

void CConfigDlg::setIniPath(const std::string & IniPath)
{
    m_strIniPath = IniPath;
}

void CConfigDlg::setRomName(const char * RomName)
{
    m_romName = RomName == NULL || strlen(RomName) == 0 ? NULL : RomName;
}

LRESULT CConfigDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    HICON hIcon = AtlLoadIconImage(IDI_APPICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
    SetIcon(hIcon, TRUE);
    HICON hIconSmall = AtlLoadIconImage(IDI_APPICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
    SetIcon(hIconSmall, FALSE);

    m_Tabs.Attach(GetDlgItem(IDC_TABS));
    AddTab(L"Video", new CVideoTab);
    AddTab(L"Emulation", new CEmulationTab);
    AddTab(L"Frame buffer", new CFrameBufferTab);
    AddTab(L"Texture enhancement", new CTextureEnhancementTab);
    AddTab(L"OSD", new COsdTab);
    AddTab(L"Debug", new CDebugTab);

    Init();
    return 0;
}

void CConfigDlg::SaveSettings()
{
    m_Saved = true;
    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        m_TabWindows[i]->SaveSettings();
    }
    writeSettings(m_strIniPath.c_str());
}

LRESULT CConfigDlg::OnRestoreDefaults(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int Res = MessageBox(L"Are you sure you want to reset all settings to default?", L"Restore Defaults?", MB_YESNO | MB_ICONWARNING);
    if (Res == IDYES)
    {
        const u32 enableCustomSettings = config.generalEmulation.enableCustomSettings;
        config.resetToDefaults();
        config.generalEmulation.enableCustomSettings = enableCustomSettings;
        setRomName(m_romName);
        Init();
    }
    return 0;
}

LRESULT CConfigDlg::OnSaveClose(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SaveSettings();
    EndDialog(wID);
    return 0;
}

LRESULT CConfigDlg::OnSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SaveSettings();
    return 0;
}

LRESULT CConfigDlg::OnTabChange(NMHDR* /*pNMHDR*/)
{
    ShowTab(m_Tabs.GetCurSel());
    return FALSE;
}

LRESULT CConfigDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

void CConfigDlg::Init(bool reInit, bool blockCustomSettings)
{
    if (m_blockReInit)
    {
        return;
    }
    m_blockReInit = true;

    if (reInit) 
    {
        loadSettings(m_strIniPath.c_str());
    }

    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        m_TabWindows[i]->LoadSettings(blockCustomSettings);
    }
    m_blockReInit = false;
}

CRect CConfigDlg::GetTabRect()
{
    CRect TabRect;
    m_Tabs.GetWindowRect(&TabRect);
    ScreenToClient(&TabRect);
    m_Tabs.AdjustRect(FALSE, &TabRect);
    return TabRect;
}

void CConfigDlg::AddTab(const wchar_t * caption, CConfigTab * tab)
{
    m_Tabs.AddItem(caption);
    tab->Create(m_hWnd, 0);
    tab->SetWindowPos(m_hWnd, 0, 0, 0, 0, SWP_HIDEWINDOW);
    m_TabWindows.push_back(tab);

    if (m_TabWindows.size() == 1)
    {
        ShowTab(0);
    }
}

void CConfigDlg::ShowTab(int nPage)
{
    for (size_t i = 0; i < m_TabWindows.size(); i++)
    {
        m_TabWindows[i]->ShowWindow(SW_HIDE);
    }

    CRect TabRect = GetTabRect();
    m_TabWindows[nPage]->SetWindowPos(HWND_TOP, TabRect.left, TabRect.top, TabRect.Width(), TabRect.Height(), SWP_SHOWWINDOW);

    CRect WinRect, ClientRect;
    m_TabWindows[nPage]->GetWindowRect(WinRect);
    m_TabWindows[nPage]->GetClientRect(ClientRect);

    m_Tabs.RedrawWindow();
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
