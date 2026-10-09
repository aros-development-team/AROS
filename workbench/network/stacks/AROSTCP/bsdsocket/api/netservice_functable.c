/*
 * Copyright (C) 2026 The AROS Dev Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Function table for netservices.library (single shared base).
 */

#include <conf.h>

#include <aros/libcall.h>
#include <exec/types.h>
#include <sys/param.h>
#include <api/amiga_raf.h>

typedef VOID (* f_void)();

extern VOID AROS_SLIB_ENTRY(Open,                 NetServices, 1)();
extern VOID AROS_SLIB_ENTRY(Close,                NetServices, 2)();
extern VOID AROS_SLIB_ENTRY(Expunge,              NetServices, 3)();
extern VOID AROS_SLIB_ENTRY(Reserved,             NetServices, 4)();
extern VOID AROS_SLIB_ENTRY(RegisterNetService,   NetServices, 5)();
extern VOID AROS_SLIB_ENTRY(UnregisterNetService, NetServices, 6)();
extern VOID AROS_SLIB_ENTRY(QueryNetServices,     NetServices, 7)();

f_void NetServices_InitFuncTable[] = {
#ifdef __MORPHOS__
    FUNCARRAY_32BIT_NATIVE,
#endif
    AROS_SLIB_ENTRY(Open,                 NetServices, 1),
    AROS_SLIB_ENTRY(Close,                NetServices, 2),
    AROS_SLIB_ENTRY(Expunge,              NetServices, 3),
    AROS_SLIB_ENTRY(Reserved,             NetServices, 4),
    AROS_SLIB_ENTRY(RegisterNetService,   NetServices, 5),
    AROS_SLIB_ENTRY(UnregisterNetService, NetServices, 6),
    AROS_SLIB_ENTRY(QueryNetServices,     NetServices, 7),
    (f_void) - 1
};
