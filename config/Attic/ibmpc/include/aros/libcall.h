#ifndef AROS_LIBCALL_H
#define AROS_LIBCALL_H
/* (C) 1995 AROS - The Amiga Replacement OS */

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
	Some macros to build functions with registerized parameters on the
	different compilers around. Usage:

	AROS_LH<n><f>(type, name, type1,  name1, reg1, ...)
	{
	    \* Function starts here. *\
	}

	<n> - Number of arguments of the function (not including the
	    library base).

	<f> - 'I' means: Function ignores library base.
	    This is useful to get rid of warnings about unused arguments.

	type - Returntype of the function.

	name - Name of the function. A underscore '_' is prepended so that
	    following functions jump over the base vector and don't call
	    the function directly.

	type<i>, name<i>, reg<i> - Type, name and register for the
	    arguments. Register names are written uppercase because they
	    are preprocessor symbols.

	Example: Define a Exec compatible RemHead function.

	AROS_LH1I(struct Node *, RemHead,
		struct List *, list, A0,
		struct ExecBase, SysBase, 43, Exec)
	{
	    // Hier *keine* ";" !!
	    AROS_LIBFUNC_INIT
	    // Das ist eigentlich nicht notwendig, da die Funktion
	    // SysBase nicht verwendet, aber ist ja nur ein Demo :-)
	    AROS_LIBBASE_EXT_DECL(struct ExecBase, SysBase)

	    struct Node *node;

	    node=list->lh_Head;
	    if(node->ln_Succ==NULL)
		return NULL;
	    Remove(node);
	    return node;

	    // NACH return !!
	    AROS_LIBFUNC_EXIT
	}

******************************************************************************/

/* System-Specific things */
#ifndef AROS_SYSTEM_H
#   include <aros/system.h>
#endif

#ifndef __typedef_VOID_FUNC
#define __typedef_VOID_FUNC
typedef void (*VOID_FUNC)();
#endif
#ifndef __typedef_LONG_FUNC
#define __typedef_LONG_FUNC
typedef long (*LONG_FUNC)();
#endif
#ifndef __typedef_ULONG_FUNC
#define __typedef_ULONG_FUNC
typedef unsigned long (*ULONG_FUNC)();
#endif

#ifndef __AROS_SLIB_ENTRY
#   define __AROS_SLIB_ENTRY(n,s)	s ## _ ## n
#endif
#ifndef AROS_SLIB_ENTRY
/*
    This is to allow the C preprocessor to expand n and s before they are
    maybe concatenated (the CPP first evaluates a ## b and the tries
    to expand ab instead of expanding a and b and then concatenating the
    results
*/
#   define AROS_SLIB_ENTRY(n,s) __AROS_SLIB_ENTRY(n,s)
#endif

/*
    I've removed some #if sections because this file will be used only with
    standalone AROS under PC
*/

/* Library functions which need the libbase */

#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)( \
	__AROS_LHQUAD(a1),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)( \
	__AROS_LHQUAD(a1),\
	__AROS_LHQUAD(a2),\
	__AROS_LH_BASE(bt,bn))

#define AROS_LH0(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LHA(a10),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LHA(a10),\
	__AROS_LHA(a11),\
	__AROS_LH_BASE(bt,bn))
#define AROS_LH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
        __AROS_LHA(a4),\
        __AROS_LHA(a5),\
        __AROS_LHA(a6),\
        __AROS_LHA(a7),\
        __AROS_LHA(a8),\
        __AROS_LHA(a9),\
        __AROS_LHA(a10),\
        __AROS_LHA(a11),\
        __AROS_LHA(a12),\
        __AROS_LH_BASE(bt,bn))
#define AROS_LH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
        __AROS_LHA(a1),\
        __AROS_LHA(a2),\
        __AROS_LHA(a3),\
        __AROS_LHA(a4),\
        __AROS_LHA(a5),\
        __AROS_LHA(a6),\
        __AROS_LHA(a7),\
        __AROS_LHA(a8),\
        __AROS_LHA(a9),\
        __AROS_LHA(a10),\
        __AROS_LHA(a11),\
        __AROS_LHA(a12),\
        __AROS_LHA(a13),\
        __AROS_LH_BASE(bt,bn))
