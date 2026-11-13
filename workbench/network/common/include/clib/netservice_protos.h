#ifndef CLIB_NETSERVICE_PROTOS_H
#define CLIB_NETSERVICE_PROTOS_H

/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: Prototypes for netservices.library
*/

#include <aros/libcall.h>
#include <exec/types.h>
#include <exec/libraries.h>
#include <utility/tagitem.h>

AROS_LP1(APTR, RegisterNetService,
         AROS_LPA(struct TagItem *, tags, A0),
         struct Library *, NetServicesBase, 5, NetServices
);

AROS_LP1(void, UnregisterNetService,
         AROS_LPA(APTR, handle, A0),
         struct Library *, NetServicesBase, 6, NetServices
);

AROS_LP1(LONG, QueryNetServices,
         AROS_LPA(struct TagItem *, tags, A0),
         struct Library *, NetServicesBase, 7, NetServices
);

#endif /* CLIB_NETSERVICE_PROTOS_H */
