#ifndef DEFINES_NETSERVICE_PROTOS_H
#define DEFINES_NETSERVICE_PROTOS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Defines for netservices.library
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <aros/preprocessor/variadic/cast2iptr.hpp>

#define __RegisterNetService_WB(__NetServicesBase, __arg1) \
        AROS_LC1(APTR, RegisterNetService, \
                 AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__NetServicesBase), 5, NetServices)

#define RegisterNetService(arg1) \
    __RegisterNetService_WB(NetServicesBase, (arg1))

#define __UnregisterNetService_WB(__NetServicesBase, __arg1) \
        AROS_LC1NR(void, UnregisterNetService, \
                 AROS_LCA(APTR, (__arg1), A0), \
        struct Library *, (__NetServicesBase), 6, NetServices)

#define UnregisterNetService(arg1) \
    __UnregisterNetService_WB(NetServicesBase, (arg1))

#define __QueryNetServices_WB(__NetServicesBase, __arg1) \
        AROS_LC1(LONG, QueryNetServices, \
                 AROS_LCA(struct TagItem *, (__arg1), A0), \
        struct Library *, (__NetServicesBase), 7, NetServices)

#define QueryNetServices(arg1) \
    __QueryNetServices_WB(NetServicesBase, (arg1))

#endif /* DEFINES_NETSERVICE_PROTOS_H */
