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

#ifndef __AROS_CPU_SPECIFIC_LH
/* Library functions which need the libbase */
#define AROS_LHQUAD1(t,n,a1,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHAQUAD(a1),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LHQUAD2(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHAQUAD(a1),\
    __AROS_LHAQUAD(a2),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH1QUAD1(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHAQUAD(a2),\
    __AROS_LH_BASE(bt,bn)) {

#define AROS_LH0(t,n,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH1(t,n,a1,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH2(t,n,a1,a2,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH3(t,n,a1,a2,a3,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    __AROS_LH_PREFIX t AROS_SLIB_ENTRY(n,s,o)(\
    __AROS_LHA(a1),\
    __AROS_LHA(a2),\
    __AROS_LHA(a3),\
    __AROS_LHA(a4),\
    __AROS_LHA(a5),\
    __AROS_LHA(a6),\
    __AROS_LHA(a7),\
    __AROS_LHA(a8),\
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
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
    __AROS_LH_BASE(bt,bn)) {
#define AROS_LH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
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
    __AROS_LHA(a15),\
    __AROS_LH_BASE(bt,bn)) {

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


/* Call a library function which requires the libbase */
#ifndef __AROS_CPU_SPECIFIC_LC
# define AROS_LCQUAD1(t,n,a1,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPAQUAD(a1),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCAQUAD(a1),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LCQUAD2(t,n,a1,a2,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPAQUAD(a1),\
    __AROS_LPAQUAD(a2),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCAQUAD(a1),\
    __AROS_LCAQUAD(a2),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC1QUAD1(t,n,a1,a2,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPAQUAD(a2),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCAQUAD(a2),\
    __AROS_LC_BASE(bt,bn)))

#define AROS_LC0(t,n,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC1(t,n,a1,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC2(t,n,a1,a2,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC3(t,n,a1,a2,a3,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14),\
    __AROS_LC_BASE(bt,bn)))
#define AROS_LC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LPA(a1),\
    __AROS_LPA(a2),\
    __AROS_LPA(a3),\
    __AROS_LPA(a4),\
    __AROS_LPA(a5),\
    __AROS_LPA(a6),\
    __AROS_LPA(a7),\
    __AROS_LPA(a8),\
    __AROS_LPA(a9),\
    __AROS_LPA(a10),\
    __AROS_LPA(a11),\
    __AROS_LPA(a12),\
    __AROS_LPA(a13),\
    __AROS_LPA(a14),\
    __AROS_LPA(a15),\
    __AROS_LP_BASE(bt,bn)))__AROS_GETVECADDR(bn,o))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    __AROS_LCA(a11),\
    __AROS_LCA(a12),\
    __AROS_LCA(a13),\
    __AROS_LCA(a14),\
    __AROS_LCA(a15),\
    __AROS_LC_BASE(bt,bn)))

/* Macros for calling library functions without a return value. These macros should
 * always be used when a library function has no return value but can also be used
 * when the return value is not used
 */
#define AROS_LC0NR AROS_LC0
#define AROS_LC1NR AROS_LC1
#define AROS_LC2NR AROS_LC2
#define AROS_LC3NR AROS_LC3
#define AROS_LC4NR AROS_LC4
#define AROS_LC5NR AROS_LC5
#define AROS_LC6NR AROS_LC6
#define AROS_LC7NR AROS_LC7
#define AROS_LC8NR AROS_LC8
#define AROS_LC9NR AROS_LC9
#define AROS_LC10NR AROS_LC10
#define AROS_LC11NR AROS_LC11
#define AROS_LC12NR AROS_LC12
#define AROS_LC13NR AROS_LC13
#define AROS_LC14NR AROS_LC14
#define AROS_LC15NR AROS_LC15

#endif /* !__AROS_CPU_SPECIFIC_LC */


/* Special calls: Call a library function without the name just by the ADDRESS */
#ifndef AROS_CALL0
#define AROS_CALL0(returntype,address,basetype,basename) \
    (((__AROS_LC_PREFIX returntype(*)(__AROS_LD_BASE(basetype,basename)))\
    (void *)(address))(basename))
#define AROS_CALL0NR AROS_CALL0
#endif

#ifndef AROS_CALL1
#define AROS_CALL1(t,a,a1,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    bn))
#define AROS_CALL1NR AROS_CALL1
#endif

#ifndef AROS_CALL2
#define AROS_CALL2(t,a,a1,a2,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    bn))
#define AROS_CALL2NR AROS_CALL2
#endif

#ifndef AROS_CALL3
#define AROS_CALL3(t,a,a1,a2,a3,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    bn))
#define AROS_CALL3NR AROS_CALL3
#endif

