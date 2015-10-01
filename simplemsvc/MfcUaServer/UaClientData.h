#pragma once

// UaClientData.h : Header-Datei
//


class UaClientData : public CAsyncSocket, UA_Connection
{
    friend class UaServer;
    friend class SrvSocket;
public:
    UaClientData *m_pNext;
    CString m_strIP;
    UINT    m_uPort;
protected:
    //
    class UaServer *m_pSrv;
    DWORD m_tLastSend;
    int m_iSendErrCnt;
    UA_ByteString m_RecBuf;
    int m_iRecBufLen;
    int m_iRecLen;
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

