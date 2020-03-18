#pragma once
#include "config-tab.h"
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
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    CVideoTab();
    ~CVideoTab();

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};
