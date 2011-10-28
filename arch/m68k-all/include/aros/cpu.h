#ifndef AROS_M68K_CPU_H
#define AROS_M68K_CPU_H

#include <aros/config.h>

/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: machine.h
    Lang: english
*/

#define EnableSetFunction	1

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1 /* Stack direction */
#define AROS_BIG_ENDIAN 	   1 /* Big or little endian */
#define AROS_SIZEOFULONG	   4 /* Size of an ULONG */
#define AROS_SIZEOFPTR		   4 /* Size of a PTR */
#define AROS_WORDALIGN		   2 /* Alignment for WORD */
#define AROS_LONGALIGN		   2 /* Alignment for LONG */
#define AROS_QUADALIGN		   2 /* Alignment for QUAD */
#define AROS_PTRALIGN		   2 /* Alignment for PTR */
#define AROS_IPTRALIGN		   2 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN	   2 /* Alignment for double */
#define AROS_WORSTALIGN 	   4 /* Worst case alignment */

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
#define _aros_empty_vector		0xc0ed0000

/* Internal macros */
#define __AROS_ASMJMP			0x4EF9
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
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&(((struct JumpVec *)(lib))[-(n)]))
#define __AROS_GETVECADDR(lib,n)        ((void *)__AROS_GETJUMPVEC(lib,n))
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_SET_VEC(__AROS_GETJUMPVEC(lib,n),(APTR)(addr)))
#define __AROS_INITVEC(lib,n)           __AROS_GETJUMPVEC(lib,n)->jmp = __AROS_ASMJMP, \
					__AROS_SETVECADDR(lib,n,_aros_empty_vector | (n << 8) | 1)


#define SIZEOF_ALL_REGISTERS		(20*4)

#define KERNEL_PANIC_BUFFER (char*)0x800

/* Macros for generating library stub functions and aliases. */

/* Macro: AROS_LIBFUNCSTUB(functionname, libbasename, lvo)
   This macro will generate code for a stub function for
   the function 'functionname' of lirary with libbase
   'libbasename' and 'lvo' number of the function in the
   vector table. lvo has to be a constant value (not a variable)

   Internals: a dummy function is used that will generate some
   unused junk code but otherwise we can't pass input arguments
   to the asm statement

   Some asm trickery performed:
   - function return address is top of stack,
     leave it there to use as argument for aros_push2_relbase
   - libbase is other argument to aros_push2_relbase
   - call aros_push2_relbase
   - put libbase back in %eax
   - remove libbase and return address from stack
   - Call lvo vector
   - push return value on stack
   - call aros_pop2_relbase,
     return value will be old return address
   - pull return value from stack
   - jmp to old return address
*/
#define __AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _wrapper(void) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
	    #fname ":\n" \
	    "\tmove.l	" #libbasename ",%%a1\n" \
	    "\tjmp %%a1@(%c0)\n" \
	    : : "i" ((-lvo*LIB_VECTSIZE)) \
	    : \
        ); \
    }
#define AROS_LIBFUNCSTUB(fname, libbasename, lvo) \
    __AROS_LIBFUNCSTUB(fname, libbasename, lvo)

