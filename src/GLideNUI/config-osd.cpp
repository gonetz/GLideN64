#include "config-osd.h"
#include "resource.h"
#include "util.h"
#include "../Config.h"
#include "FontInfo.h"

COsdTab::COsdTab() :
    CConfigTab(IDD_TAB_OSD),
    m_PosTopLeft(IDI_OSD_TOP_LEFT),
    m_PosTop(IDI_OSD_TOP),
    m_PosTopRight(IDI_OSD_TOP_RIGHT),
    m_PosCenterLeft(IDI_OSD_LEFT),
    m_PosCenter(0),
    m_PosCenterRight(IDI_OSD_RIGHT),
    m_PosBottomLeft(IDI_OSD_BOTTOM_LEFT),
    m_PosBottom(IDI_OSD_BOTTOM),
    m_PosBottomRight(IDI_OSD_BOTTOM_RIGHT)
{
}

BOOL COsdTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    m_PosTopLeft.SubclassWindow(GetDlgItem(IDC_OSD_TOP_LEFT));
    m_PosTop.SubclassWindow(GetDlgItem(IDC_OSD_TOP));
    m_PosTopRight.SubclassWindow(GetDlgItem(IDC_OSD_TOP_RIGHT));
    m_PosCenterLeft.SubclassWindow(GetDlgItem(IDC_OSD_CENTER_LEFT));
    m_PosCenter.SubclassWindow(GetDlgItem(IDC_OSD_CENTER));
    m_PosCenterRight.SubclassWindow(GetDlgItem(IDC_OSD_CENTER_RIGHT));
    m_PosBottomLeft.SubclassWindow(GetDlgItem(IDC_OSD_BOTTOM_LEFT));
    m_PosBottom.SubclassWindow(GetDlgItem(IDC_OSD_BOTTOM));
    m_PosBottomRight.SubclassWindow(GetDlgItem(IDC_OSD_BOTTOM_RIGHT));
    m_Fonts.Attach(GetDlgItem(IDC_FONTS));
    m_OsdColor.SubclassWindow(GetDlgItem(IDC_OSD_COLOR));
    m_OsdPreview.SubclassWindow(GetDlgItem(IDC_OSD_PREVIEW));

    m_FontSizeTxt.Attach(GetDlgItem(IDC_FONT_SIZE_TXT));
    m_FontSizeSpin.Attach(GetDlgItem(IDC_FONT_SIZE_SPIN));
    m_FontSizeSpin.SetBase(10);
    m_FontSizeSpin.SetRange(6, 99);
    m_FontSizeSpin.SetBuddy(m_FontSizeTxt);

    m_PosCenterLeft.EnableWindow(false);
    m_PosCenter.EnableWindow(false);
    m_PosCenterRight.EnableWindow(false);

    FontList fonts = GetFontFiles();
    HTREEITEM hCurrentItem = TVI_ROOT;
    for (FontList::const_iterator itr = fonts.begin(); itr != fonts.end(); itr++)
    {
        std::wstring FontFile = ToUTF16(itr->first.c_str());
        std::wstring FontName = ToUTF16(itr->second.c_str());

        TVINSERTSTRUCT tv = { 0 };
        wchar_t Item[500];
        tv.item.mask = TVIF_TEXT;
        tv.item.pszText = Item;
        tv.item.cchTextMax = sizeof(Item) / sizeof(Item[0]);
        tv.item.hItem = m_Fonts.GetChildItem(TVI_ROOT);
        HTREEITEM hParent = TVI_ROOT;
        while (tv.item.hItem)
        {
            m_Fonts.GetItem(&tv.item);
            if (wcscmp(FontName.c_str(), Item) == 0)
            {
                hParent = tv.item.hItem;
                break;
            }
            tv.item.hItem = m_Fonts.GetNextSiblingItem(tv.item.hItem);
        }

        if (hParent == TVI_ROOT)
        {
            tv.item.mask = TVIF_TEXT;
            tv.item.pszText = (LPWSTR)FontName.c_str();
            tv.item.cchTextMax = FontName.length();
            tv.hInsertAfter = TVI_SORT;
            tv.hParent = TVI_ROOT;
            hParent = m_Fonts.InsertItem(&tv);
        }
        tv.item.mask = TVIF_TEXT;
        tv.item.pszText = (LPWSTR)FontFile.c_str();
        tv.item.cchTextMax = FontFile.length();
        tv.hInsertAfter = TVI_SORT;
        tv.hParent = hParent;
        m_Fonts.InsertItem(&tv);
    }
    if (hCurrentItem != TVI_ROOT)
    {
        m_Fonts.SelectItem(hCurrentItem);
        m_Fonts.SetItemState(hCurrentItem, TVIF_STATE | TVIS_SELECTED, TVIF_STATE | TVIS_SELECTED);
        m_Fonts.SetFocus();
    }
    return true;
}

LRESULT COsdTab::OnScroll(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    LONG CtrlId = CWindow((HWND)lParam).GetWindowLong(GWL_ID);
    if (CtrlId == IDC_FONT_SIZE_SPIN)
    {
        m_OsdPreview.SetFontSize(m_FontSizeSpin.GetPos());
    }
    return 0;
}

void COsdTab::ClearOsdChecked()
{
    m_PosTopLeft.SetChecked(false);
    m_PosTopLeft.Invalidate();
    m_PosTop.SetChecked(false);
    m_PosTop.Invalidate();
    m_PosTopRight.SetChecked(false);
    m_PosTopRight.Invalidate();
    m_PosBottomLeft.SetChecked(false);
    m_PosBottomLeft.Invalidate();
    m_PosBottom.SetChecked(false);
    m_PosBottom.Invalidate();
    m_PosBottomRight.SetChecked(false);
    m_PosBottomRight.Invalidate();
}

LRESULT COsdTab::OnOsdTopLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosTopLeft.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnOsdTop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosTop.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnOsdTopRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosTopRight.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnOsdBottomLeft(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosBottomLeft.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnOsdBottom(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosBottom.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnOsdBottomRight(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    ClearOsdChecked();
    m_PosBottomRight.SetChecked(true);
    return 0;
}

LRESULT COsdTab::OnFontItemChanged(NMHDR* /*phdr*/)
{
    m_OsdPreview.SetFont(GetSelectedFont());
    return 0;
}

LRESULT COsdTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

LRESULT COsdTab::OnNotifyOsdColor(LPNMHDR pnmh)
{
    m_OsdPreview.SetColor(m_OsdColor.Red(), m_OsdColor.Green(), m_OsdColor.Blue());
    m_OsdPreview.Invalidate();
    return 0;
}

std::wstring COsdTab::GetSelectedFont()
{
    HTREEITEM hItem = m_Fonts.GetSelectedItem();
    if (hItem == NULL)
    {
        return L"";
    }
    HTREEITEM hChild = m_Fonts.GetChildItem(hItem);
    if (hChild != NULL)
    {
        hItem = hChild;
    }

    wchar_t ItemText[MAX_PATH];
    if (!m_Fonts.GetItemText(hItem, ItemText, sizeof(ItemText) / sizeof(ItemText[0])))
    {
        return L"";
    }
    return std::wstring(ItemText);
}