
// MfcUaServerDlg.cpp: Implementierungsdatei
//

#include "stdafx.h"
#include "MfcUaServer.h"
#include "MfcUaServerDlg.h"
#include "afxdialogex.h"
#include "UaServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg-Dialogfeld für Anwendungsbefehl "Info"

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialogfelddaten
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterstützung

// Implementierung
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


CMfcUaServerDlg::CMfcUaServerDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMfcUaServerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_val[0].dblVal = 47.11;
    m_val[1].dblVal = 13.54;
    m_val[2].dblVal = 7.9;
    m_val[0].sValName = _T("pump1.waterlevel1");
    m_val[1].sValName = _T("pump1.waterlevel2");
    m_val[2].sValName = _T("pump1.valvepos");
}

void CMfcUaServerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_VALNAME1, m_val[0].sValName);
    DDX_Text(pDX, IDC_VALNAME2, m_val[1].sValName);
    DDX_Text(pDX, IDC_VALNAME3, m_val[2].sValName);
    DDX_Text(pDX, IDC_VAL1, m_val[0].dblVal);
    DDX_Text(pDX, IDC_VAL2, m_val[1].dblVal);
    DDX_Text(pDX, IDC_VAL3, m_val[2].dblVal);
    DDX_Check(pDX, IDC_CHKTRACE, m_UatraceTrace[0]);
    DDX_Check(pDX, IDC_CHKDEBUG, m_UatraceTrace[1]);
    DDX_Check(pDX, IDC_CHKINFO, m_UatraceTrace[2]);
    DDX_Check(pDX, IDC_CHKWARN, m_UatraceTrace[3]);
}

BEGIN_MESSAGE_MAP(CMfcUaServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_SAVE, &CMfcUaServerDlg::OnBnClickedSave)
    ON_BN_CLICKED(IDC_START, &CMfcUaServerDlg::OnBnClickedStart)
    ON_BN_CLICKED(IDC_STOP, &CMfcUaServerDlg::OnBnClickedStop)
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_BN_CLICKED(IDC_CHKTRACE, &CMfcUaServerDlg::OnBnClickedChkTrace)
    ON_BN_CLICKED(IDC_CHKDEBUG, &CMfcUaServerDlg::OnBnClickedChkTrace)
    ON_BN_CLICKED(IDC_CHKINFO, &CMfcUaServerDlg::OnBnClickedChkTrace)
    ON_BN_CLICKED(IDC_CHKWARN, &CMfcUaServerDlg::OnBnClickedChkTrace)
END_MESSAGE_MAP()


// CMfcUaServerDlg-Meldungshandler

BOOL CMfcUaServerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

	// Hinzufügen des Menübefehls "Info..." zum Systemmenü.

	// IDM_ABOUTBOX muss sich im Bereich der Systembefehle befinden.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Symbol für dieses Dialogfeld festlegen.  Wird automatisch erledigt
	//  wenn das Hauptfenster der Anwendung kein Dialogfeld ist
	SetIcon(m_hIcon, TRUE);			// Großes Symbol verwenden
	SetIcon(m_hIcon, FALSE);		// Kleines Symbol verwenden

    m_UaServer.SrvStart(this, 334);
    CheckEnable();
	return TRUE;
}

void CMfcUaServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// Wenn Sie dem Dialogfeld eine Schaltfläche "Minimieren" hinzufügen, benötigen Sie
//  den nachstehenden Code, um das Symbol zu zeichnen.  Für MFC-Anwendungen, die das 
//  Dokument/Ansicht-Modell verwenden, wird dies automatisch ausgeführt.

void CMfcUaServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // Gerätekontext zum Zeichnen

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Symbol in Clientrechteck zentrieren
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Symbol zeichnen
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// Die System ruft diese Funktion auf, um den Cursor abzufragen, der angezeigt wird, während der Benutzer
//  das minimierte Fenster mit der Maus zieht.
HCURSOR CMfcUaServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CMfcUaServerDlg::OnBnClickedSave()
{
    UpdateData(TRUE);
    UpdateData(FALSE);
    CheckEnable();
}


void CMfcUaServerDlg::OnBnClickedStart()
{
    m_UaServer.SrvReStart(this, 334);
    CheckEnable();
}


void CMfcUaServerDlg::OnBnClickedStop()
{
    m_UaServer.SrvStop();
    CheckEnable();
}


void CMfcUaServerDlg::OnDestroy()
{
    m_UaServer.SrvStop();
    CDialogEx::OnDestroy();
}

void CMfcUaServerDlg::CheckEnable()
{
    bool bOn = m_UaServer.bIsOn();
    GetDlgItem(IDC_START)->EnableWindow(!bOn);
    GetDlgItem(IDC_STOP)->EnableWindow(bOn);
}

void CMfcUaServerDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == 334)
    {
        m_UaServer.SrvTimerCheck();
    }
    CDialogEx::OnTimer(nIDEvent);
}


void CMfcUaServerDlg::OnBnClickedChkTrace()
{
    UpdateData(TRUE);
}
