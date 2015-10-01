/*
* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
*/

#ifdef __cplusplus
extern "C" 
{
#endif
#include "ua_config.h"
#include "ua_types.h"
#include "ua_types_generated.h"
#ifdef __cplusplus
}
#endif

#if defined WIN32
#include "UaSystem_W32.h"
#elif defined LINUX
#include "UaSystem_Linux.h"
#elif defined __CYGWIN__
#include "UaSystem_Cygwin.h"
#elif defined __QNX__
#include "UaSystem_QNX.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif
    UA_StatusCode socket_set_nonblocking(UA_Socket sockfd);
    //-------------------------------
    //  sprintf mit Bufferlänge
    //  Rückgabestring immer '\0' abgeschlossen
    //  Rückgabe:
    //      strlen(buf)
    //-------------------------------
    int UA_vsnprintf(char *buf, size_t bufsize, const char *format, va_list argp);
    int UA_snprintf(char * buf, size_t bufsize, const char * format, ...);
#ifdef __cplusplus
}
#endif

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif
