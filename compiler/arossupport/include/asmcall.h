/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef AROS_ASMCALL_H
#define AROS_ASMCALL_H

/******************************************************************************

    MODUL
	$Id$

    DESCRIPTION
	Some macros to build and call functions with registerized parameters on
	the different compilers around. Usage:

	AROS_UFH<n><f>(type, name, type1,  name1, reg1, ...)
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

	AROS_UFH1(struct Node *, RemHead,
	    AROS_UFHA(struct List *, list, A0)
	)
	{
	    struct Node *node;

	    node=list->lh_Head;
	    if(node->ln_Succ==NULL)
		return NULL;
	    Remove(node);
	    return node;
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

#ifndef __AROS_MACHINE_H_DEFINES_ASMCALLS

#if !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS)) /* Function headers for user functions */
#define AROS_UFH0(t,n) \
    __AROS_UFH_PREFIX t n (void) {
#define AROS_UFH1(t,n,a1) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1)\
    ) {
#define AROS_UFH2S(t,n,a1,a2) \
    __AROS_UFH_PREFIX static t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2)\
    ) {
#define AROS_UFH2(t,n,a1,a2) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2)\
    ) {
#define AROS_UFH3(t,n,a1,a2,a3) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3)\
    ) {
#define AROS_UFH3S(t,n,a1,a2,a3) \
    __AROS_UFH_PREFIX static t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3)\
    ) {
#define AROS_UFH4(t,n,a1,a2,a3,a4) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4)\
    ) {
#define AROS_UFH5(t,n,a1,a2,a3,a4,a5) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5)\
    ) {
#define AROS_UFH5S(t,n,a1,a2,a3,a4,a5) \
    __AROS_UFH_PREFIX static t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5)\
    ) {
#define AROS_UFH6(t,n,a1,a2,a3,a4,a5,a6) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6)\
    ) {
#define AROS_UFH7(t,n,a1,a2,a3,a4,a5,a6,a7) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7)\
    ) {
#define AROS_UFH8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8)\
    ) {
#define AROS_UFH9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9)\
    ) {
#define AROS_UFH10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10)\
    ) {
#define AROS_UFH11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10),\
    __AROS_UFHA(a11)\
    ) {
#define AROS_UFH12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10),\
    __AROS_UFHA(a11),\
    __AROS_UFHA(a12)\
    ) {
#define AROS_UFH13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10),\
    __AROS_UFHA(a11),\
    __AROS_UFHA(a12),\
    __AROS_UFHA(a13)\
    ) {
#define AROS_UFH14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10),\
    __AROS_UFHA(a11),\
    __AROS_UFHA(a12),\
    __AROS_UFHA(a13),\
    __AROS_UFHA(a14)\
    ) {
#define AROS_UFH15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
    __AROS_UFH_PREFIX t n (\
    __AROS_UFHA(a1),\
    __AROS_UFHA(a2),\
    __AROS_UFHA(a3),\
    __AROS_UFHA(a4),\
    __AROS_UFHA(a5),\
    __AROS_UFHA(a6),\
    __AROS_UFHA(a7),\
    __AROS_UFHA(a8),\
    __AROS_UFHA(a9),\
    __AROS_UFHA(a10),\
    __AROS_UFHA(a11),\
    __AROS_UFHA(a12),\
    __AROS_UFHA(a13),\
    __AROS_UFHA(a14),\
    __AROS_UFHA(a15)\
    ) {

/* Call a user function */
#define AROS_UFC0(t,n) \
    (((__AROS_UFC_PREFIX t(*)(void))n)())
#define AROS_UFC1(t,n,a1) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1)\
    ))n)(\
    __AROS_UFCA(a1)\
    ))
#define AROS_UFC2(t,n,a1,a2) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2)\
    ))
#define AROS_UFC3(t,n,a1,a2,a3) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3)\
    ))
#define AROS_UFC4(t,n,a1,a2,a3,a4) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4)\
    ))
#define AROS_UFC5(t,n,a1,a2,a3,a4,a5) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5)\
    ))
#define AROS_UFC6(t,n,a1,a2,a3,a4,a5,a6) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6)\
    ))
#define AROS_UFC7(t,n,a1,a2,a3,a4,a5,a6,a7) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7)\
    ))
#define AROS_UFC8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8)\
    ))
#define AROS_UFC9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9)\
    ))
#define AROS_UFC10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10)\
    ))
#define AROS_UFC11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10),\
    __AROS_UFPA(a11)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10),\
    __AROS_UFCA(a11)\
    ))
#define AROS_UFC12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10),\
    __AROS_UFPA(a11),\
    __AROS_UFPA(a12)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10),\
    __AROS_UFCA(a11),\
    __AROS_UFCA(a12)\
    ))
#define AROS_UFC13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10),\
    __AROS_UFPA(a11),\
    __AROS_UFPA(a12),\
    __AROS_UFPA(a13)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10),\
    __AROS_UFCA(a11),\
    __AROS_UFCA(a12),\
    __AROS_UFCA(a13)\
    ))
#define AROS_UFC14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10),\
    __AROS_UFPA(a11),\
    __AROS_UFPA(a12),\
    __AROS_UFPA(a13),\
    __AROS_UFPA(a14)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10),\
    __AROS_UFCA(a11),\
    __AROS_UFCA(a12),\
    __AROS_UFCA(a13),\
    __AROS_UFCA(a14)\
    ))
