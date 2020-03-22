#include "config-texture.h"
#include "resource.h"
#include "util.h"
#include "../Config.h"
#include <Shlobj.h>

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

void CTextureEnhancementTab::SaveDirectory(int EditCtrl, wchar_t * txPath)
{
    CWindow EditWnd = GetDlgItem(EditCtrl);
    int TxtLen = EditWnd.GetWindowTextLength();
    std::wstring Path;
    Path.resize(TxtLen + 1);
    EditWnd.GetWindowText((wchar_t *)Path.data(), Path.size());

    WIN32_FIND_DATA	FindData;
    HANDLE hFindFile = FindFirstFile(Path.c_str(), &FindData);
    bool exists = (hFindFile != INVALID_HANDLE_VALUE);

    if (hFindFile != NULL)
    {
        FindClose(hFindFile);
    }

    if (exists)
    {
        wcscpy(txPath, Path.c_str());
    }
}

void CTextureEnhancementTab::SaveSettings()
{
    config.textureFilter.txFilterMode = CComboBox(GetDlgItem(IDC_CMB_FILTER)).GetCurSel();
    config.textureFilter.txEnhancementMode = CComboBox(GetDlgItem(IDC_CMB_ENHANCEMENT)).GetCurSel();
    config.textureFilter.txDeposterize = CButton(GetDlgItem(IDC_CHK_DEPOSTERIZE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txFilterIgnoreBG = CButton(GetDlgItem(IDC_CHK_IGNORE_BACKGROUNDS)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txEnhancedTextureFileStorage = CButton(GetDlgItem(IDC_CHK_ENHANCED_TEX_FILE_STORAGE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txHiresEnable = CButton(GetDlgItem(IDC_CHK_TEXTURE_PACK)).GetCheck() == BST_CHECKED ? 1 : 0;

    SaveDirectory(IDC_TEX_PACK_PATH_EDIT, config.textureFilter.txPath);
    SaveDirectory(IDC_TEX_CACHE_PATH_EDIT, config.textureFilter.txCachePath);
    SaveDirectory(IDC_TEX_DUMP_PATH_EDIT, config.textureFilter.txDumpPath);

    config.textureFilter.txHiresFullAlphaChannel = CButton(GetDlgItem(IDC_CHK_ALPHA_CHANNEL)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txHresAltCRC = CButton(GetDlgItem(IDC_CHK_ALTERNATIVE_CRC)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txHiresTextureFileStorage = CButton(GetDlgItem(IDC_CHK_HIRES_TEX_FILESTORAGE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txDump = CButton(GetDlgItem(IDC_CHK_TEXTURE_DUMP)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txCacheSize = m_TextureFilterCacheSpin.GetPos() * gc_uMegabyte * 50;
    config.textureFilter.txSaveCache = CButton(GetDlgItem(IDC_CHK_SAVE_TEXTURE_CACHE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txCacheCompression = CButton(GetDlgItem(IDC_CHK_COMPRESS_CACHE)).GetCheck() == BST_CHECKED ? 1 : 0;
    config.textureFilter.txForce16bpp = CButton(GetDlgItem(IDC_CHK_FORCE_16BPP)).GetCheck() == BST_CHECKED ? 1 : 0;
}

void CTextureEnhancementTab::OnSelectTexPackPath(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectDir(L"Select directory for texture pack path", IDC_TEX_PACK_PATH_EDIT);
}

void CTextureEnhancementTab::OnSelectTexCachePath(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectDir(L"Select directory for texture cache path", IDC_TEX_CACHE_PATH_EDIT);
}

void CTextureEnhancementTab::OnSelectTexDumpPath(UINT /*Code*/, int /*id*/, HWND /*ctl*/)
{
    SelectDir(L"Select directory for texture dump path", IDC_TEX_DUMP_PATH_EDIT);
}

void CTextureEnhancementTab::SelectDir(wchar_t * Title, int EditCtrl)
{
    wchar_t Buffer[MAX_PATH], Directory[MAX_PATH];
    LPITEMIDLIST pidl;
    BROWSEINFOW bi;

    CWindow EditWnd = GetDlgItem(EditCtrl);
    int TxtLen = EditWnd.GetWindowTextLength();
    std::wstring EditText;
    EditText.resize(TxtLen + 1);
    EditWnd.GetWindowText((wchar_t *)EditText.data(), EditText.size());

    bi.hwndOwner = m_hWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = Buffer;
    bi.lpszTitle = Title;
    bi.ulFlags = BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn = (BFFCALLBACK)SelectDirCallBack;
    bi.lParam = (DWORD)EditText.c_str();
    if ((pidl = SHBrowseForFolderW(&bi)) != NULL)
    {
        if (SHGetPathFromIDListW(pidl, Directory))
        {
            EditWnd.SetWindowText(Directory);
        }
    }
}

int CALLBACK CTextureEnhancementTab::SelectDirCallBack(HWND hwnd, uint32_t uMsg, uint32_t /*lp*/, uint32_t lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        // WParam is TRUE since you are passing a path.
        // It would be FALSE if you were passing a pidl.
        if (lpData)
        {
            SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        break;
    }
    return 0;
}
