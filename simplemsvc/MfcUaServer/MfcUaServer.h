
// MfcUaServer.h: Hauptheaderdatei für die PROJECT_NAME-Anwendung
//

#pragma once

#ifndef __AFXWIN_H__
	#error "'stdafx.h' vor dieser Datei für PCH einschließen"
#endif

#include "resource.h"		// Hauptsymbole


// CMfcUaServerApp:
// Siehe MfcUaServer.cpp für die Implementierung dieser Klasse
//

class CMfcUaServerApp : public CWinApp
{
public:
	CMfcUaServerApp();

// Überschreibungen
public:
	virtual BOOL InitInstance();

// Implementierung

	DECLARE_MESSAGE_MAP()
};

extern CMfcUaServerApp theApp;