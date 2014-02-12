/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
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

#define Z2SLOTS         240
#define SLOTSPERBYTE    8

/* The following layout is compatible with the AOS 1.3 include files */

struct IntExpansionBase
{
    struct Library          eb_LibNode;
    UBYTE                   eb_Flags;
    UBYTE                   eb_pad;
    struct ExecBase        *eb_SysBase;
    IPTR                    eb_SegList;
    struct CurrentBinding   eb_CurrentBinding;
    struct List             eb_BoardList;
    struct List             eb_MountList;

    UBYTE                   eb_z2Slots[Z2SLOTS/SLOTSPERBYTE];
    UWORD                   eb_z3Slot;
    UBYTE                   eb_pad2[224];

    struct SignalSemaphore  eb_BindSemaphore;
    struct SignalSemaphore  eb_BootSemaphore;
};

#define IntExpBase(eb)	((struct IntExpansionBase*)(eb))

#endif /* _EXPANSION_INTERN_H */