#define AROS_LH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
        __AROS_LHA(a1),\
        __AROS_LHA(a2),\
        __AROS_LHA(a3),\
        __AROS_LHA(a4),\
        __AROS_LHA(a5),\
        __AROS_LHA(a6),\
        __AROS_LHA(a7),\
        __AROS_LHA(a8),\
        __AROS_LHA(a9),\
        __AROS_LHA(a10),\
        __AROS_LHA(a11),\
        __AROS_LHA(a12),\
        __AROS_LHA(a13),\
        __AROS_LHA(a14),\
        __AROS_LH_BASE(bt,bn))
#define AROS_LH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
        __AROS_LHA(a1),\
        __AROS_LHA(a2),\
        __AROS_LHA(a3),\
        __AROS_LHA(a4),\
        __AROS_LHA(a5),\
        __AROS_LHA(a6),\
        __AROS_LHA(a7),\
        __AROS_LHA(a8),\
        __AROS_LHA(a9),\
        __AROS_LHA(a10),\
        __AROS_LHA(a11),\
        __AROS_LHA(a12),\
        __AROS_LHA(a13),\
        __AROS_LHA(a14),\
        __AROS_LHA(a15),\
        __AROS_LH_BASE(bt,bn))

/* Library functions which don't need library base */

#define AROS_LH0I(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(void)
#define AROS_LH1I(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1))
#define AROS_LH2I(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2))
#define AROS_LH3I(t,n,a1,a2,a3,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3))
#define AROS_LH4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4))
#define AROS_LH5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5))
#define AROS_LH6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6))
#define AROS_LH7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7))
#define AROS_LH8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8))
#define AROS_LH9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9))
#define AROS_LH10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LHA(a10))
#define AROS_LH11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s)(\
	__AROS_LHA(a1),\
	__AROS_LHA(a2),\
	__AROS_LHA(a3),\
	__AROS_LHA(a4),\
	__AROS_LHA(a5),\
	__AROS_LHA(a6),\
	__AROS_LHA(a7),\
	__AROS_LHA(a8),\
	__AROS_LHA(a9),\
	__AROS_LHA(a10),\
	__AROS_LHA(a11))

/* Call a library function which requires the library base */

#define AROS_LCQUAD1(t,n,a1,bt,bn,o,s)\
	LP1(o*4,t,n,\
	__AROS_LPAQUAD(a1),__AROS_LCAQUAD(a1),\
	,bn)
#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s)\
	LP2(o*4,t,n,\
	__AROS_LPAQUAD(a1),__AROS_LCAQUAD(a1),\
	__AROS_LPAQUAD(a2),__AROS_LCAQUAD(a2),\
	,bn)

#define AROS_LC0(t,n,bt,bn,o,s)\
	LP0(o*4,t,n,,bn)
#define AROS_LC1(t,n,a1,bt,bn,o,s)\
	LP1(o*4,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	,bn)
#define AROS_LC2(t,n,a1,a2,bt,bn,o,s)\
	LP2(o*4,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	,bn)
#define	AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s)\
	LP3(o*4,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	,bn)
#define	AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	,bn)
#define	AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	,bn)
#define	AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	,bn)
#define	AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	,bn)
#define	AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	,bn)
#define	AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	,bn)
#define	AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	,bn)
#define	AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	__AROS_LPA(a11),__AROS_LCA(a11),\
	,bn)
#define	AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	__AROS_LPA(a11),__AROS_LCA(a11),\
	__AROS_LPA(a12),__AROS_LCA(a12),\
	,bn)
#define	AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	__AROS_LPA(a11),__AROS_LCA(a11),\
	__AROS_LPA(a12),__AROS_LCA(a12),\
	__AROS_LPA(a13),__AROS_LCA(a13),\
	,bn)
