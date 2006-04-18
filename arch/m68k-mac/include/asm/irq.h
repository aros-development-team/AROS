#ifndef ASM_IRQ_H
#define ASM_IRQ_H
/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: irq.h 15760 2002-11-16 20:51:51Z bergers $

    Desc: Interrupt core, part of kernel.resource
    Lang: english
*/

#include <exec/ptrace.h>

/*********************************************************************/

#define __STR(x) #x
#define STR(x) __STR(x)

/*********************************************************************/

#define NR_IRQS             16      /* Use all XT-PIC interrupts */
#define NR_SYSCALLS         4

/*
 * Structure used to describe interrupt controler. Sufficient to describe
 * the low-level hardware
 */
struct irqController
{
    const char  *ic_Name;                       /* Controller name */
    void        (*ic_startup)(unsigned int);    /* All functions here! */
    void        (*ic_shutdown)(unsigned int);
    void        (*ic_handle)(unsigned int, unsigned int, struct pt_regs *);
    void        (*ic_enable)(unsigned int);
    void        (*ic_disable)(unsigned int);
};

struct irqServer
{
    void        (*is_handler)(int, void *, struct pt_regs *); 
    const char  *is_name;       /* Server name */
    void        *is_UserData;   /* Will be class data or similar */
};

struct irqDescriptor
{
    unsigned int            id_status;      /* IRQ status. See below for details */
    struct irqController    *id_handler;    /* how to do emable/disable */
    struct irqServer        *id_server;     /* Server pointer */
    unsigned int            id_depth;       /* Disable depth for nested IRQs */
    unsigned int            id_count;       /* IRQ counter */
    unsigned int            id_unused[3];
};

/* IRQ status */

#define IRQ_INPROGRESS   1  /* Handler active - DO NOT enter */
#define IRQ_DISABLED     2  /* IRQ disabled - DO NOT enter */
#define IRQ_PENDING      4  /* IRQ pending - replay on enable */
#define IRQ_REPLAY       8  /* IRQ has been replayed but not acked yet */
#define IRQ_AUTODETECT  16  /* IRQ is beeing autodetected */
#define IRQ_WAITING     32  /* IRQ not seen yet - for autodetection */

#endif /* ASM_IRQ_H */
