/*
* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
*/
#include "UaNetLayer.h"


/***************************/
/* Server NetworkLayer TCP */
/***************************/

/**
 * For the multithreaded mode, assume a single thread that periodically "gets work" from the network
 * layer. In addition, several worker threads are asynchronously calling into the callbacks of the
 * UA_Connection that holds a single connection.
 *
 * Creating a connection: When "GetWork" encounters a new connection, it creates a UA_Connection
 * with the socket information. This is added to the mappings array that links sockets to
 * UA_Connection structs.
 *
 * Reading data: In "GetWork", we listen on the sockets in the mappings array. If data arrives (or
 * the connection closes), a WorkItem is created that carries the work and a pointer to the
 * connection.
 *
 * Closing a connection: Closing can happen in two ways. Either it is triggered by the server in an
 * asynchronous callback. Or the connection is close by the client and this is detected in
 * "GetWork". The server needs to do some internal cleanups (close attached securechannels, etc.).
 * So even when a closed connection is detected in "GetWork", we trigger the server to close the
 * connection (with a WorkItem) and continue from the callback.
 *
 * - Server calls close-callback: We close the socket, set the connection-state to closed and add
 *   the connection to a linked list from which it is deleted later. The connection cannot be freed
 *   right away since other threads might still be using it.
 *
 * - GetWork: We remove the connection from the mappings array. In the non-multithreaded case, the
 *   connection is freed. For multithreading, we return a workitem that is delayed, i.e. that is
 *   called only after all workitems created before are finished in all threads. This workitems
 *   contains a callback that goes through the linked list of connections to be freed.
 *
 */


UA_StatusCode UaNetLayer::Start(UA_Logger UaLogger)
{
    m_Ua_logger = UaLogger;
    return UA_STATUSCODE_GOOD;
}

UA_Int32 UaNetLayer::Stop(UA_Job **jobs)
{
    return 0;
}

void UaNetLayer::Deletemembers()
{
    UA_ByteString_deleteMembers(&m_SenBuf);
}


UaNetLayer::UaNetLayer()
{

}

UaNetLayer::~UaNetLayer()
{
    free(m_SenBuf.data);
}


void UaNetLayer::Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort)
{
    // ZeroInit()
    memset(this, 0, sizeof(*this));
    m_UaConf = ConConfig;
    m_uListenport = uListenPort;
    char hostname[256];
    hostname[0] = '\0';     // if gethostname() returns error
    gethostname(hostname, sizeof(hostname));
    UA_snprintf(m_sUaUrl, sizeof(m_sUaUrl), "opc.tcp://%s:%d", hostname, m_uListenport);
    discoveryUrl.data = (UA_Byte *)m_sUaUrl;
    discoveryUrl.length = strlen(m_sUaUrl);
    // 2 static Buffers for sen and receive
    m_SenBuf.length = m_UaConf.maxMessageSize;
    m_SenBuf.data = (UA_Byte *)malloc(m_SenBuf.length);
}

