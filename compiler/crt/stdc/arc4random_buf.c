/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BSD extension arc4random_buf().
*/

#include <stddef.h>

#include "__stdc_intbase.h"
#include "__arc4random.h"

/*****************************************************************************

    NAME */
#include <stdlib.h>

        void arc4random_buf (

/*  SYNOPSIS */
        void  *buf,
        size_t nbytes)

/*  FUNCTION
        Fill a buffer with cryptographically-strong random bytes.

        The bytes are obtained from entropy.resource (a ChaCha20 CSPRNG seeded
        from software and, where available, CPU hardware entropy).  If the
        resource is not present a self-contained fallback is used so that the
        buffer is always filled.

    INPUTS
        buf    - the buffer to fill.
        nbytes - the number of bytes to write.

    RESULT
        None.

    NOTES
        A NULL buf or a zero nbytes is a harmless no-op.

    EXAMPLE
        unsigned char key[32];
        arc4random_buf(key, sizeof(key));

    BUGS

    SEE ALSO
        arc4random(), arc4random_uniform()

    INTERNALS

******************************************************************************/
{
    struct StdCIntBase *StdCBase = (struct StdCIntBase *)__aros_getbase_StdCBase();

    if (buf == NULL || nbytes == 0)
        return;

    __arc4random_buf(StdCBase, buf, nbytes);
} /* arc4random_buf */