#define AROS_UFC15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
    (((__AROS_UFC_PREFIX t(*)(\
    __AROS_UFPA(a1),\
    __AROS_UFPA(a2),\
    __AROS_UFPA(a3),\
    __AROS_UFPA(a4),\
    __AROS_UFPA(a5),\
    __AROS_UFPA(a6),\
    __AROS_UFPA(a7),\
    __AROS_UFPA(a8),\
    __AROS_UFPA(a9),\
    __AROS_UFPA(a10),\
    __AROS_UFPA(a11),\
    __AROS_UFPA(a12),\
    __AROS_UFPA(a13),\
    __AROS_UFPA(a14),\
    __AROS_UFPA(a15)\
    ))n)(\
    __AROS_UFCA(a1),\
    __AROS_UFCA(a2),\
    __AROS_UFCA(a3),\
    __AROS_UFCA(a4),\
    __AROS_UFCA(a5),\
    __AROS_UFCA(a6),\
    __AROS_UFCA(a7),\
    __AROS_UFCA(a8),\
    __AROS_UFCA(a9),\
    __AROS_UFCA(a10),\
    __AROS_UFCA(a11),\
    __AROS_UFCA(a12),\
    __AROS_UFCA(a13),\
    __AROS_UFCA(a14),\
    __AROS_UFCA(a15)\
    ))
#endif /* !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS)) */

/* Prototypes for user functions */
#   define AROS_UFP0(t,n) \
	__AROS_UFP_PREFIX t n (void)
#   define AROS_UFP1(t,n,a1) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1))
#   define AROS_UFP2(t,n,a1,a2) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2))
#   define AROS_UFP3(t,n,a1,a2,a3) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3))
#   if !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS))
#	define AROS_UFP3S(t,n,a1,a2,a3) \
	    __AROS_UFP_PREFIX static t n (\
	    __AROS_UFPA(a1),\
	    __AROS_UFPA(a2),\
	    __AROS_UFPA(a3))
#   else
#	define AROS_UFP3S(t,n,a1,a2,a3) \
	    __AROS_UFP_PREFIX t n (\
	    __AROS_UFPA(a1),\
	    __AROS_UFPA(a2),\
	    __AROS_UFPA(a3))
#   endif /* !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS)) */
#   define AROS_UFP4(t,n,a1,a2,a3,a4) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4))
#   define AROS_UFP5(t,n,a1,a2,a3,a4,a5) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5))
#   if !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS))
#	define AROS_UFP5S(t,n,a1,a2,a3,a4,a5) \
	    __AROS_UFP_PREFIX static t n (\
	    __AROS_UFPA(a1),\
	    __AROS_UFPA(a2),\
	    __AROS_UFPA(a3),\
	    __AROS_UFPA(a4),\
	    __AROS_UFPA(a5))
#   else
#	define AROS_UFP5S(t,n,a1,a2,a3,a4,a5) \
	    __AROS_UFP_PREFIX t n (\
	    __AROS_UFPA(a1),\
	    __AROS_UFPA(a2),\
	    __AROS_UFPA(a3),\
	    __AROS_UFPA(a4),\
	    __AROS_UFPA(a5))
#   endif /* !(UseRegisterArgs && defined(AROS_COMPILER_NO_REGARGS)) */
#   define AROS_UFP6(t,n,a1,a2,a3,a4,a5,a6) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6))
#   define AROS_UFP7(t,n,a1,a2,a3,a4,a5,a6,a7) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7))
#   define AROS_UFP8(t,n,a1,a2,a3,a4,a5,a6,a7,a8) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8))
#   define AROS_UFP9(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9))
#   define AROS_UFP10(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10))
#   define AROS_UFP11(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10),\
	__AROS_UFPA(a11))
#   define AROS_UFP12(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10),\
	__AROS_UFPA(a11),\
	__AROS_UFPA(a12))
#   define AROS_UFP13(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10),\
	__AROS_UFPA(a11),\
	__AROS_UFPA(a12),\
	__AROS_UFPA(a13))
#   define AROS_UFP14(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10),\
	__AROS_UFPA(a11),\
	__AROS_UFPA(a12),\
	__AROS_UFPA(a13),\
	__AROS_UFPA(a14))
#   define AROS_UFP15(t,n,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
	__AROS_UFP_PREFIX t n (\
	__AROS_UFPA(a1),\
	__AROS_UFPA(a2),\
	__AROS_UFPA(a3),\
	__AROS_UFPA(a4),\
	__AROS_UFPA(a5),\
	__AROS_UFPA(a6),\
	__AROS_UFPA(a7),\
	__AROS_UFPA(a8),\
	__AROS_UFPA(a9),\
	__AROS_UFPA(a10),\
	__AROS_UFPA(a11),\
	__AROS_UFPA(a12),\
	__AROS_UFPA(a13),\
	__AROS_UFPA(a14),\
	__AROS_UFPA(a15))

#endif /* !__AROS_MACHINE_H_DEFINES_ASMCALLS */
#define AROS_UFHA(type,name,reg)    type,name,reg
#define AROS_UFPA(type,name,reg)    type,name,reg
#define AROS_UFCA(type,name,reg)    type,name,reg

#ifndef AROS_USERFUNC_INIT
#   define AROS_USERFUNC_INIT {
#endif
#ifndef AROS_USERFUNC_EXIT
#   define AROS_USERFUNC_EXIT }}
#endif

/******************************************************************************
*****  ENDE aros/asmcall.h
******************************************************************************/

#endif /* AROS_ASMCALL_H */
