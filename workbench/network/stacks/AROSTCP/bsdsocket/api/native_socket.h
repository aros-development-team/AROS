#ifndef API_NATIVE_SOCKET_H
#define API_NATIVE_SOCKET_H
/* Copyright (C) 2026 John KNIPPER <code@jkn.me>.
 * Licensed under the AROS Public License (APL), Version 1.1. */
#include <bsdsocket/native.h>
BOOL native_socket_setup(void);
BOOL native_socket_withdraw(void);
LONG native_bind(LONG,caddr_t,LONG,struct SocketBase *);
LONG native_listen(LONG,LONG,struct SocketBase *);
LONG native_accept(LONG,caddr_t,socklen_t *,struct SocketBase *,struct socket **);
LONG native_shutdown(LONG,LONG,struct SocketBase *);
LONG native_setsockopt(LONG,LONG,LONG,caddr_t,ULONG,struct SocketBase *);
LONG native_getsockopt(LONG,LONG,LONG,caddr_t,ULONG *,struct SocketBase *);
LONG native_getsockname(LONG,caddr_t,ULONG *,struct SocketBase *);
LONG native_getpeername(LONG,caddr_t,ULONG *,struct SocketBase *);
LONG native_recvfrom(LONG,caddr_t,LONG,LONG,caddr_t,LONG *,struct SocketBase *);
#endif
