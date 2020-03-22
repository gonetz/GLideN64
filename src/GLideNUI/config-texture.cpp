#include "config-texture.h"
#include "resource.h"
#include "util.h"
#include "../Config.h"

CTextureEnhancementTab::CTextureEnhancementTab() :
    CConfigTab(IDD_TAB_TEXTURE_ENHANCEMENT)
{
}

BOOL CTextureEnhancementTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    CComboBox filterComboBox(GetDlgItem(IDC_CMB_FILTER));
    filterComboBox.AddString(L"None");
    filterComboBox.AddString(L"Smooth filtering 1");
    filterComboBox.AddString(L"Smooth filtering 2");
    filterComboBox.AddString(L"Smooth filtering 3");
    filterComboBox.AddString(L"Smooth filtering 4");
    filterComboBox.AddString(L"Sharp filtering 1");
    filterComboBox.AddString(L"Sharp filtering 2");

    CComboBox enhancementComboBox(GetDlgItem(IDC_CMB_ENHANCEMENT));
    enhancementComboBox.AddString(L"None");
    enhancementComboBox.AddString(L"Store");
    enhancementComboBox.AddString(L"X2");
    enhancementComboBox.AddString(L"X2SAI");
    enhancementComboBox.AddString(L"HQ2X");
    enhancementComboBox.AddString(L"HQ2XS");
    enhancementComboBox.AddString(L"LQ2X");
    enhancementComboBox.AddString(L"LQ2XS");
    enhancementComboBox.AddString(L"HQ4X");
    enhancementComboBox.AddString(L"2xBRZ");
    enhancementComboBox.AddString(L"3xBRZ");
    enhancementComboBox.AddString(L"4xBRZ");
    enhancementComboBox.AddString(L"5xBRZ");
    enhancementComboBox.AddString(L"6xBRZ");

    m_TextureFilterCacheTxt.Attach(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_EDIT));
    m_TextureFilterCacheSpin.Attach(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_SPIN));
    m_TextureFilterCacheSpin.SetBase(10);
    m_TextureFilterCacheSpin.SetRange(0, 20);
    m_TextureFilterCacheSpin.SetPos(0);
    m_TextureFilterCacheSpin.SetBuddy(m_TextureFilterCacheTxt);
    return true;
}

LRESULT CTextureEnhancementTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

void CTextureEnhancementTab::OnFileStorage(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    bool UseFileStorage = !CButton(GetDlgItem(IDC_CHK_ENHANCED_TEX_FILE_STORAGE)).GetCheck() == BST_CHECKED;
    CButton(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_STATIC)).EnableWindow(UseFileStorage);
    CButton(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_EDIT)).EnableWindow(UseFileStorage);
    CButton(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_SPIN)).EnableWindow(UseFileStorage);
}

void CTextureEnhancementTab::OnTexturePack(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    bool UseTextPack = CButton(GetDlgItem(IDC_CHK_TEXTURE_PACK)).GetCheck() == BST_CHECKED;
    CButton(GetDlgItem(IDC_TEX_PACK_PATH_STATIC)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_PACK_PATH_EDIT)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_PACK_PATH_BTN)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_CACHE_PATH_STATIC)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_CACHE_PATH_EDIT)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_CACHE_PATH_BTN)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_DUMP_PATH_STATIC)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_DUMP_PATH_EDIT)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_TEX_DUMP_PATH_BTN)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_CHK_ALPHA_CHANNEL)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_CHK_ALTERNATIVE_CRC)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_CHK_HIRES_TEX_FILESTORAGE)).EnableWindow(UseTextPack);
    CButton(GetDlgItem(IDC_CHK_TEXTURE_DUMP)).EnableWindow(UseTextPack);
}

LRESULT CTextureEnhancementTab::OnScroll(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG CtrlId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (CtrlId == IDC_TEXTURE_FILTER_CACHE_SPIN)
    {
        int Pos = m_TextureFilterCacheSpin.GetPos();
        if (Pos == 20)
        {
            m_TextureFilterCacheTxt.SetWindowText(L"999 mb");
        }
        else
        {
            m_TextureFilterCacheTxt.SetWindowText(FormatStrW(L"%d mb", Pos * 50).c_str());
        }
    }
    return 0;
}

void CTextureEnhancementTab::LoadSettings(bool /*blockCustomSettings*/)
{
    CComboBox(GetDlgItem(IDC_CMB_FILTER)).SetCurSel(config.textureFilter.txFilterMode);
    CComboBox(GetDlgItem(IDC_CMB_ENHANCEMENT)).SetCurSel(config.textureFilter.txEnhancementMode);
    CButton(GetDlgItem(IDC_CHK_DEPOSTERIZE)).SetCheck(config.textureFilter.txDeposterize != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_IGNORE_BACKGROUNDS)).SetCheck(config.textureFilter.txFilterIgnoreBG != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_ENHANCED_TEX_FILE_STORAGE)).SetCheck(config.textureFilter.txEnhancedTextureFileStorage != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_TEXTURE_PACK)).SetCheck(config.textureFilter.txHiresEnable != 0 ? BST_CHECKED : BST_UNCHECKED);
        
    GetDlgItem(IDC_TEX_PACK_PATH_EDIT).SetWindowText(config.textureFilter.txPath);
    GetDlgItem(IDC_TEX_CACHE_PATH_EDIT).SetWindowText(config.textureFilter.txCachePath);
    GetDlgItem(IDC_TEX_DUMP_PATH_EDIT).SetWindowText(config.textureFilter.txDumpPath);

    CButton(GetDlgItem(IDC_CHK_ALPHA_CHANNEL)).SetCheck(config.textureFilter.txHiresFullAlphaChannel != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_ALTERNATIVE_CRC)).SetCheck(config.textureFilter.txHresAltCRC != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_HIRES_TEX_FILESTORAGE)).SetCheck(config.textureFilter.txHiresTextureFileStorage != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_TEXTURE_DUMP)).SetCheck(config.textureFilter.txDump != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_SAVE_TEXTURE_CACHE)).SetCheck(config.textureFilter.txSaveCache != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_COMPRESS_CACHE)).SetCheck(config.textureFilter.txCacheCompression != 0 ? BST_CHECKED : BST_UNCHECKED);
    CButton(GetDlgItem(IDC_CHK_FORCE_16BPP)).SetCheck(config.textureFilter.txForce16bpp != 0 ? BST_CHECKED : BST_UNCHECKED);
   
    m_TextureFilterCacheSpin.SetPos((config.textureFilter.txCacheSize / gc_uMegabyte) /  50);

    OnFileStorage(0, 0, NULL);
    OnTexturePack(0, 0, NULL);
    BOOL bHandled;
    OnScroll(0, 0, (LPARAM)(GetDlgItem(IDC_TEXTURE_FILTER_CACHE_SPIN).Detach()), bHandled);
}
