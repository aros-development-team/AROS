#ifndef AROS_X86_64_CPU_H
#define AROS_X86_64_CPU_H

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: machine.h include file for Linux/x86_64 (work in progres...)
    Lang: english
*/

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1
#define AROS_BIG_ENDIAN            0
#define AROS_SIZEOFULONG           4 /* Size of an ULONG */
#define AROS_SIZEOFPTR             8 /* Size of a PTR */
#define AROS_WORDALIGN             2 /* Alignment for WORD */
#define AROS_LONGALIGN             4 /* Alignment for LONG */
#define AROS_QUADALIGN             8 /* Alignment for QUAD */
#define AROS_PTRALIGN              8 /* Alignment for PTR */
#define AROS_IPTRALIGN             8 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN           8 /* Alignment for double */
#define AROS_WORSTALIGN            8 /* Worst case alignment */
#define AROS_STACKALIGN           16 /* Clean stack alignment */

/* define this if we have no support for linear varargs in the compiler */
#define NO_LINEAR_VARARGS       1

#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1

#define AROS_32BIT_TYPE         int
#define AROS_64BIT_TYPE         long
#define AROS_64BIT_STACKTYPE    long

#define AROS_MAKE_INT64(i)  i ## L
#define AROS_MAKE_UINT64(i) i ## UL

#define __WORDSIZE              64

#define STACKED __attribute__((aligned(8)))

/* Use C pointer and string for the BCPL pointers and strings
 * For a normal ABI these should not be defined for maximum source code
 * compatibility.
 */
#define AROS_FAST_BPTR 1
#define AROS_FAST_BSTR 1

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

#ifndef __TINYC__
register unsigned char * AROS_GET_SP __asm__("%rsp");
#endif

/*do we need a function attribute to get parameters on the stack? */
#define __stackparm

/*
 * Structure representing a jump code.
 * Used to build proper seglist header, for example by LoadSeg()
 * On x86-64 we use this code:
 *     movabsq $vec, %r11
 *     jmpq    *%r11
*/
struct FullJumpVec
{
    unsigned short movabsq;
    void          *vec;
    unsigned short jmp1;
    unsigned char  jmp2;
} __attribute__((packed));

#define __AROS_SET_FULLJMP(v,a) \
do \
{  \
    struct FullJumpVec *_v = v; \
    _v->movabsq = 0xBB49;       \
    _v->vec     = (a);          \
    _v->jmp1    = 0xFF41;       \
    _v->jmp2    = 0xE3;         \
} while (0)

/*
 * One entry in libraries' vector table.
 * On x86-64 we use vector table consisting only of pointers.
 * We do not include jump code in them.
 */
struct JumpVec
{
    void *vec;
};

/* Use these to access a vector table */
#define LIB_VECTSIZE                    ((int)sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(long)(n)])
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
            "\tmovq " #libbasename "(%%rip), %%r11\n" \
            "\tjmp  *%c0(%%r11)\n" \
            : : "i" ((-lvo)*LIB_VECTSIZE) \
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
            "\tcall __GM_GetBase\n" \
            "\taddq " #libbasename "_offset(%%rip), %%rax\n" \
            "\tmovq (%%rax),%%r11\n" \
            "\tjmp  *%c0(%%r11)\n" \
            : : "i" ((-lvo)*LIB_VECTSIZE) \
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
extern void _aros_not_implemented (char *);

/* How much stack do we need ? Lots :-) */
#define AROS_STACKSIZE  40960

/* How to map function arguments to CPU registers */
/*
    The i386 processor doesn't have enough registers to map the m68k
    register set onto them - so simply use the compiler's calling
    convention. The library base is mapped to the last argument so that
    it can be ignored by the function.
*/

/* What to do with the library base in header, prototype and call */
#define __AROS_LH_BASE(basetype,basename)   basetype basename
#define __AROS_LP_BASE(basetype,basename)   void * 
#define __AROS_LC_BASE(basetype,basename)   basename
#define __AROS_LD_BASE(basetype,basename)   basetype

/* How to transform an argument in header, opt prototype, call and forced
   prototype. */
#define __AROS_LHA(type,name,reg)     type name
#define __AROS_LPA(type,name,reg)     type
#define __AROS_LCA(type,name,reg)     name
#define __AROS_LDA(type,name,reg)     type
#define __AROS_UFHA(type,name,reg)    type name
#define __AROS_UFPA(type,name,reg)    type
#define __AROS_UFCA(type,name,reg)    name
#define __AROS_UFDA(type,name,reg)    type
#define __AROS_LHAQUAD(type,name,reg1,reg2)     type name
#define __AROS_LPAQUAD(type,name,reg1,reg2)     type
#define __AROS_LCAQUAD(type,name,reg1,reg2)     name
#define __AROS_LDAQUAD(type,name,reg1,reg2)     type

/* Prefix for library function in header, prototype and call */
#define __AROS_LH_PREFIX    /* eps */
#define __AROS_LP_PREFIX    /* eps */
#define __AROS_LC_PREFIX    /* eps */
#define __AROS_LD_PREFIX    /* eps */
#define __AROS_UFH_PREFIX   /* eps */
#define __AROS_UFP_PREFIX   /* eps */
#define __AROS_UFC_PREFIX   /* eps */
#define __AROS_UFD_PREFIX   /* eps */

#endif /* AROS_X86_64_CPU_H */
