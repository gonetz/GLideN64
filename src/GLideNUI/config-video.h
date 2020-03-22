#pragma once
#include "config-tab.h"
#include "config-overscan.h"
#include "wtl-BitmapPicture.h"
#include "resource.h"
#include <vector>

class CVideoTab :
    public CConfigTab
{
public:
    BEGIN_MSG_MAP(CVideoTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        NOTIFY_HANDLER_EX(IDC_TAB_OVERSCAN, TCN_SELCHANGE, OnOverscanTabChange)
        COMMAND_HANDLER_EX(IDC_CMB_FULL_SCREEN_RES, CBN_SELCHANGE, OnFullScreenChanged)
        COMMAND_HANDLER_EX(IDC_CHK_OVERSCAN, BN_CLICKED, OnOverscan)
        MESSAGE_HANDLER(WM_HSCROLL, OnScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    CVideoTab();
    ~CVideoTab();

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    LRESULT OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    void OnOverscan(UINT /*Code*/, int id, HWND /*ctl*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOverscanTabChange(NMHDR* /*pNMHDR*/);
    void OnFullScreenChanged(UINT /*Code*/, int id, HWND /*ctl*/);
    void AddOverScanTab(const wchar_t * caption);
    void ShowOverScanTab(int nTab);
    void LoadSettings(bool blockCustomSettings);

    CTabCtrl m_OverScanTab;
    std::vector<COverScanTab *> m_OverscanTabs;
    CTrackBarCtrl m_AliasingSlider;
    CTrackBarCtrl m_AnisotropicSlider;
    CBitmapPicture m_AAInfoIcon;
};
