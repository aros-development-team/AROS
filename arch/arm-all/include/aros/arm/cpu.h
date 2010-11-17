#ifndef AROS_ARM_CPU_H
#define AROS_ARM_CPU_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: cpu.h include file for arm-le systems
    Lang: english
*/

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN 	   0 /* Big or little endian */
#define AROS_SIZEOFULONG	   4 /* Size of an ULONG */
#define AROS_SIZEOFPTR		   4 /* Size of a PTR */
#define AROS_WORDALIGN		   2 /* Alignment for WORD */
#define AROS_LONGALIGN		   4 /* Alignment for LONG */
#define AROS_QUADALIGN		   4 /* Alignment for QUAD */
#define AROS_PTRALIGN		   4 /* Alignment for PTR */
#define AROS_IPTRALIGN		   4 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN	   4 /* Alignment for double */
#define AROS_WORSTALIGN 	  16 /* Worst case alignment */

/* define this if we have no support for linear varargs in the compiler */
#define NO_LINEAR_VARARGS       1

/* Do not use patched compiler. */
#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1

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

register unsigned char* AROS_GET_SP __asm__("%sp");

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct FullJumpVec
{
    unsigned long jmp;
    unsigned long vec;
};
#define __AROS_SET_FULLJMP(v,a) \
do \
{  \
    struct FullJumpVec *_v = (v); \
    _v->jmp = 0xe51ff004; 		/* ldr pc, [pc, #-4] */ 	\
    _v->vec = (ULONG)(a); 		/* .word target_address */ 	\
} while (0)

struct JumpVec
{
	void * vec;
};

/* Use these to acces a vector table */
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GETJUMPVEC(lib,n)->vec)
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_GETJUMPVEC(lib,n)->vec = (addr))
#define __AROS_INITVEC(lib,n)		__AROS_SETVECADDR(lib,n,_aros_not_implemented)

/*
   Code to use to generate stub functions.
   It must be *printed* with a function like printf in a file
   to be compiled with gcc.

   - The first parameter is the function name,
   - The second parameter is the basename,
   - The third parameter is the library vector to be called.
     It's value must be computed by the stub generator with this code:
     &(__AROS_GETJUMPVEC(0, n+1)->vec), where n is the library vector position in
     the library vectors list.

*/

#define STUBCODE_INIT                          \
		"#define EMITSTUB(fname, bname, vec) " \
		".weak fname ; "                       \
		"fname : "                             \
		"ldr  r12, 1f; "                       \
		"ldr  r12, [r12]; "                    \
		"ldr  pc , [r12, $ vec ]; "            \
		"1: .word bname;\n"                    \
		"#define EMITALIAS(fname, alias) "     \
		".weak alias; .set alias, fname\n"
#define STUBCODE                               \
		"EMITSTUB(%s, %s, %d)\n"
#define ALIASCODE                              \
		"EMITALIAS(%s, %s)\n"

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
extern void _aros_not_implemented ();
extern void aros_not_implemented ();

/* How much stack do we need ? Lots :-) */
#define AROS_STACKSIZE	40960

/* How to map function arguments to CPU registers */
/*
    The ARM processor does have enough registers to map the m68k
    register set onto them but we will ignore this and use the compiler's calling
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

/* Untested! This macro *may* be wrong... */
#define __UFC3R(t,n,t1,n1,r1,t2,n2,r2,t3,n3,r3,p) \
({\
    register long _n1 asm("r0") = (long)(n1);\
    register long _n2 asm("r1") = (long)(n2);\
    register long _n3 asm("r2") = (long)(n3);\
    register long _re asm("r0");\
    __asm__ __volatile__(\
        "ldr    %%ip, %1\n\t"\
        "str    %%sp,[%%ip, $0]\n\t"\
        "blx    %2\n\t"\
        : "=r"(_re), "=m"(*(int *)p)\
        : "r"(n), "r"(_n1), "r"(_n2), "r"(_n3)\
        : "cc", "memory");\
    (t)_re;\
})
#define AROS_UFC3R(t,n,a1,a2,a3,p,ss) __UFC3R(t,n,a1,a2,a3,p)

/*
 * This must be placed before every call to host OS on hosted AROS.
 * On ARM it tells that r9 register is clobbered. Some OSes (iOS again)
 * define r9 as volatile. It can be used as scratchpad but calls will
 * not preserve it.
 * AROS code expects it is nonvolatile (as defined in the AAPCS).
 */
#define AROS_HOST_BARRIER asm volatile("":::"r9");

#endif /* AROS_ARM_CPU_H */
