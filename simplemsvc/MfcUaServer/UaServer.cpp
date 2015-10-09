// UaServer.cpp: Implementierungsdatei
//

#include "StdAfx.h"
#include "UaServer.h"
extern "C"
{
    #include "../../src/server/ua_server_internal.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const TCHAR __MODNAME[] = _T("UaServer");

#define UA_STRINGCONST(s) {sizeof(s)-1, (UA_Byte*)s }

/*************************/
/* Read-only data source */
/*************************/
static UA_StatusCode readTimeData(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *value)
{
    if (range)
    {
        value->hasStatus = UA_TRUE;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime *currentTime = UA_DateTime_new();
    if (!currentTime)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *currentTime = UA_DateTime_now();
    value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
    value->value.arrayLength = -1;
    value->value.data = currentTime;
    value->hasValue = UA_TRUE;
    if (sourceTimeStamp)
    {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = *currentTime;
    }
    return UA_STATUSCODE_GOOD;
}

const char m_strLang[] = "en_US";
const char m_strCurrTime[] = "currenttime";
const char m_strCounter[] = "counter";

UA_Logger m_Ualogger;

const UA_QualifiedName m_dateName =
{
    1, UA_STRINGCONST(m_strCurrTime)
};
const UA_LocalizedText m_dateNameBrowseName =
{
    UA_STRINGCONST(m_strLang), UA_STRINGCONST(m_strCurrTime)
};

const UA_QualifiedName m_CounterName =
{
    1, UA_STRINGCONST(m_strCounter)
};

const UA_LocalizedText m_CounterBrowseName =
{
    UA_STRINGCONST(m_strLang), UA_STRINGCONST(m_strCounter)
};


const UA_LocalizedText m_EmptyLocal_en_Us =
{
    UA_STRINGCONST(m_strLang), { 0, (UA_Byte *)"" }
};

UA_Double m_dblCounter = 42.3;

static UA_StatusCode readCounter(void *handle, UA_NodeId nodeid, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue)
{
    dataValue->hasValue = UA_TRUE;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)handle, &UA_TYPES[UA_TYPES_DOUBLE]);
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "Node read %.*s", nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "read value %g", *(UA_Double*)handle);
    m_dblCounter += 3.2;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeCounter(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range)
{
    if (UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_DOUBLE] && data->data)
    {
        *(UA_Double*)handle = *(UA_Double*)data->data;
    }
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "Node written %.*s", nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "written value %g", *(UA_Double*)handle);
    return UA_STATUSCODE_GOOD;
}


// Server ist Global
UaServer m_UaServer;


UaServer::UaServer()
{
    m_uListenport = 16664u;
    m_tLiveCheck = 60 * 60 * 1000l;
    m_pCliList = NULL;
    m_pTimerWnd = 0;
    m_iTimerID = 0;
}

UaServer::~UaServer()
{
    SrvStop();
}

BOOL m_UatraceTrace[6] = { 0, 1, 1, 1, 1, 1 };

void UaLogMsg(UA_LogLevel level, UA_LogCategory category, const char *msg, ...) 
{
    char msgBuf[300];
    va_list args;
    switch (level)
    {
    case UA_LOGLEVEL_TRACE: if (!m_UatraceTrace[0]) return; break;
    case UA_LOGLEVEL_DEBUG: if (!m_UatraceTrace[1]) return; break;
    case UA_LOGLEVEL_INFO: if (!m_UatraceTrace[2]) return; break;
    case UA_LOGLEVEL_WARNING: if (!m_UatraceTrace[3]) return; break;
    case UA_LOGLEVEL_ERROR: if (!m_UatraceTrace[4]) return; break;
    case UA_LOGLEVEL_FATAL: 
    default:                if (!m_UatraceTrace[5]) return; break;
    }
    va_start(args, msg);
    vsnprintf_s(msgBuf, _countof(msgBuf), _TRUNCATE, msg, args);
    strcat_s(msgBuf, _countof(msgBuf), "\n");
    va_end(args);
    TRACE(msgBuf);
}

