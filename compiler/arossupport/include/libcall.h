/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_LIBCALL_H
#define AROS_LIBCALL_H

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
	Some macros to build functions with registerized parameters on the
	different compilers around. Usage:

	AROS_LH<n><f>(type, name,
	    [AROS_LHA(type1,  name1, reg1),] ...
	    libtype, libvariable, lvo, basename
	)
	{
	    AROS_LIBFUNC_INIT
	    \* Function starts here. *\
	    AROS_LIBFUNC_EXIT
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

	Example: Define an Exec-compatible RemHead function.

	AROS_LH1I(struct Node *, RemHead,
		struct List *, list, A0,
		struct ExecBase, SysBase, 43, Exec)
	{
	    // No ";" !!
	    AROS_LIBFUNC_INIT

	    struct Node *node;

	    node=list->lh_Head;
	    if(node->ln_Succ==NULL)
		return NULL;
	    Remove(node);
	    return node;

	    // after return !!
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
typedef int (*LONG_FUNC)();
#endif
#ifndef __typedef_ULONG_FUNC
#define __typedef_ULONG_FUNC
typedef unsigned int (*ULONG_FUNC)();
#endif

/* Declare all macros which the systems' libcall didn't */
#ifndef __AROS_SLIB_ENTRY
#   define __AROS_SLIB_ENTRY(n,s,o)   s ## _ ## o ## _ ## n
#endif
#ifndef AROS_SLIB_ENTRY
/*
    This is to allow the C preprocessor to expand n and s before they are
    maybe concatenated (the CPP first evaluates a ## b and the tries
    to expand ab instead of expanding a and b and then concatenating the
    results
*/
#   define AROS_SLIB_ENTRY(n,s,o)  __AROS_SLIB_ENTRY(n,s,o)
#endif

/*
 * Libraries using base-relative addressing of global variables
 * need specific way to pass the library base.
 * Ordinary libraries get base explicitly.
 */
#ifdef AROS_BASEREL_LIBRARY
#include <aros/libcall_baserel.h>
#else
#include <aros/libcall_base.h>
#endif

#ifndef __AROS_CPU_SPECIFIC_LH
/* Library functions which don't need the libbase */
#define AROS_LH0I(t,n,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(void) {
#define AROS_LH1I(t,n,a1,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1)\
    ) {
#define AROS_LH2I(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2)\
    ) {
#define AROS_LH3I(t,n,a1,a2,a3,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3)\
    ) {
#define AROS_LH4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4)\
    ) {
#define AROS_LH5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5)\
    ) {
#define AROS_LH6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6)\
    ) {
#define AROS_LH7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7)\
    ) {
#define AROS_LH8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8)\
    ) {
#define AROS_LH9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9)\
    ) {
#define AROS_LH10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LHA(a9),\
    __AROS_LHA(a10)\
    ) {
#define AROS_LH11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
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
    __AROS_LHA(a11)\
    ) {
#define AROS_LH12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
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
    __AROS_LHA(a12)\
    ) {
#define AROS_LH13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
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
    __AROS_LHA(a13)\
    ) {
#define AROS_LH14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
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
    __AROS_LHA(a14)\
    ) {
#define AROS_LH15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
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
    __AROS_LHA(a15)\
    ) {
#endif /* !__AROS_CPU_SPECIFIC_LH */

#ifndef __AROS_CPU_SPECIFIC_LP
#   define AROS_LPQUAD1(t,n,a1,bt,bn,o,s) \
t n(__AROS_LPAQUAD(a1))
#   define AROS_LPQUAD2(t,n,a1,a2,bt,bn,o,s) \
t n(__AROS_LPAQUAD(a1),__AROS_LPAQUAD(a2))

#   define AROS_LP0(t,n,bt,bn,o,s) \
t n(void)
#   define AROS_LP1(t,n,a1,bt,bn,o,s) \
t n(__AROS_LPA(a1))
#   define AROS_LP2(t,n,a1,a2,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2))
#   define AROS_LP3(t,n,a1,a2,a3,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3))
#   define AROS_LP4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4))
#   define AROS_LP5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5))
#   define AROS_LP6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6))
#   define AROS_LP7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7))
#   define AROS_LP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8))
#   define AROS_LP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9))
#   define AROS_LP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10))
#   define AROS_LP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10),__AROS_LPA(a11))
#   define AROS_LP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10),__AROS_LPA(a11),__AROS_LPA(a12))
#   define AROS_LP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10),__AROS_LPA(a11),__AROS_LPA(a12),__AROS_LPA(a13))
#   define AROS_LP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10),__AROS_LPA(a11),__AROS_LPA(a12),__AROS_LPA(a13),__AROS_LPA(a14))
#   define AROS_LP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
t n(__AROS_LPA(a1),__AROS_LPA(a2),__AROS_LPA(a3),__AROS_LPA(a4),__AROS_LPA(a5),__AROS_LPA(a6),__AROS_LPA(a7),__AROS_LPA(a8),__AROS_LPA(a9),__AROS_LPA(a10),__AROS_LPA(a11),__AROS_LPA(a12),__AROS_LPA(a13),__AROS_LPA(a14),__AROS_LPA(a15))
#  endif /* !__AROS_CPU_SPECIFIC_LP */

/* Declarations for library functions which don't need the libbase */
#ifndef __AROS_CPU_SPECIFIC_LD
#   define AROS_LD0I(t,n,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) (void)
#   define AROS_LD1I(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1))
#   define AROS_LD2I(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2))
#   define AROS_LD3I(t,n,a1,a2,a3,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3))
#   define AROS_LD4I(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4))
#   define AROS_LD5I(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5))
#   define AROS_LD6I(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6))
#   define AROS_LD7I(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7))
#   define AROS_LD8I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8))
#   define AROS_LD9I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9))
#   define AROS_LD10I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10))
#   define AROS_LD11I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11))
#   define AROS_LD12I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12))
#   define AROS_LD13I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13))
#   define AROS_LD14I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14))
#   define AROS_LD15I(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), \
	__AROS_LDA(a10), \
	__AROS_LDA(a11), \
	__AROS_LDA(a12), \
	__AROS_LDA(a13), \
	__AROS_LDA(a14), \
	__AROS_LDA(a15))
#endif /* !__AROS_CPU_SPECIFIC_LD */

#define AROS_LHA(type,name,reg) type,name,reg
#define AROS_LPA(type,name,reg) type,name,reg
#define AROS_LCA(type,name,reg) type,name,reg
#define AROS_LDA(type,name,reg) type,name,reg

#define AROS_LHAQUAD(type,name,reg1,reg2) type,name,reg1,reg2
#define AROS_LPAQUAD(type,name,reg1,reg2) type,name,reg1,reg2
#define AROS_LCAQUAD(type,name,reg1,reg2) type,name,reg1,reg2
#define AROS_LDAQUAD(type,name,reg1,reg2) type,name,reg1,reg2

#ifndef AROS_LIBFUNC_INIT
#   define AROS_LIBFUNC_INIT {
#endif
#ifndef AROS_LIBFUNC_EXIT
#   define AROS_LIBFUNC_EXIT }}
#endif

/* Tagging of private functions, so that they can be distinguished from
   official ones. But they have to compile the same way, so: */
#define AROS_PLH0  AROS_LH0
#define AROS_PLH1  AROS_LH1
#define AROS_PLH2  AROS_LH2
#define AROS_PLH3  AROS_LH3
#define AROS_PLH4  AROS_LH4
#define AROS_PLH5  AROS_LH5
#define AROS_PLH6  AROS_LH6
#define AROS_PLH7  AROS_LH7
#define AROS_PLH8  AROS_LH8
#define AROS_PLH9  AROS_LH9
#define AROS_PLH10 AROS_LH10
#define AROS_PLH11 AROS_LH11
#define AROS_PLH12 AROS_LH12
#define AROS_PLH13 AROS_LH13
#define AROS_PLH14 AROS_LH14
#define AROS_PLH15 AROS_LH15

/* NT stands for No Tags, which means that the functions which are defined with these headers
   are not subject to tagcall generation by the script used to generate include files */
#define AROS_NTLH0  AROS_LH0
#define AROS_NTLH1  AROS_LH1
#define AROS_NTLH2  AROS_LH2
#define AROS_NTLH3  AROS_LH3
#define AROS_NTLH4  AROS_LH4
#define AROS_NTLH5  AROS_LH5
#define AROS_NTLH6  AROS_LH6
#define AROS_NTLH7  AROS_LH7
#define AROS_NTLH8  AROS_LH8
#define AROS_NTLH9  AROS_LH9
#define AROS_NTLH10 AROS_LH10
#define AROS_NTLH11 AROS_LH11
#define AROS_NTLH12 AROS_LH12
#define AROS_NTLH13 AROS_LH13
#define AROS_NTLH14 AROS_LH14
#define AROS_NTLH15 AROS_LH15

/******************************************************************************
*****  ENDE aros/libcall.h
******************************************************************************/

#endif /* AROS_LIBCALL_H */
