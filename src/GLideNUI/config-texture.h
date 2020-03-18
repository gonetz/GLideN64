#pragma once
#include "config-tab.h"
#include "resource.h"

class CTextureEnhancementTab :
    public CConfigTab
{
public:
    BEGIN_MSG_MAP(CTextureEnhancementTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        COMMAND_HANDLER_EX(IDC_CHK_ENHANCED_TEX_FILE_STORAGE, BN_CLICKED, OnFileStorage)
        COMMAND_HANDLER_EX(IDC_CHK_TEXTURE_PACK, BN_CLICKED, OnTexturePack)
        MESSAGE_HANDLER(WM_VSCROLL, OnScroll)
    END_MSG_MAP()

    CTextureEnhancementTab();
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/);
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/);
    void OnFileStorage(UINT /*Code*/, int id, HWND /*ctl*/);
    void OnTexturePack(UINT /*Code*/, int id, HWND /*ctl*/);
    void LoadSettings(bool blockCustomSettings);
    void SaveSettings();

private:
    CEdit m_TextureFilterCacheTxt;
    CUpDownCtrl m_TextureFilterCacheSpin;
};

