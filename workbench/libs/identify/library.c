/*
    Copyright © 2010-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
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
