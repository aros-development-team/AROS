#ifndef AROS_PPC_CPU_H
#define AROS_PPC_CPU_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: cpu.h include file for powerpc arch
    Lang: english
*/

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

/*
    Extracts and stores the start address from a loaded
    executable segment. start_address may then be used by gdb.
*/
#define __AROS_SET_START_ADDR(debug_segnode)\
{\
    struct FullJumpVec *_v = (struct FullJumpVec *) ((debug_segnode)->seglist + 4);\
    (debug_segnode)->start_address  = (IPTR) ((_v->addis & 0x0000FFFF) << 16);     \
    (debug_segnode)->start_address |= (IPTR)  (_v->ori   & 0x0000FFFF);            \
}

struct JumpVec
{
    unsigned char vec[4];
};

/* Internal macros */
#define __AROS_SET_VEC(v,a)             (*(ULONG*)(v)->vec=(ULONG)(a))
#define __AROS_GET_VEC(v)               ((APTR)(*(ULONG*)(v)->vec))

/* Use these to acces a vector table */
#define LIB_VECTSIZE			(sizeof (struct JumpVec))
#define __AROS_GETJUMPVEC(lib,n)        (&((struct JumpVec *)lib)[-(n)])
#define __AROS_GETVECADDR(lib,n)        (__AROS_GET_VEC(__AROS_GETJUMPVEC(lib,n)))
#define __AROS_SETVECADDR(lib,n,addr)   (__AROS_SET_VEC(__AROS_GETJUMPVEC(lib,n),(APTR)(addr)))
#define __AROS_INITVEC(lib,n)		__AROS_SETVECADDR(lib,n,_aros_not_implemented)

/*
   Code used to generate stub functions.
   It must be *printed* with a function like printf into
   a file to be compiled with gcc.

   - The first parameter is the function name,
   - The second parameter is the basename,
     i.e. bname is the address of a pointer to the library base,
   - The third parameter is the library vector to be called.

   It's value must be computed by the stub generator with this code:
   &(__AROS_GETJUMPVEC(0, n+1)->vec), where n is the library vector position in
   the library vectors list.

*/
#define STUBCODE_INIT                          \
	"#define EMITSTUB(fname, bname, vec) " \
	".globl fname; "                       \
	"fname : "                             \
	"lis   11,bname@ha; "                  \
	"lwz   11,bname@l(11); "               \
	"lwz   11,vec(11); "                   \
	"mtctr 11; "                           \
	"bctr;\n "                             \
	"#define EMITALIAS(fname, alias) "     \
	".weak alias; .set alias, fname\n"
#define STUBCODE                               \
	"EMITSTUB(%s, %s, %d) "
#define ALIASCODE                              \
        "EMITALIAS(%s, %s)\n"
/*
   No, we do not want to preserve the all registers in case of Semaphore and
   multitasking handling functions. It made sence on m68k native target. On all
   other targets the standard ABI rules the AROS world anyway...
*/
#undef UseExecstubs

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
	"mr     3,%3\n\t"    \
	"mr     4,%4\n\t"    \
	"mr     5,%5\n\t"    \
	"stw    1,%1\n\t"    \
	"mflr   0\n\t"       \
	"stwu   1,-16(1)\n\t" \
	"stw    0,12(1)\n\t" \
	"mr     0,%2\n\t"    \
	"mtlr   0\n\t"       \
	"blrl\n\t"           \
	"mr     %0,3\n\t"    \
	"lwz    11,0(1)\n\t" \
	"lwz    0,-4(11)\n\t" \
	"mtlr   0\n\t"       \
	"mr     1,11\n\t"    \
	: "=r"(_re), "=m"(*(APTR *)p)\
	: "r"(_n), "r"(_n1), "r"(_n2), "r"(_n3)\
	: "cc", "memory", "0", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13");\
    (_t)_re;\
})
#define AROS_UFC3R(t,n,a1,a2,a3,p,ss) __UFC3R(t,n,a1,a2,a3,p)
#endif /* AROS_PPC_CPU_H */
