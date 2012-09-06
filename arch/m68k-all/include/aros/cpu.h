#ifndef AROS_M68K_CPU_H
#define AROS_M68K_CPU_H

/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc:CPU-specific definitions for Motorola m68k processors
    Lang: english
*/

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN            1 /* Big or little endian */
#define AROS_SIZEOFULONG           4 /* Size of an ULONG */
#define AROS_SIZEOFPTR             4 /* Size of a PTR */
#define AROS_WORDALIGN             2 /* Alignment for WORD */
#define AROS_LONGALIGN             2 /* Alignment for LONG */
#define AROS_QUADALIGN             2 /* Alignment for QUAD */
#define AROS_PTRALIGN              2 /* Alignment for PTR */
#define AROS_IPTRALIGN             2 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN           2 /* Alignment for double */
#define AROS_WORSTALIGN            4 /* Worst case alignment */

#define AROS_NOFPU 1

/* do we need a function attribute to get parameters on the stack? */
#define __stackparm

#define STACKED

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

#define AROS_GET_SP ({register unsigned char * sp asm("%sp"); sp;})

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct JumpVec
{
    unsigned short jmp;
    void *vec;
};


/* Any jump to an unimplemented vector will cause an access to this address */
#define _aros_empty_vector              0xc0ed0000

/* Internal macros */
#define __AROS_ASMJMP                   0x4EF9
#define __AROS_SET_VEC(v,a)             ((v)->vec=(a))
#define __AROS_GET_VEC(v)               ((v)->vec)
#define __AROS_USE_FULLJMP

struct FullJumpVec
{
    unsigned short jmp;
    void *vec;
};
#define __AROS_SET_FULLJMP(v,a)                          \
do                                                       \
{                                                        \
        struct FullJumpVec *_v = v;                      \
        _v->jmp = __AROS_ASMJMP;                         \
        _v->vec = ((void *)(a));                         \
} while(0)


/* Use these to acces a vector table */
#define LIB_VECTSIZE                    (sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&(((struct JumpVec *)(lib))[-(n)]))
#define __AROS_GETVECADDR(lib,n)        ((void *)__AROS_GETJUMPVEC(lib,n))
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_SET_VEC(__AROS_GETJUMPVEC(lib,n),(APTR)(addr)))
#define __AROS_INITVEC(lib,n)           __AROS_GETJUMPVEC(lib,n)->jmp = __AROS_ASMJMP, \
                                        __AROS_SETVECADDR(lib,n,_aros_empty_vector | (n << 8) | 1)

#define KERNEL_PANIC_BUFFER (char*)0x800

/*
   We want to activate the execstubs and preserve all registers
   when calling obtainsemaphore, obtainsemaphoreshared, releasesemaphore,
   getcc, permit, forbid, enable, disable
*/
#undef UseExecstubs
//#define UseExecstubs 1

/* Macros to test/set failure of AllocEntry() */
#define AROS_ALLOCENTRY_FAILED(memType) \
        ((struct MemList *)((IPTR)(memType) | 0x80ul<<(sizeof(APTR)-1)*8))
#define AROS_CHECK_ALLOCENTRY(memList) \
        (!((IPTR)(memList) & 0x80ul<<(sizeof(APTR)-1)*8))

/*
    Find the next valid alignment for a structure if the next x bytes must
    be skipped.
*/
#define AROS_ALIGN(x)        (((x)+AROS_WORSTALIGN-1)&-AROS_WORSTALIGN)

/* Prototypes */
extern void _aros_not_implemented ();
extern void aros_not_implemented ();

/*
    How much stack do we need ? Lots :-) ?
    Not so much, I think (schulz) ;-))
*/

#define AROS_STACKSIZE  0x4000

//#define AROS_NEEDS___MAIN

#define AROS_COMPAT_SETD0(x)    do { asm volatile ( "move.l %0,%%d0\n" : "=g" (x)); return; } while (0)

#define A0      a0
#define A1      a1
#define A2      a2
#define A3      a3
#define A4      a4
#define A5      a5
#define A6      a6
#define D0      d0
#define D1      d1
#define D2      d2
#define D3      d3
#define D4      d4
#define D5      d5
#define D6      d6
#define D7      d7

/* This must match your compiler's idea of where the
 * Frame Pointer register is. AROS's m68k gcc patches
 * set this as A5, but unpatched GCCs can be made to
 * generate correct code by setting these two defines
 * to 'A6' and '"A6"'
 */
#define __AROS_FP_REG   A5
#define __AROS_FP_SREG  "A5"

#define ___AROS_ISREG(reg,regcmp)               (0x##reg == 0x##regcmp)
#define __AROS_ISREG(type,name,reg,regcmp)      ___AROS_ISREG(reg,regcmp)

/* #defines for cpu specific asm and lib calls */
#define __AROS_ASMCALL_H_FILE "aros/m68k/asmcall.h"
#define __AROS_LIBCALL_H_FILE "aros/m68k/libcall.h"

/* Function declaration for program startup style code
 */
#define AROS_PROCH(n, _argptr, _argsize, _SysBase) \
    AROS_UFH2(SIPTR, n,                            \
            AROS_UFHA(STRPTR, _argptr, A0),        \
            AROS_UFHA(ULONG , _argsize, D0))       \
        struct ExecBase *_SysBase = *((struct ExecBase **)4);
#define AROS_PROCP(n)                              \
    AROS_UFP2(SIPTR, n,                            \
            AROS_UFPA(STRPTR, _argptr, A0),        \
            AROS_UFPA(ULONG , _argsize, D0))
#define AROS_PROCC(n, _argptr, _argsize, _SysBase) \
    AROS_UFC2(SIPTR, n,                            \
            AROS_UFCA(STRPTR, _argptr, A0),        \
            AROS_UFCA(ULONG , _argsize, D0))

#define AROS_SWAP_BYTES_LONG_CPU(l)     \
    ({ ULONG v; __asm__ __volatile__("ror.w #8,%0\nswap %0\nror.w #8,%0":"=d"(v):"0"(l)); v;})
#define AROS_SWAP_BYTES_WORD_CPU(l)     \
    ({ UWORD w; __asm__ __volatile__("ror.w #8,%d0\n":"=d"(w):"0"(l)); w;})

#endif /* AROS_M68K_CPU_H */
