#ifndef ASM_IRQ_H
#define ASM_IRQ_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Interrupt core, part of kernel.resource
    Lang: english
*/

#include <asm/segments.h>
#include <asm/linkage.h>
#include <asm/ptrace.h>

/* interrupt control.. */

#define __sti() __asm__ __volatile__ ("sti": : :"memory")
#define __cli() __asm__ __volatile__ ("cli": : :"memory")

/*********************************************************************/

#define __STR(x) #x
#define STR(x) __STR(x)

/*********************************************************************/

/*
 * This is standard macro used to save register frame on the supervisor
 * stack. Supposes that orig_eax is saved already.
 */
#define SAVE_REGS       	\
    "cld		\n\t"	\
    "pushl   %eax       \n\t"	\
    "pushl   %ecx       \n\t"	\
    "pushl   %edx       \n\t"	\
    "push    %ds        \n\t"	\
    "push    %es        \n\t"	\
    "movl    $"STR(KERNEL_DS)",%edx\n\t" \
    "mov     %dx,%ds    \n\t"	\
    "mov     %dx,%es	\n\t"

#define RESTORE_REGS    	\
    "pop     %es	\n\t"   \
    "pop     %ds        \n\t"	\
    "popl    %edx	\n\t"   \
    "popl    %ecx       \n\t"	\
    "popl    %eax	\n\t"

#define R_es        0x00
#define R_ds        0x04
#define R_edx       0x08
#define R_ecx       0x0c
#define R_eax       0x10
#define R_param     0x14

/*
    Save/restore all registers. These macros are used mostly inside Switch
    procedure.
*/

#define SAVE_ALL        	\
    "cld		\n\t"	\
    "push    %es        \n\t"	\
    "push    %ds        \n\t"	\
    "pushl   %eax       \n\t"	\
    "pushl   %ebp       \n\t"	\
    "pushl   %edi       \n\t"	\
    "pushl   %esi       \n\t"	\
    "pushl   %edx       \n\t"	\
    "pushl   %ecx       \n\t"	\
    "pushl   %ebx       \n\t"	\
    "movl    $"STR(KERNEL_DS)",%edx \n\t"\
    "mov     %dx,%ds    \n\t"	\
    "mov     %dx,%es	\n\t"

#define RESTORE_ALL     	\
    "popl    %ebx       \n\t"	\
    "popl    %ecx       \n\t"	\
    "popl    %edx       \n\t"	\
    "popl    %esi       \n\t"	\
    "popl    %edi       \n\t"	\
    "popl    %ebp       \n\t"	\
    "popl    %eax       \n\t"	\
    "pop     %ds        \n\t"	\
    "pop     %es	\n\t"

#define FIRST_EXT_VECTOR    0x20    /* This is exactly what Intel says about */
#define SYSTEM_VECTOR       0x80    /* Vector callable from user mode */

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
    void        (*ic_handle)(unsigned int, struct pt_regs *);
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
    unsigned int                id_status;      /* IRQ status. See below for details */
    const struct irqController  *id_handler;    /* how to do emable/disable */
    struct irqServer            *id_server;     /* Server pointer */
    unsigned int                 id_depth;       /* Disable depth for nested IRQs */
    unsigned int                 id_count;       /* IRQ counter */
    unsigned int id_unused[3];
};

/* IRQ status */

#define IRQ_INPROGRESS   1  /* Handler active - DO NOT enter */
#define IRQ_DISABLED     2  /* IRQ disabled - DO NOT enter */
#define IRQ_PENDING      4  /* IRQ pending - replay on enable */
#define IRQ_REPLAY       8  /* IRQ has been replayed but not acked yet */
#define IRQ_AUTODETECT  16  /* IRQ is beeing autodetected */
#define IRQ_WAITING     32  /* IRQ not seen yet - for autodetection */

#endif /* ASM_IRQ_H */
