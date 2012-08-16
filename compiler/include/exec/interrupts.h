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

#ifdef __AROS__
/* AROS helper macros.
 * These are used to abstract away architectural
 * differences between AROS ports.
 */
#include <aros/asmcall.h>

/* Define a function prototype and call methods
 * for Cause(), SetIntVector(), and AddIntServer()
 * struct Interrupt is_Code fields.
 *
 * The prototype matches:
 *
 * ULONG func(APTR data, ULONG intmask, struct Custom *custom, 
 *            VOID_FUNC code, struct ExecBase *sysBase)
 *            (A1, D1, A0, A5, A6)
 *
 * Handler should return TRUE (interrupt handled)
 * or FALSE (continue interrupt processing)
 */
#define AROS_INTP(n)                                       \
        AROS_UFP5(ULONG, n,                                \
          AROS_UFPA(APTR,      __ufi_data,    A1),         \
          AROS_UFPA(ULONG,     __ufi_intmask, D1),         \
          AROS_UFPA(APTR,      __ufi_custom,  A0),         \
          AROS_UFPA(VOID_FUNC, __ufi_code,    A5),         \
          AROS_UFPA(struct ExecBase *, __ufi_SysBase, A6))

#define AROS_INTC4(n, data, intmask, custom, code)         \
        AROS_UFC5(ULONG, n,                                \
                AROS_UFCA(APTR,      data,    A1),         \
                AROS_UFCA(ULONG,     intmask, D1),         \
                AROS_UFCA(APTR,      custom,  A0),         \
                AROS_UFCA(VOID_FUNC, code,    A5),         \
                AROS_UFCA(struct ExecBase *, SysBase, A6))
#define AROS_INTC3(n, data, intmask, custom) AROS_INTC4(n, data, intmask, custom, (VOID_FUNC)(n))
#define AROS_INTC2(n, data, intmask)         AROS_INTC4(n, data, intmask, (APTR)(IPTR)0xdff000, (VOID_FUNC)(n));
#define AROS_INTC1(n, data)                  AROS_INTC4(n, data, 0, (APTR)(IPTR)0xdff000, (VOID_FUNC)(n));

#define AROS_INTH4(n, type, data, intmask, custom, code)   \
        AROS_UFH5(ULONG, n,                                \
          AROS_UFHA(APTR,      __ufi_data, A1),            \
          AROS_UFHA(ULONG,     intmask,    D1),            \
          AROS_UFHA(APTR,      custom,     A0),            \
          AROS_UFHA(VOID_FUNC, code,       A5),            \
          AROS_UFHA(struct ExecBase *, SysBase, A6)        \
        ) { AROS_USERFUNC_INIT                             \
            type __unused data = __ufi_data;

#define AROS_INTH3(n, type, data, intmask, custom) AROS_INTH4(n, type, data, intmask, custom, __ufi_code)
#define AROS_INTH2(n, type, data, intmask)         AROS_INTH4(n, type, data, intmask, __ufi_custom, __ufi_code)
#define AROS_INTH1(n, type, data)                  AROS_INTH4(n, type, data, __ufi_intmask, __ufi_custom, __ufi_code)
#define AROS_INTH0(n)                              AROS_INTH4(n, APTR, data, __ufi_intmask, __ufi_custom, __ufi_code)

#ifdef __mc68000
/* Special hack for setting the 'Z' condition code upon exit
 * for m68k architectures.
 */
#define AROS_INTFUNC_INIT inline ULONG _handler(void) {
#define AROS_INTFUNC_EXIT }; register ULONG _res asm ("d0") = _handler();     \
                             asm volatile ("tst.l %0\n" : : "r" (_res)); \
                             return _res; /* gcc only generates movem/unlk/rts */   \
                             AROS_USERFUNC_EXIT }
#else  /* ! __mc68000 */
/* Everybody else */
#define AROS_INTFUNC_INIT
#define AROS_INTFUNC_EXIT       AROS_USERFUNC_EXIT }
#endif /* ! __mc68000 */

#endif /* __AROS__ */

#endif /* EXEC_INTERRUPTS_H */
