#ifndef BSDSOCKET_INTERN_H
#define BSDSOCKET_INTERN_H

/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal definitions for bsdsocket.library
    Lang: English
*/

#include <aros/debug.h>
#include <aros/libcall.h>
#include <exec/avl.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>

#include <sys/types.h>
#include <netdb.h>

#include "winsock2.h"

struct WinSockInterface
{
    int		      __stdcall (*WSAStartup)(UWORD wVersionRequested, LPWSADATA lpWSAData);
    int		      __stdcall (*WSACleanup)(void);
    int		      __stdcall (*WSAGetLastError)(void);
    ULONG	      __stdcall (*WSinet_addr)(const char* cp);
    struct PROTOENT * __stdcall (*WSgetprotobyname)(const char* name);
};

struct bsdsocketBase
{
    struct Library lib;
    APTR HostLibBase;
    APTR winsock;
    struct WinSockInterface *WSIFace;
    int state;
    struct AVLNode *tasks;
};

#define WSAStartup	 SocketBase->WSIFace->WSAStartup
#define WSACleanup	 SocketBase->WSIFace->WSACleanup
#define WSAGetLastError	 SocketBase->WSIFace->WSAGetLastError
#define WSinet_addr	 SocketBase->WSIFace->WSinet_addr
#define WSgetprotobyname SocketBase->WSIFace->WSgetprotobyname

struct TaskBase;

struct TaskNode
{
    struct AVLNode node;
    struct Task *task;
    struct TaskBase *self;
};

struct TaskBase
{
    struct Library lib;		/* Standard header		  */
    struct TaskNode n;		/* AVLNode for linking and lookup */
    struct bsdsocketBase *glob;	/* Pointer to global SocketBase	  */
    APTR pool;			/* Memory pool			  */
    void *errnoPtr;		/* errno stuff			  */
    int errnoSize;
    ULONG errnoVal;		/* Default errno storage	  */
    ULONG sigintr;		/* Signals definition		  */
    ULONG sigio;
    ULONG sigurg;
    struct protoent *pe;	/* protoent buffer		  */
};

#endif /* BSDSOCKET_INTERN_H */
