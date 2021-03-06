/*
    Copyright (C) 1995-2016, The AROS Development Team. All rights reserved.

    Desc: Varargs version of EasyRequestArgs() (intuition.library)
*/

#include <stdarg.h>
#include "easystruct_util.h"
#include <proto/intuition.h>
#include <proto/alib.h>

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

