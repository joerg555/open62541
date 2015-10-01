
// MfcUaServerDlg.h: Headerdatei
//

#pragma once


// CMfcUaServerDlg-Dialogfeld
class CMfcUaServerDlg : public CDialogEx
{
// Konstruktion
public:
	CMfcUaServerDlg(CWnd* pParent = NULL);	// Standardkonstruktor

// Dialogfelddaten
	enum { IDD = IDD_MFCUASERVER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterstützung


// Implementierung
protected:
	HICON m_hIcon;

	// Generierte Funktionen für die Meldungstabellen
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    void CheckEnable();
	DECLARE_MESSAGE_MAP()
public:
    class ValItem
    {
    public:
        CString sValName;
        double dblVal;
    } m_val[3];
    afx_msg void OnBnClickedSave();
    afx_msg void OnBnClickedStart();
    afx_msg void OnBnClickedStop();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
};
