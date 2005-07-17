#ifndef _ENDIAN_H_
#define	_ENDIAN_H_

/*
    Copyright © 2003, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
    BSD systems like to see BYTE_ORDER and friends.

    FreeBSD 5 in its pedantic namespace also has versions with underscores
    to make it easier to port stuff, create them as well.
*/

#include <sys/cdefs.h>

#define _LITTLE_ENDIAN      1234
#define _BIG_ENDIAN         4321
#define _PDP_ENDIAN         3412

#if AROS_BIG_ENDIAN
#   define _BYTE_ORDER  _BIG_ENDIAN
#else
#   define _BYTE_ORDER  _LITTLE_ENDIAN
#endif

#if __BSD_VISIBLE
#   define LITTLE_ENDIAN    _LITTLE_ENDIAN
#   define BIG_ENDIAN       _BIG_ENDIAN
#   define PDP_ENDIAN       _PDP_ENDIAN
#   define BYTE_ORDER       _BYTE_ORDER
#endif

#endif /* _ENDIAN_H_ */
