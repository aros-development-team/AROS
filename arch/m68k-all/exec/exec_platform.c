/*
 * Copyright (C) 2011-2017, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 *
 */

#include <aros/libcall.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
    
extern VOID AROS_SLIB_ENTRY(ExitIntr, Exec, 6)(VOID);
extern VOID AROS_SLIB_ENTRY(Schedule, Exec, 7)(VOID);
extern VOID AROS_SLIB_ENTRY(Switch, Exec, 9)(VOID);
extern VOID AROS_SLIB_ENTRY(Dispatch, Exec, 10)(VOID);

extern void AROS_SLIB_ENTRY(CopyMem_020,Exec,104)(void);
extern void AROS_SLIB_ENTRY(CopyMem_040,Exec,104)(void);
extern void AROS_SLIB_ENTRY(CopyMemQuick_040,Exec,105)(void);
extern void AROS_SLIB_ENTRY(CopyMem_060,Exec,104)(void);
extern void AROS_SLIB_ENTRY(CopyMemQuick_060,Exec,105)(void);

static int Exec_init_platform(struct ExecBase *lh)
{
    __AROS_SETVECADDR(lh, 6, AROS_SLIB_ENTRY(ExitIntr, Exec, 6));
    __AROS_SETVECADDR(lh, 7, AROS_SLIB_ENTRY(Schedule, Exec, 7));
    __AROS_SETVECADDR(lh, 9, AROS_SLIB_ENTRY(Switch, Exec, 9));
    __AROS_SETVECADDR(lh,10, AROS_SLIB_ENTRY(Dispatch, Exec,10));

    if (lh->AttnFlags & AFF_68060) {
        /* MC68060+ */
        __AROS_SETVECADDR(lh, 104, AROS_SLIB_ENTRY(CopyMem_060, Exec, 104));
        __AROS_SETVECADDR(lh, 105, AROS_SLIB_ENTRY(CopyMemQuick_060, Exec, 105));
    }
    else if (lh->AttnFlags & AFF_68040) {
        /* MC68040+ */
        __AROS_SETVECADDR(lh, 104, AROS_SLIB_ENTRY(CopyMem_040, Exec, 104));
        __AROS_SETVECADDR(lh, 105, AROS_SLIB_ENTRY(CopyMemQuick_060, Exec, 105));
    }
    else if (lh->AttnFlags & AFF_68020) {
        /* MC68020+ */
        __AROS_SETVECADDR(lh, 104, AROS_SLIB_ENTRY(CopyMem_020, Exec, 104));
    }

    return TRUE;
}

ADD2INITLIB(Exec_init_platform,0)
