/*
* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
*/
#include "UaSystem.h"
#include "UaSocketNetLayer.h"

#define MAXBACKLOG 100

void UaSocketNetLayer::Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort)
{
    UaNetLayer::Init(ConConfig, uListenPort);
    m_ConnectMappingsSize = 0;
    m_ConnectMappings = NULL;
    //handle = this;
    start = Ua_start;
    getJobs = Ua_getJobs; // ServerNetMfcTCP_getJobs;
    stop = Ua_stop;
    deleteMembers = Ua_deleteMembers;
}

/* after every select, we need to reset the sockets we want to listen on */
int UaSocketNetLayer::SetFDSet()
{
    int nFds = 1;
    FD_ZERO(&fdset);
    FD_SET(serversockfd, &fdset);
    ConnectionMapping *pMapping;
    size_t i;
    for (i = 0, pMapping = m_ConnectMappings; i < m_ConnectMappingsSize; i++, pMapping++)
    {
        FD_SET((UA_Socket)pMapping->sockfd, &fdset);
        nFds++;
    }
    return nFds;
}

UA_StatusCode UaSocketNetLayer::Start(UA_Logger UaLogger)
{
    UaNetLayer::Start(UaLogger);
#ifdef _WIN32
    if ((serversockfd = socket(PF_INET, SOCK_STREAM, 0)) == (UA_Int32)INVALID_SOCKET) {
        UA_LOG_WARNING((m_Ua_logger), UA_LOGCATEGORY_COMMUNICATION, "Error opening socket, code: %d",
            WSAGetLastError());
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#else
    if ((serversockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        UA_LOG_WARNING((*logger), UA_LOGCATEGORY_COMMUNICATION, "Error opening socket");
        return UA_STATUSCODE_BADINTERNALERROR;
    }
#endif
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons((u_short)m_uListenport);
    int optval = 1;
    if (setsockopt(serversockfd, SOL_SOCKET,
        SO_REUSEADDR, (const char *)&optval,
        sizeof(optval)) == -1) {
        UA_LOG_WARNING((m_Ua_logger), UA_LOGCATEGORY_COMMUNICATION, "Error during setting of socket options");
        CLOSESOCKET(serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    if (bind(serversockfd, (const struct sockaddr *)&serv_addr,
        sizeof(serv_addr)) < 0) {
        UA_LOG_WARNING((m_Ua_logger), UA_LOGCATEGORY_COMMUNICATION, "Error during socket binding");
        CLOSESOCKET(serversockfd);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    socket_set_nonblocking(serversockfd);
    listen(serversockfd, MAXBACKLOG);
    UA_LOG_INFO((m_Ua_logger), UA_LOGCATEGORY_COMMUNICATION, "Listening on %.*s",
        discoveryUrl.length, discoveryUrl.data);
    return UA_STATUSCODE_GOOD;
}

UA_Int32 UaSocketNetLayer::Stop(UA_Job **jobs)
{
    UaNetLayer::Stop(jobs);
    UA_Job *items = (UA_Job *)malloc(sizeof(UA_Job) * m_ConnectMappingsSize * 2);
    if (!items)
        return 0;
    ConnectionMapping *pMapping;
    size_t i;
    size_t n = 0;
    for (i = 0, pMapping = m_ConnectMappings; i < m_ConnectMappingsSize; i++, pMapping++)
    {
        pMapping->connection->CloseSock();
        items[n].type = UA_Job::UA_JOBTYPE_DETACHCONNECTION;
        items[n].job.closeConnection = pMapping->connection;
        n++;
        items[n].type = UA_Job::UA_JOBTYPE_METHODCALL_DELAYED;
        items[n].job.methodCall.method = FreeConnectionCallback;
        items[n].job.methodCall.data = pMapping->connection;
        n++;
    }
    *jobs = items;
    return n;
}

/* run only when the server is stopped */
void UaSocketNetLayer::Deletemembers()
{
    UaNetLayer::Deletemembers();
    ConnectionMapping *pMapping;
    size_t i;
    for (i = 0, pMapping = m_ConnectMappings; i < m_ConnectMappingsSize; i++, pMapping++)
        free(pMapping->connection);
    free(m_ConnectMappings);
}

void UaSocketNetLayer::FreeConnectionCallback(UA_Server *server, void *ptr)
{
    UA_Connection_deleteMembers((UaConnection*)ptr);
    free(ptr);
}

/* call only from the single networking thread */
UA_StatusCode UaSocketNetLayer::AddConnection(UA_Int32 newsockfd)
{
    UaConnection *c = new UaConnection;
    if (!c)
        return UA_STATUSCODE_BADINTERNALERROR;
    UA_Connection_init(c);
    c->sockfd = newsockfd;
    c->handle = this;
    c->localConf = m_UaConf;
    ConnectionMapping *nm = (ConnectionMapping *)realloc(m_ConnectMappings, sizeof(ConnectionMapping)*(m_ConnectMappingsSize + 1));
    if (!nm) {
        free(c);
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    m_ConnectMappings = nm;
    m_ConnectMappings[m_ConnectMappingsSize] = { c, newsockfd };
    m_ConnectMappingsSize++;
    return UA_STATUSCODE_GOOD;
}

UA_Int32 UaSocketNetLayer::GetJobs(UA_Job **jobs, UA_UInt16 timeout)
{
    int nFds = SetFDSet();
    struct timeval tmptv = { 0, timeout };
    UA_Int32 resultsize;
    while (true)
    {
        resultsize = select(nFds, &fdset, NULL, NULL, &tmptv);
        if (resultsize < 0)
        {
            if (errno == EINTR)
                continue;
            *jobs = NULL;
            return resultsize;
        }
        break;
    }

    /* accept new connections (can only be a single one) */
    if (FD_ISSET(serversockfd, &fdset))
    {
        resultsize--;
        struct sockaddr_in cli_addr;
        socklen_t cli_len = sizeof(cli_addr);
        int newsockfd = accept(serversockfd, (struct sockaddr *) &cli_addr, &cli_len);
        int i = 1;
        setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, (char *)&i, sizeof(i));
        if (newsockfd >= 0)
        {
            socket_set_nonblocking(newsockfd);
            AddConnection(newsockfd);
        }
    }

    /* alloc enough space for a cleanup-connection and free-connection job per resulted socket */
    if (resultsize == 0)
        return 0;
    UA_Job *js = (UA_Job *)malloc(sizeof(UA_Job) * resultsize * 2);
    if (js == NULL)
        return 0;

    /* read from established sockets */
    UA_Int32 j = 0;
    UA_ByteString buf = UA_BYTESTRING_NULL;
    ConnectionMapping *pMapping;
    size_t i;
    for (i = 0, pMapping = m_ConnectMappings; i < m_ConnectMappingsSize; i++, pMapping++)
    {
        if (!(FD_ISSET(pMapping->sockfd, &fdset)))
            continue;
        if (pMapping->connection->Recv(&buf, 0) == UA_STATUSCODE_GOOD)
        {
            if (!buf.data)
                continue;
            js[j].type = UA_Job::UA_JOBTYPE_BINARYMESSAGE_NETWORKLAYER;
            js[j].job.binaryMessage.message = buf;
            js[j].job.binaryMessage.connection = pMapping->connection;
        }
        else
        {
            UaConnection *c = pMapping->connection;
            /* the socket is already closed */
            js[j].type = UA_Job::UA_JOBTYPE_DETACHCONNECTION;
            js[j].job.closeConnection = pMapping->connection;
            m_ConnectMappings[i] = m_ConnectMappings[m_ConnectMappingsSize - 1];
            m_ConnectMappingsSize--;
            j++;
            i--; // iterate over the same index again
            js[j].type = UA_Job::UA_JOBTYPE_METHODCALL_DELAYED;
            js[j].job.methodCall.method = FreeConnectionCallback;
            js[j].job.methodCall.data = c;
        }
        j++;
    }

    if (j == 0)
    {
        free(js);
        js = NULL;
    }

    *jobs = js;
    return j;
}

/****************************/
/* Generic Socket Functions */
/****************************/
void UaConnection::CloseSock()
{
    state = UA_CONNECTION_CLOSED;
    shutdown(sockfd, 2);
    CLOSESOCKET(sockfd);
}

UA_StatusCode UaConnection::Send(UA_ByteString *buf)
{
    UA_Int32 nWritten = 0;
    while (nWritten < buf->length)
    {
        UA_Int32 n = 0;
        do {
#ifdef _WIN32
            n = ::send(sockfd, (const char*)buf->data, buf->length, 0);
            if (n == SOCKET_ERROR)
            {
                int err = WSAGetLastError();
                if (err != WSAEINTR || err != WSAEWOULDBLOCK)
                {
                    close(this);
                    CloseSock();
                    return UA_STATUSCODE_BADCONNECTIONCLOSED;
                }
            }
#else
            n = ::send(sockfd, (const char*)buf->data, buflen, MSG_NOSIGNAL);
            if (n == -1L && errno != EINTR && errno != EAGAIN)
            {
                socket_close(connection);
                return UA_STATUSCODE_BADCONNECTIONCLOSED;
            }
#endif
        } while (n == -1L);
        nWritten += n;
    }
#ifdef UA_MULTITHREADING
    UA_ByteString_deleteMembers(buf);
#endif
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode UaConnection::Recv(UA_ByteString *response, UA_UInt32 timeout)
{
    response->data = (UA_Byte *)malloc(localConf.recvBufferSize);
    if (response->data == NULL)
    {
        UA_ByteString_init(response);
        return UA_STATUSCODE_GOOD; /* not enough memory retry */
    }
    struct timeval tmptv = { 0, (long)timeout * 1000 };
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tmptv, sizeof(tmptv)) != 0)
    {
        free(response->data);
        UA_ByteString_init(response);
        CloseSock();
        return UA_STATUSCODE_BADINTERNALERROR;
    }
    int ret = ::recv(sockfd, (char*)response->data, localConf.recvBufferSize, 0);
    UA_StatusCode rc;
    if (ret > 0)
    {
        response->length = ret;
        return UA_STATUSCODE_GOOD;
    }
    if (ret == 0)
    {
        CloseSock();
        rc = UA_STATUSCODE_BADCONNECTIONCLOSED; /* ret == 0 -> server has closed the connection */
    }
    else
    {
        // ret < 0
#ifdef _WIN32
        if (WSAGetLastError() == WSAEINTR || WSAGetLastError() == WSAEWOULDBLOCK)
#else
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
#endif
        {
            rc = UA_STATUSCODE_GOOD; /* retry */
        }
        else
        {
            CloseSock();
            rc = UA_STATUSCODE_BADCONNECTIONCLOSED;
        }
    }
    free(response->data);
    UA_ByteString_init(response);
    return rc;
}

UA_StatusCode UaConnection::GetSendBuffer(UA_Int32 length, UA_ByteString *buf)
{
    if ((UA_UInt32)length > remoteConf.recvBufferSize)
        return UA_STATUSCODE_BADCOMMUNICATIONERROR;
    UaNetLayer *layer = (UaNetLayer *)handle;
    *buf = layer->m_SenBuf;
    return UA_STATUSCODE_GOOD;
}

void UaConnection::ReleaseSendBuffer(UA_ByteString *buf)
{

}

void UaConnection::ReleaseRecvBuffer(UA_ByteString *buf)
{

}


/* callback triggered from the server */
void UaConnection::CloseConn()
{
#ifdef UA_MULTITHREADING
    if (uatomic_xchg(&state, UA_CONNECTION_CLOSED) == UA_CONNECTION_CLOSED)
        return;
#else
    if (state == UA_CONNECTION_CLOSED)
        return;
    state = UA_CONNECTION_CLOSED;
#endif
    shutdown(sockfd, 2); /* only shut down here. this triggers the select, where the socket
                         is closed in the main thread */
}

UaConnection::UaConnection()
{
    handle = this;
    getSendBuffer = UaC_getSendBuffer;
    releaseSendBuffer = UaC_releaseSendBuffer;
    send = UaC_send;
    recv = UaC_recv;
    releaseRecvBuffer = UaC_releaseRecvBuffer;
    close = UaC_close;
}
