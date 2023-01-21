#include "config-angle.h"
#include "../Config.h"
#include "Language.h"
#include "resource.h"

CAngleTab::CAngleTab() :
	CConfigTab(IDD_TAB_ANGLE)
{
}

BOOL CAngleTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/) {
	return true;
}

void CAngleTab::ApplyLanguage(void) {
	SetDlgItemTextW(IDC_RENDERER, wGS(ANGLE_RENDERER).c_str());
}

LRESULT CAngleTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return (LRESULT)GetStockObject(WHITE_BRUSH);
}

void CAngleTab::LoadSettings(bool /*blockCustomSettings*/) {
	//CButton(GetDlgItem(IDC_CHK_DUMP_LOW)).SetCheck((config.debug.dumpMode & DEBUG_LOW) != 0 ? BST_CHECKED : BST_UNCHECKED);
	//CButton(GetDlgItem(IDC_CHK_DUMP_NORMAL)).SetCheck((config.debug.dumpMode & DEBUG_NORMAL) != 0 ? BST_CHECKED : BST_UNCHECKED);
	//CButton(GetDlgItem(IDC_CHK_DUMP_DETAIL)).SetCheck((config.debug.dumpMode & DEBUG_DETAIL) != 0 ? BST_CHECKED : BST_UNCHECKED);
}

void CAngleTab::SaveSettings() {
	//config.debug.dumpMode = 0;
	//if (CButton(GetDlgItem(IDC_CHK_DUMP_LOW)).GetCheck() == BST_CHECKED)
	//	config.debug.dumpMode |= DEBUG_LOW;
	//if (CButton(GetDlgItem(IDC_CHK_DUMP_NORMAL)).GetCheck() == BST_CHECKED)
	//	config.debug.dumpMode |= DEBUG_NORMAL;
	//if (CButton(GetDlgItem(IDC_CHK_DUMP_DETAIL)).GetCheck() == BST_CHECKED)
	//	config.debug.dumpMode |= DEBUG_DETAIL;
}
