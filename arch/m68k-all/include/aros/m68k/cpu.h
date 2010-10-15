#ifndef AROS_M68K_CPU_H
#define AROS_M68K_CPU_H

/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id: cpu.h 30792 2009-03-07 22:40:04Z neil $

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
#define AROS_WORSTALIGN 	   8 /* Worst case alignment */

#define AROS_NOFPU 1

/* do we need a function attribute to get parameters on the stack? */
#define __stackparm

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

register unsigned char * AROS_GET_SP asm("%sp");

/* ??? */
#define SP_OFFSET 0

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct JumpVec
{
    unsigned short jmp;
    unsigned char vec[4];
};


/* Any jump to an unimplemented vector will cause an access to this address */
#define _aros_empty_vector		0xc0edbabe

/* Internal macros */
#define __AROS_ASMJMP			0x4EF9
#define __AROS_SET_VEC(v,a)             (*(ULONG*)(v)->vec=(ULONG)(a))
#define __AROS_GET_VEC(v)               ((APTR)(*(ULONG*)(v)->vec))

struct FullJumpVec
{
    unsigned short jmp;
    unsigned char  vec[4];
};
#define __AROS_SET_FULLJMP(v,a)                          \
do                                                       \
{                                                        \
	struct FullJumpVec *_v = v;                      \
	_v->jmp = __AROS_ASMJMP;                         \
	*((ULONG *)(_v->vec)) = (ULONG)(a)-(ULONG)(_v)-6;\
} while(0)


/* Use these to acces a vector table */
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        ((struct JumpVec *)(((unsigned char *)lib)-(n*LIB_VECTSIZE)))
//#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(n*LIB_VECTSIZE)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GET_VEC(__AROS_GETJUMPVEC(lib,n)))
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_SET_VEC(__AROS_GETJUMPVEC(lib,n),(APTR)(addr)))
#define __AROS_INITVEC(lib,n)           __AROS_GETJUMPVEC(lib,n)->jmp = __AROS_ASMJMP, \
					__AROS_SETVECADDR(lib,n,_aros_empty_vector)


#define SIZEOF_ALL_REGISTERS		(20*4)

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

#define STUBCODE_INIT                                       \
		"#define EMITSTUB(fname, bname, vec) " \
		".globl fname ; "                      \
		"fname : "                             \
		"movl bname , %%a0; "                  \
		"movl vec(%%a0),%%a0;"                 \
		"jmp (%%a0);\n"                        \
		"#define EMITALIAS(fname, alias) "     \
		".weak alias; .set alias, fname\n"
#define STUBCODE					\
		"EMITSTUB(%s, %s, %d) "
#define ALIASCODE                              \
		"EMITALIAS(%s, %s)\n"

/*
   We want to activate the execstubs and preserve all registers
   when calling obtainsemaphore, obtainsemaphoreshared, releasesemaphore,
   getcc, permit, forbid, enable, disable
*/
#undef UseExecstubs
//#define UseExecstubs 1

/* For debugging only: Pass errnos from the emulated OS. dos/Fault() will
   recognise them */
#undef PassThroughErrnos
//#define PassThroughErrnos 0x40000000
#define PassThroughErrnos 0

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

/*
    This define is only used only M68K as some programs depend on the
    contents of the D0 register
*/
#define AROS_COMPAT_SETD0(x) \
    do { \
        register IPTR d0 __asm("d0") = x; \
        asm volatile("": : "r" d0); \
    } while(0)

/* Prototypes */
extern void _aros_not_implemented ();
extern void aros_not_implemented ();

/*
    How much stack do we need ? Lots :-) ?
    Not so much, I think (schulz) ;-))
*/

#define AROS_STACKSIZE	0x4000

/* How to map function arguments to CPU registers */
/*
    The i386 processor doesn't have enough registers to map the m68k
    register set onto them - so simply use the compiler's calling
    convention. The library base is mapped to the last argument so that
    it can be ignored by the function.
*/

/* What to do with the library base in header, prototype and call */
#define __AROS_LH_BASE(basetype,basename)   register basetype basename __asm("a6")
#define __AROS_LP_BASE(basetype,basename)   register void * __asm("a6")
#define __AROS_LC_BASE(basetype,basename)   basename
#define __AROS_LD_BASE(basetype,basename)   register basetype __asm("a6")

/* How to transform an argument in header, opt prototype, call and forced
   prototype. */
#define __AROS_LHA(type,name,reg)     register type name __asm(reg)
#define __AROS_LPA(type,name,reg)     register type __asm(reg)
#define __AROS_LCA(type,name,reg)     name
#define __AROS_LDA(type,name,reg)     register type __asm(reg)
#define __AROS_UFHA(type,name,reg)    register type name __asm(reg)
#define __AROS_UFPA(type,name,reg)    register type __asm(reg)
#define __AROS_UFCA(type,name,reg)    name
#define __AROS_UFDA(type,name,reg)    register type __asm(reg)
#define __AROS_LHAQUAD(type,name,reg1,reg2)     type name
#define __AROS_LPAQUAD(type,name,reg1,reg2)     type
#define __AROS_LCAQUAD(type,name,reg1,reg2)     name

/* Prefix for library function in header, prototype and call */
#define __AROS_LH_PREFIX    /* eps */
#define __AROS_LP_PREFIX    /* eps */
#define __AROS_LC_PREFIX    /* eps */
#define __AROS_LD_PREFIX    /* eps */
#define __AROS_UFH_PREFIX   /* eps */
#define __AROS_UFP_PREFIX   /* eps */
#define __AROS_UFC_PREFIX   /* eps */
#define __AROS_UFD_PREFIX   /* eps */


#define __UFC3R(t,n,t1,n1,r1,t2,n2,r2,t3,n3,r3,p) \
({\
    long _n1 = (long)(n1);\
    long _n2 = (long)(n2);\
    long _n3 = (long)(n3);\
    long _re;\
    __asm__ __volatile__(\
	"move.l %5,-(%%sp)\n\t"\
	"move.l %4,-(%%sp)\n\t"\
	"move.l %3,-(%%sp)\n\t"\
	"move.l %%sp,%1\n\t"\
	"move.l %2,%%a0\n\t"\
	"jsr    (%%a0)\n\t"\
	"adda.l #12,%%sp\n\t"\
	"movl   %%d0,%0"\
	: "=g"(_re), "=m"(*(APTR *)p)\
	: "ad"(n), "g"(_n1), "g"(_n2), "g"(_n3)\
	: "cc", "memory", "%d0" );\
    (t)_re;\
})
#define AROS_UFC3R(t,n,a1,a2,a3,p,ss) __UFC3R(t,n,a1,a2,a3,p)

/* Library prototypes expand to nothing */
#define __AROS_CPU_SPECIFIC_LP

#define AROS_LPQUAD1(t,n,a1,bt,bn,o,s)
#define AROS_LPQUAD2(t,n,a1,a2,bt,bn,o,s)

#define AROS_LP0(t,n,bt,bn,o,s)
#define AROS_LP1(t,n,a1,bt,bn,o,s)
#define AROS_LP2(t,n,a1,a2,bt,bn,o,s)
#define AROS_LP3(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LP4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LP5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LP6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LP7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#define AROS_LP0I(t,n,bt,bn,o,s)
#define AROS_LP1I(t,n,a1,bt,bn,o,s)
#define AROS_LP2I(t,n,a1,a2,bt,bn,o,s)
#define AROS_LP3I(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LP4I(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LP5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LP6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LP7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LP8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LP9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LP10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LP11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LP12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LP13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LP14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LP15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#endif /* AROS_M68K_CPU_H */
