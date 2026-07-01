/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    POSIX.1-2024 function getentropy().
*/

#include <errno.h>
#include <stddef.h>

#include <proto/exec.h>
#include <proto/entropy.h>

#include "__posixc_intbase.h"
#include "__optionallibs.h"

/* Largest request POSIX guarantees getentropy() will satisfy in one call. */
#define GETENTROPY_MAX  256

/* Route entropy.resource calls through the base cached in the posixc base. */
#define EntropyBase     PosixCIntBase->PosixCEntropyBase

/*****************************************************************************

    NAME */
#include <unistd.h>

        int getentropy (

/*  SYNOPSIS */
        void  *buffer,
        size_t length)

/*  FUNCTION
        Fill a buffer with high-quality random bytes suitable for seeding a
        pseudo-random generator or for cryptographic keys.

        The bytes are obtained from entropy.resource.  The call does not block
        and always draws from the best source the running system provides
        (software collection plus, where available, CPU hardware entropy).

    INPUTS
        buffer - where to store the random bytes.
        length - the number of bytes wanted; must not exceed 256.

    RESULT
        0 on success.  On failure -1 is returned and errno is set:

        EINVAL - length is greater than 256.
        EFAULT - buffer is NULL.
        EIO    - no entropy source is available, or it failed to deliver.

    NOTES
        Unlike rand()/random(), getentropy() is not seeded by the caller and
        has no reproducible sequence.  It is the POSIX counterpart of the BSD
        arc4random_buf() (see <stdlib.h>).

    EXAMPLE
        unsigned char seed[32];
        if (getentropy(seed, sizeof(seed)) == 0)
            ; // seed[] now holds 32 random bytes

    BUGS

    SEE ALSO
        arc4random(), arc4random_buf(), rand()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCIntBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();

    if (length > GETENTROPY_MAX)
    {
        errno = EINVAL;
        return -1;
    }
    if (buffer == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    if (length == 0)
        return 0;

    if (!__entropy_available(PosixCIntBase))
    {
        errno = EIO;
        return -1;
    }

    if (GetEntropy(buffer, (ULONG)length) != (LONG)length)
    {
        errno = EIO;
        return -1;
    }

    return 0;
} /* getentropy */
