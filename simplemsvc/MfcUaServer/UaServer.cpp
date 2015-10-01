// UaServer.cpp: Implementierungsdatei
//

#include "StdAfx.h"
#include "UaServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const TCHAR __MODNAME[] = _T("UaServer");

#undef UA_STRING_NULL

const UA_String UA_STRING_NULL =
{
    -1, NULL
};

#undef UA_NODEID_NULL

const UA_NodeId UA_NODEID_NULL =
{
    0, UA_NODEIDTYPE_NUMERIC, { 0 }
};

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
}

UaServer::~UaServer()
{
    SrvStop();
}

static void UaLogMsg(UA_LogLevel level, UA_LogCategory category, const char *msg, ...) 
{
    printf(":");
    //print_time();
    va_list ap;
    va_start(ap, msg);
    //printf("] %s/%s\t", LogLevelNames[level], LogCategoryNames[category]);
    vprintf(msg, ap);
    printf("\n");
    va_end(ap);
}

bool UaServer::SrvStart()
{
	bool bOK = false;
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

bool UaServer::SrvReStart()
{
    Close();
    return SrvStart();
}

void UaServer::SrvStop()
{
    Close();
    if (m_pCliList == NULL)
        return; // Nichts zu tun

    UaClientData *pCli, *pNext;

    // Liste sofort sperren
    pCli = m_pCliList;
    m_pCliList = NULL;
    for ( ; pCli;)
    {
        pNext = pCli->m_pNext;
        if (pNext == NULL)
            break;
        pCli = pNext;
    }
}



bool UaServer::NotifyConnect(CAsyncSocket *pNewCli, const TCHAR *strIP, UINT uPort)
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
    pCli->Attach(pNewCli->Detach());
    pCli->AsyncSelect(FD_READ | FD_CLOSE);
    pCli->m_uPort = uPort;
    pCli->m_strIP = strIP;
    TRACE_MSG((T_3, _T("IP %s:%u Connected"), pCli->m_strIP, pCli->m_uPort));
    pCli->AfterConnect();
    return true;
}

void UaServer::NotifyDisconn(UaClientData *pCli)
{
    pCli->FreeEntry(_T("no Webconn"));
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
// nach 10 Sekunden alle Client ohne IdentNr trennen
//
void UaServer::SrvTimerCheck()
{
    UaClientData *pCli;
    int i;
    for (pCli = m_pCliList, i = 0; pCli; pCli = pCli->m_pNext, i++)
    {
        pCli->TimerCheck();
    }
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
    while (m_pCliList != NULL);
    {
        m_pCliList->CloseConn();
        pNext = m_pCliList->m_pNext;
            delete m_pCliList;
        m_pCliList = pNext;
    }
}

struct AddRepeatedJob 
{
    UA_Job job;
    UA_Guid jobid;
    UA_UInt32 interval;
};

static UA_UInt32 random_seed;

extern "C"
{

    UA_StatusCode UA_Server_addRepeatedJob(UA_Server *server, UA_Job job, UA_UInt32 interval, UA_Guid *jobId)
    {
        /* the interval needs to be at least 5ms */
        if (interval < 5)
            return UA_STATUSCODE_BADINTERNALERROR;
        interval *= 10000; // from ms to 100ns resolution

        struct AddRepeatedJob arw;
        arw.interval = interval;
        arw.job = job;
        if (jobId)
        {
            if (random_seed == 0)
                (UA_UInt32)UA_DateTime_now();
            arw.jobid = UA_Guid_random(&random_seed);
            *jobId = arw.jobid;
        }
        else
            UA_Guid_init(&arw.jobid);
        //addRepeatedJob(server, &arw);
        return UA_STATUSCODE_GOOD;
    }
    UA_StatusCode UA_Server_removeRepeatedJob(UA_Server *server, UA_Guid jobId) 
    {
        //removeRepeatedJob(server, &jobId);
        return UA_STATUSCODE_GOOD;
    }

    void UA_Server_deleteAllRepeatedJobs(UA_Server *server)
    {
    }
}
