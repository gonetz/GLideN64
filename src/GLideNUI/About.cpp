﻿#include "About.h"
#include "resource.h"

extern HINSTANCE hInstance;

class CAboutContributersTab :
    public CAboutTab
{
public:
    BEGIN_MSG_MAP(CAboutContributersTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

    CAboutContributersTab() :
        CAboutTab(IDD_TAB_CONTRIBUTORS)
    {
    }

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
    {
        const TCHAR Contributors[] =
        {
            L"Logan McNaughton\r\n"
            L"Francisco Zurita\r\n"
            L"gizmo98\r\n"
            L"purplemarshmallow\r\n"
            L"zilmar\r\n"
            L"matthewharvey\r\n"
            L"lioncash\r\n"
            L"Predator82Germany\r\n"
            L"AmbientMalice\r\n"
            L"baptiste0602\r\n"
            L"Gilles Siberlin\r\n"
            L"Daniel Eck\r\n"
            L"Víctor \"IlDucci\"\r\n"
            L"orbea\r\n"
            L"BenjaminSiskoo\r\n"
            L"ptitSeb\r\n"
            L"Kimberly J.Ortega\r\n"
            L"Maxime Morel\r\n"
            L"tony971\r\n"
            L"SigmaVirus\r\n"
            L"Jools Wills\r\n"
            L"Nekokabu\r\n"
            L"nicklauslittle\r\n"
            L"Nebuleon\r\n"
            L"sergiobenrocha2\r\n"
            L"Michał Durak\r\n"
            L"Mushman\r\n"
        };
        GetDlgItem(IDC_CONTRIBUTORS).SetWindowText(Contributors);
        return true;
    }
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CRect WinRect;
        GetWindowRect(&WinRect);
        ScreenToClient(&WinRect);

        GetDlgItem(IDC_CONTRIBUTORS).SetWindowPos(HWND_TOP, WinRect.left + 20, WinRect.top + 20, WinRect.Width() - 40, WinRect.Height() - 40, SWP_SHOWWINDOW);
        return 0;
    }
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        if ((HWND)lParam == GetDlgItem(IDC_CONTRIBUTORS))
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
        }
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
};

