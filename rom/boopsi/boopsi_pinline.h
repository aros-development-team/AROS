#ifndef _BOOPSI_PINLINE_H
#define _BOOPSI_PINLINE_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Private m68k inlines for boopsi.library
    Lang: english
*/

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef BOOPSI_BASE_NAME
#define BOOPSI_BASE_NAME BOOPSIBase
#endif

#define DoNotify(cl, o, ic, msg) \
	LP4(0x60, IPTR, DoNotify, \
	    Class *, (cl), a0, \
	    Object *, (o), a1, \
	    struct ICData *, (ic), a2, \
	    struct opUpdate *, (msg), a3, \
	    , BOOPSI_BASE_NAME )

#define FreeICData(ic) \
	LP1NR(0x5a, FreeICData, \
	    struct ICData *, (ic), a0, \
	    , BOOPSI_BASE_NAME )

#endif /* _BOOPSI_PINLINE_H */