bool UaServer::SrvStart(CWnd *pTimerWnd, int iTimerID)
{
	bool bOK = false;
    m_pTimerWnd = pTimerWnd;
    m_iTimerID = iTimerID;
    // 
    m_pUaServer = UA_Server_new(UA_ServerConfig_standard);
    m_pUa_logger = &m_Ualogger;
    m_Ualogger = UaLogMsg;
    UA_Server_setLogger(m_pUaServer, m_Ualogger);
    Init(UA_ConnectionConfig_standard, 16664);
    Start(m_Ualogger);
    UA_Server_addNetworkLayer(m_pUaServer, this);
#ifdef IPV6_V6ONLY
    if (IsWindows7OrGreater())
	{
		union {
			struct sockaddr     sadr;
			struct sockaddr_in  in;
			struct sockaddr_in6 in6;
		} s_adr;
		memset(&s_adr, 0, sizeof(s_adr));
		s_adr.in6.sin6_family = PF_INET6;
        s_adr.in6.sin6_port = htons((u_short)m_uListenport);
		// IPV6 Listen
		if (Socket(SOCK_STREAM, FD_ACCEPT, 0, PF_INET6) == TRUE)
		{
			DWORD dwVal = 0;
			// Listen IPV4 und IPV6 
			SetSockOpt(IPV6_V6ONLY, &dwVal, sizeof(dwVal), IPPROTO_IPV6);
			if (Bind(&s_adr.sadr, sizeof(s_adr.in6)) == TRUE)
			{
				bOK = true;
			}
		}
	}
#endif
	if (!bOK)
	{
        if (Create(m_uListenport, SOCK_STREAM, FD_ACCEPT) == TRUE)
			bOK = true;
	}
	if (!bOK)
	{
		TRACE_MSG((T_ERR, _T("Listen Port: %d in use ?"), m_uListenPort));
		return false;
	}
    Listen();
    TRACE_MSG((T_2, _T("Listen Port: %d"), m_uListenPort));
    if (m_pTimerWnd)
        m_pTimerWnd->SetTimer(m_iTimerID, 2000, NULL);
    return true;
}

void UaServer::Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort)
{
    UaNetLayer::Init(ConConfig, uListenPort);
    //handle = this;
    start = Ua_start;
    getJobs = Ua_getJobs; 
    stop = Ua_stop;
    deleteMembers = Ua_deleteMembers;
}

bool UaServer::SrvReStart(CWnd *pTimerWnd, int iTimerID)
{
    Close();
    return SrvStart(pTimerWnd, iTimerID);
}

void UaServer::SrvStop()
{
    Close();
    if (m_pUaServer)
    {
        UA_ServerNetworkLayer *pSrvNetLayer = m_pUaServer->networkLayersSize > 0 ? m_pUaServer->networkLayers[0] : NULL;
        m_pUaServer->networkLayersSize = 0;
        UA_Server_delete(m_pUaServer);
        if (pSrvNetLayer)
        {
            // destroy our networklayer
            UA_String_deleteMembers(&pSrvNetLayer->discoveryUrl);
            pSrvNetLayer->deleteMembers(pSrvNetLayer);
            // have own netlayer don't delete
            //UA_free(pSrvNetLayer);
        }
        m_pUaServer = NULL;
    }
    if (m_pTimerWnd != NULL)
    {
        m_pTimerWnd->KillTimer(m_iTimerID);
        m_pTimerWnd = NULL;
    }
}



bool UaServer::NotifyConnect(CAsyncSocket *pNewCli, const TCHAR *sPeerName, UINT uPeerPort)
{
    UaClientData *pCli;
    // leeren Eintrag suchen oder neuen Eintrag hinzufügen
    pCli = pAddUaClient();
    if (pCli == NULL)
    {
        //  hier kommt keiner an
        // Liste voll
        pNewCli->Close();
        return false;
    }
    // Neue Verbindung
    pCli->m_sPeerName = sPeerName;
    pCli->m_uPeerPort = uPeerPort;
    pCli->Attach(pNewCli->Detach());
    pCli->AsyncSelect(FD_READ | FD_CLOSE);
    TRACE_MSG((T_3, _T("IP %s:%u Connected"), pCli->m_sPeerName, pCli->m_uPeerPort));
    pCli->AfterConnect();
    return true;
}

