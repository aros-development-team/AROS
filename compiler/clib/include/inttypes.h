#ifndef _INTTYPES_H
#define _INTTYPES_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Fixed sized integral types.
*/

#include <sys/types.h>

/* sys/types.h already typedef's a number of these. Here I do the rest. */
#ifndef _STDINT_H_
typedef UQUAD		uint64_t;
typedef ULONG		uint32_t;
typedef UWORD		uint16_t;
typedef UBYTE		uint8_t;

typedef SIPTR		intptr_t;
typedef IPTR		uintptr_t;
#endif

#endif /* _INTTYPES_H */
