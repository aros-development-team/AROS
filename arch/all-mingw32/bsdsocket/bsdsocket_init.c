/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/symbolsets.h>
#include <proto/alib.h>
#include <proto/hostlib.h>

#include "bsdsocket_intern.h"

static const char *ws_functions[] = {
    "WSAGetLastError",
    "inet_addr",
    "inet_ntoa",
    "getprotobyname",
    "socket",
    "closesocket",
    "ioctlsocket",
    "setsockopt",
    "recvfrom",
    "sendto",
    "WSAEventSelect",
    NULL
};

static const char *res_functions[] = {
    "sock_init",
    "sock_shutdown",
    NULL
};

static int bsdsocket_Init(struct bsdsocketBase *SocketBase)
{
    APTR HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
	return FALSE;
    SocketBase->HostLibBase = HostLibBase;

    SocketBase->winsock = HostLib_Open("Ws2_32.dll", NULL);
    if (!SocketBase->winsock)
	return FALSE;

    SocketBase->resolver = HostLib_Open("Libs\\Host\\bsdsocket.dll", NULL);
    if (!SocketBase->resolver)
	return FALSE;

    SocketBase->WSIFace = (struct WinSockInterface *)HostLib_GetInterface(SocketBase->winsock, ws_functions, NULL);
    if (!SocketBase->WSIFace)
    {
	D(bug("[socket] Failed to obtain winsock interface\n"));
	return FALSE;
    }

    SocketBase->ResIFace = (struct HostSocketInterface *)HostLib_GetInterface(SocketBase->resolver, res_functions, NULL);
    if (!SocketBase->ResIFace)
	return FALSE;

    NewList((struct List *)&SocketBase->socks);

    Forbid();
    SocketBase->ctl = SocketBase->ResIFace->sock_init();
    Permit();

    if (!SocketBase->ctl)
	return FALSE;

    return TRUE;
}

static int bsdsocket_Cleanup(struct bsdsocketBase *SocketBase)
{
    APTR HostLibBase = SocketBase->HostLibBase;

    D(bug("[socket] Cleanup, HostLibBase is 0x%p\n", HostLibBase));
    if (!HostLibBase)
	return TRUE;

    if (SocketBase->ResIFace)
    {
	if (SocketBase->ctl)
	{
	    int res;

	    Forbid();
	    res = SocketBase->ResIFace->sock_shutdown(SocketBase->ctl);
	    Permit();
	    
	    if (res)
		return FALSE;
	}
    }
	
    if (SocketBase->WSIFace)
	HostLib_DropInterface((void **)SocketBase->WSIFace);

    if (SocketBase->winsock)
	HostLib_Close(SocketBase->winsock, NULL);

    return TRUE;
}

ADD2INITLIB(bsdsocket_Init, 0);
ADD2EXPUNGELIB(bsdsocket_Cleanup, 0);
