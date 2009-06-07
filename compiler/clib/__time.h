/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef ___TIME_H
#define ___TIME_H

/*
	2922 is the number of days between 1.1.1970 and 1.1.1978 (2 leap
	years and 6 normal). The former number is the start value
	for time(), the latter the start time for the AmigaOS
	time functions.
*/

#include "__arosc_privdata.h"

#define OFFSET_FROM_1970 2922*24*60*60

#endif /* !___TIME_H */
