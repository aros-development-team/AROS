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

#include <strings.h>

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

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

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
