/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal data structures for expansion.library
    Lang: english
*/

#ifndef _EXPANSION_INTERN_H
#define _EXPANSION_INTERN_H

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/semaphores.h>
#include <libraries/expansion.h>
#include <libraries/expansionbase.h>
#include <libraries/configvars.h>

/* Needed for aros_print_not_implemented macro */
#include <aros/debug.h>

#define Z2SLOTS         256
#define Z3SLOTS         2048
#define SLOTSPERBYTE    8

/* I got this info from the 1.3 include file libraries/expansionbase.h */

struct IntExpansionBase
{
    struct Library          eb_LibNode;
    UBYTE                   eb_Flags;
    UBYTE                   eb_pad;
    struct ExecBase        *eb_SysBase;
    ULONG                   eb_SegList;
    struct CurrentBinding   eb_CurrentBinding;
    struct List             eb_BoardList;
    struct List             eb_MountList;

    struct SignalSemaphore  eb_BindSemaphore;

    UBYTE                   eb_z2Slots[Z2SLOTS/SLOTSPERBYTE];
    UBYTE                   eb_z3Slots[Z3SLOTS/SLOTSPERBYTE];
};

#define IntExpBase(eb)	((struct IntExpansionBase*)(eb))
#define SysBase		(((struct IntExpansionBase *)ExpansionBase)->eb_SysBase)

#endif /* _EXPANSION_INTERN_H */
