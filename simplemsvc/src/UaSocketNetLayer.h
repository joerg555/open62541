/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef UaSocketNetLayer__h
#define UaSocketNetLayer__h
#include "UaNetLayer.h"

class UaSocketNetLayer;
class UaConnection : public UA_Connection
{
public:
    UaConnection(UA_Int32 newsockfd, UaSocketNetLayer *pNetLayer);
    void CloseSock();
    void Init(UA_Int32 newsockfd, UA_ConnectionConfig Conf);
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
    UaSocketNetLayer *m_pNetLayer;
    static UA_StatusCode UaC_getSendBuffer(UA_Connection *connection, size_t length, UA_ByteString *buf)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->GetSendBuffer(length, buf);
    }
    static void UaC_releaseSendBuffer(UA_Connection *connection, UA_ByteString *buf)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->ReleaseSendBuffer(buf);
    }
    static UA_StatusCode UaC_send(UA_Connection *connection, UA_ByteString *buf)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->Send(buf);
    }
    static UA_StatusCode UaC_recv(UA_Connection *connection, UA_ByteString *response, UA_UInt32 timeout)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->Recv(response, timeout);
    }
    static void UaC_releaseRecvBuffer(UA_Connection *connection, UA_ByteString *buf)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->ReleaseRecvBuffer(buf);
    }
    static void UaC_close(UA_Connection *connection)
    {
        UaConnection *pConn = (UaConnection *)connection;
        return pConn->CloseConn();
    }
};

class ConnectionMapping
{
public:
    class UaConnection *connection;
    UA_Int32 sockfd;
};

class UaSocketNetLayer : public UaNetLayer
{
public:
    /* open sockets and connections */
    fd_set fdset;
    UA_Socket serversockfd;
    ConnectionMapping *m_ConnectMappings;
    size_t m_ConnectMappingsSize;
public:
    int SetFDSet();
    UA_StatusCode AddConnection(UA_Int32 newsockfd);
    static void FreeConnectionCallback(UA_Server *server, void *ptr);
    void Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort);
    UA_Int32 GetJobs(UA_Job **jobs, UA_UInt16 timeout);
    UA_StatusCode Start(UA_Logger UaLogger);
    UA_Int32 Stop(UA_Job **jobs);
    void Deletemembers();
    static size_t Ua_getJobs(struct UA_ServerNetworkLayer *nl, UA_Job **jobs, UA_UInt16 timeout)
    {
        UaSocketNetLayer *pLayer = (UaSocketNetLayer *)nl;
        return pLayer->GetJobs(jobs, timeout);
    }
    static UA_StatusCode Ua_start(struct UA_ServerNetworkLayer *nl, UA_Logger logger)
    {
        UaSocketNetLayer *pLayer = (UaSocketNetLayer *)nl;
        return pLayer->Start(logger);
    }
    static size_t Ua_stop(struct UA_ServerNetworkLayer *nl, UA_Job **jobs)
    {
        UaSocketNetLayer *pLayer = (UaSocketNetLayer *)nl;
        return pLayer->Stop(jobs);
    }
    static void Ua_deleteMembers(struct UA_ServerNetworkLayer *nl)
    {
        UaSocketNetLayer *pLayer = (UaSocketNetLayer *)nl;
        pLayer->Deletemembers();
    }
};


#endif
