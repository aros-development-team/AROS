#ifndef LIBRARIES_EXPANSIONBASE_H
#define LIBRARIES_EXPANSIONBASE_H

/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Publicly visible ExpansionBase data.
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_LIBRARIES_H
#include <exec/libraries.h>
#endif

#ifndef EXEC_LISTS_H
#include <exec/lists.h>
#endif

#ifndef LIBRARIES_CONVIGVARS_H
#include <libraries/configvars.h>
#endif

/*
    BootNodes are used by dos.library to determine which device to boot
    from. Items found on the list are added to DOS's list of available
    devices before the system boot, and the highest priority node will
    be used to attempt to boot. You add BootNodes with the expansion
    AddBootNode() call.

    If you use the AddDosNode() call, you will have to create and add
    your own BootNode. It is preferred to use AddBootNode().
*/

struct BootNode
{
    struct Node bn_Node;
    UWORD       bn_Flags;
    APTR        bn_DeviceNode;
};

/*
    Most of this data is private, but you can use the expansion.library
    functions to scan the information.

    Use FindConfigDev() to scan the board list.
*/

struct ExpansionBase
{
    struct Library      LibNode;
    UBYTE               Flags;          /* Flags, read only */
    UBYTE               eb_Private01;
    IPTR                eb_Private02;
    IPTR                eb_Private03;
    struct CurrentBinding eb_Private04;
    struct List         BoardList;      /* BoardList - private */
    struct List         MountList;      /* BootNode entries - public */
};

/*  The error codes from expansion boards */
#define EE_OK           0   /* no error */
#define EE_LASTBOARD    40  /* board could not be shut up */
#define EE_NOEXPANSION  41  /* no space expansion slots, board shut up */
#define EE_NOMEMORY     42  /* no normal memory */
#define EE_NOBOARD      43  /* no board at that address */
#define EE_BADMEM       44  /* tried to add a bad memory card */

/* ExpansionBase flags, READ ONLY !! */

#define EBB_CLOGGED     0       /* a board could not be shut up */
#define EBF_CLOGGED     (1L<<0)
#define EBB_SHORTMEM    1       /* ran out of expansion memory */
#define EBF_SHORTMEM    (1L<<1)
#define EBB_BADMEM      2       /* tried to add bad memory card */
#define EBF_BADMEM      (1L<<2)
#define EBB_DOSFLAG     3       /* reserved by AmigaDOS */
#define EBF_DOSFLAG     (1L<<3)
#define EBB_KICKBACK33  4       /* reserved by AmigaDOS */
#define EBF_KICKBACK33  (1L<<4)
#define EBB_KICKBACK36  5       /* reserved by AmigaDOS */
#define EBF_KICKBACK36  (1L<<5)

/*  If the following flag is set by a floppies boot code, then when DOS
    awakes, it will not open its initial console window until the first
    output is written to that shell. Otherwise the old behaviour will
    apply.
*/
#define EBB_SILENTSTART 6
#define EBF_SILENTSTART (1L<<6)

#define EBB_START_CC0   7       /* allow/try boot from CC0 */
#define EBF_START_CC0   (1L<<7)

#endif /* LIBRARIES_EXPANSIONBASE_H */
