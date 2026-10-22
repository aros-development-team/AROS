#ifndef BSDSOCKET_NATIVE_H
#define BSDSOCKET_NATIVE_H
/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1.
 * Native task-independent descriptors. This is an AROS API, not a host ABI.
 */
#include <exec/types.h>

#define AROS_SOCKET_ABI 1
#define AROS_SOCKET_VECTOR 139
#define AROS_SOCKET_CREATE 1
#define AROS_SOCKET_BIND 2
#define AROS_SOCKET_LISTEN 3
#define AROS_SOCKET_ACCEPT 4
#define AROS_SOCKET_CONNECT 5
#define AROS_SOCKET_SEND 6
#define AROS_SOCKET_RECV 7
#define AROS_SOCKET_SHUTDOWN 8
#define AROS_SOCKET_GETOPT 9
#define AROS_SOCKET_SETOPT 10
#define AROS_SOCKET_LOCALNAME 11
#define AROS_SOCKET_PEERNAME 12
#define AROS_SOCKET_CLONE 13
#define AROS_SOCKET_CLOSE 14
#define AROS_SOCKET_NONBLOCK 15
#define AROS_SOCKET_POLL 16
#define AROS_SOCKET_POLL_READ 1
#define AROS_SOCKET_POLL_WRITE 2
#define AROS_SOCKET_POLL_EXCEPTION 4

/* Call vector139 with (request:A0, sizeof(*request):D0), returning LONG errno
 * (0 on success). No global SocketBase: caller keeps its own library reference
 * over the call. The descriptor survives that reference and its creator task.
 * Scalars are native Exec types, pointers refer to the native SDK structures.
 * All storage is caller-owned and is retained only until the call returns.
 *
 * CREATE: arg0=family,arg1=type,arg2=protocol,result=fd.
 * BIND/CONNECT: address,*address_length. LISTEN: arg0=backlog.
 * ACCEPT: optional address/address_length,result=fd.
 * SEND/RECV: buffer,length,arg0=flags; optional UDP address/address_length.
 * SHUTDOWN: arg0=how. GETOPT/SETOPT: arg0=level,arg1=option,buffer,length;
 * GETOPT returns actual copied length in length.
 * LOCALNAME/PEERNAME: address,address_length.
 * CLONE: arg0=minimum fd,result=newfd. CLOSE: retires exactly fd.
 * NONBLOCK: arg0=boolean. POLL: arg0=requested mask,arg1=timeout milliseconds
 * (-1 indefinite),result=ready mask; 0 means timeout. Mask4 denotes native
 * exceptional/urgent readiness, not SO_ERROR. Connection completion must use
 * GETOPT(SO_ERROR) after writable readiness. Errors use native errno.
 * SO_LINGER with enabled linger is unsupported: deferred close cannot borrow
 * a caller's sleep context after that caller returns.
 */
struct aros_socket_request {
    ULONG version;
    ULONG operation;
    LONG fd;
    LONG arg0;
    LONG arg1;
    LONG arg2;
    SIPTR result;
    APTR buffer;
    IPTR length;
    APTR address;
    ULONG *address_length;
};
#endif
