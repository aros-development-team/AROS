/*
    Copyright © 2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <aros/symbolsets.h>

#include <proto/identify.h>

#include <strings.h>

#include "identify_intern.h"

static int InitFunc(struct IdentifyBaseIntern *lh)
{
    lh->dirtyflag = TRUE;
    memset(&lh->hwb, 0, sizeof lh->hwb);

    return TRUE;
}

static int ExpungeFunc(struct IdentifyBaseIntern *lh)
{
    return TRUE;
}

ADD2INITLIB(InitFunc, 0);
ADD2EXPUNGELIB(ExpungeFunc, 0);
