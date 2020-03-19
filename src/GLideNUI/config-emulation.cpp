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