/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_lrintl.c,v 1.1 2008/01/14 02:12:06 das Exp $");

#define type		long double
#define	roundit		rintl
#define dtype		long
#define	fn		lrintl

#include "s_lrint.c"
