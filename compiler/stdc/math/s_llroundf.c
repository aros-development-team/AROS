/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

__FBSDID("$FreeBSD: src/lib/msun/src/s_llroundf.c,v 1.2 2005/04/08 00:52:27 das Exp $");

#define type		float
#define	roundit		roundf
#define dtype		long long
#define	DTYPE_MIN	LLONG_MIN
#define	DTYPE_MAX	LLONG_MAX
#define	fn		llroundf

#include "s_lround.c"
