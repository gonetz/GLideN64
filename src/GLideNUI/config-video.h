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
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    CVideoTab();
    ~CVideoTab();

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
};
