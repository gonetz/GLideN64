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
    CButton(GetDlgItem(IDC_CHK_DUMP_LOW)).SetCheck((config.debug.dumpMode & DEBUG_LOW) != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_DUMP_NORMAL)).SetCheck((config.debug.dumpMode & DEBUG_NORMAL) != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_DUMP_DETAIL)).SetCheck((config.debug.dumpMode & DEBUG_DETAIL) != 0 ? BST_CHECKED : BST_UNCHECKED);
}

void CDebugTab::SaveSettings()
{
}
