/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/client_subscriptions.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/util.h>

#include <signal.h>

#ifdef _MSC_VER
#pragma warning(                                                                         \
    disable : 4996)  // warning C4996: 'UA_Client_Subscriptions_addMonitoredEvent': was
                     // declared deprecated
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

static UA_Boolean running = true;
static void
stopHandler(int sig) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "received ctrl-c");
    running = false;
}

const char *m_sEventBrNameTab[] = {
    "Time",        // OpcUa_BrowseName_
    "SourceName",  // OpcUa_BrowseName_SourceName
    "Message",     // OpcUa_BrowseName_
    "Severity",    // OpcUa_BrowseName_
    "EventType",   // OpcUa_BrowseName_EventType
    //"EventId",     // OpcUa_BrowseName_EventId
    //"SourceNode",  // OpcUa_BrowseName_SourceNode
};

const size_t nSelectClauses = _countof(m_sEventBrNameTab);

#ifdef UA_ENABLE_SUBSCRIPTIONS

static bool
SetSimpleBasicEvent(UA_SimpleAttributeOperand *pSA, const char *sBrowsePath) {
    UA_SimpleAttributeOperand_init(pSA);
    pSA->typeDefinitionId = UA_NODEID_NUMERIC(0, UA_NS0ID_BASEEVENTTYPE);
    pSA->browsePathSize = 1;
    pSA->browsePath = (UA_QualifiedName *)UA_Array_new(pSA->browsePathSize,
                                                       &UA_TYPES[UA_TYPES_QUALIFIEDNAME]);
    if(pSA->browsePath == NULL)
        return false;
    pSA->attributeId = UA_ATTRIBUTEID_VALUE;
    pSA->browsePath[0] = UA_QUALIFIEDNAME_ALLOC(0, sBrowsePath);
    return true;
}

char
XtoA(unsigned uHex) {
    uHex &= 0x0f;
    if(uHex >= 0 && uHex <= 9)
        return uHex + '0';
    return uHex + 'a';
}

void
UA_BytesToHexString(const UA_Byte *pBytes, unsigned nlen, UA_String *out) {
    int n = nlen * 3 - 1;
    out->data = UA_malloc(n);
    out->length = n;

    const UA_Byte *pByte = pBytes;
    UA_Byte *pOut = out->data;
    for(size_t nn = 0; true; pByte++) {
        *pOut++ = XtoA(*pByte);
        *pOut++ = XtoA(*pByte >> 4);
        if(++nn >= nlen)
            break;
        *pOut++ = ' ';
    }
}

static void
handler_events(UA_Client *client, UA_UInt32 subId, void *subContext, UA_UInt32 monId,
               void *monContext, size_t nEventFields, UA_Variant *eventFields) {
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "*** Notification ***");

    /* The context should point to the monId on the stack */
    UA_assert(*(UA_UInt32 *)monContext == monId);
    UA_Variant *pEvField = eventFields;
    for(size_t i = 0; i < nEventFields; i++, pEvField++) {
        const char *sName = m_sEventBrNameTab[i];
        if(!UA_Variant_isScalar(pEvField)) {
            ;
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_UINT16]) {
            UA_UInt16 severity = *(UA_UInt16 *)pEvField->data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s: %u", sName,
                        severity);
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_LOCALIZEDTEXT]) {
            UA_LocalizedText *lt = (UA_LocalizedText *)pEvField->data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s: '%.*s'", sName,
                        (int)lt->text.length, lt->text.data);
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_NODEID]) {
            UA_String nodeIdName = UA_STRING_NULL;
            UA_NodeId_print((UA_NodeId *)pEvField->data, &nodeIdName);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s: '%.*s'", sName,
                        (int)nodeIdName.length, nodeIdName.data);
            UA_String_clear(&nodeIdName);
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_STRING]) {
            UA_String *pstr = (UA_String *)pEvField->data;
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s: '%.*s'", sName,
                        (int)pstr->length, pstr->data);
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_BYTESTRING]) {
            const UA_ByteString *pBytes = (UA_ByteString *)pEvField->data;
            int n = pBytes->length * 3;
            UA_String UaValName;
            UA_BytesToHexString(pBytes->data, pBytes->length, &UaValName);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "%s: '%.*s'", sName,
                        (int)UaValName.length, UaValName.data);
            UA_String_clear(&UaValName);
        } else if(pEvField->type == &UA_TYPES[UA_TYPES_DATETIME]) {
            UA_DateTimeStruct dts = UA_DateTime_toStruct(*(UA_DateTime *)pEvField->data);
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "%s: %02u-%02u-%04u %02u:%02u:%02u.%03u", sName, dts.day,
                        dts.month, dts.year, dts.hour, dts.min, dts.sec, dts.milliSec);
        } else {
#ifdef UA_ENABLE_TYPEDESCRIPTION
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle %s: type: '%s'", sName,
                        pEvField->type->typeName);
