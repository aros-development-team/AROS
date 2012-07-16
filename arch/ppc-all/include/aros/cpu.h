#ifndef AROS_PPC_CPU_H
#define AROS_PPC_CPU_H
/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: cpu.h include file for powerpc arch
    Lang: english
*/

/*
 * Some compilers may not provide these legacy defines
 * but many code out there relies on them
 */
#ifndef __ppc__
#define __ppc__
#endif
#ifndef __PPC__
#define __PPC__
#endif

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN 	   1 /* Big or little endian */
#define AROS_SIZEOFULONG	   4 /* Size of an ULONG */
#define AROS_SIZEOFPTR		   4 /* Size of a PTR */
#define AROS_WORDALIGN		   2 /* Alignment for WORD */
#define AROS_LONGALIGN		   4 /* Alignment for LONG */
#define AROS_QUADALIGN		   8 /* Alignment for QUAD */
#define AROS_PTRALIGN		   4 /* Alignment for PTR */
#define AROS_IPTRALIGN		   4 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN	   4 /* Alignment for double */
#define AROS_WORSTALIGN 	  16 /* Worst case alignment */
#define AROS_STACKALIGN		  16 /* Stack alignment, see note below */

/*
 * PowerPC note:
 * Some (or maybe even all) PowerPC machines expect the stack to be aligned on 16-byte boundary.
 * This is the reason number one for AROS_WORSTALIGN=16. The second reason is, the 440 CPU will
 * generate an alignment exception if floating point data *crosses* the 16-byte boundary.
 */

#define AROS_32BIT_TYPE         int
/* For ABI V1 this should be done in common cpu.h */
#define AROS_LARGEST_TYPE   long long

/* Use C pointer and string for the BCPL pointers and strings
 * For a normal ABI these should not be defined for maximum source code
 * compatibility.
 */
#define AROS_FAST_BPTR 1
#define AROS_FAST_BSTR 1

/* do we need a function attribute to get parameters on the stack? */
#define __stackparm

/* define this if we have no support for linear varargs in the compiler */
#define NO_LINEAR_VARARGS 1

#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

register unsigned char* AROS_GET_SP __asm__("%sp");

/*
     An offset value sometimes added to
     or subtracted from the stack limits.
*/

#define SP_OFFSET 0x10

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct FullJumpVec
{
    unsigned long addis; // simplified: lis   11, a@h
    unsigned long ori;   //           : li    11, a@l
    unsigned long mtspr; //           : mtctr 11
    unsigned long jmp;   // bcctr     : bctr  11
};
/* browse MPCFPE32B.pdf to understand opcode creation */
#define __AROS_SET_FULLJMP(v,a)\
{\
    struct FullJumpVec *_v = (v);                                                \
    _v->addis = (15 << 26) | (11 << 21) | ((ULONG)(a) >> 16);                    \
    _v->ori   = (24 << 26) | (11 << 21) | (11 << 16) | ((ULONG)(a) & 0x0000FFFF);\
    _v->mtspr = (31 << 26) | (11 << 21) | ( 9 << 16) | (467 << 1);               \
    _v->jmp   = (19 << 26) | (20 << 21) | (528 << 1);                            \
}

struct JumpVec
{
    void *vec;
};

/* Use these to access a vector table */
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GETJUMPVEC(lib,n)->vec)
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_GETJUMPVEC(lib,n)->vec = (addr))
#define __AROS_INITVEC(lib,n)		__AROS_SETVECADDR(lib,n,_aros_not_implemented)
#define __AROS_USE_FULLJMP

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
	    "\t" #fname ":\n" \
	    "\tlis   12," #libbasename "@ha\n" \
	    "\tlwz   12," #libbasename "@l(12)\n" \
	    "\tlwz   11,%c0(12)\n" \
	    "\tmtctr 11\n" \
	    "\tbctr\n" \
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
	    "\t" #fname ":\n" \
            "\tbl   __GM_GetBase\n" \
	    "\tlis  11," #libbasename "_offset@ha\n" \
	    "\tlwz  11," #libbasename "_offset@l(11)\n" \
            "\tadd  11, 11, 3\n" \
	    "\tlis  12, %c0@ha\n" \
	    "\tlwz  12, %c0@l(12)\n" \
	    "\tadd  11, 11, 12\n" \
	    "\tmtctr 11\n" \
	    "\tbctr\n" \
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
   No, we do not want to preserve the all registers in case of Semaphore and
   multitasking handling functions. It made sence on m68k native target. On all
   other targets the standard ABI rules the AROS world anyway...
*/
#undef UseExecstubs

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
#define AROS_STACKSIZE	40960

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

#endif /* AROS_PPC_CPU_H */
