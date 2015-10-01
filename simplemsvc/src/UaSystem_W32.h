/*
* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
* See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
*/
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h> // memset
#include <errno.h>

# include <malloc.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# define CLOSESOCKET(S) closesocket(S)
#pragma warning (disable: 4996)
typedef SOCKET UA_Socket;
