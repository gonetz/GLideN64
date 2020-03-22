#include "config-video.h"
#include "FullscreenResolutions.h"
#include "util.h"
#include "../Config.h"

static struct
{
    unsigned short width, height;
    LPCTSTR description;
}
WindowedModes[] =
{
    { 320, 240, _T("320 x 240") },
    { 400, 300, _T("400 x 300") },
    { 480, 360, _T("480 x 360") },
    { 640, 480, _T("640 x 480") },
    { 800, 600, _T("800 x 600") },
    { 960, 720, _T("960 x 720") },
    { 1024, 768, _T("1024 x 768") },
    { 1152, 864, _T("1152 x 864") },
    { 1280, 960, _T("1280 x 960") },
    { 1280, 1024, _T("1280 x 1024") },
    { 1440, 1080, _T("1440 x 1080") },
    { 1600, 1024, _T("1600 x 1024") },
    { 1600, 1200, _T("1600 x 1200") }
};
static const unsigned int numWindowedModes = sizeof(WindowedModes) / sizeof(WindowedModes[0]);


CVideoTab::CVideoTab() :
    CConfigTab(IDD_TAB_VIDEO)
{
}

CVideoTab::~CVideoTab()
{
    for (size_t i = 0; i < m_OverscanTabs.size(); i++)
    {
        delete m_OverscanTabs[i];
    }
    m_OverscanTabs.clear();
}

BOOL CVideoTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_OverScanTab.Attach(GetDlgItem(IDC_TAB_OVERSCAN));
    AddOverScanTab(L"NTSC");
    AddOverScanTab(L"PAL");

    m_AliasingSlider.Attach(GetDlgItem(IDC_ALIASING_SLIDER));
    m_AliasingSlider.SetTicFreq(1);
    m_AliasingSlider.SetRangeMin(0);
    m_AliasingSlider.SetRangeMax(3);

    m_AnisotropicSlider.Attach(GetDlgItem(IDC_ANISOTROPIC_SLIDER));
    m_AnisotropicSlider.SetTicFreq(1);
    m_AnisotropicSlider.SetRangeMin(0);
    m_AnisotropicSlider.SetRangeMax(16);

    CComboBox aspectComboBox(GetDlgItem(IDC_CMB_ASPECT_RATIO));
    aspectComboBox.AddString(L"4:3 (recommended)");
    aspectComboBox.AddString(L"16:9");
    aspectComboBox.AddString(L"Stretch");
    aspectComboBox.AddString(L"Try to adjust game to fit");

    m_AAInfoIcon.SubclassWindow(GetDlgItem(IDC_AA_INFO_ICON));
    m_AAInfoIcon.SetIcon(MAKEINTRESOURCE(IDI_ICON_INFO), 16, 16);
    m_AAInfoIcon.SetBackroundBrush((HBRUSH)GetStockObject(WHITE_BRUSH));
    return true;
}

LRESULT CVideoTab::OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG SliderId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (SliderId == IDC_ALIASING_SLIDER)
    {
        int32_t value = m_AliasingSlider.GetPos();
        std::wstring AliasingText = FormatStrW(L"%dx", value > 0 ? 1 << value : 0);
        CWindow(GetDlgItem(IDC_ALIASING_LABEL)).SetWindowTextW(AliasingText.c_str());
        CButton(GetDlgItem(value != 0 ? IDC_MSAA_RADIO : IDC_NOAA_RADIO)).SetCheck(BST_CHECKED);
        CButton(GetDlgItem(value != 0 ? IDC_NOAA_RADIO : IDC_MSAA_RADIO)).SetCheck(BST_UNCHECKED);
        CButton(GetDlgItem(IDC_FXAA_RADIO)).SetCheck(BST_UNCHECKED);
    }
    else if (SliderId == IDC_ANISOTROPIC_SLIDER)
    {
        CWindow(GetDlgItem(IDC_ANISOTROPIC_LABEL)).SetWindowTextW(FormatStrW(L"%dx", m_AnisotropicSlider.GetPos()).c_str());
    }
    return 0;
}

