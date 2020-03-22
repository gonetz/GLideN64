#include "config-debug.h"
#include "../Config.h"
#include "../DebugDump.h"
#include "resource.h"

CDebugTab::CDebugTab() :
    CConfigTab(IDD_TAB_DEBUG)
{
}

BOOL CDebugTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    return true;
}

LRESULT CDebugTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

void CDebugTab::LoadSettings(bool /*blockCustomSettings*/)
{
}