class CAboutFundersTab :
    public CAboutTab
{
public:
    BEGIN_MSG_MAP(CAboutFundersTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
    END_MSG_MAP()

    CAboutFundersTab() :
        CAboutTab(IDD_TAB_FUNDERS)
    {
    }

    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
    {
        const TCHAR Funders1[] =
        {
            L"zolcos\r\n"
            L"Mush Man\r\n"
            L"nesplayer4life\r\n"
            L"neko9876\r\n"
            L"AnthonyHeathcoat\r\n"
            L"daman6009\r\n"
            L"Paul Lamb\r\n"
            L"zilmar\r\n"
        };
        const TCHAR FundersList[] =
        {
            L"Ryan Rosser\r\n"
            L"Amadeus Sterl\r\n"
            L"Narann\r\n"
            L"camara_luiz\r\n"
            L"weinerschnitzel\r\n"
            L"microdev\r\n"
            L"Thomas Ginelli\r\n"
            L"ace977\r\n"
            L"patryk.szalanski\r\n"
            L"Detomine\r\n"
            L"itasovski\r\n"
            L"keithclark1985\r\n"
            L"josephrmoore\r\n"
            L"fckyourlies\r\n"
            L"dougforr\r\n"
            L"camdenfurse\r\n"
            L"grandslam810\r\n"
            L"rictic\r\n"
            L"Fred Lambes\r\n"
            L"David Vercruyssen\r\n"
            L"danielgormly\r\n"
            L"lukecool\r\n"
            L"rhilsky\r\n"
            L"phillipstuerzl\r\n"
            L"killjoy1337\r\n"
            L"ratop46\r\n"
            L"william.a.moore\r\n"
            L"RSP16\r\n"
            L"kzidek127\r\n"
            L"Dan Holberg\r\n"
            L"178amm\r\n"
            L"peterchrjoergensen\r\n"
            L"hill_jm\r\n"
            L"petercullenbryan\r\n"
            L"Christopher M Rock\r\n"
            L"Kenny.R.Mitchell\r\n"
            L"Kevin Grasso\r\n"
            L"mtgyure\r\n"
            L"Anthony Heathcoat\r\n"
            L"Liam Burns\r\n"
            L"Steven Impson\r\n"
            L"Gwyn.Whieldon\r\n"
            L"hipnotoad\r\n"
            L"shmuklidooha\r\n"
            L"bcanard123\r\n"
            L"Ben Slater\r\n"
            L"Mike Nagy\r\n"
            L"littlegreendude55\r\n"
            L"Jay Loring\r\n"
            L"Damion D\r\n"
            L"heranbago\r\n"
            L"baptiste.guilbert\r\n"
            L"shadowpower69\r\n"
            L"j.mcguirk72\r\n"
            L"Peter Greenwood\r\n"
            L"fla56\r\n"
            L"Sergio\r\n"
            L"theboy_181\r\n"
            L"Jindo Fox\r\n"
            L"s1n.pcc\r\n"
            L"rafaelvasco\r\n"
            L"copileo\r\n"
            L"hugues.fabien\r\n"
            L"seanmcm157\r\n"
            L"David Morris\r\n"
            L"Jason Lightner\r\n"
            L"olivier_crepin77\r\n"
            L"Paul Lamb\r\n"
            L"thegump2.0\r\n"
            L"Bates\r\n"
            L"cdoublejj\r\n"
            L"buddybenj\r\n"
            L"don.carmical\r\n"
            L"kyussgreen\r\n"
            L"info1092\r\n"
            L"YQ\r\n"
            L"Allan Nordhøy\r\n"
            L"christian010\r\n"
            L"creuseur2patateradio\r\n"
            L"chrisbevanlee\r\n"
            L"theschklingen\r\n"
            L"Thomas Lindstrøm\r\n"
            L"Djipi\r\n"
            L"Dartus\r\n"
            L"Oscar Abraham\r\n"
            L"nwstrathdee\r\n"
            L"will7046\r\n"
            L"Richard42\r\n"
            L"V1del\r\n"
            L"AnthonyBentley\r\n"
            L"buddybenj\r\n"
            L"nickshooter251\r\n"
            L"sicurella12\r\n"
            L"jcspringer\r\n"
            L"Gru So\r\n"
            L"Vinícius dos Santos Oliveira\r\n"
            L"Jimmy Haugh\r\n"
            L"Malcolm\r\n"
            L"Alex Strange\r\n"
            L"Espen Jensen\r\n"
            L"m.johnsondelta\r\n"
            L"alexzandar.toxic2\r\n"
            L"Ben Slater\r\n"
            L"WC-Predator\r\n"
            L"Mush Man\r\n"
            L"Ben Slater\r\n"
            L"aznlucidx\r\n"
            L"Nathan Dick\r\n"
            L"paulanocom\r\n"
            L"Ryan Rosser\r\n"
            L"nekow42\r\n"
            L"mgos1\r\n"
            L"ian.macdonald996\r\n"
            L"itasovski\r\n"
            L"vikingpower1\r\n"
            L"DukeX007X\r\n"
            L"palaciosgabriel\r\n"
            L"Franz-Josef Haider\r\n"
            L"e-male\r\n"
            L"aweath\r\n"
            L"famicom4\r\n"
            L"Keith_at_UMR\r\n"
            L"sweatypickle\r\n"
            L"jeremydmiller\r\n"
        };
        CWindow Funders = GetDlgItem(IDC_FUNDERS);
        Funders.SetWindowText(Funders1);
        GetDlgItem(IDC_FUNDERS_LIST).SetWindowText(FundersList);

        CClientDC dc(m_hWnd);
        int nHeight = -MulDiv(9, dc.GetDeviceCaps(LOGPIXELSY), 72);

        CFontHandle cfh;
        LOGFONT lf = { 0 };
        cfh.Attach(Funders.GetFont());
        cfh.GetLogFont(&lf);
        lf.lfHeight = nHeight;
        lf.lfWeight = 800;
        m_FounderFont.CreateFontIndirect(&lf);
        Funders.SetFont(m_FounderFont);

        return true;
    };
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        CRect WinRect;
        GetWindowRect(&WinRect);
        ScreenToClient(&WinRect);

        GetDlgItem(IDC_FUNDERS).SetWindowPos(HWND_TOP, WinRect.left + 20, WinRect.top + 20, (WinRect.Width() / 2) - 20, WinRect.Height() - 40, SWP_SHOWWINDOW);
        GetDlgItem(IDC_FUNDERS_LIST).SetWindowPos(HWND_TOP, (WinRect.Width() / 2) + WinRect.left + 20, WinRect.top + 20, (WinRect.Width() / 2) - 40, WinRect.Height() - 40, SWP_SHOWWINDOW);
        return 0;
    }
    LRESULT OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
    {
        if ((HWND)lParam == GetDlgItem(IDC_FUNDERS_LIST))
        {
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
        }
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }

    CFont m_FounderFont;
};

