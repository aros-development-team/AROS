#ifndef AROS_X86_64_CPU_H
#define AROS_X86_64_CPU_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: machine.h include file for Linux/x86_64 (work in progres...)
    Lang: english
*/

/* Information about size and alignment,
 * the defines have to be numeric constants */
#define AROS_STACK_GROWS_DOWNWARDS 1
#define AROS_BIG_ENDIAN 	   0
#define AROS_SIZEOFULONG	   4 /* Size of an ULONG */
#define AROS_SIZEOFPTR		   8 /* Size of a PTR */
#define AROS_WORDALIGN		   2 /* Alignment for WORD */
#define AROS_LONGALIGN		   4 /* Alignment for LONG */
#define AROS_QUADALIGN		   8 /* Alignment for QUAD */
#define AROS_PTRALIGN		   8 /* Alignment for PTR */
#define AROS_IPTRALIGN		   8 /* Alignment for IPTR */
#define AROS_DOUBLEALIGN	   8 /* Alignment for double */
#define AROS_WORSTALIGN 	   8 /* Worst case alignment */

/* define this if we have no support for linear varargs in the compiler */
#define NO_LINEAR_VARARGS       1

#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1

#define SIZEOF_FPU_CONTEXT      512
#define SIZEOF_ALL_REGISTERS    (184 + SIZEOF_FPU_CONTEXT + 16)

#define AROS_32BIT_TYPE		int
#define AROS_64BIT_TYPE		long
#define AROS_64BIT_STACKTYPE	long

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

/* ??? */
#define SP_OFFSET 0

/*
    One entry in a libraries' jumptable. For assembler compatibility, the
    field jmp should contain the code for an absolute jmp to a 32bit
    address. There are also a couple of macros which you should use to
    access the vector table from C.
*/
struct FullJumpVec
{
    unsigned char jmp;
    unsigned char vec[4];
};
#define __AROS_SET_FULLJMP(v,a) \
do \
{  \
    struct FullJumpVec *_v = v; \
    _v->jmp = 0xE9; \
    *((unsigned int *)(_v->vec))=(unsigned int)(a)-(unsigned int)(_v)-5;\
} while (0)


/*
    Extracts and stores the start address from a loaded
    executable segment. start_address may then be used by gdb.
    It is calculated from _v->vec set in __AROS_SET_FULLJMP.
*/
#define __AROS_SET_START_ADDR(debug_segnode)\
do \
{  \
    (debug_segnode)->start_address \
	= (IPTR)(BADDR((debug_segnode)->seglist) + 8) + 5 + *(IPTR *)((char *)(BADDR((debug_segnode)->seglist) + 8) + 1); \
} while (0)

struct JumpVec
{
    unsigned char vec[8];
};
/* Internal macros */
#define __AROS_SET_VEC(v,a)             (*(unsigned long*)(v)->vec=(unsigned long)(a))
#define __AROS_GET_VEC(v)               ((void *)(*(unsigned long*)(v)->vec))

/* Use these to acces a vector table */
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(long)(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GET_VEC(__AROS_GETJUMPVEC(lib,n)))
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_SET_VEC(__AROS_GETJUMPVEC(lib,n),(APTR)(addr)))
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

#define STUBCODE_INIT                                  \
		"#define EMITSTUB(fname, bname, vec) " \
		".weak fname ; "                       \
		"fname : "                             \
		"movq bname , %%r11; "                  \
		"jmp *vec(%%r11);\n"                   \
	        "#define EMITALIAS(fname, alias) "     \
	        ".weak alias; .set alias, fname\n"
#define STUBCODE                                       \
		"EMITSTUB(%s, %s, %d)\n"
#define ALIASCODE                                      \
                "EMITALIAS(%s, %s)\n"

/*
   We want to activate the execstubs and preserve all registers
   when calling obtainsemaphore, obtainsemaphoreshared, releasesemaphore,
   getcc, permit, forbid, enable, disable
*/
#undef UseExecstubs
#define UseExecstubs 1

/* For debugging only: Pass errnos from the emulated OS. dos/Fault() will
   recognise them */
#undef PassThroughErrnos
#define PassThroughErrnos 0x40000000

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


#define __UFC3R(_t,_n,t1,n1,r1,t2,n2,r2,t3,n3,r3,p) \
({\
    long _n1 = (long)(n1);\
    long _n2 = (long)(n2);\
    long _n3 = (long)(n3);\
    long _re;\
    __asm__ __volatile__(\
	"movq   %5,%%rdx\n\t"\
	"movq   %4,%%rsi\n\t"\
	"movq   %3,%%rdi\n\t"\
	"movq   %2,%%rcx\n\t"\
	"movl   %%esp,%1\n\t"\
	"call  *%%rcx\n\t"\
	"movl   %%eax,%0"\
	: "=m"(_re), "=m"(*(APTR *)p)\
	: "g"(_n), "g"(_n1), "g"(_n2), "g"(_n3)\
	: "cc", "memory", "%rcx", "%rax", "%rdx", "%rsi", "%rdi" );\
    (_t)_re;\
})
#define AROS_UFC3R(t,n,a1,a2,a3,p,ss) __UFC3R(t,n,a1,a2,a3,p)

#endif /* AROS_X86_64_CPU_H */
