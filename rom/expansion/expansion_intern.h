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
    struct Library          LibNode;
    UBYTE                   Flags;
    UBYTE                   pad;
    struct ExecBase        *ExecBase;
    IPTR                    SegList;
    struct CurrentBinding   CurrentBinding;
    struct List             BoardList;
    struct List             MountList;

    UBYTE                   z2Slots[Z2SLOTS/SLOTSPERBYTE];
    UWORD                   z3Slot;
    UBYTE                   pad2[224];

    struct SignalSemaphore  BindSemaphore;
    struct SignalSemaphore  BootSemaphore;
    ULONG                   BootFlags;
};

#define IntExpBase(eb)	((struct IntExpansionBase*)(eb))

#endif /* _EXPANSION_INTERN_H */
