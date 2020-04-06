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
#include "util.h"

CConfigDlg::CConfigDlg() :
    m_blockReInit(false),
    m_EmulationTab(NULL),
    m_Saved(false),
    m_TabLeft(0),
    m_ProfileLeft(0)
{
}

CConfigDlg::~CConfigDlg()
{
    m_EmulationTab = NULL;
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

    m_EmulationTab = new CEmulationTab(*this);

    m_Tabs.Attach(GetDlgItem(IDC_TABS));
    AddTab(L"Video", new CVideoTab);
    AddTab(L"Emulation", m_EmulationTab);
    AddTab(L"Frame buffer", new CFrameBufferTab);
    AddTab(L"Texture enhancement", new CTextureEnhancementTab);
    AddTab(L"OSD", new COsdTab);
    AddTab(L"Debug", new CDebugTab);

    RECT Rect;
    GetDlgItem(IDC_TABS).GetWindowRect(&Rect);
    ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);
    m_TabLeft = Rect.left;

    if (m_romName != NULL)
    {
        std::wstring RomName(ToUTF16(m_romName));
        CWindow dlgItem = GetDlgItem(IDC_GAME_PROFILE_NAME);
        CDC dc;
        dc.CreateCompatibleDC(NULL);
        dc.SelectFont(dlgItem.GetFont());
        SIZE size;
        dc.GetTextExtent(RomName.c_str(), RomName.length(), &size);

        RECT Rect;
        dlgItem.GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);
        Rect.right = Rect.left + size.cx;
        dlgItem.MoveWindow(&Rect);
        dlgItem.SetWindowText(RomName.c_str());

        m_ProfileLeft = Rect.right + 10;
    }
    else
    {
        GetDlgItem(IDC_SETTINGS_PROFILE_STATIC).GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);
        m_ProfileLeft = Rect.left;
    }

    Init();
    return 0;
}

void CConfigDlg::OnCustomSettingsToggled(bool checked)
{
    if (m_hWnd == NULL)
    {
        return;
    }
    checked = m_romName != NULL ? checked : false;
    GetDlgItem(IDC_GAME_PROFILE).ShowWindow(checked ? SW_SHOWNORMAL : SW_HIDE);
    GetDlgItem(IDC_SAVE_SETTINGS_STATIC).ShowWindow(checked ? SW_SHOWNORMAL : SW_HIDE);
    GetDlgItem(IDC_GAME_PROFILE_NAME).ShowWindow(checked ? SW_SHOWNORMAL : SW_HIDE);
    GetDlgItem(IDC_USE_PROFILE).ShowWindow(checked ? SW_SHOWNORMAL : SW_HIDE);

    int32_t Move = 0;
    if (checked)
    {
        RECT Rect;
        CWindow UseProfile = GetDlgItem(IDC_USE_PROFILE);
        UseProfile.GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);
        Move = Rect.left - m_ProfileLeft;
        if (Move != 0)
        {
            Rect.left -= Move;
            Rect.right -= Move;
            UseProfile.MoveWindow(&Rect);
        }
        uint32_t Left = Rect.right + 2;

        CWindow ProfileStatic = GetDlgItem(IDC_SETTINGS_PROFILE_STATIC);
        ProfileStatic.GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);
        Move = Rect.left - Left;
    }
    else
    {
        RECT Rect;
        GetDlgItem(IDC_SETTINGS_PROFILE_STATIC).GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);

        Move = Rect.left - m_TabLeft;
    }
    if (Move != 0)
    {
        int nID[] =
        {
            IDC_SETTINGS_PROFILE_STATIC,
            IDC_PROFILE,
            IDC_REMOVE_PROFILE,
        };

        RECT Rect;
        GetDlgItem(nID[0]).GetWindowRect(&Rect);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);

        for (size_t i = 0, n = sizeof(nID) / sizeof(nID[0]); i < n; i++)
        {
            CWindow window = GetDlgItem(nID[i]);
            window.GetWindowRect(&Rect);
            ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&Rect, 2);

            Rect.left -= Move;
            Rect.right -= Move;
            window.MoveWindow(&Rect);
        }
    }
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
    bool CustomSettings = m_EmulationTab != NULL && CButton(m_EmulationTab->GetDlgItem(IDC_CHK_USE_PER_GAME)).GetCheck() == BST_CHECKED;

    /*if (reInit && m_romName != NULL &&
        m_EmulationTab
        ui->customSettingsCheckBox->isChecked() && ui->settingsDestGameRadioButton->isChecked()) 
    {
        loadCustomRomSettings(m_strIniPath, m_romName);
    }
    else*/ if (reInit)
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
