/*
 * Copyright (c) 2010-2011 Matthias Rustler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *   
 * $Id$
 */

#include <string.h>

#include "identify_intern.h"

/*****************************************************************************

    NAME */
#include <proto/identify.h>

        AROS_LH0(void, IdHardwareUpdate,

/*  SYNOPSIS */
        /* void */

/*  LOCATION */
        struct IdentifyBaseIntern *, IdentifyBase, 10, Identify)

/*  FUNCTION
        Once a hardware information has been evaluated, the result will be
        stored in an internal cache. All subsequent queries return the cache
        contents, irregarding of any changes.

        This function invalidates the cache and forces identify to re-check
        ALL hardware features. Useful if e.g. the amount of memory has changed
        after VMM has been started.

        Use this function wisely. DO NOT call it just to make sure to get the
        latest information, let the user decide to do so. Also, DO NOT call
        it when you will only query hardware information that will for sure
        not change while run-time, e.g. CPU.

        IDHW_VBR, IDHW_LASTALERT and IDHW_TCPIP are NOT cached, so there is
        absolutely no need to call IdHardwareUpdate() just to check them out.

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
        Calling this function in identify V8.0 will lead to a system crash.
        This has been fixed in V8.1.

    SEE ALSO
        IdHardware(), IdHardwareNum()

    INTERNALS

    HISTORY


*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    ObtainSemaphore(&IdentifyBase->sem);

    IdentifyBase->dirtyflag = TRUE;
    memset(&IdentifyBase->hwb, 0, sizeof IdentifyBase->hwb);

    ReleaseSemaphore(&IdentifyBase->sem);

    AROS_LIBFUNC_EXIT
} /* IdHardwareUpdate */
