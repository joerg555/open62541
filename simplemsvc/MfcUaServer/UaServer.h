#pragma once
// UaServer.h : Header-Datei
//
#include "../src/UaNetLayer.h"
#include "UaClientData.h"

extern BOOL m_UatraceTrace[6];
void UaLogMsg(UA_LogLevel level, UA_LogCategory category, const char *msg, ...);

#define TRACE_MSG(p)
#define TRACE_MSGx(p)

class RepeatedJob
{
public:
    UA_Job job;
    UA_Guid Guid;
    UA_UInt32 msInterval;
    UA_UInt32 tStart;
};

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
    // Liste mit verbundenen Clients
    UaClientData *m_pCliList;
    // Timer-verarbeitung 
    CWnd *m_pTimerWnd;
    int m_iTimerID;
    //
    UA_Server *m_pUaServer;
    //// Repeated jobs
protected:
    //CArray<RepeatedJob, RepeatedJob> m_JobList;
public:
    //UA_StatusCode AddRepeatedJob(UA_Job job, UA_UInt32 IntervalMs, UA_Guid *pJobId);
    //UA_StatusCode RemoveRepeatedJob(UA_Guid jobId);
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
    bool SrvReStart(CWnd *pTimerWnd, int iTimerID);
    bool SrvStart(CWnd *pTimerWnd, int iTimerID);
    void SrvStop();
    void SrvTimerCheck();
    void SrvCheckJobs();
    bool NotifyConnect(CAsyncSocket *pCli, const TCHAR *strIP, UINT uPort);
    void NotifyDisconn(UaClientData *pCli);
protected:
    UaClientData *pAddUaClient();
};

extern UaServer m_UaServer;

