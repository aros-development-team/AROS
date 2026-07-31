#ifndef AROS_RISCV64_CPU_H
#define AROS_RISCV64_CPU_H

/*
    Copyright (c) 2026, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: cpu.h include file for risc-v (RV64, LP64D) systems
    Lang: english
*/

typedef	unsigned int	cpuid_t;
typedef	unsigned int	cpumask_t;

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN 	   0 /* Big or little endian */
#define AROS_SIZEOFULONG	   4 /* Size of an ULONG */
#define AROS_SIZEOFPTR		   8 /* Size of a PTR */
#define AROS_WORDALIGN		   2 /* Alignment for WORD */
#define AROS_LONGALIGN		   4 /* Alignment for LONG */
#define AROS_QUADALIGN		   8 /* Alignment for QUAD */
#define AROS_PTRALIGN		   8 /* Alignment for PTR */
#define AROS_IPTRALIGN		   8 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN	   8 /* Alignment for double */
#define AROS_WORSTALIGN 	  16 /* Worst case alignment */
#define AROS_STACKALIGN		  16 /* The RISC-V psABI requires a 16-byte aligned stack */

/* define this if we have no support for linear varargs in the compiler */
#define NO_LINEAR_VARARGS       1

/* Do not use patched compiler. */
#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1
#define AROS_SLOWSTACKFORMAT    1

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

/* do we need a function attribute to get parameters on the stack? */
#define __stackparm

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

#if defined(__clang__)
/* clang has no global "sp" register variable on RISC-V; read it inline. */
#define AROS_GET_SP ({ unsigned char *__sp; __asm__ __volatile__("mv %0, sp" : "=r"(__sp)); __sp; })
#elif defined(__GNUC__)
register unsigned char* AROS_GET_SP __asm__("sp");
#endif

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 64bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct FullJumpVec
{
    unsigned int  jmp[4];   /* auipc t0,0 ; ld t0,16(t0) ; jr t0 ; nop */
    unsigned long vec;      /* 64-bit absolute target address          */
};
/* RISC-V has no full-width absolute jump instruction, so the trampoline
   loads the target (stored in 'vec', at offset 16 from the trampoline
   start) into t0 via an AUIPC-relative load and jumps to it. The caller
   must flush the instruction cache (fence.i / CacheClearE) afterwards. */
#define __AROS_SET_FULLJMP(v,a) \
do \
{  \
    struct FullJumpVec *_v = (v); \
    _v->jmp[0] = 0x00000297;	/* auipc t0, 0      */	\
    _v->jmp[1] = 0x0102B283;	/* ld    t0, 16(t0) */	\
    _v->jmp[2] = 0x00028067;	/* jr    t0         */	\
    _v->jmp[3] = 0x00000013;	/* nop (pad)        */	\
    _v->vec    = (unsigned long)(a); \
} while (0)

struct JumpVec
{
	void * vec;
};

/* Use these to acces a vector table */
#define LIB_VECTSIZE			((int)sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(long)(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GETJUMPVEC(lib,n)->vec)
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_GETJUMPVEC(lib,n)->vec = (addr))
#define __AROS_INITVEC(lib,n)		__AROS_SETVECADDR(lib,n,_aros_not_implemented)

/* Register preservation is still useful. At least for stack swapping functions. */
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

/* How much stack do we need ? Lots :-) */
#define AROS_STACKSIZE	40960

/* How to map function arguments to CPU registers */
/*
    RISC-V has enough registers to map the m68k register set onto them,
    but we will ignore this and use the compiler's calling convention.
    The library base is mapped to the last argument so that it can be
    ignored by the function.
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
#define __AROS_LHA2(type,name,reg1,reg2)     type name
#define __AROS_LPA2(type,name,reg1,reg2)     type
#define __AROS_LCA2(type,name,reg1,reg2)     name
#define __AROS_LDA2(type,name,reg1,reg2)     type

/* Prefix for library function in header, prototype and call */
#define __AROS_LH_PREFIX    /* eps */
#define __AROS_LP_PREFIX    /* eps */
#define __AROS_LC_PREFIX    /* eps */
#define __AROS_LD_PREFIX    /* eps */
#define __AROS_UFH_PREFIX   /* eps */
#define __AROS_UFP_PREFIX   /* eps */
#define __AROS_UFC_PREFIX   /* eps */
#define __AROS_UFD_PREFIX   /* eps */

#define AROS_HOST_BARRIER

#define AROS_SWAP_BYTES_LONG_CPU(l)     __builtin_bswap32(l)
#define AROS_SWAP_BYTES_WORD_CPU(l)     __builtin_bswap16(l)

#endif /* AROS_RISCV64_CPU_H */
