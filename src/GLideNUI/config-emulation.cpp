#include "config-emulation.h"
#include "util.h"
#include "../Config.h"
#include "resource.h"

CEmulationTab::CEmulationTab() :
    CConfigTab(IDD_TAB_EMULATION)
{
}

BOOL CEmulationTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CComboBox nativeRes2DComboBox(GetDlgItem(IDC_CMB_NATIVE_RES_2D));
    nativeRes2DComboBox.ResetContent();
    nativeRes2DComboBox.AddString(L"Disable");
    nativeRes2DComboBox.AddString(L"Enable optimized");
    nativeRes2DComboBox.AddString(L"Enable unoptimized");

    m_GamaTxt.Attach(GetDlgItem(IDC_GAMMA_VALUE));
    m_GamaSpin.Attach(GetDlgItem(IDC_GAMMA_SPIN));
    m_GamaSpin.SetBase(10);
    m_GamaSpin.SetRange(10, 40);
    m_GamaSpin.SetPos(20);
    m_GamaSpin.SetBuddy(m_GamaTxt);

    m_N64ResMultiplerTxt.Attach(GetDlgItem(IDC_N64_RES_MULTIPLER_TXT));
    m_N64ResMultiplerSpin.Attach(GetDlgItem(IDC_N64_RES_MULTIPLER_SPIN));
    m_N64ResMultiplerSpin.SetBase(10);
    m_N64ResMultiplerSpin.SetRange(2, 16);
    m_N64ResMultiplerSpin.SetPos(2);
    m_N64ResMultiplerSpin.SetBuddy(m_N64ResMultiplerTxt);

    m_GammaIcon.SubclassWindow(GetDlgItem(IDC_GAMMA_ICON));
    m_GammaIcon.SetIcon(MAKEINTRESOURCE(IDI_ICON_INFO), 16, 16);
    m_GammaIcon.SetBackroundBrush((HBRUSH)GetStockObject(WHITE_BRUSH));
    return true;
}

LRESULT CEmulationTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

void CEmulationTab::OnGammaCorrection(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    CButton OverScan(GetDlgItem(IDC_CHK_GAMMA_CORRECTION));
    if (OverScan.GetCheck() == BST_CHECKED)
    {
        GetDlgItem(IDC_GAMMA_ICON).ShowWindow(SW_SHOW);
        GetDlgItem(IDC_GAMMA_INFO).ShowWindow(SW_SHOW);
    }
    else
    {
        GetDlgItem(IDC_GAMMA_ICON).ShowWindow(SW_HIDE);
        GetDlgItem(IDC_GAMMA_INFO).ShowWindow(SW_HIDE);
    }
}

void CEmulationTab::LoadSettings(bool blockCustomSettings)
{
    if (!blockCustomSettings)
    {
        CButton(GetDlgItem(IDC_CHK_USE_PER_GAME)).SetCheck(config.generalEmulation.enableCustomSettings != 0 ? BST_CHECKED : BST_UNCHECKED);
    }
    CButton(GetDlgItem(IDC_CHK_N64_STYLE_MIP_MAPPING)).SetCheck(config.generalEmulation.enableLOD != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_NOISE)).SetCheck(config.generalEmulation.enableNoise != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_HWLIGHTING)).SetCheck(config.generalEmulation.enableHWLighting != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_SHADERS_STORAGE)).SetCheck(config.generalEmulation.enableShadersStorage != 0 ? BST_CHECKED : BST_UNCHECKED);
    
    CButton(GetDlgItem(IDC_CHK_HALOS_REMOVAL)).SetCheck(config.texture.enableHalosRemoval != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FIXTEXRECT_NEVER)).SetCheck(config.graphics2D.correctTexrectCoords == Config::tcDisable ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FIXTEXRECT_SMART)).SetCheck(config.graphics2D.correctTexrectCoords == Config::tcSmart ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FIXTEXRECT_FORCE)).SetCheck(config.graphics2D.correctTexrectCoords == Config::tcForce ? BST_CHECKED : BST_UNCHECKED);

    CButton(GetDlgItem(IDC_BGMODE_ONEPIECE)).SetCheck(config.graphics2D.bgMode == Config::BGMode::bgOnePiece ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_BGMODE_STRIPPED)).SetCheck(config.graphics2D.bgMode == Config::BGMode::bgStripped ? BST_CHECKED : BST_UNCHECKED);

    CComboBox nativeRes2DComboBox(GetDlgItem(IDC_CMB_NATIVE_RES_2D));
    nativeRes2DComboBox.SetCurSel(config.graphics2D.enableNativeResTexrects);

    CButton(GetDlgItem(IDC_CHK_GAMMA_CORRECTION)).SetCheck(config.gammaCorrection.force != 0 ? BST_CHECKED : BST_UNCHECKED);
    int GammaPos = (int)(config.gammaCorrection.force != 0 ? config.gammaCorrection.level : 2.0) * 10;
    m_GamaSpin.SetPos(GammaPos);
    m_GamaTxt.SetWindowText(FormatStrW(L"%0.1f", (float)GammaPos / 10.0f).c_str());
    GetDlgItem(IDC_GAMMA_ICON).ShowWindow(config.gammaCorrection.force != 0 ? SW_SHOW : SW_HIDE);
    GetDlgItem(IDC_GAMMA_INFO).ShowWindow(config.gammaCorrection.force != 0 ? SW_SHOW : SW_HIDE);
 
    CButton(GetDlgItem(IDC_FACTOR0X_RADIO)).SetCheck(config.frameBufferEmulation.nativeResFactor == 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FACTOR1X_RADIO)).SetCheck(config.frameBufferEmulation.nativeResFactor == 1 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_FACTORXX_RADIO)).SetCheck(config.frameBufferEmulation.nativeResFactor > 1 ? BST_CHECKED : BST_UNCHECKED);
    m_N64ResMultiplerSpin.SetPos(config.frameBufferEmulation.nativeResFactor > 1 ? config.frameBufferEmulation.nativeResFactor : 2);
    m_N64ResMultiplerTxt.SetWindowText(FormatStrW(L"%dx", m_N64ResMultiplerSpin.GetPos()).c_str());
}

