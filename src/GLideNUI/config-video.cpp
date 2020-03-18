#include "config-video.h"

CVideoTab::CVideoTab() :
    CConfigTab(IDD_TAB_VIDEO)
{
}

CVideoTab::~CVideoTab()
{
}

BOOL CVideoTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    return true;
}

LRESULT CVideoTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}