#define	AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	__AROS_LPA(a11),__AROS_LCA(a11),\
	__AROS_LPA(a12),__AROS_LCA(a12),\
	__AROS_LPA(a13),__AROS_LCA(a13),\
	__AROS_LPA(a14),__AROS_LCA(a14),\
	,bn)
#define	AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)\
	LP4(o*5,t,n,\
	__AROS_LPA(a1),__AROS_LCA(a1),\
	__AROS_LPA(a2),__AROS_LCA(a2),\
	__AROS_LPA(a3),__AROS_LCA(a3),\
	__AROS_LPA(a4),__AROS_LCA(a4),\
	__AROS_LPA(a5),__AROS_LCA(a5),\
	__AROS_LPA(a6),__AROS_LCA(a6),\
	__AROS_LPA(a7),__AROS_LCA(a7),\
	__AROS_LPA(a8),__AROS_LCA(a8),\
	__AROS_LPA(a9),__AROS_LCA(a9),\
	__AROS_LPA(a10),__AROS_LCA(a10),\
	__AROS_LPA(a11),__AROS_LCA(a11),\
	__AROS_LPA(a12),__AROS_LCA(a12),\
	__AROS_LPA(a13),__AROS_LCA(a13),\
	__AROS_LPA(a14),__AROS_LCA(a14),\
	__AROS_LPA(a15),__AROS_LCA(a15),\
	,bn)


/*
    Call a library function which doesn't need the library base
    A bit useless with LP macros.
*/

#define AROS_LC0I(t,n,bt,bn,o,s)\
	AROS_LC0(t,n,bt,bn,o,s)
#define	AROS_LC1I(t,n,a1,bt,bn,o,s)\
	AROS_LC1(t,n,a1,bt,bn,o,s)
#define	AROS_LC2I(t,n,a1,a2,bt,bn,o,s)\
	AROS_LC2(t,n,a1,a2,bt,bn,o,s)
#define	AROS_LC3I(t,n,a1,a2,a3,bt,bn,o,s)\
	AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s)
#define	AROS_LC4I(t,n,a1,a2,a3,a4,bt,bn,o,s)\
	AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#define	AROS_LC5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)\
	AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#define	AROS_LC6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)\
	AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#define	AROS_LC7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)\
	AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#define	AROS_LC8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)\
	AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#define	AROS_LC9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)\
	AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#define	AROS_LC10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)\
	AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#define	AROS_LC11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)\
	AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#define	AROS_LC12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)\
	AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#define	AROS_LC13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)\
	AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#define	AROS_LC14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)\
	AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#define	AROS_LC15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)\
	AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

/* Dummy prototypes - we're using macros */

#   define AROS_LPQUAD1(t,n,a1,bt,bn,o,s)
#   define AROS_LPQUAD2(t,n,a1,a2,bt,bn,o,s)

#   define AROS_LP0(t,n,bt,bn,o,s)
#   define AROS_LP1(t,n,a1,bt,bn,o,s)
#   define AROS_LP2(t,n,a1,a2,bt,bn,o,s)
#   define AROS_LP3(t,n,a1,a2,a3,bt,bn,o,s)
#   define AROS_LP4(t,n,a1,a2,a3,a4,bt,bn,o,s)
#   define AROS_LP5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#   define AROS_LP6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#   define AROS_LP7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#   define AROS_LP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#   define AROS_LP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#   define AROS_LP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#   define AROS_LP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#   define AROS_LP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#   define AROS_LP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#   define AROS_LP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#   define AROS_LP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#   define AROS_LP0I(t,n,bt,bn,o,s)
#   define AROS_LP1I(t,n,a1,bt,bn,o,s)
#   define AROS_LP2I(t,n,a1,a2,bt,bn,o,s)
#   define AROS_LP3I(t,n,a1,a2,a3,bt,bn,o,s)
#   define AROS_LP4I(t,n,a1,a2,a3,a4,bt,bn,o,s)
#   define AROS_LP5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s)
#   define AROS_LP6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s)
#   define AROS_LP7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s)
#   define AROS_LP8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s)
#   define AROS_LP9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s)
#   define AROS_LP10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s)
#   define AROS_LP11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s)
#   define AROS_LP12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s)
#   define AROS_LP13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s)
#   define AROS_LP14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s)
#   define AROS_LP15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s)

