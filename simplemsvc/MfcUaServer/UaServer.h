#pragma once
// UaServer.h : Header-Datei
//
#include "../src/UaNetLayer.h"
#include "UaClientData.h"


#define TRACE_MSG(p)
#define TRACE_MSGx(p)

/////////////////////////////////////////////////////////////////////////////
// Befehlsziel UaServer 
class UaServer : public UaNetLayer, CAsyncSocket
{
    friend class UaClientData;
public:
    // Attribute
    DWORD m_tLiveCheck;    // Standard 60*60*1000
    UA_Logger *m_pUa_logger;
    UA_UInt32 m_uListenport;
protected:
    //SrvSocket m_Sock;
    // Liste mit verbundenen Clients
    UaClientData *m_pCliList;
    UA_Server *m_pUaServer;
public:
    UaServer();
    ~UaServer();
    // CAsyncSocket
    virtual void OnAccept(int nErrorCode);
    UaClientData *pGetUaClient(SOCKET so);
    void Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort);
    // 
    // UA_ServerNetworkLayer interface
    //
    UA_Int32 GetJobs(UA_Job **jobs, UA_UInt16 timeout);
    UA_StatusCode Start(UA_Logger UaLogger);
    UA_Int32 Stop(UA_Job **jobs);
    void Deletemembers();
    static size_t Ua_getJobs(struct UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout)
    {
        UaServer *pLayer = (UaServer *)nl;
        return pLayer->GetJobs(jobs, timeout);
    }
    static UA_StatusCode Ua_start(struct UA_ServerNetworkLayer *nl, UA_Logger logger)
    {
        UaServer *pLayer = (UaServer *)nl;
        return pLayer->Start(logger);
    }
    static size_t Ua_stop(struct UA_ServerNetworkLayer *nl, UA_Job **jobs)
    {
        UaServer *pLayer = (UaServer *)nl;
        return pLayer->Stop(jobs);
    }
    static void Ua_deleteMembers(struct UA_ServerNetworkLayer *nl)
    {
        UaServer *pLayer = (UaServer *)nl;
        pLayer->Deletemembers();
    }
    //
    bool bIsOn()
    {
        return m_hSocket != INVALID_SOCKET;
    }
    bool SrvReStart();
    bool SrvStart();
    void SrvStop();
    void SrvTimerCheck();
    bool NotifyConnect(CAsyncSocket *pCli, const TCHAR *strIP, UINT uPort);
    void NotifyDisconn(UaClientData *pCli);
protected:
    UaClientData *pAddUaClient();
};

extern UaServer m_UaServer;