#ifndef AROS_CALL4
#define AROS_CALL4(t,a,a1,a2,a3,a4,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    bn))
#define AROS_CALL4NR AROS_CALL4
#endif

#ifndef AROS_CALL5
#define AROS_CALL5(t,a,a1,a2,a3,a4,a5,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    bn))
#define AROS_CALL5NR AROS_CALL5
#endif

#ifndef AROS_CALL6
#define AROS_CALL6(t,a,a1,a2,a3,a4,a5,a6,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    bn))
#define AROS_CALL6NR AROS_CALL6
#endif

#ifndef AROS_CALL7
#define AROS_CALL7(t,a,a1,a2,a3,a4,a5,a6,a7,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    bn))
#define AROS_CALL7NR AROS_CALL7
#endif

#ifndef AROS_CALL8
#define AROS_CALL8(t,a,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    bn))
#define AROS_CALL8NR AROS_CALL8
#endif

#ifndef AROS_CALL9
#define AROS_CALL9(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    bn))
#define AROS_CALL9NR AROS_CALL9
#endif

#ifndef AROS_CALL10
#define AROS_CALL10(t,a,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn) \
    (((__AROS_LC_PREFIX t(*)(\
    __AROS_LDA(a1),\
    __AROS_LDA(a2),\
    __AROS_LDA(a3),\
    __AROS_LDA(a4),\
    __AROS_LDA(a5),\
    __AROS_LDA(a6),\
    __AROS_LDA(a7),\
    __AROS_LDA(a8),\
    __AROS_LDA(a9),\
    __AROS_LDA(a10),\
    __AROS_LD_BASE(bt,bn)))\
    (void *)(a))(\
    __AROS_LCA(a1),\
    __AROS_LCA(a2),\
    __AROS_LCA(a3),\
    __AROS_LCA(a4),\
    __AROS_LCA(a5),\
    __AROS_LCA(a6),\
    __AROS_LCA(a7),\
    __AROS_LCA(a8),\
    __AROS_LCA(a9),\
    __AROS_LCA(a10),\
    bn))
#define AROS_CALL10NR AROS_CALL10
#endif

/* Special calls: Call a library function without the name just by the OFFSET */

#ifndef AROS_LVO_CALL0
#define AROS_LVO_CALL0(returntype,basetype,basename,offset,system) \
    AROS_CALL0(returntype,__AROS_GETVECADDR(basename,offset),basetype,basename)
#endif

#ifndef AROS_LVO_CALL0NR
#define AROS_LVO_CALL0NR(returntype,basetype,basename,offset,system) \
    AROS_CALL0NR(returntype,__AROS_GETVECADDR(basename,offset),basetype,basename)
#endif

#ifndef AROS_LVO_CALL1
#define AROS_LVO_CALL1(t,a1,bt,bn,o,s) \
    AROS_CALL1(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),bt,bn)
#endif

#ifndef AROS_LVO_CALL1NR
#define AROS_LVO_CALL1NR(t,a1,bt,bn,o,s) \
    AROS_CALL1NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),bt,bn)
#endif

#ifndef AROS_LVO_CALL2
#define AROS_LVO_CALL2(t,a1,a2,bt,bn,o,s) \
    AROS_CALL2(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),bt,bn)
#endif

#ifndef AROS_LVO_CALL2NR
#define AROS_LVO_CALL2NR(t,a1,a2,bt,bn,o,s) \
    AROS_CALL2NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),bt,bn)
#endif

#ifndef AROS_LVO_CALL3
#define AROS_LVO_CALL3(t,a1,a2,a3,bt,bn,o,s) \
    AROS_CALL3(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),bt,bn)
#endif

#ifndef AROS_LVO_CALL3NR
#define AROS_LVO_CALL3NR(t,a1,a2,a3,bt,bn,o,s) \
    AROS_CALL3NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),bt,bn)
#endif

#ifndef AROS_LVO_CALL4
#define AROS_LVO_CALL4(t,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_CALL4(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),bt,bn)
#endif

#ifndef AROS_LVO_CALL4NR
#define AROS_LVO_CALL4NR(t,a1,a2,a3,a4,bt,bn,o,s) \
    AROS_CALL4NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),bt,bn)
#endif

#ifndef AROS_LVO_CALL5
#define AROS_LVO_CALL5(t,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_CALL5(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),bt,bn)
#endif

#ifndef AROS_LVO_CALL5NR
#define AROS_LVO_CALL5NR(t,a1,a2,a3,a4,a5,bt,bn,o,s) \
    AROS_CALL5NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),bt,bn)
#endif

#ifndef AROS_LVO_CALL6
#define AROS_LVO_CALL6(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_CALL6(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),bt,bn)
#endif

