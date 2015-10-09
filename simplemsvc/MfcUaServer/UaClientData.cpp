// UaClientData.cpp: Implementierungsdatei
//

#include "StdAfx.h"
#include "UaServer.h"
#include "UaClientData.h"
extern "C"
{
    #include "../../src/server/ua_server_internal.h"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
static const char __MODNAME[] = "UaClientData";

/////////////////////////////////////////////////////////////////////////////
// UaClientData
DWORD m_tPacketTimeout   = 5*1000;
DWORD m_tWCAnswerTimeout = 8*1000;
DWORD m_tDrvStatDelay    = 10*1000;
bool  m_bVarStatusDelay;
bool  m_bRealTimeStamp=1;             // Realen Zeitstempel Melden
bool  m_bCheckPass;
DWORD m_tTimeStampDelay;
CString  m_PassWordList[2];
CString  m_StdFirmwareFile;
WORD  m_wS7RedirType=1;

UaClientData::UaClientData(UaServer *pSrv)
{
    m_pSrv = pSrv;
    m_pNext = NULL;
    m_iSendErrCnt = 0;
    m_Rec.Init(pSrv->m_UaConf.maxMessageSize);
    ConnectionInit();
    //FreeEntry(_T(""));
}

void UaClientData::ConnectionInit()
{
    UA_Connection_init(this);
    handle = this;
    getSendBuffer = UaC_getSendBuffer;
    releaseSendBuffer = UaC_releaseSendBuffer;
    send = UaC_send;
    recv = UaC_recv;
    releaseRecvBuffer = UaC_releaseRecvBuffer;
    close = UaC_close;
    state = UA_CONNECTION_OPENING;
}

UaClientData::~UaClientData()
{
    OnClose(0);
}

/////////////////////////////////////////////////////////////////////////////
// Behandlungsroutinen für Nachrichten UaClientData 
void UaClientData::OnReceive(int /*nErrorCode*/)
{
	CheckReceive();
}

#define UABYTE2ULONG(d, off) ((d)[off] | ((d)[off+1] << 8) | ((d)[off+2] << 16) | ((d)[off+3] << 24));

int UaClientData::CheckReceive()
{
	// get the bytes....

    int nBytes, nRest;
    bool bCheckJobs = false;
    while (true)
    {
        nRest = m_Rec.m_uBufLen - m_Rec.m_uRecLen;
        nBytes = Receive(m_Rec.m_Buf.data + m_Rec.m_uRecLen, nRest);
        if (nBytes <= 0)
            break;
        m_Rec.m_uRecLen += nBytes;
        while (m_Rec.m_uRecLen >= 8)
        {
            //UA_Int32 uType = UABYTE2ULONG(m_Rec.m_Buf.data, 0);
            UA_UInt32 n = UABYTE2ULONG(m_Rec.m_Buf.data, 4);
            if (m_Rec.m_Buf.data[3] != 'F' || n > localConf.maxMessageSize)
            {
                // Bad telegramm Bye
                CloseConnection("bad telegrammformat");
            }
            if (m_Rec.m_uRecLen < n)
                break;
            m_Rec.m_Buf.length = n;
            UA_Server_processBinaryMessage(m_pSrv->m_pUaServer, this, &m_Rec.m_Buf);
            bCheckJobs = true;
            if (m_Rec.m_uRecLen == 0)
                return 0;   // Closed
            m_Rec.m_uRecLen -= n;
            if (m_Rec.m_uRecLen > 0)
            {
                // rest for next packet
                memcpy(m_Rec.m_Buf.data, m_Rec.m_Buf.data + n, m_Rec.m_uRecLen);
            }
        }
        if (m_Rec.m_uRecLen <= 0)
            break;
    }
    if (bCheckJobs)
        m_pSrv->SrvCheckJobs();
    return 0;
}


void UaClientData::OnSend(int /*nErrorCode*/)
{
}

void UaClientData::OnClose(int nErrorCode)
{
    CString strTmp;
    //TRACE_MSG((T_2,_T("IP %s, Nr %d: Close Err %d h=%d"),m_strIP,m_iIdent,nErrorCode,m_hSocket));
    if (nErrorCode == WSAECONNABORTED /*10053*/)
        strTmp = _T("Timeout");
    else if (nErrorCode == WSAECONNRESET /*10054*/)
        strTmp = _T("Web-Connector");
    else if (nErrorCode == 0)
        strTmp = _T("");
    else
        strTmp.Format(_T("Close Err %d"),nErrorCode);
    CloseConnection(strTmp);
    if (m_pSrv)
		m_pSrv->NotifyDisconn(this);
}

void UaClientData::AfterConnect()
{
    TRACE_MSG((T_3, _T("%s:%u has connected"), m_strIP, m_uPort));
}


void UaClientData::CloseConnection(LPCTSTR strReason)
{
    if (m_hSocket != INVALID_SOCKET)
    {
        if (strReason[0])
        {
            UaLogMsg(UA_LOGLEVEL_ERROR, UA_LOGCATEGORY_SERVER, "IP %s Disconnect: %s", m_sPeerName, strReason);
        }
    }
    Close();
}

void UaClientData::Close()
{
    if (m_hSocket != INVALID_SOCKET)
    {
        UaLogMsg(UA_LOGLEVEL_INFO, UA_LOGCATEGORY_SERVER, "Close %s:%u", m_sPeerName, m_uPeerPort);
        CAsyncSocket::Close();
    }
    else
    {
        CAsyncSocket::Close();
    }
    UA_Connection_detachSecureChannel(this);
    UA_Connection_deleteMembers(this);
    m_Rec.Reset();
}

void UaClientData::TimerCheck()
{
}

bool UaClientData::SendAndWait(const void* lpBuf, int nBufLen)
{
    int n;
    HCURSOR hOldCur = 0;
    const char *pBuf = (const char *)lpBuf;
    if (!bIsSockConnected())
        return false;
    m_tLastSend = GetTickCount();
    do {
        n = CAsyncSocket::Send(pBuf, nBufLen, 0);
        if (n == -1)
        {
            DWORD err = GetLastError();
            if (err != WSAEWOULDBLOCK)
            {
                TRACE_MSG((T_ERR, _T("Error %d IP %s, Nr %d: Send %d Bytes"), err, m_strIP, m_iIdent, nBufLen));
                return false;
            }
            // abwarten bis TCP/IP-Buffer frei, Send() würde blockieren
            if (hOldCur == 0)
            {
                hOldCur = SetCursor(LoadCursor(NULL, IDC_WAIT));
            }
            Sleep(100);
            n = 0;
        }
        if ((GetTickCount() - m_tLastSend) > 10000l)
        {
            m_iSendErrCnt++;
            TRACE_MSG((T_ERR, _T("Error IP %s, Nr %d: Timeout Send %d Bytes left, retry %d"), m_strIP, m_iIdent, nBufLen, m_iSendErrCnt));
            if (m_iSendErrCnt >= 10)
            {
                // Mehr Fehler als OK
                CloseConnection("Send Timeout");
            }
            return false;
        }
        nBufLen -= n;
        pBuf += n;
    } while (nBufLen > 0);
    // Retry wieder langsam zurück drehen
    if (m_iSendErrCnt > 0)
        m_iSendErrCnt--;
    if (hOldCur != 0)
    {
        SetCursor(hOldCur);
    }
    return true;
}

UA_StatusCode UaClientData::Send(UA_ByteString *buf)
{
    if (SendAndWait(buf->data, buf->length) == true)
        return UA_STATUSCODE_GOOD;
    return UA_STATUSCODE_BADCONNECTIONCLOSED;
}

UA_StatusCode UaClientData::Recv(UA_ByteString *response, UA_UInt32 timeout)
{
    UA_ByteString_init(response);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UaClientData::GetSendBuffer(UA_Int32 length, UA_ByteString *buf)
{
    *buf = m_pSrv->m_SenBuf;
    return UA_STATUSCODE_GOOD;
}

void UaClientData::ReleaseSendBuffer(UA_ByteString *buf)
{

}

void UaClientData::ReleaseRecvBuffer(UA_ByteString *buf)
{

}


void UaClientData::CloseConn()
{
    if (state == UA_CONNECTION_CLOSED)
        return;
    state = UA_CONNECTION_CLOSED;
    Close();
}
