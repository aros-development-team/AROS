/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <stdarg.h>
#include "alib_intern.h"
#include <proto/intuition.h>
#include <proto/exec.h>
#include <proto/alib.h>

static STRPTR CreateFormatStringFromEasyStruct(struct EasyStruct *easyStruct)
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

static void FreeFormatString(STRPTR format)
{
    FreeVec(format);
}

/*****************************************************************************

    NAME */

    LONG EasyRequest (

/*  SYNOPSIS */
    struct Window       *window,
    struct EasyStruct   *easyStruct,
    ULONG               *IDCMP_ptr,
    ...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    LONG retval;
    STRPTR format = CreateFormatStringFromEasyStruct(easyStruct);

    AROS_SLOWSTACKFORMAT_PRE_USING(IDCMP_ptr, format);
    retval = EasyRequestArgs(window, easyStruct, IDCMP_ptr, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    FreeFormatString(format);

    return retval;
} /* EasyRequest */



/**i**************************************************************************

    NAME */

    struct Window * BuildEasyRequest (

/*  SYNOPSIS */
    struct Window       *RefWindow,
    struct EasyStruct   *easyStruct,
    ULONG               IDCMP,
    ...)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************/
{
    struct Window * retval;
    STRPTR format = CreateFormatStringFromEasyStruct(easyStruct);

    AROS_SLOWSTACKFORMAT_PRE_USING(IDCMP, format);
    retval = BuildEasyRequestArgs(RefWindow, easyStruct, IDCMP, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    FreeFormatString(format);

    return retval;
} /* BuildEasyRequest */
