/*
 * This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 */
//to compile with single file releases:
// * single-threaded: gcc -std=c99 server.c open62541.c -o server
// * multi-threaded: gcc -std=c99 server.c open62541.c -o server -lurcu-cds -lurcu -lurcu-common -lpthread
#include "UaSocketNetLayer.h"

#ifdef UA_NO_AMALGAMATION
# include <time.h>
# include "ua_types.h"
# include "ua_server.h"
# include "../examples/logger_stdout.h"
#include <WinSock2.h>
#include "ua_server.h"
#include "ua_client.h"
//#include "../examples/networklayer_tcp.h"
#else
# include "open62541.h"
#endif
#include "../src/server/ua_server_internal.h"

#include <signal.h>
#include <errno.h> // errno, EINTR
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
# include <io.h> //access
#else
# include <unistd.h> //access
#endif

#ifdef UA_MULTITHREADING
# ifdef UA_NO_AMALGAMATION
#  ifndef __USE_XOPEN2K
#   define __USE_XOPEN2K
#  endif
# endif
#include <pthread.h>
#endif

#undef UA_LOGLEVEL
#define UA_LOGLEVEL 4000

//// this dose not work in C++ for MSVC
//#undef UA_QUALIFIEDNAME
//#undef UA_LOCALIZEDTEXT
//#undef UA_STRING
//#undef UA_EXPANDEDNODEID_NUMERIC
//
//#define UA_STRING(CHARS)                            {(UA_Int32) strlen(CHARS), (UA_Byte*)CHARS }
//#define UA_QUALIFIEDNAME(NSID, CHARS)               { NSID, {(UA_Int32) strlen(CHARS), (UA_Byte*)CHARS } }
//#define UA_LOCALIZEDTEXT(LOCALE, TEXT)              { UA_STRING(LOCALE), UA_STRING(TEXT) }
//#define UA_EXPANDEDNODEID_NUMERIC(NSID, NUMERICID)  { { NSID,  UA_NODEIDTYPE_NUMERIC, NUMERICID }, { -1, NULL}, 0 }
//
//
//#undef UA_STRING_NULL
//
//const UA_String UA_STRING_NULL =
//{
//    -1, NULL
//};
//
//#undef UA_NODEID_NULL
//
//const UA_NodeId UA_NODEID_NULL =
//{
//    0, UA_NODEIDTYPE_NUMERIC, { 0 }
//};

//
// this works only with char [] strings
// Sample:
// const char m_strLang[] = "en_US";
// const char m_strCurrTime[] = "currenttime";
// const UA_QualifiedName m_dateName = { 1, UA_STRINGCONST(m_strCurrTime) };
// const UA_LocalizedText m_dateNameBrowseName = { UA_STRINGCONST(m_strLang), UA_STRINGCONST(m_strCurrTime) };
#define UA_STRINGCONST(s) {sizeof(s)-1, (UA_Byte*)s }


/****************************/
/* Server-related variables */
/****************************/

UA_Boolean running = 1;
UA_Logger m_Ualogger;

/*************************/
/* Read-only data source */
/*************************/
static UA_StatusCode readTimeData(void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *value)
{
    if (range) 
    {
        value->hasStatus = UA_TRUE;
        value->status = UA_STATUSCODE_BADINDEXRANGEINVALID;
        return UA_STATUSCODE_GOOD;
    }
    UA_DateTime *currentTime = UA_DateTime_new();
    if (!currentTime)
        return UA_STATUSCODE_BADOUTOFMEMORY;
    *currentTime = UA_DateTime_now();
    value->value.type = &UA_TYPES[UA_TYPES_DATETIME];
    value->value.arrayLength = -1;
    value->value.data = currentTime;
    value->hasValue = UA_TRUE;
    if (sourceTimeStamp) 
    {
        value->hasSourceTimestamp = UA_TRUE;
        value->sourceTimestamp = *currentTime;
    }
    return UA_STATUSCODE_GOOD;
}


static void stopHandler(int sign) 
{
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_SERVER, "Received Ctrl-C\n");
    running = 0;
}