#ifndef AROS_LVO_CALL6NR
#define AROS_LVO_CALL6NR(t,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
    AROS_CALL6NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),bt,bn)
#endif

#ifndef AROS_LVO_CALL7
#define AROS_LVO_CALL7(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_CALL7(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),bt,bn)
#endif

#ifndef AROS_LVO_CALL7NR
#define AROS_LVO_CALL7NR(t,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
    AROS_CALL7NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),bt,bn)
#endif

#ifndef AROS_LVO_CALL8
#define AROS_LVO_CALL8(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_CALL8(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),bt,bn)
#endif

#ifndef AROS_LVO_CALL8NR
#define AROS_LVO_CALL8NR(t,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
    AROS_CALL8NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),bt,bn)
#endif

#ifndef AROS_LVO_CALL9
#define AROS_LVO_CALL9(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL9(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),bt,bn)
#endif

#ifndef AROS_LVO_CALL9NR
#define AROS_LVO_CALL9NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL9NR(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),bt,bn)
#endif

#ifndef AROS_LVO_CALL10
#define AROS_LVO_CALL10(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
    AROS_CALL10(t,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCA(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),AROS_LCA(a10),bt,bn)
#endif

#ifndef AROS_LVO_CALL10NR
#define AROS_LVO_CALL10NR(t,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
    AROS_CALL10NR(void,__AROS_GETVECADDR(bn,o),AROS_LCA(a1),AROS_LCA(a2),AROS_LCA(a3),AROS_LCA(a4),AROS_LCS(a5),AROS_LCA(a6),AROS_LCA(a7),AROS_LCA(a8),AROS_LCA(a9),AROS_LCA(a10),bt,bn)
#endif

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


#ifndef __AROS_CPU_SPECIFIC_LD
/* Declarations for library functions which need the libbase */
#   define AROS_LDQUAD1(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1), __AROS_LD_BASE(bt,bn))
#   define AROS_LDQUAD2(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDAQUAD(a1), \
	__AROS_LDAQUAD(a2),__AROS_LD_BASE(bt,bn))

#   define AROS_LD0(t,n,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) (__AROS_LD_BASE(bt,bn))
#   define AROS_LD1(t,n,a1,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1),__AROS_LD_BASE(bt,bn))
#   define AROS_LD2(t,n,a1,a2,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2),__AROS_LD_BASE(bt,bn))
#   define AROS_LD3(t,n,a1,a2,a3,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), __AROS_LD_BASE(bt,bn))
#   define AROS_LD4(t,n,a1,a2,a3,a4,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), __AROS_LD_BASE(bt,bn))
#   define AROS_LD5(t,n,a1,a2,a3,a4,a5,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), __AROS_LD_BASE(bt,bn))
#   define AROS_LD6(t,n,a1,a2,a3,a4,a5,a6,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), __AROS_LD_BASE(bt,bn))
#   define AROS_LD7(t,n,a1,a2,a3,a4,a5,a6,a7,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), __AROS_LD_BASE(bt,bn))
#   define AROS_LD8(t,n,a1,a2,a3,a4,a5,a6,a7,a8,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), __AROS_LD_BASE(bt,bn))
#   define AROS_LD9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,bt,bn,o,s) \
	__AROS_LD_PREFIX t AROS_SLIB_ENTRY(n,s,o) ( \
	__AROS_LDA(a1), \
	__AROS_LDA(a2), \
	__AROS_LDA(a3), \
	__AROS_LDA(a4), \
	__AROS_LDA(a5), \
	__AROS_LDA(a6), \
	__AROS_LDA(a7), \
	__AROS_LDA(a8), \
	__AROS_LDA(a9), __AROS_LD_BASE(bt,bn))
#   define AROS_LD10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,bt,bn,o,s) \
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
	__AROS_LDA(a10), __AROS_LD_BASE(bt,bn))
#   define AROS_LD11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,bt,bn,o,s) \
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
	__AROS_LDA(a11), __AROS_LD_BASE(bt,bn))
#   define AROS_LD12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,bt,bn,o,s) \
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
	__AROS_LDA(a12), __AROS_LD_BASE(bt,bn))
#   define AROS_LD13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,bt,bn,o,s) \
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
	__AROS_LDA(a13), __AROS_LD_BASE(bt,bn))
#   define AROS_LD14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,bt,bn,o,s) \
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
	__AROS_LDA(a14), __AROS_LD_BASE(bt,bn))
#   define AROS_LD15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,bt,bn,o,s) \
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
	__AROS_LDA(a15), __AROS_LD_BASE(bt,bn))

/* Declarations for library functions which don't need the libbase */
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
