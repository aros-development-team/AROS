#include <aros/symbolsets.h>
#include <proto/hostlib.h>

#include "bsdsocket_intern.h"

static const char *ws_functions[] = {
    "WSAStartup",
    "WSACleanup",
    "WSAGetLastError",
    "inet_addr",
    "getprotobyname",
    NULL
};

static int bsdsocket_Init(struct bsdsocketBase *SocketBase)
{
    struct WSAData wsdata;
    APTR HostLibBase = OpenResource("hostlib.resource");

    if (!HostLibBase)
	return FALSE;
    SocketBase->HostLibBase = HostLibBase;

    SocketBase->winsock = HostLib_Open("Ws2_32.dll", NULL);
    if (!SocketBase->winsock)
	return FALSE;

    SocketBase->WSIFace = (struct WinSockInterface *)HostLib_GetInterface(SocketBase->winsock, ws_functions, NULL);
    if (!SocketBase->WSIFace)
    {
	D(bug("[socket] Failed to obtain winsock interface\n"));
	return FALSE;
    }

    Forbid();
    SocketBase->state = WSAStartup(0x0002, &wsdata);
    Permit();

    D(bug("[socket] WSAStartup reply: %u\n", SocketBase->state));
    D(bug("[socket] Using WinSock v%u.%u (%s)\n", wsdata.wVersion & 0x00FF, wsdata.wVersion >> 8, wsdata.szDescription));
    D(bug("[socket] Status: %s\n", wsdata.szSystemStatus));
    if (SocketBase->state)
    {
	/* TODO: report failure using Intuition requester */
	return FALSE;
    }

    return TRUE;
}

static int bsdsocket_Cleanup(struct bsdsocketBase *SocketBase)
{
    APTR HostLibBase = SocketBase->HostLibBase;

    D(bug("[socket] Cleanup, HostLibBase is 0x%p\n", HostLibBase));
    if (!HostLibBase)
	return TRUE;

    if (SocketBase->WSIFace)
    {
	if (!SocketBase->state)
	{
	    int res;

	    Forbid();
	    res = WSACleanup();
	    Permit();

	    if (res)
		return FALSE;
	}
	HostLib_DropInterface((void **)SocketBase->WSIFace);
    }

    if (SocketBase->winsock)
	HostLib_Close(SocketBase->winsock, NULL);

    return TRUE;
}

ADD2INITLIB(bsdsocket_Init, 0);
ADD2EXPUNGELIB(bsdsocket_Cleanup, 0);