void CVideoTab::OnOverscan(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    CButton OverScan(GetDlgItem(IDC_CHK_OVERSCAN));
    if (OverScan.GetCheck() == BST_CHECKED)
    {
        GetDlgItem(IDC_TAB_OVERSCAN).ShowWindow(SW_SHOW);
        ShowOverScanTab(m_OverScanTab.GetCurSel());
    }
    else
    {
        GetDlgItem(IDC_TAB_OVERSCAN).ShowWindow(SW_HIDE);
        m_OverscanTabs[m_OverScanTab.GetCurSel()]->ShowWindow(SW_HIDE);
    }
}

LRESULT CVideoTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

LRESULT CVideoTab::OnOverscanTabChange(NMHDR* /*pNMHDR*/)
{
    ShowOverScanTab(m_OverScanTab.GetCurSel());
    return FALSE;
}

void CVideoTab::OnFullScreenChanged(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    CComboBox fullScreenResolutionComboBox(GetDlgItem(IDC_CMB_FULL_SCREEN_RES));
    int32_t index = fullScreenResolutionComboBox.GetCurSel();
    StringList fullscreenRatesList;
    int fullscreenRate;
    fillFullscreenRefreshRateList(index, fullscreenRatesList, fullscreenRate);

    CComboBox RefreshRateComboBox(GetDlgItem(IDC_CMB_REFRESH_RATE));
    RefreshRateComboBox.ResetContent();
    for (size_t i = 0, n = fullscreenRatesList.size(); i < n; i++)
    {
        std::wstring fullscreenRateStr(fullscreenRatesList[i].begin(), fullscreenRatesList[i].end());
        int index = RefreshRateComboBox.AddString(fullscreenRateStr.c_str());
        if (fullscreenRate == i)
        {
            RefreshRateComboBox.SetCurSel(index);
        }
    }
}

void CVideoTab::AddOverScanTab(const wchar_t * caption)
{
    m_OverScanTab.AddItem(caption);

    COverScanTab * tab = new COverScanTab;
    tab->Create(m_hWnd, 0);
    tab->SetWindowPos(m_hWnd, 0, 0, 0, 0, SWP_HIDEWINDOW);
    m_OverscanTabs.push_back(tab);

    if (m_OverscanTabs.size() == 1)
    {
        ShowOverScanTab(0);
    }
}

void CVideoTab::ShowOverScanTab(int nTab)
{
    for (size_t i = 0; i < m_OverscanTabs.size(); i++)
    {
        m_OverscanTabs[i]->ShowWindow(SW_HIDE);
    }

    CRect TabRect;
    m_OverScanTab.GetWindowRect(&TabRect);
    ScreenToClient(&TabRect);
    m_OverScanTab.AdjustRect(FALSE, &TabRect);

    m_OverscanTabs[nTab]->SetWindowPos(HWND_TOP, TabRect.left, TabRect.top, TabRect.Width(), TabRect.Height(), SWP_SHOWWINDOW);

    CRect WinRect, ClientRect;
    m_OverscanTabs[nTab]->GetWindowRect(WinRect);
    m_OverscanTabs[nTab]->GetClientRect(ClientRect);

    m_OverScanTab.RedrawWindow();
}

