/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Varargs version of BuildEasyRequestArgs() (intuition.library)
*/

#include <stdarg.h>
#include "easystruct_util.h"
#include <proto/intuition.h>
#include <proto/alib.h>

/*****************************************************************************

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