void UaServer::NotifyDisconn(UaClientData *pCli)
{
    pCli->FreeEntry(_T("disconnect"));
}

UaClientData *UaServer::pAddUaClient()
{
    UaClientData *pCli;
    // leeren Eintrag suchen oder neuen Eintrag hinzufügen
    for (pCli = m_pCliList; pCli != NULL; pCli = pCli->m_pNext)
    {
        // es könnte auch eine alte Verbindung da sein
        if (pCli->bIsFree())
        {
            pCli->ConnectionInit();
            return pCli;
        }
    }
    // am Anfang der Liste Eintragen
    pCli = new UaClientData(this);
    pCli->m_pNext = m_pCliList;
    m_pCliList = pCli;
    return pCli;
}

// Client suchen
UaClientData *UaServer::pGetUaClient(SOCKET so)
{
    UaClientData *pCli;
    for (pCli = m_pCliList; pCli; pCli = pCli->m_pNext)
    {
        if (pCli->m_hSocket == so)
        {
            return pCli;
        }
    }
    return NULL;
}

//
// nach 10 Sekunden alle Client ohne daten trennen
//
void UaServer::SrvTimerCheck()
{
    UaClientData *pCli;
    int i;
    uint32_t tNetJob = 2000;
    for (pCli = m_pCliList, i = 0; pCli; pCli = pCli->m_pNext, i++)
    {
        pCli->TimerCheck();
    }
    if (m_pTimerWnd == NULL)
        return;
    if (m_pUaServer != NULL)
        tNetJob = processRepeatedJobs(m_pUaServer) / 1000;
    m_pTimerWnd->SetTimer(m_iTimerID, tNetJob, NULL);
}