/* Macro: AROS_RELLIBFUNCSTUB(functionname, libbasename, lvo)
   Same as AROS_LIBFUNCSTUB but finds libbase at an offset in
   the current libbase
*/
#define __AROS_RELLIBFUNCSTUB(fname, libbasename, lvo) \
    void __ ## fname ## _ ## libbasename ## _relwrapper(IPTR args) \
    { \
        asm volatile( \
            ".weak " #fname "\n" \
            #fname " :\n" \
            "\tjsr __comp_get_relbase\n" \
            "\tmove.l " #libbasename "_offset, %%a0\n" \
            "\tmove.l %%a0@(%%d0), %%a1\n" \
            "\tjmp %%a1@(%c0)\n" \
            : : "i" ((-lvo*LIB_VECTSIZE)) \
            : \
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

#define AROS_STACKSIZE	0x4000

//#define AROS_NEEDS___MAIN

/* How to map function arguments to CPU registers */

/* Prefix for library function in header, prototype and call */
#define __AROS_LH_PREFIX    /* eps */
#define __AROS_LP_PREFIX    /* eps */
#define __AROS_LC_PREFIX    /* eps */
#define __AROS_LD_PREFIX    /* eps */
#define __AROS_UFH_PREFIX   /* eps */
#define __AROS_UFP_PREFIX   /* eps */
#define __AROS_UFC_PREFIX   /* eps */
#define __AROS_UFD_PREFIX   /* eps */

#define AROS_COMPAT_SETD0(x)	do { asm volatile ( "move.l %0,%%d0\n" : "=g" (x)); return; } while (0)

#define A0	a0
#define A1	a1
#define A2	a2
#define A3	a3
#define A4	a4
#define A5	a5
#define A6	a6
#define D0	d0
#define D1	d1
#define D2	d2
#define D3	d3
#define D4	d4
#define D5	d5
#define D6	d6
#define D7	d7

#ifdef CONFIG_GCC_FP_A6
#define __AROS_FP_REG	A6
#define __AROS_FP_SREG	"A6"
#else
#define __AROS_FP_REG	A5
#define __AROS_FP_SREG	"A5"
#endif

#define ___AROS_ISREG(reg,regcmp)		(0x##reg == 0x##regcmp)
#define __AROS_ISREG(type,name,reg,regcmp)	___AROS_ISREG(reg,regcmp)

/* What to do with the library base in header, prototype and call */
#define __AROS_LH_BASE(basetype,basename)   basetype basename
#define __AROS_LP_BASE(basetype,basename)   basetype
#define __AROS_LC_BASE(basetype,basename)   basename
#define __AROS_LD_BASE(basetype,basename)   basetype

/* Get the register from a triplet */
#define __AROS_LRA(type,name,reg)             reg
#define __AROS_UFRA(type,name,reg)            reg
#define __AROS_LRAQUAD1(type,name,reg1,reg2)  reg1
#define __AROS_LRAQUAD2(type,name,reg1,reg2)  reg2

/* Temporary variables */
#define __AROS_LTA(type,name,reg)             reg##_tmp
#define __AROS_UFTA(type,name,reg)            reg##_tmp
#define __AROS_LTAQUAD(type,name,reg1,reg2)   reg1##_##reg2##_tmp
#define __AROS_LTAQUAD1(type,name,reg1,reg2)  reg1##_tmp
#define __AROS_LTAQUAD2(type,name,reg1,reg2)  reg2##_tmp

/* Get the register as a string from the triplet */
#define __AROS_LSA(type,name,reg)             "%"#reg
#define __AROS_UFSA(type,name,reg)            "%"#reg
#define __AROS_LSAQUAD1(type,name,reg1,reg2)  "%"#reg1
#define __AROS_LSAQUAD2(type,name,reg1,reg2)  "%"#reg2

/* How to transform an argument in header, opt prototype, call and forced
   prototype. */
#define __AROS_LHA(type,name,reg)     type name
#define __AROS_LPA(type,name,reg)     type
#define __AROS_LCA(type,name,reg)     (name)
#define __AROS_LDA(type,name,reg)     type
#define __AROS_UFHA(type,name,reg)    type name
#define __AROS_UFPA(type,name,reg)    type
#define __AROS_UFCA(type,name,reg)    (name)
#define __AROS_UFDA(type,name,reg)    type
#define __AROS_LHAQUAD(type,name,reg1,reg2)     type name
#define __AROS_LPAQUAD(type,name,reg1,reg2)     type
#define __AROS_LCAQUAD(type,name,reg1,reg2)     (name)
#define __AROS_LDAQUAD(type,name,reg1,reg2)     type

/* Call a libary function which requires the libbase */
#include <aros/m68k/libcall.h>

#define AROS_LC_CALL(t,x,bn)	ERROR IN DEFINITIONS - AROS_LC_CALL
#define AROS_LC_CALLNR(x,bn)	ERROR IN DEFINITIONS - AROS_LC_CALLNR
#define AROS_LC_CALLI(t,x,bn)	ERROR IN DEFINITIONS - AROS_LC_CALLI
#define AROS_LC_CALLINR(x,bn)	ERROR IN DEFINITIONS - AROS_LC_CALLNR

#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
	AROS_LH2(t,n, \
		AROS_LHA(ULONG, __AROS_LTAQUAD1(a1), __AROS_LRAQUAD1(a1)), \
		AROS_LHA(ULONG, __AROS_LTAQUAD2(a1), __AROS_LRAQUAD2(a1)), \
		bt, bn, o, s) \
		union { \
			__AROS_LPAQUAD(a1) val; \
			ULONG reg[2]; \
		} __AROS_LTAQUAD(a1); \
		__AROS_LTAQUAD(a1).reg[0] = __AROS_LTAQUAD1(a1); \
		__AROS_LTAQUAD(a1).reg[1] = __AROS_LTAQUAD2(a1); \
		__AROS_LPAQUAD(a1) __attribute__((unused)) __AROS_LCAQUAD(a1) = __AROS_LTAQUAD(a1).val;

#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
	AROS_LH4(t,n, \
		AROS_LHA(ULONG, __AROS_LTAQUAD1(a1), __AROS_LRAQUAD1(a1)), \
		AROS_LHA(ULONG, __AROS_LTAQUAD2(a1), __AROS_LRAQUAD2(a1)), \
		AROS_LHA(ULONG, __AROS_LTAQUAD1(a2), __AROS_LRAQUAD1(a2)), \
		AROS_LHA(ULONG, __AROS_LTAQUAD2(a2), __AROS_LRAQUAD2(a2)), \
		bt, bn, o, s) \
		union { \
			__AROS_LPAQUAD(a1) val; \
			ULONG reg[2]; \
		} __AROS_LTAQUAD(a1); \
		union { \
			__AROS_LPAQUAD(a2) val; \
			ULONG reg[2]; \
		} __AROS_LTAQUAD(a2); \
		__AROS_LTAQUAD(a1).reg[0] = __AROS_LTAQUAD1(a1); \
		__AROS_LTAQUAD(a1).reg[1] = __AROS_LTAQUAD2(a1); \
		__AROS_LPAQUAD(a1) __attribute__((unused)) __AROS_LCAQUAD(a1) = __AROS_LTAQUAD(a1).val; \
		__AROS_LTAQUAD(a2).reg[0] = __AROS_LTAQUAD1(a2); \
		__AROS_LTAQUAD(a2).reg[1] = __AROS_LTAQUAD2(a2); \
		__AROS_LPAQUAD(a2) __attribute__((unused)) __AROS_LCAQUAD(a2) = __AROS_LTAQUAD(a2).val;

#define AROS_LH1QUAD1(t,n,a1,a2,bt,bn,o,s) \
	AROS_LH3(t,n, \
		AROS_LHA(a1), \
		AROS_LHA(ULONG, __AROS_LTAQUAD1(a2), __AROS_LRAQUAD1(a2)), \
		AROS_LHA(ULONG, __AROS_LTAQUAD2(a2), __AROS_LRAQUAD2(a2)), \
		bt, bn, o, s) \
		union { \
			__AROS_LPAQUAD(a2) val; \
			ULONG reg[2]; \
		} __AROS_LTAQUAD(a2); \
		__AROS_LTAQUAD(a2).reg[0] = __AROS_LTAQUAD1(a2); \
		__AROS_LTAQUAD(a2).reg[1] = __AROS_LTAQUAD2(a2); \
		__AROS_LPAQUAD(a2) __attribute__((unused)) __AROS_LCAQUAD(a2) = __AROS_LTAQUAD(a2).val;


#define AROS_LCQUAD1(t,n,a1,bt,bn,o,s) \
	({ \
		union { \
			__AROS_LPAQUAD(a1) val; \
			ULONG reg[2]; \
		} _q1 = { .val = __AROS_LCAQUAD(a1) }; \
	 	AROS_LC2##t(t, n,  \
	 		AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(a1)), \
	 		AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(a1)), \
	 		bt, bn, o, s); \
	 })

