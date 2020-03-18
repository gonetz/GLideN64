#include "wtl-BitmapPicture.h"

CBitmapPicture::CBitmapPicture() :
    m_hBitmap(NULL),
    m_nResourceID(-1),
    m_ResourceIcon(false)
{
    memset(&m_bmInfo, 0, sizeof(m_bmInfo));
    m_BackgroundBrush.CreateSolidBrush(::GetSysColor(COLOR_3DFACE));
}

LRESULT CBitmapPicture::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL &/*bHandled*/)
{
    CPaintDC dc(m_hWnd);
    CRect rect;
    GetClientRect(&rect);

    CDC dcMem;
    dcMem.CreateCompatibleDC(dc);

    if (dcMem.IsNull() || m_hBitmap.IsNull())
    {
        CBrush PaintBrush;
        HBRUSH OldBrush = dc.SelectBrush(m_BackgroundBrush);
        dc.PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);
        dc.SelectBrush(OldBrush);
        return 0;
    }

    HBITMAP pBmpOld = dcMem.SelectBitmap(m_hBitmap);
    dc.StretchBlt(rect.left, rect.top, rect.Width(), rect.Height(), dcMem, 0, 0, m_bmInfo.bmWidth - 1, m_bmInfo.bmHeight - 1, SRCCOPY);
    dcMem.SelectBitmap(pBmpOld);
    return 0;
}

bool CBitmapPicture::SetIcon(LPCWSTR lpszResourceName, uint32_t nWidth, uint32_t nHeight)
{
    CIcon hIcon = ::LoadIcon(ModuleHelper::GetResourceInstance(), lpszResourceName);
    if (hIcon.IsNull())
    {
        return false;
    }
    ICONINFO IconInfo;
    if (!hIcon.GetIconInfo(&IconInfo))
    {
        return false;
    }
    if (IS_INTRESOURCE(lpszResourceName))
    {
        m_nResourceID = (int)lpszResourceName;
    }
    else
    {
        m_strResourceName = lpszResourceName;
    }
    m_ResourceIcon = true;
    m_IconWidth = nWidth;
    m_IconHeight = nHeight;
    DrawIcon();
    return true;

}

void CBitmapPicture::DrawIcon(void)
{
    if (!m_ResourceIcon)
    {
        return;
    }
    CIcon hIcon = ::LoadIcon(ModuleHelper::GetResourceInstance(), m_nResourceID > 0 ? MAKEINTRESOURCE(m_nResourceID): m_strResourceName.c_str());
    if (hIcon.IsNull())
    {
        return;
    }

    CDC dc = ::GetDC(m_hWnd);

    CBitmap hMemBmp;
    hMemBmp.CreateCompatibleBitmap(dc, m_IconWidth + 1, m_IconHeight + 1);

    CDC MemDC;
    MemDC.CreateCompatibleDC(dc);
    MemDC.SelectBitmap(hMemBmp);

    HBRUSH OldBrush = MemDC.SelectBrush(m_BackgroundBrush);
    MemDC.PatBlt(0, 0, m_IconWidth, m_IconHeight, PATCOPY);
    MemDC.DrawIconEx(0, 0, hIcon, m_IconWidth, m_IconHeight, 0, NULL, DI_NORMAL);
    MemDC.SelectBrush(OldBrush);

    SetBitmap(hMemBmp.Detach());
}

bool CBitmapPicture::SetBitmap(HBITMAP hBitmap)
{
    m_hBitmap.Attach(hBitmap);
    return ::GetObject(m_hBitmap, sizeof(BITMAP), &m_bmInfo) != 0;
}

void CBitmapPicture::SetBackroundBrush(HBRUSH brush)
{
    m_BackgroundBrush.Attach(brush);
    DrawIcon();
}
