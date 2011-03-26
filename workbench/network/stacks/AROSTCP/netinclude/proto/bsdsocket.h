#ifndef PROTO_BSDSOCKET_H
#define PROTO_BSDSOCKET_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <sys/cdefs.h>

#include <clib/bsdsocket_protos.h>

#if !defined(SocketBase) && !defined(__NOLIBBASE__) && !defined(__BSDSOCKET_NOLIBBASE__)
 #ifdef __BSDSOCKET_STDLIBBASE__
  extern struct Library *SocketBase;
 #else
  extern struct Library *SocketBase;
 #endif
#endif

#if !defined(NOLIBDEFINES) && !defined(BSDSOCKET_NOLIBDEFINES)
#   include <defines/bsdsocket.h>
#endif

#endif /* PROTO_BSDSOCKET_H */
