#pragma once
#include "config-tab.h"
#include "wtl-BitmapPicture.h"
#include "resource.h"

class CEmulationTab :
    public CConfigTab
{
public:
    BEGIN_MSG_MAP(CEmulationTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
        COMMAND_HANDLER_EX(IDC_CHK_GAMMA_CORRECTION, BN_CLICKED, OnGammaCorrection)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    CEmulationTab();
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    void OnGammaCorrection(UINT /*Code*/, int id, HWND /*ctl*/);
    void LoadSettings(bool /*blockCustomSettings*/);

private:
    LRESULT OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);

    CEdit m_GamaTxt, m_N64ResMultiplerTxt;
    CUpDownCtrl m_GamaSpin, m_N64ResMultiplerSpin;
    CBitmapPicture m_GammaIcon;
};
