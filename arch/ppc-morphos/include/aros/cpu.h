#ifndef AROS_MORPHOS_CPU_H
#define AROS_MORPHOS_CPU_H
/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    NOTE: This file must compile *without* any other header !

    Desc: cpu.h include file for morphos arch
    Lang: english
*/

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

/*
 * PowerPC note:
 * Some (or maybe even all) PowerPC machines expect the stack to be aligned on 16-byte boundary.
 * This is the reason number one for AROS_WORSTALIGN=16. The second reason is, the 440 CPU will
 * generate an alignment exception if floating point data *crosses* the 16-byte boundary.
 */

#define AROS_32BIT_TYPE         int
/* For ABI V1 this should be done in common cpu.h */
#define AROS_LARGEST_TYPE   long long

/* Note that MorphOS ABI DOES have linear varargs as an option */
#define __stackparm __attribute__((varargs68k))

#define AROS_SLOWSTACKTAGS      1
#define AROS_SLOWSTACKMETHODS   1
#define AROS_SLOWSTACKHOOKS     1

/* types and limits for sig_atomic_t */
#define AROS_SIG_ATOMIC_T       int
#define AROS_SIG_ATOMIC_MIN     (-0x7fffffff-1)
#define AROS_SIG_ATOMIC_MAX     0x7fffffff

#if defined(__GNUC__) && !defined(__clang__)
register unsigned char* AROS_GET_SP __asm__("%sp");
#endif

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
    unsigned short jmp;
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
#define __AROS_INITVEC(lib,n)		__AROS_GETJUMPVEC(lib,n)->jmp = TRAP_LIB, \
					__AROS_SETVECADDR(lib,n,_aros_not_implemented)

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
#define AROS_STACKSIZE	32768

/* How to map function arguments to CPU registers */

/* What to do with the library base in header, prototype and call */
#define __AROS_LH_BASE(basetype,basename)   basetype basename = (basetype)REG_A6
#define __AROS_LP_BASE(basetype,basename)   basetype __base = basename
#define __AROS_LC_BASE(basetype,basename)   REG_A6 = (LONG)__base
#define __AROS_LD_BASE(basetype,basename)   basetype basename

/* How to transform an argument in header, opt prototype, call and forced
   prototype. */
#define __AROS_LHA(type,name,reg)     type name = (type) REG_##reg
#define __AROS_LPA(type,name,reg)     type r##reg = name
#define __AROS_LCA(type,name,reg)     REG_##reg = (LONG)r##reg
#define __AROS_LDA(type,name,reg)     type
#define __AROS_UFHA(type,name,reg)    type name = (type) REG_##reg
#define __AROS_UFPA(type,name,reg)    type r##reg = name
#define __AROS_UFCA(type,name,reg)    REG_##reg = (LONG)r##reg
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

/* Library function calls. They are made through m68k emulator. */
#define __AROS_CPU_SPECIFIC_LC

/* Call a library function which requires the libbase */
#define AROS_LC0(t,n,bt,bn,o,s) \
	({ \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })

#define AROS_LC1(t,n,a1,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC2(t,n,a1,a2,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LPA(a11); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LCA(a11); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LPA(a11); \
	    __AROS_LPA(a12); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LCA(a11); \
	    __AROS_LCA(a12); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LPA(a11); \
	    __AROS_LPA(a12); \
	    __AROS_LPA(a13); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LCA(a11); \
	    __AROS_LCA(a12); \
	    __AROS_LCA(a13); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })
#define AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LPA(a11); \
	    __AROS_LPA(a12); \
	    __AROS_LPA(a13); \
	    __AROS_LPA(a14); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LCA(a11); \
	    __AROS_LCA(a12); \
	    __AROS_LCA(a13); \
	    __AROS_LCA(a14); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(- 6 *o); \
	 })
