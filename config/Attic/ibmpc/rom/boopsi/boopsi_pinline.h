#ifndef _BOOPSI_PINLINE_H
#define _BOOPSI_PINLINE_H

/*
    Copyright (C) 1995-1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Private m68k inlines for boopsi.library
    Lang: english
*/

#ifndef INLINE_MACROS_H
#include <inline/macros.h>
#endif

#ifndef BOOPSI_BASE_NAME
#define BOOPSI_BASE_NAME BOOPSIBase
#endif

#define DoNotify(cl, o, ic, msg) \
	LP4( 0x40 , IPTR, DoNotify, \
	    Class *, (cl), \
	    Object *, (o), \
	    struct ICData *, (ic), \
	    struct opUpdate *, (msg), \
	    , BOOPSI_BASE_NAME )

#define FreeICData(ic) \
	LP1NR( 0x3c, FreeICData, \
	    struct ICData *, (ic), \
	    , BOOPSI_BASE_NAME )

#endif /* _BOOPSI_PINLINE_H */
