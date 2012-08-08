#ifndef EXEC_INTERRUPTS_H
#define EXEC_INTERRUPTS_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
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

/* CPU-dependent struct ExceptionContext */
#if defined __i386__
#include <aros/i386/cpucontext.h>
#elif defined __x86_64__
#include <aros/x86_64/cpucontext.h>
#elif defined __mc68000__
#include <aros/m68k/cpucontext.h>
#elif defined __powerpc__
#include <aros/ppc/cpucontext.h>
#elif defined __arm__
#include <aros/arm/cpucontext.h>
#else
#error unsupported CPU type
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

/* Offset of kernel interrupt vectors.
 *
 * Usage:
 *   AddIntServer(INTB_KERNEL + irq, irq_handler);
 *   RemIntServer(INTB_KERNEL + irq, irq_handler);
 */
#define INTB_KERNEL       (16)

#endif /* EXEC_INTERRUPTS_H */
