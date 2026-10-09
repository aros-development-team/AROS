#ifndef PROTO_NETSERVICE_H
#define PROTO_NETSERVICE_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <exec/types.h>
#include <aros/system.h>

#include <clib/netservice_protos.h>

#if !defined(NetServicesBase) && !defined(__NOLIBBASE__) && !defined(__NETSERVICE_NOLIBBASE__)
  extern struct Library *NetServicesBase;
#endif

#if !defined(NOLIBDEFINES) && !defined(NETSERVICE_NOLIBDEFINES)
#   include <defines/netservice.h>
#endif

#endif /* PROTO_NETSERVICE_H */
