/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */

#ifndef UaNetLayer__h
#define UaNetLayer__h
#include "UaSystem.h"
#include "ua_server.h"

class UaNetLayer : public UA_ServerNetworkLayer
{
public:
    /* config */
    UA_Logger m_Ua_logger;
    UA_UInt32 m_uListenport;
    UA_ConnectionConfig m_UaConf; 
    UA_ByteString m_SenBuf; // message buffer that is reused
    char m_sUaUrl[250];
public:
    UaNetLayer();
    UaNetLayer(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort)
    {
        Init(ConConfig, uListenPort);
    }
    ~UaNetLayer();
    void Init(UA_ConnectionConfig ConConfig, UA_UInt32 uListenPort);
    // 
    // UA_ServerNetworkLayer interface
    //
    //UA_Int32 GetJobs(UA_Job **jobs, UA_UInt16 timeout);
    UA_StatusCode Start(UA_Logger pUaLogger);
    UA_Int32 Stop(UA_Job **jobs);
    void Deletemembers();
};


#endif