#define AROS_LHA(type,name,reg) type,name,reg
#define AROS_LPA(type,name,reg) type,name,reg
#define AROS_LCA(type,name,reg) type,name,reg
#define AROS_LDA(type,name,reg) type,name,reg

#define AROS_LD0(t,n,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) (__AROS_LD_BASE(bt,bn))

#define AROS_LD1(t,n,a1,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) ( \
	__AROS_LDA(a1), __AROS_LD_BASE(bt,bn))

#define AROS_LD2(t,n,a1,a2,bt,bn,o,s) \
	t AROS_SLIB_ENTRY(n,s) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), __AROS_LD_BASE(bt,bn))

#ifndef AROS_LVO_CALL0
#define AROS_LVO_CALL0(t, bt, bn, o, s) \
    LP0((o*4), t, LVOCall0, bt, bn)
#endif

#ifndef AROS_LVO_CALL0NR
#define AROS_LVO_CALL0NR(bt, bn, o, s) \
    LP0NR((o*4), LVOCall0NR, bt, bn)
#endif

#ifndef AROS_LVO_CALL1
#define AROS_LVO_CALL1(t, a1, bt, bn, o, s) \
    LP1((o*4), t, LVOCall1, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL1NR
#define AROS_LVO_CALL1NR(a1, bt, bn, o, s) \
    LP1NR((o*4), LVOCall1NR, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL2
#define AROS_LVO_CALL2(t, a1, a2, bt, bn, o, s) \
    LP2((o*4), t, LVOCall2, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL2NR
#define AROS_LVO_CALL2NR(a1, a2, bt, bn, o, s) \
    LP2NR((o*4), LVOCall2NR, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL3
#define AROS_LVO_CALL3(t, a1, a2, a3, bt, bn, o, s) \
    LP3((o*4), t, LVOCall3, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL3NR
#define AROS_LVO_CALL3NR(a1, a2, a3, bt, bn, o, s) \
    LP3NR((o*4), LVOCall3NR, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL4
#define AROS_LVO_CALL4(t, a1, a2, a3, a4, bt, bn, o, s) \
    LP3((o*4), t, LVOCall3, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    __AROS_LDA(a4), __AROS_LCA(a4), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL4NR
#define AROS_LVO_CALL4NR(a1, a2, a3, a4, bt, bn, o, s) \
    LP3NR((o*4), LVOCall3NR, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    __AROS_LDA(a4), __AROS_LCA(a4), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL5
#define AROS_LVO_CALL5(t, a1, a2, a3, a4, a5, bt, bn, o, s) \
    LP3((o*4), t, LVOCall3, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    __AROS_LDA(a4), __AROS_LCA(a4), \
    __AROS_LDA(a5), __AROS_LCA(a5), \
    bt, bn)
#endif

#ifndef AROS_LVO_CALL5NR
#define AROS_LVO_CALL5NR(a1, a2, a3, a4, a5, bt, bn, o, s) \
    LP3NR((o*4), LVOCall3NR, \
    __AROS_LDA(a1), __AROS_LCA(a1), \
    __AROS_LDA(a2), __AROS_LCA(a2), \
    __AROS_LDA(a3), __AROS_LCA(a3), \
    __AROS_LDA(a4), __AROS_LCA(a4), \
    __AROS_LDA(a5), __AROS_LCA(a5), \
    bt, bn)
#endif

#ifndef INLINE_MACROS_H
#   include <inline/macros.h>
#endif

#ifndef AROS_LIBFUNC_INIT
#   define AROS_LIBFUNC_INIT {
#endif

#ifndef AROS_LIBFUNC_EXIT
#   define AROS_LIBFUNC_EXIT }
#endif

#ifndef AROS_LIBBASE_EXT_DECL
#   define AROS_LIBBASE_EXT_DECL(a,b)
#endif

#endif /* _AROS_LIBCALL_H */
