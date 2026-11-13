#ifndef _EASYSTRUCT_UTIL_H
#define _EASYSTRUCT_UTIL_H

/*
    Copyright © 2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include "alib_intern.h"
#include <intuition/intuition.h>
#include <proto/exec.h>

static inline STRPTR CreateFormatStringFromEasyStruct(struct EasyStruct *easyStruct)
{
    STRPTR format = NULL;
    LONG lentext = 0, lengadget = 0;

    if (easyStruct->es_TextFormat) lentext = STRLEN(easyStruct->es_TextFormat);
    if (easyStruct->es_GadgetFormat) lengadget = STRLEN(easyStruct->es_GadgetFormat);

    format = AllocVec(lentext + lengadget + 1, MEMF_PUBLIC);
    CopyMem(easyStruct->es_TextFormat, format, lentext);
    CopyMem(easyStruct->es_GadgetFormat, format + lentext, lengadget);
    format[lentext + lengadget] = '\0';

    return format;
}

static inline void FreeFormatString(STRPTR format)
{
    FreeVec(format);
}


#endif /* _EASYSTRUCT_UTIL_H */