class CAboutCreditsTab :
    public CAboutTab
{
public:
    BEGIN_MSG_MAP(CAboutCreditsTab)
        MSG_WM_INITDIALOG(OnInitDialog)
        MESSAGE_HANDLER(WM_CTLCOLORDLG, OnColorStatic)
        MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnColorStatic)
    END_MSG_MAP()

    CAboutCreditsTab() :
        CAboutTab(IDD_TAB_CREDITS)
    {
    }
    BOOL OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
    {
        CWindow Author = GetDlgItem(IDC_ORKIN);
        
        CClientDC dc(m_hWnd);
        int nHeight = -MulDiv(12, dc.GetDeviceCaps(LOGPIXELSY), 72);

        CFontHandle cfh;
        LOGFONT lf = { 0 };
        cfh.Attach(Author.GetFont());
        cfh.GetLogFont(&lf);
        lf.lfHeight = nHeight;
        lf.lfWeight = 800;
        m_AuthorFont.CreateFontIndirect(&lf);
        Author.SetFont(m_AuthorFont);

        GetDlgItem(IDC_YONGZH).SetFont(m_AuthorFont);
        GetDlgItem(IDC_HIROSHI).SetFont(m_AuthorFont);
        GetDlgItem(IDC_ZIGGY).SetFont(m_AuthorFont);
        return true;
    };

    CFont m_AuthorFont;
};

CAboutTab::CAboutTab(uint32_t _IDD) :
    IDD(_IDD)
{
}

CAboutTab::~CAboutTab()
{
}

BOOL CAboutTab::OnInitDialog(CWindow /*wndFocus*/, LPARAM /*lInitParam*/)
{
    return true;
}

LRESULT CAboutTab::OnColorStatic(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return (LRESULT)GetStockObject(WHITE_BRUSH);
}

CAboutDlg::~CAboutDlg()
{
    for (size_t i = 0, n = m_TabWindows.size(); i < n; i++)
    {
        delete m_TabWindows[i];
    }
    m_TabWindows.clear();
}

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	HICON hIcon = AtlLoadIconImage(IDI_APPICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDI_APPICON, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	CStatic AboutTitle;
	AboutTitle.Attach(GetDlgItem(IDC_ABOUT_TITLE));
	
	CClientDC dc(m_hWnd);
	int nHeight = -MulDiv(29, dc.GetDeviceCaps(LOGPIXELSY), 72);

	CFontHandle cfh;
	LOGFONT lf = { 0 };
	cfh.Attach(AboutTitle.GetFont());
	cfh.GetLogFont(&lf);
	lf.lfHeight = nHeight;
	lf.lfWeight = 800;
	m_TitleFont.CreateFontIndirect(&lf);
	AboutTitle.SetFont(m_TitleFont);

	m_Tabs.Attach(GetDlgItem(IDC_TABS));
    AddTab(L"About", new CAboutTab(IDD_TAB_ABOUT));
    AddTab(L"Contributors", new CAboutContributersTab);
    AddTab(L"Funders", new CAboutFundersTab);
    AddTab(L"Credits", new CAboutCreditsTab);
	return 0;
}

CRect CAboutDlg::GetTabRect()
{
	CRect TabRect;
	m_Tabs.GetWindowRect(&TabRect);
	ScreenToClient(&TabRect);
	m_Tabs.AdjustRect(FALSE, &TabRect);
	return TabRect;
}

void CAboutDlg::AddTab(const wchar_t * caption, CAboutTab * tab)
{
	m_Tabs.AddItem(caption);
    tab->Create(m_hWnd, 0);
    tab->SetWindowPos(m_hWnd, 0, 0, 0, 0, SWP_HIDEWINDOW);
	m_TabWindows.push_back(tab);

	if (m_TabWindows.size() == 1)
	{
		ShowTab(0);
	}
}

void CAboutDlg::ShowTab(int nPage)
{
	for (size_t i = 0; i < m_TabWindows.size(); i++)
	{
		m_TabWindows[i]->ShowWindow(SW_HIDE);
	}

	CRect TabRect = GetTabRect();
	m_TabWindows[nPage]->SetWindowPos(HWND_TOP, TabRect.left, TabRect.top, TabRect.Width(), TabRect.Height(), SWP_SHOWWINDOW);
	
    CRect WinRect, ClientRect;
    m_TabWindows[nPage]->GetWindowRect(WinRect);
    m_TabWindows[nPage]->GetClientRect(ClientRect);

    m_Tabs.RedrawWindow();
}

LRESULT CAboutDlg::OnTabChange(NMHDR* /*pNMHDR*/)
{
	ShowTab(m_Tabs.GetCurSel());
	return FALSE;
}

LRESULT CAboutDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	EndDialog(wID);
	return 0;
}