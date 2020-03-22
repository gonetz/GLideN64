#include "config-framebuffer.h"
#include "resource.h"
#include "../Config.h"

CFrameBufferTab::CFrameBufferTab() :
    CConfigTab(IDD_TAB_FRAME_BUFFER)
{
}

BOOL CFrameBufferTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_EmulateFBIcon.SubclassWindow(GetDlgItem(IDC_EMULATE_FB_ICON));
    m_EmulateFBIcon.SetIcon(MAKEINTRESOURCE(IDI_ICON_WARNING), 16, 16);
    m_EmulateFBIcon.SetBackroundBrush((HBRUSH)GetStockObject(WHITE_BRUSH));

    CComboBox frameBufferSwapComboBox(GetDlgItem(IDC_CMB_FRAMEBUFFER_SWAP));
    frameBufferSwapComboBox.AddString(L"Vertical interrupt (recommended, fewest game issues)");
    frameBufferSwapComboBox.AddString(L"VI origin change (faster, few game issues)");
    frameBufferSwapComboBox.AddString(L"Color buffer change (fastest, some game issues)");

    CComboBox copyColorBufferComboBox(GetDlgItem(IDC_CMB_COPY_COLOR_BUFFER));
    copyColorBufferComboBox.AddString(L"Never (fastest, many game issues)");
    copyColorBufferComboBox.AddString(L"Synchronous (slowest, fewest game issues)");
    copyColorBufferComboBox.AddString(L"Asynchronous (fast, few game issues)");

    CComboBox copyDepthBufferComboBox(GetDlgItem(IDC_CMB_COPY_DEPTH_BUFFER));
    copyDepthBufferComboBox.AddString(L"Never (fastest, most game issues)");
    copyDepthBufferComboBox.AddString(L"From VRAM (slow, some game issues)");
    copyDepthBufferComboBox.AddString(L"In software (fast, fewest game issues)");

    CComboBox n64DepthCompareComboBox(GetDlgItem(IDC_CMB_N64_DEPTH_COMPARE));
    n64DepthCompareComboBox.AddString(L"Disable");
    n64DepthCompareComboBox.AddString(L"Fast");
    n64DepthCompareComboBox.AddString(L"Compatible");
    return true;
}

LRESULT CFrameBufferTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

LRESULT CFrameBufferTab::OnEnableFramebuffer(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    bool fbEmulationEnabled = CButton(GetDlgItem(IDC_CHK_ENABLE_FRAMEBUFFER)).GetCheck() == BST_CHECKED;
    bool fbInfoEnabled = CButton(GetDlgItem(IDC_CHK_FB_INFO_ENABLE)).GetCheck() == BST_CHECKED;
    CButton(GetDlgItem(IDC_CHK_COPY_AUX_BUFFERS)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CMB_FRAMEBUFFER_SWAP)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CHK_FB_INFO_ENABLE)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CMB_COPY_COLOR_BUFFER)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CMB_COPY_DEPTH_BUFFER)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CMB_N64_DEPTH_COMPARE)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CHK_FORCE_DEPTH_BUFFER_CLEAR)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CHK_RENDER_FRAMEBUFFER)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CHK_COPY_DEPTH_TO_MAIN_DEPTH_BUFFER)).EnableWindow(fbEmulationEnabled);
    CButton(GetDlgItem(IDC_CHK_READ_COLOR_CHUNK)).EnableWindow(fbEmulationEnabled && fbInfoEnabled);
    CButton(GetDlgItem(IDC_CHK_READ_DEPTH_CHUNK)).EnableWindow(fbEmulationEnabled && fbInfoEnabled);
    return 0;
}


LRESULT CFrameBufferTab::OnFbInfoEnable(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    bool fbInfoEnabled = CButton(GetDlgItem(IDC_CHK_FB_INFO_ENABLE)).GetCheck() == BST_CHECKED;
    CButton(GetDlgItem(IDC_CHK_READ_COLOR_CHUNK)).EnableWindow(fbInfoEnabled);
    CButton(GetDlgItem(IDC_CHK_READ_DEPTH_CHUNK)).EnableWindow(fbInfoEnabled);
    return 0;
}

void CFrameBufferTab::LoadSettings(bool /*blockCustomSettings*/)
{
}
