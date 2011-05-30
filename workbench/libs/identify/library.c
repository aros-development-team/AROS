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

#include <aros/symbolsets.h>

#include <proto/identify.h>

#include <strings.h>

#include "identify_intern.h"

// Global library base so that we can call library functions,
// e.g. IdFormatString calls IdHardware.
struct Library *IdentifyBase;

static int InitFunc(struct IdentifyBaseIntern *lh)
{
    IdentifyBase = (struct Library *)lh;

    lh->dirtyflag = TRUE;
    NEWLIST(&lh->libList);
    InitSemaphore(&lh->sem);
    lh->poolMem = CreatePool(MEMF_ANY, 1024, 1024);
    if (!lh->poolMem)
        return FALSE;

    return TRUE;
}

static int ExpungeFunc(struct IdentifyBaseIntern *lh)
{
    DeletePool(lh->poolMem);
    lh->poolMem = NULL;
    return TRUE;
}

ADD2INITLIB(InitFunc, 0);
ADD2EXPUNGELIB(ExpungeFunc, 0);
