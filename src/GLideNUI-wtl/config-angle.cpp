#include "config-angle.h"
#include "UIConfig.h"
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
}

LRESULT CAngleTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	return (LRESULT)GetStockObject(WHITE_BRUSH);
}

void CAngleTab::LoadSettings(bool /*blockCustomSettings*/) {
	CButton(GetDlgItem(IDC_RADIO_DIRECT3D)).SetCheck(config.angle.renderer == Config::arDirectX11 ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_RADIO_VULKAN)).SetCheck(config.angle.renderer == Config::arVulkan ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_RADIO_OPENGL)).SetCheck(config.angle.renderer == Config::arOpenGL ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_CHECK_DIRECT_COMPOSITION)).SetCheck((config.angle.directComposition) != 0 ? BST_CHECKED : BST_UNCHECKED);
	CButton(GetDlgItem(IDC_CHECK_ENABLE_FRAGMENT_DEPTH_WRITE)).SetCheck((config.generalEmulation.enableFragmentDepthWrite) != 0 ? BST_CHECKED : BST_UNCHECKED);
}

void CAngleTab::SaveSettings() {
	if (CButton(GetDlgItem(IDC_RADIO_DIRECT3D)).GetCheck() == BST_CHECKED)
	{
		config.angle.renderer = Config::arDirectX11;
	}
	if (CButton(GetDlgItem(IDC_RADIO_VULKAN)).GetCheck() == BST_CHECKED)
	{
		config.angle.renderer = Config::arVulkan;
	}
	if (CButton(GetDlgItem(IDC_RADIO_OPENGL)).GetCheck() == BST_CHECKED)
	{
		config.angle.renderer = Config::arOpenGL;
	}
	config.angle.directComposition = CButton(GetDlgItem(IDC_CHECK_DIRECT_COMPOSITION)).GetCheck() == BST_CHECKED;
	config.generalEmulation.enableFragmentDepthWrite = CButton(GetDlgItem(IDC_CHECK_ENABLE_FRAGMENT_DEPTH_WRITE)).GetCheck() == BST_CHECKED;
}
