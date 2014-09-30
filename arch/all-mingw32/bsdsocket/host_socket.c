/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/irq.h>

#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

#include "host_socket.h"

#define D(x) x

static struct SocketController ctl;

static DWORD WINAPI ResolverThread(struct SocketController *ctl)
{
    D(printf("[Resolver] Thread started\n"));

    for (;;)
    {
	WaitForSingleObject(ctl->ResolverEvent, INFINITE);
    
	switch(ctl->Command)
	{
	case SOCK_CMD_SHUTDOWN:
	    D(printf("[Resolver] Shutdown requested\n"));

	    CloseHandle(ctl->ResolverEvent);
	    return 0;

	}
	ctl->Command = 0;
	KrnCauseIRQ(ctl->ResolverIRQ);
    }
}

struct SocketController * __declspec(dllexport) __aros sock_init(void)
{
    struct WSAData wsdata;
    HANDLE thread;
    int irq;
    int state = WSAStartup(0x0002, &wsdata);

    D(printf("[sock_init] WSAStartup reply: %u\n", state));
    D(printf("[sock_init] Using WinSock v%u.%u (%s)\n", wsdata.wVersion & 0x00FF, wsdata.wVersion >> 8, wsdata.szDescription));
    D(printf("[sock_init] Status: %s\n", wsdata.szSystemStatus));
    if (state)
	return NULL;

    irq = KrnAllocIRQ();
    if (irq != -1)
    {
	ctl.SocketIRQ = irq;

	irq = KrnAllocIRQ();
	if (irq != -1)
	{
	    ctl.ResolverIRQ = irq;

	    ctl.ResolverEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	    if (ctl.ResolverEvent)
	    {
		DWORD id;

		thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ResolverThread, &ctl, 0, &id);
		if (thread)
		{
		    CloseHandle(thread);

		    ctl.SocketEvent = KrnGetIRQObject(ctl.SocketIRQ);
		    return &ctl;
		}
		CloseHandle(ctl.ResolverEvent);
	    }
	}
	KrnFreeIRQ(ctl.SocketIRQ);
    }

    return NULL;
}

int __declspec(dllexport) __aros sock_shutdown(struct SocketController *ctl)
{
    int res = WSACleanup();
    
    if (res)
	return res;

    ctl->Command = SOCK_CMD_SHUTDOWN;
    SetEvent(ctl->ResolverEvent);

    KrnFreeIRQ(ctl->ResolverIRQ);
    KrnFreeIRQ(ctl->SocketIRQ);

    return 0;
}
