#ifndef EXEC_INTERRUPTS_H
#define EXEC_INTERRUPTS_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt structures
    Lang: english
*/

#ifndef EXEC_LISTS_H
#   include <exec/lists.h>
#endif
#ifndef EXEC_NODES_H
#   include <exec/nodes.h>
#endif

struct Interrupt
{
    struct Node is_Node;
    APTR        is_Data;
    VOID     (* is_Code)(); /* server code entry */
};

/* PRIVATE */
struct IntVector
{
    APTR          iv_Data;
    VOID       (* iv_Code)();
    struct Node * iv_Node;
};

/* PRIVATE */
struct SoftIntList
{
    struct List sh_List;
    UWORD       sh_Pad;
};

#define SIH_PRIMASK (0xf0)

#define INTB_NMI      15
#define INTF_NMI (1L<<15)

#endif /* EXEC_INTERRUPTS_H */