#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s) \
	({ \
		union { \
			__AROS_LPAQUAD(a1) val; \
			ULONG reg[2]; \
		} _q1 = { .val = __AROS_LCAQUAD(a1) }; \
		union { \
			__AROS_LPAQUAD(a2) val; \
			ULONG reg[2]; \
		} _q2 = { .val = __AROS_LCAQUAD(a2) }; \
	 	AROS_LC4##t(t, n,  \
	 		AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(a1)), \
	 		AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(a1)), \
	 		AROS_LCA(ULONG, _q2.reg[0], __AROS_LRAQUAD1(a2)), \
	 		AROS_LCA(ULONG, _q2.reg[1], __AROS_LRAQUAD2(a2)), \
	 		bt, bn, o, s); \
	 })

#define AROS_LC1QUAD1(t,n,a1,a2,bt,bn,o,s) \
	({ \
		union { \
			__AROS_LPAQUAD(a2) val; \
			ULONG reg[2]; \
		} _q1 = { .val = __AROS_LCAQUAD(a2) }; \
	 	AROS_LC3##t(t, n,  \
	 		AROS_LCA(a1), \
	 		AROS_LCA(ULONG, _q1.reg[0], __AROS_LRAQUAD1(a2)), \
	 		AROS_LCA(ULONG, _q1.reg[1], __AROS_LRAQUAD2(a2)), \
	 		bt, bn, o, s); \
	 })

#define AROS_LC2double AROS_LC2D
#define AROS_LC3double AROS_LC3D
#define AROS_LC4double AROS_LC4D
#define AROS_LC2LONG   AROS_LC2
#define AROS_LC3LONG   AROS_LC3
#define AROS_LC4LONG   AROS_LC4

#   define AROS_LDQUAD1(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1), __AROS_LD_BASE(bt,bn))
#   define AROS_LDQUAD2(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1), \
	__AROS_LDAQUAD(a2),__AROS_LD_BASE(bt,bn))

#define AROS_LPQUAD1(t,n,a1,bt,bn,o,s) \
		t n (__AROS_LPAQUAD(a1))
#define AROS_LPQUAD2(t,n,a1,a2,bt,bn,o,s) \
		t n (__AROS_LPAQUAD(a1), __AROS_LPAQUAD(a2))

/* Function declaration for program startup style code
 */
#define AROS_ENTRY(t, n, a1, a2, bt, bn)	\
    AROS_UFH2(t, n, AROS_UFHA(a1), AROS_UFHA(a2)) \
    	bt bn = *((bt *)4);

#endif /* AROS_M68K_CPU_H */
