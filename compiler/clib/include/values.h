#ifndef _VALUES_H_
#define _VALUES_H_

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Sizes of c(++) types
    Lang: english
*/

/* How many bits/bytes has a type? */
#define BITSPERBYTE 8
#define BYTESPERLONG 4
#define BITSPERLONG (BITSPERBYTE * BYTESPERLONG)
#define BYTESPERQUAD 8
#define BITSPERQUAD (BITSPERBYTE * BYTESPERQUAD)

/* Minimum and maximum value of a signed type */
#define MININT (1 << (((sizeof(int) * BITSPERBYTE) - 1))
#define MAXINT (~MININT)

#endif /* _VALUES_H_ */