#define AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	({ \
	    __AROS_LPA(a1); \
	    __AROS_LPA(a2); \
	    __AROS_LPA(a3); \
	    __AROS_LPA(a4); \
	    __AROS_LPA(a5); \
	    __AROS_LPA(a6); \
	    __AROS_LPA(a7); \
	    __AROS_LPA(a8); \
	    __AROS_LPA(a9); \
	    __AROS_LPA(a10); \
	    __AROS_LPA(a11); \
	    __AROS_LPA(a12); \
	    __AROS_LPA(a13); \
	    __AROS_LPA(a14); \
	    __AROS_LPA(a15); \
	    __AROS_LP_BASE(bt,bn); \
	    __AROS_LCA(a1); \
	    __AROS_LCA(a2); \
	    __AROS_LCA(a3); \
	    __AROS_LCA(a4); \
	    __AROS_LCA(a5); \
	    __AROS_LCA(a6); \
	    __AROS_LCA(a7); \
	    __AROS_LCA(a8); \
	    __AROS_LCA(a9); \
	    __AROS_LCA(a10); \
	    __AROS_LCA(a11); \
	    __AROS_LCA(a12); \
	    __AROS_LCA(a13); \
	    __AROS_LCA(a14); \
	    __AROS_LCA(a15); \
	    __AROS_LC_BASE(bt,bn); \
	    (t)MyEmulHandle->EmulCallDirectOS(-6 * o); \
	 })

/* Call a library function which doesn't require the libbase */
#define AROS_LC0I(t,n,bt,bn,o,s) \
	AROS_LC0(t,n,bt,bn,o,s)
#define AROS_LC1I(t,n,a1,bt,bn,o,s) \
	AROS_LC1(t,n,a1,bt,bn,o,s)
#define AROS_LC2I(t,n,a1,a2,bt,bn,o,s) \
	AROS_LC2(t,n,a1,a2,bt,bn,o,s)
#define AROS_LC3I(t,n,a1,a2,a3,bt,bn,o,s) \
	AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s)
#define AROS_LC4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define AROS_LC5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define AROS_LC6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define AROS_LC7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define AROS_LC8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define AROS_LC9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define AROS_LC10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define AROS_LC11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define AROS_LC12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define AROS_LC13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define AROS_LC14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define AROS_LC15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

/* asmcall stuff follows. Also use m68k emulator traps as entry points. */
#define __AROS_CPU_SPECIFIC_ASMCALLS

#define AROS_ASMSYMNAME(s)	(&s##_Gate)

#if !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS))
/* Function headers for user functions */
#define AROS_UFH0(t,n) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{

#define AROS_UFH1(t,n,a1) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1);

#define AROS_UFH2(t,n,a1,a2) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2);

#define AROS_UFH3(t,n,a1,a2,a3) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3);

#define AROS_UFH3S(t,n,a1,a2,a3) \
	static __AROS_UFH_PREFIX t n##_Func (void); \
	static const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	static __AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3);

#define AROS_UFH4(t,n,a1,a2,a3,a4) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4);

#define AROS_UFH5(t,n,a1,a2,a3,a4,a5) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5);

#define AROS_UFH5S(t,n,a1,a2,a3,a4,a5) \
	__AROS_UFH_PREFIX static t n##_Func (void); \
	static const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX static t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5);

#define AROS_UFH6(t,n,a1,a2,a3,a4,a5,a6) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6);

#define AROS_UFH7(t,n,a1,a2,a3,a4,a5,a6,a7) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7);

#define AROS_UFH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8);

#define AROS_UFH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9);

#define AROS_UFH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10);

#define AROS_UFH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10); \
	__AROS_UFHA(a11);

#define AROS_UFH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10); \
	__AROS_UFHA(a11); \
	__AROS_UFHA(a12);

#define AROS_UFH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10); \
	__AROS_UFHA(a11); \
	__AROS_UFHA(a12); \
	__AROS_UFHA(a13);

#define AROS_UFH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10); \
	__AROS_UFHA(a11); \
	__AROS_UFHA(a12); \
	__AROS_UFHA(a13); \
	__AROS_UFHA(a14);

