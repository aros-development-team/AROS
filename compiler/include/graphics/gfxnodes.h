#ifndef GRAPHICS_GFXNODES_H
#define GRAPHICS_GFXNODES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Extended node for graphics.library.
    Lang: english
*/

#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

struct ExtendedNode
{
    struct Node * xln_Succ;
    struct Node * xln_Pred;

    UBYTE   xln_Type;      /* see below */
    BYTE    xln_Pri;
    char  * xln_Name;
    UBYTE   xln_Subsystem;
    UBYTE   xln_Subtype;
    LONG    xln_Library;
    LONG (* xln_Init)();
};

/* xln_Type */
#define VIEW_EXTRA_TYPE      1
#define VIEWPORT_EXTRA_TYPE  2
#define SPECIAL_MONITOR_TYPE 3
#define MONITOR_SPEC_TYPE    4

#define SS_GRAPHICS 0x02

#endif /* GRAPHICS_GFXNODES_H */