static UA_ByteString loadCertificate(void) 
{
    UA_ByteString certificate = UA_STRING_NULL;
    FILE *fp = NULL;
    if ((fp = fopen("server_cert.der", "rb")) == NULL) 
    {
        errno = 0; // we read errno also from the tcp layer...
        return certificate;
    }

    fseek(fp, 0, SEEK_END);
    certificate.length = ftell(fp);
    certificate.data = (UA_Byte *)(certificate.length*sizeof(UA_Byte));
    if (!certificate.data)
    {
        fclose(fp);
        return certificate;
    }

    fseek(fp, 0, SEEK_SET);
    if (fread(certificate.data, sizeof(UA_Byte), certificate.length, fp) < (size_t)certificate.length)
        UA_ByteString_deleteMembers(&certificate); // error reading the cert
    fclose(fp);
    return certificate;
}

UA_StatusCode nodeIter(UA_NodeId childId, UA_Boolean isInverse, UA_NodeId referenceTypeId, void *handle) 
{
    printf("References ns=%d;i=%d using i=%d ", childId.namespaceIndex, childId.identifier.numeric, referenceTypeId.identifier.numeric);
    if (isInverse == UA_TRUE) 
    {
        printf(" (inverse)");
    }
    printf("\n");
    return UA_STATUSCODE_GOOD;
}

const char m_strLang[] = "en_US";
const char m_strCurrTime[] = "currenttime";
const char m_strCounter[] = "counter";


const UA_QualifiedName m_dateName = 
{
    1, UA_STRINGCONST(m_strCurrTime)
};
const UA_LocalizedText m_dateNameBrowseName =
{
    UA_STRINGCONST(m_strLang), UA_STRINGCONST(m_strCurrTime)
};

const UA_QualifiedName m_CounterName =
{
    1, UA_STRINGCONST(m_strCounter)
};

const UA_LocalizedText m_CounterBrowseName =
{
    UA_STRINGCONST(m_strLang), UA_STRINGCONST(m_strCounter)
};


const UA_LocalizedText m_EmptyLocal_en_Us =
{
    UA_STRINGCONST(m_strLang), { 0, (UA_Byte *)"" }
};

UA_Double m_dblCounter = 42.3;

static UA_StatusCode readCounter(void *handle, UA_NodeId nodeid, UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue) 
{
    dataValue->hasValue = UA_TRUE;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)handle, &UA_TYPES[UA_TYPES_DOUBLE]);
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "Node read %.*s", nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "read value %g", *(UA_Double*)handle);
    m_dblCounter += 3.2;
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode writeCounter(void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range)
{
    if (UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_DOUBLE] && data->data)
    {
        *(UA_Double*)handle = *(UA_Double*)data->data;
    }
    //note that this is only possible if the identifier is a string - but we are sure to have one here
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "Node written %.*s", nodeid.identifier.string.length, nodeid.identifier.string.data);
    UA_LOG_INFO(m_Ualogger, UA_LOGCATEGORY_USERLAND, "written value %g", *(UA_Double*)handle);
    return UA_STATUSCODE_GOOD;
}

#ifdef UaSocketNetLayer__h
UaSocketNetLayer m_NetLayer;
#endif

