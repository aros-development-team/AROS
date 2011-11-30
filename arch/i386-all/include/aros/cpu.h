#ifndef AROS_I386_CPU_H
#define AROS_I386_CPU_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: CPU-specific definitions for 32-bit x86 processors
    Lang: english
*/

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN            0 /* Big or little endian */
#define AROS_SIZEOFULONG           4 /* Size of an ULONG */
#define AROS_SIZEOFPTR             4 /* Size of a PTR */
#define AROS_WORDALIGN             2 /* Alignment for WORD */
#define AROS_LONGALIGN             4 /* Alignment for LONG */
#define AROS_QUADALIGN             4 /* Alignment for QUAD */
#define AROS_PTRALIGN              4 /* Alignment for PTR */
#define AROS_IPTRALIGN             4 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN           4 /* Alignment for double */
#define AROS_WORSTALIGN            16 /* Worst case alignment */
#define AROS_STACKALIGN           16 /* Clean stack must be aligned to this */

#define AROS_32BIT_TYPE         int

/* Use C pointer and string for the BCPL pointers and strings
 * For a normal ABI these should not be defined for maximum source code
 * compatibility.
 */
#define AROS_FAST_BPTR 1
#define AROS_FAST_BSTR 1

/* do we need a function attribute to get parameters on the stack? */
#define __stackparm

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

#ifndef __TINYC__
#define AROS_GET_SP ({register unsigned char *sp asm("%esp"); sp;})
#endif

/*
    One entry in a library's jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct FullJumpVec
{
    unsigned char jmp;
    void         *vec;
} __attribute__((packed));

#define __AROS_SET_FULLJMP(v,a) \
do \
{  \
    struct FullJumpVec *_v = v; \
    _v->jmp = 0xE9; \
    _v->vec = (void *) ((ULONG)(a) - (ULONG)(_v) - 5);\
} while (0)

struct JumpVec
{
    void *vec;
};

/* Use these to access a vector table */
#define LIB_VECTSIZE                    (sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GETJUMPVEC(lib,n)->vec)
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_GETJUMPVEC(lib,n)->vec = (addr))
#define __AROS_INITVEC(lib,n)           __AROS_SETVECADDR(lib,n,_aros_not_implemented)


/* Macros for generating library stub functions and aliases. */

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Internals: a dummy function is used that will generate some
   unused junk code but otherwise we can't pass input arguments
   to the asm statement
*/
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _wrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            #fname " :\n" \
            "\tmovl " #libbasename ", %%edx\n" \
            "\tjmp *%c0(%%edx)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
        ); \
    }
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            "\t" #fname " :\n" \
            "\tcall __comp_get_relbase\n" \
            "\taddl " #libbasename "_offset, %%eax\n" \
            "\tmovl (%%eax), %%edx\n" \
            "\tjmp *%c0(%%edx)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
        ); \
    }
#define AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_FUNCALIAS(functionname, alias)
   This macro will generate an alias 'alias' for function
   'functionname'
*/
#define __AROS_FUNCALIAS(fname, alias) \
    asm(".weak " #alias "\n" \
        "\t.set " #alias "," #fname \
    );
#define AROS_FUNCALIAS(fname, alias) \
    __AROS_FUNCALIAS(fname, alias)

/*
   We want to activate the execstubs and preserve all registers
   when calling obtainsemaphore, obtainsemaphoreshared, releasesemaphore,
   getcc, permit, forbid, enable, disable
*/
#undef UseExecstubs
#define UseExecstubs 1

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

#define AROS_STACKSIZE  40960

/* Some defines to set the cpu specific libcall.h interface */
#define __AROS_LCGCCREG_REGNAME "%edx"
#define __AROS_LCGCCREG_ASMSET "movl %1, %%edx"
#define __AROS_LIBCALL_H_FILE "aros/libcall_gccreg.h"

#endif /* AROS_I386_CPU_H */