#else
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                        "Don't know how to handle type, enable UA_ENABLE_TYPEDESCRIPTION "
                        "for typename");
#endif
        }
    }
}

static UA_SimpleAttributeOperand *
setupSelectClauses(void) {
    UA_SimpleAttributeOperand *selectClauses = (UA_SimpleAttributeOperand *)UA_Array_new(
        nSelectClauses, &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
    if(!selectClauses)
        return NULL;

    for(size_t i = 0; i < nSelectClauses; i++) {
        SetSimpleBasicEvent(&selectClauses[i], m_sEventBrNameTab[i]);
    }
    return selectClauses;
}

#endif

int
main(int argc, char *argv[]) {
    signal(SIGINT, stopHandler);
    signal(SIGTERM, stopHandler);

    if(argc < 2) {
        printf("Usage: tutorial_client_events <opc.tcp://server-url>\n");
        return EXIT_SUCCESS;
    }

    UA_Client *client = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(client));

    /* opc.tcp://uademo.prosysopc.com:53530/OPCUA/SimulationServer */
    /* opc.tcp://opcua.demo-this.com:51210/UA/SampleServer */
    UA_StatusCode retval = UA_Client_connect(client, argv[1]);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Could not connect");
        UA_Client_delete(client);
        return EXIT_SUCCESS;
    }

#ifdef UA_ENABLE_SUBSCRIPTIONS
    /* Create a subscription */
    UA_CreateSubscriptionRequest request = UA_CreateSubscriptionRequest_default();
    UA_CreateSubscriptionResponse response =
        UA_Client_Subscriptions_create(client, request, NULL, NULL, NULL);
    if(response.responseHeader.serviceResult != UA_STATUSCODE_GOOD) {
        UA_Client_disconnect(client);
        UA_Client_delete(client);
        return EXIT_FAILURE;
    }
    UA_UInt32 subId = response.subscriptionId;
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "Create subscription succeeded, id %u", subId);

    /* Add a MonitoredItem */
    UA_MonitoredItemCreateRequest item;
    UA_MonitoredItemCreateRequest_init(&item);
    item.itemToMonitor.nodeId = UA_NODEID_NUMERIC(0, 2253);  // Root->Objects->Server
    item.itemToMonitor.attributeId = UA_ATTRIBUTEID_EVENTNOTIFIER;
    item.monitoringMode = UA_MONITORINGMODE_REPORTING;

    UA_EventFilter filter;
    UA_EventFilter_init(&filter);
    filter.selectClauses = setupSelectClauses();
    filter.selectClausesSize = nSelectClauses;

    item.requestedParameters.filter.encoding = UA_EXTENSIONOBJECT_DECODED;
    item.requestedParameters.filter.content.decoded.data = &filter;
    item.requestedParameters.filter.content.decoded.type =
        &UA_TYPES[UA_TYPES_EVENTFILTER];

    UA_UInt32 monId = 0;

    UA_MonitoredItemCreateResult result = UA_Client_MonitoredItems_createEvent(
        client, subId, UA_TIMESTAMPSTORETURN_BOTH, item, &monId, handler_events, NULL);

    if(result.statusCode != UA_STATUSCODE_GOOD) {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Could not add the MonitoredItem with %s",
                    UA_StatusCode_name(retval));
        goto cleanup;
    } else {
        UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                    "Monitoring 'Root->Objects->Server', id %u", response.subscriptionId);
    }

    monId = result.monitoredItemId;

    while(running)
        retval = UA_Client_run_iterate(client, 100);

    /* Delete the subscription */
cleanup:
    UA_MonitoredItemCreateResult_clear(&result);
    UA_Client_Subscriptions_deleteSingle(client, response.subscriptionId);
    UA_Array_delete(filter.selectClauses, nSelectClauses,
                    &UA_TYPES[UA_TYPES_SIMPLEATTRIBUTEOPERAND]);
#endif

    UA_Client_disconnect(client);
    UA_Client_delete(client);
    return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