#define AROS_UFH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
	__AROS_UFH_PREFIX t n##_Func (void); \
	const struct EmulLibEntry n##_Gate = { TRAP_LIB, 0, (void(*))n##_Func }; \
	__AROS_UFH_PREFIX t n##_Func(void) \
	{ \
	__AROS_UFHA(a1); \
	__AROS_UFHA(a2); \
	__AROS_UFHA(a3); \
	__AROS_UFHA(a4); \
	__AROS_UFHA(a5); \
	__AROS_UFHA(a6); \
	__AROS_UFHA(a7); \
	__AROS_UFHA(a8); \
	__AROS_UFHA(a9); \
	__AROS_UFHA(a10); \
	__AROS_UFHA(a11); \
	__AROS_UFHA(a12); \
	__AROS_UFHA(a13); \
	__AROS_UFHA(a14); \
	__AROS_UFHA(a15);

/* Call a user function */
#define AROS_UFC0(t,n) \
	((t)MyEmulHandle->EmulCallDirect68k((APTR)(n)))

#define AROS_UFC1(t,n,a1) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFCA(a1); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	 })

#define AROS_UFC2(t,n,a1,a2) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC3(t,n,a1,a2,a3) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC4(t,n,a1,a2,a3,a4) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC5(t,n,a1,a2,a3,a4,a5) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC6(t,n,a1,a2,a3,a4,a5,a6) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC7(t,n,a1,a2,a3,a4,a5,a6,a7) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFPA(a11); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    __AROS_UFCA(a11); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFPA(a11); \
	    __AROS_UFPA(a12); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    __AROS_UFCA(a11); \
	    __AROS_UFCA(a12); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFPA(a11); \
	    __AROS_UFPA(a12); \
	    __AROS_UFPA(a13); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    __AROS_UFCA(a11); \
	    __AROS_UFCA(a12); \
	    __AROS_UFCA(a13); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFPA(a11); \
	    __AROS_UFPA(a12); \
	    __AROS_UFPA(a13); \
	    __AROS_UFPA(a14); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    __AROS_UFCA(a11); \
	    __AROS_UFCA(a12); \
	    __AROS_UFCA(a13); \
	    __AROS_UFCA(a14); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#define AROS_UFC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
	({ \
	    __AROS_UFPA(a1); \
	    __AROS_UFPA(a2); \
	    __AROS_UFPA(a3); \
	    __AROS_UFPA(a4); \
	    __AROS_UFPA(a5); \
	    __AROS_UFPA(a6); \
	    __AROS_UFPA(a7); \
	    __AROS_UFPA(a8); \
	    __AROS_UFPA(a9); \
	    __AROS_UFPA(a10); \
	    __AROS_UFPA(a11); \
	    __AROS_UFPA(a12); \
	    __AROS_UFPA(a13); \
	    __AROS_UFPA(a14); \
	    __AROS_UFPA(a15); \
	    __AROS_UFCA(a1); \
	    __AROS_UFCA(a2); \
	    __AROS_UFCA(a3); \
	    __AROS_UFCA(a4); \
	    __AROS_UFCA(a5); \
	    __AROS_UFCA(a6); \
	    __AROS_UFCA(a7); \
	    __AROS_UFCA(a8); \
	    __AROS_UFCA(a9); \
	    __AROS_UFCA(a10); \
	    __AROS_UFCA(a11); \
	    __AROS_UFCA(a12); \
	    __AROS_UFCA(a13); \
	    __AROS_UFCA(a14); \
	    __AROS_UFCA(a15); \
	    (t)MyEmulHandle->EmulCallDirect68k((APTR)(n)); \
	})

#endif /* !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS)) */

/* Prototypes for user functions */
#   define AROS_UFP0(t,n) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP1(t,n,a1) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP2(t,n,a1,a2) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP3(t,n,a1,a2,a3) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP3S(t,n,a1,a2,a3) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP4(t,n,a1,a2,a3,a4) \
	const struct EmulLibEntry Xj(n,_Gate);

#   define AROS_UFP5(t,n,a1,a2,a3,a4,a5) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP5S(t,n,a1,a2,a3,a4,a5) \
	extern const struct EmulLibEntry n##_Gate;
#   define AROS_UFP6(t,n,a1,a2,a3,a4,a5,a6) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP7(t,n,a1,a2,a3,a4,a5,a6,a7) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
	extern const struct EmulLibEntry n##_Gate;

#   define AROS_UFP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
	extern const struct EmulLibEntry n##_Gate;

#endif /* AROS_MORPHOS_CPU_H */