void CEmulationTab::SaveSettings()
{
    config.generalEmulation.enableCustomSettings = CButton(GetDlgItem(IDC_CHK_USE_PER_GAME)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.generalEmulation.enableLOD = CButton(GetDlgItem(IDC_CHK_N64_STYLE_MIP_MAPPING)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.generalEmulation.enableNoise = CButton(GetDlgItem(IDC_CHK_NOISE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.generalEmulation.enableHWLighting = CButton(GetDlgItem(IDC_CHK_HWLIGHTING)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.generalEmulation.enableShadersStorage = CButton(GetDlgItem(IDC_CHK_SHADERS_STORAGE)).GetCheck() == BST_CHECKED ? 1 : 0;
    
    if (CButton(GetDlgItem(IDC_FACTOR0X_RADIO)).GetCheck() == BST_CHECKED)
    {
        config.frameBufferEmulation.nativeResFactor = 0;
    }
    else if (CButton(GetDlgItem(IDC_FACTOR1X_RADIO)).GetCheck() == BST_CHECKED)
    {
        config.frameBufferEmulation.nativeResFactor = 1;
    }
    else if (CButton(GetDlgItem(IDC_FACTORXX_RADIO)).GetCheck() == BST_CHECKED)
    {
        config.frameBufferEmulation.nativeResFactor = m_N64ResMultiplerSpin.GetPos();
    }
    config.gammaCorrection.force = CButton(GetDlgItem(IDC_CHK_GAMMA_CORRECTION)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.gammaCorrection.level = ((float)m_GamaSpin.GetPos()) / 10;
    config.graphics2D.enableNativeResTexrects = CComboBox(GetDlgItem(IDC_CMB_NATIVE_RES_2D)).GetCurSel();
    config.texture.enableHalosRemoval = CButton(GetDlgItem(IDC_CHK_HALOS_REMOVAL)).GetCheck() == BST_CHECKED ? 1 : 0;

    if (CButton(GetDlgItem(IDC_FIXTEXRECT_NEVER)).GetCheck() == BST_CHECKED)
    {
        config.graphics2D.correctTexrectCoords = Config::tcDisable;
    }
    else if (CButton(GetDlgItem(IDC_FIXTEXRECT_SMART)).GetCheck() == BST_CHECKED)
    {
        config.graphics2D.correctTexrectCoords = Config::tcSmart;
    }
    else if (CButton(GetDlgItem(IDC_FIXTEXRECT_FORCE)).GetCheck() == BST_CHECKED)
    {
        config.graphics2D.correctTexrectCoords = Config::tcForce;
    }

    if (CButton(GetDlgItem(IDC_BGMODE_ONEPIECE)).GetCheck() == BST_CHECKED)
    {
        config.graphics2D.bgMode = Config::BGMode::bgOnePiece;
    }
    else if (CButton(GetDlgItem(IDC_BGMODE_STRIPPED)).GetCheck() == BST_CHECKED)
    {
        config.graphics2D.bgMode = Config::BGMode::bgStripped;
    }
}

LRESULT CEmulationTab::OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG CtrlId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (CtrlId == IDC_GAMMA_SPIN)
    {
        int Pos = m_GamaSpin.GetPos();
        m_GamaTxt.SetWindowText(FormatStrW(L"%0.1f", (float)Pos / 10.0f).c_str());
    }
    else if (CtrlId == IDC_N64_RES_MULTIPLER_SPIN)
    {
        int Pos = m_N64ResMultiplerSpin.GetPos();
        m_N64ResMultiplerTxt.SetWindowText(FormatStrW(L"%dx", Pos).c_str());
    }
    return 0;
}
