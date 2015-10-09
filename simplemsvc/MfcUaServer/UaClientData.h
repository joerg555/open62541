#pragma once

// UaClientData.h : Header-Datei
//


class UaClientData : public CAsyncSocket, UA_Connection
{
    friend class UaServer;
    friend class SrvSocket;
public:
    UaClientData *m_pNext;
    // Partner Name / Port
    CString m_sPeerName;
    UINT    m_uPeerPort;
protected:
    //
    class UaServer *m_pSrv;
    DWORD m_tLastSend;
    int m_iSendErrCnt;
    class RecBuf
    {
    public:
        UA_ByteString m_Buf;
        UA_UInt32 m_uBufLen;
        UA_UInt32 m_uRecLen;
    public:
        RecBuf()
        {
            m_uBufLen=0;
        }
        ~RecBuf()
        {
            Free();
        }
        void Init(UA_UInt32 uBufSize)
        {
            m_uBufLen = uBufSize;
            m_Buf.data = (UA_Byte *)malloc(m_uBufLen);
            Reset();
        }
        void Reset()
        {
            m_Buf.length = 0;    // Alles frei
            m_uRecLen = 0;
        }
        void Free()
        {
            if (m_uBufLen > 0)
            {
                free(m_Buf.data);
                m_uBufLen = 0;
            }
        }
    } m_Rec;
public:
    UaClientData(UaServer *m_pSrv);
    virtual ~UaClientData();
    // Daten senden Counter hochzählen
    bool SendAndWait(const void* lpBuf, int nBufLen);
    void FreeEntry(LPCTSTR strReason)
    {
        CloseConnection(strReason);
    }

    bool bIsFree()
    {
        return m_hSocket == INVALID_SOCKET;
    }

    void AfterConnect();
    void CloseConnection(LPCTSTR strReason);
    void Close();
    void TimerCheck();
    bool bIsSockConnected()
    {
        return m_hSocket != INVALID_SOCKET;
    }
    int CheckReceive();
public:
    virtual void OnClose(int nErrorCode);
protected:
    virtual void OnReceive(int nErrorCode);
    virtual void OnSend(int nErrorCode);
public:
    //
    // UA_Connection interface
    //
    UA_StatusCode GetSendBuffer(UA_Int32 length, UA_ByteString *buf);
    void ReleaseSendBuffer(UA_ByteString *buf);
    UA_StatusCode Send(UA_ByteString *buf);
    UA_StatusCode Recv(UA_ByteString *response, UA_UInt32 timeout);
    void ReleaseRecvBuffer(UA_ByteString *buf);
    void CloseConn();
    void ConnectionInit();

private:
    static UA_StatusCode UaC_getSendBuffer(UA_Connection *connection, UA_Int32 length, UA_ByteString *buf)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->GetSendBuffer(length, buf);
    }
    static void UaC_releaseSendBuffer(UA_Connection *connection, UA_ByteString *buf)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->ReleaseSendBuffer(buf);
    }
    static UA_StatusCode UaC_send(UA_Connection *connection, UA_ByteString *buf)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->Send(buf);
    }
    static UA_StatusCode UaC_recv(UA_Connection *connection, UA_ByteString *response, UA_UInt32 timeout)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->Recv(response, timeout);
    }
    static void UaC_releaseRecvBuffer(UA_Connection *connection, UA_ByteString *buf)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->ReleaseRecvBuffer(buf);
    }
    static void UaC_close(UA_Connection *connection)
    {
        UaClientData *pConn = (UaClientData *)connection->handle;
        return pConn->CloseConn();
    }
};

