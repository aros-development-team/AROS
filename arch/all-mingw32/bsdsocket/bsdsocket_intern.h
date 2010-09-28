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

#include "host_socket.h"
#include "winsock2.h"

struct WinSockInterface
{
    int		      __stdcall (*WSAGetLastError)(void);
    ULONG	      __stdcall (*WSinet_addr)(const char* cp);
    struct PROTOENT * __stdcall (*WSgetprotobyname)(const char* name);
    int		      __stdcall (*WSsocket)(int af, int type, int protocol);
    int		      __stdcall (*WSclosesocket)(int s);
    int		      __stdcall (*WSAEventSelect)(int s, APTR hEventObject, ULONG lNetworkEvents);
};

struct HostSocketInterface
{
    struct SocketController *(*sock_init)(void);
    int (*sock_shutdown)(struct SocketController *ctl);
};

struct bsdsocketBase
{
    struct Library lib;			  /* Standard header		*/
    APTR HostLibBase;			  /* hostlib.resource base	*/
    APTR winsock;			  /* Ws2_32.dll handle		*/
    APTR resolver;			  /* bsdsocket.dll handle	*/
    struct WinSockInterface *WSIFace;	  /* WinSock interface		*/
    struct HostSocketInterface *ResIFace; /* Resolver DLL interface	*/
    struct SocketController *ctl;	  /* Resolver control registers */
    struct AVLNode *tasks;		  /* TaskBases tree		*/
    struct MinList socks;		  /* Sockets list		*/
};

#define WSAGetLastError	 SocketBase->WSIFace->WSAGetLastError
#define WSinet_addr	 SocketBase->WSIFace->WSinet_addr
#define WSgetprotobyname SocketBase->WSIFace->WSgetprotobyname
#define WSsocket	 SocketBase->WSIFace->WSsocket
#define WSclosesocket	 SocketBase->WSIFace->WSclosesocket
#define WSAEventSelect	 SocketBase->WSIFace->WSAEventSelect

struct Socket;
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
    ULONG dTableSize;		/* Size of dtable		  */
    struct Socket **dTable;	/* Socket descriptors table	  */
    struct protoent *pe;	/* protoent buffer		  */
};

int IntCloseSocket(int s, struct TaskBase *taskBase);

#endif /* BSDSOCKET_INTERN_H */