void UaServer::SrvCheckJobs()
{
    uint32_t tNetJob = 2000;
    if (m_pTimerWnd == NULL)
        return;
    if (m_pUaServer != NULL)
        tNetJob = nextRepeatedJob(m_pUaServer, UA_DateTime_now()) / 1000;
    m_pTimerWnd->SetTimer(m_iTimerID, tNetJob, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// Member-Funktion SrvSocket 
void UaServer::OnAccept(int /*nErrorCode*/)
{
    CAsyncSocket req;
    if (Accept(req))
    {
        CString strPeerName;
        UINT uPort= 0;
        BOOL bOpt = 1;
        req.SetSockOpt(TCP_NODELAY, &bOpt, sizeof(bOpt), IPPROTO_TCP);
		//SOCKADDR_STORAGE sockAddr;
		union {
			struct sockaddr     sadr;
			struct sockaddr_in  in4;
			struct sockaddr_in6 in6;
		} sockAddr;
		memset(&sockAddr, 0, sizeof(sockAddr));

		int nSockAddrLen = sizeof(sockAddr);
		req.GetPeerName(&sockAddr.sadr, &nSockAddrLen);
		if (sockAddr.sadr.sa_family == PF_INET6)
		{
			if (sockAddr.in6.sin6_addr.u.Word[0] == 0)
				strPeerName.Format(_T("%d.%d.%d.%d"), sockAddr.in6.sin6_addr.u.Byte[12], sockAddr.in6.sin6_addr.u.Byte[13], sockAddr.in6.sin6_addr.u.Byte[14], sockAddr.in6.sin6_addr.u.Byte[15]);
			else
				strPeerName.Format(_T("%x:%x:%x:%x:%x:%x:%x:%x"),
					sockAddr.in6.sin6_addr.u.Word[0],
					sockAddr.in6.sin6_addr.u.Word[1],
					sockAddr.in6.sin6_addr.u.Word[2],
					sockAddr.in6.sin6_addr.u.Word[3],
					sockAddr.in6.sin6_addr.u.Word[4],
					sockAddr.in6.sin6_addr.u.Word[5],
					sockAddr.in6.sin6_addr.u.Word[6],
					sockAddr.in6.sin6_addr.u.Word[7]
				);
				uPort = ntohs(sockAddr.in6.sin6_port);
		}
		else
		{
			strPeerName.Format(_T("%d.%d.%d.%d"), sockAddr.in4.sin_addr.S_un.S_un_b.s_b1, sockAddr.in4.sin_addr.S_un.S_un_b.s_b2, sockAddr.in4.sin_addr.S_un.S_un_b.s_b3, sockAddr.in4.sin_addr.S_un.S_un_b.s_b4);
			uPort = ntohs(sockAddr.in4.sin_port);
		}
        UaLogMsg(UA_LOGLEVEL_INFO, UA_LOGCATEGORY_SERVER, "Accept %s:%u", strPeerName, uPort);
        NotifyConnect(&req, strPeerName, uPort);
    }
}

UA_Int32 UaServer::GetJobs(UA_Job **jobs, UA_UInt16 timeout)
{
    *jobs = NULL;
    return 0;
}

UA_StatusCode UaServer::Start(UA_Logger UaLogger)
{
    UaNetLayer::Start(UaLogger);
    return UA_STATUSCODE_GOOD;
}

UA_Int32 UaServer::Stop(UA_Job **jobs)
{
    return UaNetLayer::Stop(jobs);
}

void UaServer::Deletemembers()
{
    UaNetLayer::Deletemembers();
    UaClientData *pNext;
    while (m_pCliList != NULL)
    {
        m_pCliList->CloseConn();
        pNext = m_pCliList->m_pNext;
        delete m_pCliList;
        m_pCliList = pNext;
    }
}

static UA_UInt32 random_seed;

//UA_StatusCode UaServer::AddRepeatedJob(UA_Job job, UA_UInt32 IntervalMs, UA_Guid *pJobId)
//{
//    /* the interval needs to be at least 5ms */
//    if (IntervalMs < 5)
//        return UA_STATUSCODE_BADINTERNALERROR;
//    RepeatedJob *pJob;
//    int i;
//    for (i = 0, pJob = m_JobList.GetData(); true; i++, pJob++)
//    {
//        if (i >= m_JobList.GetCount())
//        {
//            int n = m_JobList.GetCount();
//            m_JobList.SetSize(n + 1);
//            pJob = m_JobList.GetData() + n;
//            break;
//        }
//        if (pJob->msInterval == 0)
//            break;
//    }
//    pJob->msInterval = IntervalMs;
//    pJob->tStart = GetTickCount();
//    pJob->job = job;
//    if (pJobId)
//    {
//        if (random_seed == 0)
//            random_seed = (UA_UInt32)UA_DateTime_now();
//        pJob->Guid = UA_Guid_random(&random_seed);
//        *pJobId = pJob->Guid;
//    }
//    else
//    {
//        UA_Guid_init(&pJob->Guid);
//    }
//    //addRepeatedJob(server, &arw);
//    return UA_STATUSCODE_GOOD;
//}
//
//UA_StatusCode UaServer::RemoveRepeatedJob(UA_Guid jobId)
//{
//    RepeatedJob *pJob;
//    int i;
//    for (i = 0, pJob = m_JobList.GetData(); i < m_JobList.GetCount(); i++, pJob++)
//    {
//        if (!UA_Guid_equal(&jobId, &pJob->Guid))
//        {
//            pJob->msInterval = 0;
//            UA_Guid_init(&pJob->Guid);
//            return UA_STATUSCODE_GOOD;
//        }
//    }
//    return UA_STATUSCODE_GOOD;
//}
//
//UA_StatusCode UA_Server_addRepeatedJob(UA_Server *server, UA_Job job, UA_UInt32 IntervalMs, UA_Guid *pJobId)
//{
//    if (server->networkLayersSize == 0)
//    {
//        // Krücke Job Aufheben
//        server->repeatedJobs.lh_first = (struct RepeatedJobs *)job.job.methodCall.method;
//        return UA_STATUSCODE_GOOD;
//    }
//    UaServer *pUaServer = (UaServer *)server->networkLayers[0];
//    return pUaServer->AddRepeatedJob(job, IntervalMs, pJobId);
//}
//
//UA_StatusCode UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobGuid) 
//{
//    UaServer *pUaServer = (UaServer *)server->networkLayers[0];
//
//    pUaServer->RemoveRepeatedJob(jobGuid);
//    return UA_STATUSCODE_GOOD;
//}
//
//void UA_Server_deleteAllRepeatedJobs(UA_Server *server)
//{
//}

