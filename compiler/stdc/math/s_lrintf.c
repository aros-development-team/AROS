/*
    Copyright � 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_lrintf.c,v 1.1 2005/01/11 23:12:55 das Exp $");

#define type		float
#define	roundit		rintf
#define dtype		long
#define	fn		lrintf

#include "s_lrint.c"
