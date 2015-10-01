/*
* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
*/
#include "UaSystem.h"

UA_StatusCode socket_set_nonblocking(UA_Socket sockfd)
{
#ifdef _WIN32
    u_long iMode = 1;
    if (ioctlsocket(sockfd, FIONBIO, &iMode) != NO_ERROR)
        return UA_STATUSCODE_BADINTERNALERROR;
#else
    int opts = fcntl(sockfd, F_GETFL);
    if(opts < 0 || fcntl(sockfd, F_SETFL, opts|O_NONBLOCK) < 0)
        return UA_STATUSCODE_BADINTERNALERROR;
#endif
    return UA_STATUSCODE_GOOD;
}


/*
* sprintf mit Bufferlänge
* Rückgabestring immer '\0' abgeschlossen
* Rückgabe:
* strlen(buf)
*/

int UA_vsnprintf(char *buf, size_t bufsize, const char *format, va_list argp)
{
    int cnt;
    cnt = _vsnprintf(buf, bufsize - 1, format, argp);
    if (cnt < 0)
    {
        cnt = bufsize - 1;
    }
    buf[cnt] = '\0';
    return cnt;
}

int UA_snprintf(char * buf, size_t bufsize, const char * format, ...)
{
    int     cnt;
    va_list argp;
    va_start(argp, format);
    cnt = UA_vsnprintf(buf, bufsize, format, argp);
    va_end(argp);
    return cnt;
}