void CVideoTab::LoadSettings(bool /*blockCustomSettings*/)
{
    CComboBox WindowedResolutionComboBox(GetDlgItem(IDC_CMB_WINDOWED_RESOLUTION));
    for (unsigned int i = 0; i < numWindowedModes; ++i)
    {
        int index = WindowedResolutionComboBox.AddString(WindowedModes[i].description);
        WindowedResolutionComboBox.SetItemData(index, i);
        if (WindowedModes[i].width == config.video.windowedWidth &&
            WindowedModes[i].height == config.video.windowedHeight)
        {
            WindowedResolutionComboBox.SetCurSel(index);
        }
    }
    if (WindowedResolutionComboBox.GetCount() > 0 && WindowedResolutionComboBox.GetCurSel() < 0)
    {
        WindowedResolutionComboBox.SetCurSel(0);
    }

    CButton overscanCheckBox(GetDlgItem(IDC_CHK_OVERSCAN));
    overscanCheckBox.SetCheck(config.frameBufferEmulation.enableOverscan != 0 ? BST_CHECKED : BST_UNCHECKED);
    OnOverscan(0, 0, NULL);

    m_OverscanTabs[0]->SetValue(
        config.frameBufferEmulation.overscanNTSC.left,
        config.frameBufferEmulation.overscanNTSC.right,
        config.frameBufferEmulation.overscanNTSC.top,
        config.frameBufferEmulation.overscanNTSC.bottom
    );

    m_OverscanTabs[1]->SetValue(
        config.frameBufferEmulation.overscanPAL.left,
        config.frameBufferEmulation.overscanPAL.right,
        config.frameBufferEmulation.overscanPAL.top,
        config.frameBufferEmulation.overscanPAL.bottom
    );

    StringList fullscreenModesList, fullscreenRatesList;
    int fullscreenMode, fullscreenRate;
    fillFullscreenResolutionsList(fullscreenModesList, fullscreenMode, fullscreenRatesList, fullscreenRate);

    CComboBox fullScreenResolutionComboBox(GetDlgItem(IDC_CMB_FULL_SCREEN_RES));
    fullScreenResolutionComboBox.ResetContent();
    for (size_t i = 0, n = fullscreenModesList.size(); i < n; i++)
    {
        std::wstring fullscreenModeStr(fullscreenModesList[i].begin(), fullscreenModesList[i].end());
        int index = fullScreenResolutionComboBox.AddString(fullscreenModeStr.c_str());
        if (fullscreenMode == i)
        {
            fullScreenResolutionComboBox.SetCurSel(index);
        }
    }
    OnFullScreenChanged(0, 0, NULL);
    m_AliasingSlider.SetPos(config.video.multisampling >> 1);
    std::wstring AliasingText = FormatStrW(L"%dx", config.video.multisampling > 0 ? 1 << config.video.multisampling : 0);
    CWindow(GetDlgItem(IDC_ALIASING_LABEL)).SetWindowTextW(AliasingText.c_str());

    CButton(GetDlgItem(IDC_NOAA_RADIO)).SetCheck(config.video.multisampling == 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FXAA_RADIO)).SetCheck(config.video.fxaa != 0 && config.video.multisampling != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_MSAA_RADIO)).SetCheck(config.video.fxaa == 0 && config.video.multisampling != 0 ? BST_CHECKED : BST_UNCHECKED);
    m_AnisotropicSlider.SetPos(config.texture.maxAnisotropy);
    CWindow(GetDlgItem(IDC_ANISOTROPIC_LABEL)).SetWindowTextW(FormatStrW(L"%dx", m_AnisotropicSlider.GetPos()).c_str());

    CButton(GetDlgItem(IDC_CHK_VERTICAL_SYNC)).SetCheck(config.video.verticalSync != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_THREADED_VIDEO)).SetCheck(config.video.threadedVideo != 0 ? BST_CHECKED : BST_UNCHECKED);

    CButton(GetDlgItem(IDC_BILINEAR_3POINT)).SetCheck(config.texture.bilinearMode == BILINEAR_3POINT ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_BILINEAR_STANDARD)).SetCheck(config.texture.bilinearMode == BILINEAR_STANDARD ? BST_CHECKED : BST_UNCHECKED);

    CButton(GetDlgItem(IDC_SCREENSHOT_PNG)).SetCheck(config.texture.screenShotFormat == 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_SCREENSHOT_JPEG)).SetCheck(config.texture.screenShotFormat == 1 ? BST_CHECKED : BST_UNCHECKED);

    CComboBox aspectComboBox(GetDlgItem(IDC_CMB_ASPECT_RATIO));
    switch (config.frameBufferEmulation.aspect)
    {
    case Config::aStretch:
        aspectComboBox.SetCurSel(2);
        break;
    case Config::a43:
        aspectComboBox.SetCurSel(0);
        break;
    case Config::a169:
        aspectComboBox.SetCurSel(1);
        break;
    case Config::aAdjust:
        aspectComboBox.SetCurSel(3);
        break;
    }
}
