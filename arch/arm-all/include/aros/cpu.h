#ifndef AROS_ARM_CPU_H
#define AROS_ARM_CPU_H
/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
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
#define AROS_STACKALIGN		  16 /* FIXME: is this really needed? */

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


/* Macros for generating library stub functions and aliases. */

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Some asm trickery performed:
   - Push register arguments on stack
   - Use aros_push2_relbase to store old return address (LR register)
     and library base on alternative stack
   - Pop register arguments back
   - Call lvo vector
   - push return value on stack
   - call aros_pop2_relbase,
     return value will be old return address
   - pull return value from stack
   - jmp to old return address
   
   !!! WATCH OUT !!! LIB_VECTSIZE is hardcoded here as 4. C preprocessor
   can't expand sizeof() into a number.
*/
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo)				\
asm(										\
	    "	.weak " #fname "\n"						\
	    #fname " :\n"							\
	    /* return address is in lr register */				\
	    /* Up to four parameters are in r0 - r3 , the rest are on stack */	\
	    "	push	{r0, r1, r2, r3}\n"					\
	    /* r0 = libbase, r1 = lr */						\
	    "	ldr	r12, 1f\n"						\
            "	ldr	r0, [r12]\n"						\
	    "	mov	r1, lr\n"						\
	    /* aros_push2_relbase(r0, r1) */					\
            "	ldr	r12, 2f\n"						\
            "	blx	r12\n"							\
            /* Restore original arguments */					\
            "	pop	{r0, r1, r2, r3}\n"					\
            /* Call library function */						\
            "	ldr 	r12, 1f\n"						\
            "	ldr	r12, [r12]\n"						\
            "	ldr  	r12, [r12, #-" #lvo "* 4]\n"				\
            "	blx	r12\n"							\
            /* Push return value (possibly 64-bit one) */			\
            "	push	{r0, r1}\n"						\
	    /* lr = aros_pop2_relbase() */					\
            "	ldr	r12, 3f\n"						\
            "	blx	r12\n"							\            
            "	mov	lr, r0\n"						\
            /* Pop return value */						\
            "	pop	{r0, r1}\n"						\
            /* Return to the caller */						\
            "	bx	lr\n"							\
            "1:	.word	" #libbasename "\n"					\
            "2:	.word	aros_push2_relbase\n"					\
            "3:	.word	aros_pop2_relbase\n"					\
);
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo)				\
asm(										\
	    "	.weak " #fname "\n"						\
	    #fname " :\n"							\
	    /* return address is in lr register */				\
	    /* Up to four parameters are in r0 - r3 , the rest are on stack */	\
	    "	push	{r0, r1, r2, r3}\n"					\
	    "	push	{lr}\n"							\
	    /* r0 = aros_get_relbase() (base of currently running library) */	\
	    "	ldr	r12, 4f\n"						\
	    "	blx	r12\n"							\
	    /* r0 = libbase (we area asked for), r1 = lr (was pushed above) */	\
	    "	ldr	r12, 1f\n"						\
	    "	ldr	r12, [r12]\n"						\
	    "	ldr	r0, [r0, r12]\n"					\
	    "	pop	{r1}\n"							\
	    /* aros_push2_relbase(r0, r1) */					\
            "	ldr	r12, 2f\n"						\
            "	blx	r12\n"							\
            /* Restore original arguments */					\
            "	pop	{r0, r1, r2, r3}\n"					\
            /* Call library function */						\
            "	ldr 	r12, 1f\n"						\
            "	ldr	r12, [r12]\n"						\
            "	ldr  	r12, [r12, #-" #lvo " * 4]\n"				\
            "	blx	r12\n"							\
            /* Push return value (possibly 64-bit one) */			\
            "	push	{r0, r1}\n"						\
	    /* lr = aros_pop2_relbase() */					\
            "	ldr	r12, 3f\n"						\
            "	blx	r12\n"							\            
            "	mov	lr, r0\n"						\
            /* Pop return value */						\
            "	pop	{r0, r1}\n"						\
            /* Return to the caller */						\
            "	bx	lr\n"							\
	    "1:	.word	" #libbasename "_offset\n"				\
            "2:	.word	aros_push2_relbase\n"					\
            "3:	.word	aros_pop2_relbase\n"					\
            "4: .word	aros_get_relbase\n"					\
);
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

/*
 * This must be placed before every call to host OS on hosted AROS.
 * On ARM it tells that r9 register is clobbered. Some OSes (iOS again)
 * define r9 as volatile. It can be used as scratchpad but calls will
 * not preserve it.
 * AROS code expects it is nonvolatile (as defined in the AAPCS).
 */
#define AROS_HOST_BARRIER asm volatile("":::"r9");

#endif /* AROS_ARM_CPU_H */