int main(int argc, char** argv) 
{
    signal(SIGINT, stopHandler); /* catches ctrl-c */

    UA_Server *server = UA_Server_new(UA_ServerConfig_standard);
    m_Ualogger = Logger_Stdout_new();
    UA_Server_setLogger(server, m_Ualogger);
    UA_ByteString certificate = loadCertificate();
    UA_Server_setServerCertificate(server, certificate);
    UA_ByteString_deleteMembers(&certificate);
#ifdef _WIN32
    {
        WORD wVersionRequested;
        WSADATA wsaData;
        wVersionRequested = MAKEWORD(2, 2);
        WSAStartup(wVersionRequested, &wsaData);

    }
#endif
#ifdef UaSocketNetLayer__h
    m_NetLayer.Init(UA_ConnectionConfig_standard, 16664);
    UA_Server_addNetworkLayer(server, &m_NetLayer);
#else
    UA_Server_addNetworkLayer(server, ServerNetworkLayerTCP_new(UA_ConnectionConfig_standard, 16664));
#endif

    //// add node with the datetime data source
    //UA_NodeId nodeId_currentTime;
    //UA_DataSource dateDataSourceDate = { NULL, readTimeData, NULL };
    //UA_Server_addDataSourceVariableNode(server, UA_NODEID_NULL, m_dateName, m_dateNameBrowseName, m_dateNameBrowseName, 0, 0,
    //    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
    //    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
    //    dateDataSourceDate, &nodeId_currentTime);
    //UA_NodeId myCounterNodeId = UA_NODEID_STRING(0, "the.counter"); /* UA_NODEID_NULL would assign a random free nodeid */
    //UA_DataSource dateDataSourceCounter = { &m_dblCounter, readCounter, writeCounter };
    //UA_Server_addDataSourceVariableNode(server, myCounterNodeId, m_CounterName, m_CounterBrowseName, m_CounterBrowseName, 0, 0,
    //    UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
    //    UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
    //    dateDataSourceCounter, NULL);

    //// Get and reattach the datasource
    //UA_DataSource *dataSourceCopy = NULL;
    //UA_Server_getAttribute_DataSource(server, nodeId_currentTime, &dataSourceCopy);
    //if (dataSourceCopy == NULL)
    //    UA_LOG_WARNING(m_Ualogger, UA_LOGCATEGORY_USERLAND, "The returned dataSource is invalid");
    //else if (dataSourceCopy->read != dateDataSourceDate.read)
    //    UA_LOG_WARNING(m_Ualogger, UA_LOGCATEGORY_USERLAND, "The returned dataSource is not the same as we set?");
    //else
    //    UA_Server_setAttribute_DataSource(server, nodeId_currentTime, *dataSourceCopy);
    //free(dataSourceCopy);

//    // add a static variable node to the adresspace
//    UA_Variant *myIntegerVariant = UA_Variant_new();
//    UA_Int32 myInteger = 42;
//    UA_Variant_setScalarCopy(myIntegerVariant, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
//    const UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
//    const UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
//    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
//    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
//    UA_Server_addVariableNode(server, myIntegerNodeId, myIntegerName, 
//        UA_LOCALIZEDTEXT("en_US", "the answer"), 
//        UA_LOCALIZEDTEXT("en_US", "the answer"), 
//        0, 0,
//        parentNodeId, parentReferenceNodeId, myIntegerVariant, NULL);
//    /**************/
//    /* Demo Nodes */
//    /**************/
//
//#define DEMOID 50000
//    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, DEMOID), UA_QUALIFIEDNAME(1, "Demo"), UA_LOCALIZEDTEXT("en_US", "Demo"),
//        UA_LOCALIZEDTEXT("en_US", "Demo"), 0, 0, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
//        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), NULL);
//
//#define SCALARID 50001
//    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, SCALARID), UA_QUALIFIEDNAME(1, "Scalar"), UA_LOCALIZEDTEXT("en_US", "Demo"),
//        UA_LOCALIZEDTEXT("en_US", "Demo"), 0, 0, UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
//        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), NULL);
//
//#define ARRAYID 50002
//    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, ARRAYID), UA_QUALIFIEDNAME(1, "Array"), UA_LOCALIZEDTEXT("en_US", "Demo"),
//        UA_LOCALIZEDTEXT("en_US", "Demo"), 0, 0, UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
//        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), NULL);
//
//#define MATRIXID 50003
//    UA_Server_addObjectNode(server, UA_NODEID_NUMERIC(1, MATRIXID), UA_QUALIFIEDNAME(1, "Matrix"), UA_LOCALIZEDTEXT("en_US", "Demo"),
//        UA_LOCALIZEDTEXT("en_US", "Demo"), 0, 0, UA_NODEID_NUMERIC(1, DEMOID), UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
//        UA_EXPANDEDNODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE), NULL);
//
//    UA_UInt32 id = 51000; //running id in namespace 0
//    for (UA_UInt32 type = 0; UA_IS_BUILTIN(type); type++) 
//    {
//        if (type == UA_TYPES_VARIANT || type == UA_TYPES_DIAGNOSTICINFO)
//            continue;
//        //add a scalar node for every built-in type
//        void *value = UA_new(&UA_TYPES[type]);
//        UA_Variant *variant = UA_Variant_new();
//        UA_Variant_setScalar(variant, value, &UA_TYPES[type]);
//        char name[15];
//        sprintf(name, "%02d", type);
//        UA_QualifiedName qualifiedName = UA_QUALIFIEDNAME(1, name);
//        UA_Server_addVariableNode(server, 
//            UA_NODEID_NUMERIC(1, ++id), 
//            qualifiedName, 
//            m_EmptyLocal_en_Us,
//            m_EmptyLocal_en_Us, 0, 0,
//            UA_NODEID_NUMERIC(1, SCALARID), 
//            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), 
//            variant, NULL);
//
//        //add an array node for every built-in type
//        UA_Variant *arrayvar = UA_Variant_new();
//        UA_Variant_setArray(arrayvar, UA_Array_new(&UA_TYPES[type], 10), 10, &UA_TYPES[type]);
//        UA_Server_addVariableNode(server, 
//            UA_NODEID_NUMERIC(1, ++id), 
//            qualifiedName, 
//            m_EmptyLocal_en_Us,
//            m_EmptyLocal_en_Us, 0, 0,
//            UA_NODEID_NUMERIC(1, ARRAYID), 
//            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), 
//            arrayvar, NULL);
//
//        //add an matrix node for every built-in type
//        arrayvar = UA_Variant_new();
//        void* myMultiArray = UA_Array_new(&UA_TYPES[type], 9);
//        arrayvar->arrayDimensions = (UA_Int32 *)UA_Array_new(&UA_TYPES[UA_TYPES_INT32], 2);
//        arrayvar->arrayDimensions[0] = 3;
//        arrayvar->arrayDimensions[1] = 3;
//        arrayvar->arrayDimensionsSize = 2;
//        arrayvar->arrayLength = 9;
//        arrayvar->data = myMultiArray;
//        arrayvar->type = &UA_TYPES[type];
//        UA_Server_addVariableNode(server, 
//            UA_NODEID_NUMERIC(1, ++id), 
//            qualifiedName, 
//            m_EmptyLocal_en_Us,
//            m_EmptyLocal_en_Us, 0, 0,
//            UA_NODEID_NUMERIC(1, MATRIXID), 
//            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES), 
//            arrayvar, NULL);
//    }
//
//    // Example for iterating over all nodes referenced by "Objects":
//    printf("Nodes connected to 'Objects':\n=============================\n");
//    UA_Server_forEachChildNodeCall(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), nodeIter, NULL);
//
//    // Some easy localization
//    UA_LocalizedText objectsName = UA_LOCALIZEDTEXT("de_DE", "Objekte");
//    UA_Server_setAttribute_displayName(server, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), &objectsName);

    //start server
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
//  retval = UA_Server_run(server, 1, &running); //blocks until running=false
    {
        if (UA_STATUSCODE_GOOD == UA_Server_run_startup(server, 1, &running))
        {
            while (running) 
            {
                UA_Server_run_mainloop(server, &running);
			}
        }
        UA_Server_run_shutdown(server, 1);
        retval =  UA_STATUSCODE_GOOD;
    }

    //ctrl-c received -> clean up
    // network-layer ist static
    for (size_t i = 0; i < server->networkLayersSize; i++) 
    {
        UA_String_deleteMembers(&server->networkLayers[i]->discoveryUrl);
        server->networkLayers[i]->deleteMembers(server->networkLayers[i]);
        //UA_free(server->networkLayers[i]);
    }
    server->networkLayersSize = 0;
    UA_Server_delete(server);
    return retval;
}
