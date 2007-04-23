#ifndef LIBRARIES_CONFIGVARS_H
#define LIBRARIES_CONFIGVARS_H

/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Software structures used by expansion boards
    Lang: english
*/

#ifndef EXEC_TYPES_H
#include <exec/types.h>
#endif

#ifndef EXEC_NODES_H
#include <exec/nodes.h>
#endif

#ifndef LIBRARIES_CONFIGREGS_H
#include <libraries/configregs.h>
#endif

/*
    Each expansion board that is found has a ConfigDev structure created
    for it very early at system startup. Software can search for boards
    by the manufacturer and product id (for Zorro/AutoConfig(TM) boards).

    For debugging, you can also look at the entire list of expansion
    boards. See the expansion.library FindConfigDev() function for more
    information.
*/

struct ConfigDev
{
    struct Node         cd_Node;
    UBYTE               cd_Flags;       /* read/write device flags */
    UBYTE               cd_Pad;
    struct ExpansionRom cd_Rom;         /* copy of boards expansion ROM */
    APTR                cd_BoardAddr;   /* physical address of exp. board */
    ULONG               cd_BoardSize;   /* size in bytes of exp. board */
    UWORD               cd_SlotAddr;    /* private */
    UWORD               cd_SlotSize;    /* private */
    APTR                cd_Driver;      /* pointer to node of driver */
    struct ConfigDev   *cd_NextCD;      /* linked list of devices to configure */
    ULONG               cd_Unused[4];   /* for the drivers use - private */
};

/* Flags definitions for cd_Flags */
#define CDB_SHUTUP      0       /* this board has been shut up */
#define CDF_SHUTUP      0x01
#define CDB_CONFIGME    1       /* board needs a driver to claim it */
#define CDF_CONFIGME    0x02
#define CDB_BADMEMORY   2       /* board contains bad memory */
#define CDF_BADMEMORY   0x04
#define CDB_PROCESSED   3       /* private */
#define CDF_PROCESSED   0x08

/*
    Boards without their own drivers are normally bound to software
    drivers. This structure is used by GetCurrentBinding(), and
    SetCurrentBinding().
*/
struct CurrentBinding
{
    struct ConfigDev *cb_ConfigDev;     /* SLL of devices to configure */
    UBYTE            *cb_FileName;      /* disk file name of driver */
    UBYTE            *cb_ProductString; /* PRODUCT= tool type from icon */
    UBYTE           **cb_ToolTypes;     /* tool types from disk object */
};

#endif /* LIBRARIES_CONFIGVARS_H */
