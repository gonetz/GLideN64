#include "wtl.h"
#include <stdint.h>

class COSDButton : 
    public CBitmapButtonImpl<COSDButton>
{
public:
    DECLARE_WND_SUPERCLASS(_T("WTL_BitmapButton"), GetWndClassName())

    COSDButton(uint32_t IconId, DWORD dwExtendedStyle = BMPBTN_AUTOSIZE | BMPBTN_HOVER, HIMAGELIST hImageList = NULL);
    void DoPaint(CDCHandle dc);
    void SetChecked(bool checked);

private:
    CIcon m_Icon;
    bool m_Checked;
};
