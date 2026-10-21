/*
  Copyright (C) 2014 Szilard Biro

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "pthread_intern.h"

#include <aros/debug.h>
#include <errno.h>

/*****************************************************************************

    NAME */

#include <signal.h>

	int pthread_sigmask (

/*  SYNOPSIS */
	int  how,
	const  sigset_t *set,
	sigset_t *oldset)

/*  FUNCTION
        Inspect or modify the calling thread's signal mask through its
        task-local POSIX library base.

    INPUTS
        how - Signal-mask operation accepted by sigprocmask().
        set - Requested signal set, or NULL to inspect the existing mask.
        oldset - Optional destination for the previous signal mask.

    RESULT
        Returns zero on success or the positive error number on failure.
        Leaves errno unchanged.

    NOTES
        Uses the same per-task signal-mask implementation as sigprocmask().

******************************************************************************/
{
    int saved_errno = errno;
    int error = 0;

    if (sigprocmask(how, set, oldset) != 0)
        error = errno ? errno : EINVAL;

    /* POSIX: pthread_sigmask() returns the error and leaves errno alone. */
    errno = saved_errno;

    return error;
} /* pthread_sigmask */
