#ifndef _INTTYPES_H
#define _INTTYPES_H

/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Fixed sized integral types.
*/

#include <sys/types.h>

/* sys/types.h already typedef's a number of these. Here I do the rest. */
typedef UQUAD		uint64_t;
typedef ULONG		uint32_t;
typedef UWORD		uint16_t;
typedef UBYTE		uint8_t;

typedef SIPTR		intptr_t;
typedef IPTR		uintptr_t;

#endif /* _INTTYPES_H */
