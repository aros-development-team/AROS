/*
    Copyright (C) 2025, The AROS Development Team. All rights reserved.

    Desc: Varargs stub for VSNPrintf()
*/

#define NO_INLINE_STDARG /* turn off inline def */
#include <proto/utility.h>
#include <clib/alib_protos.h>

/*****************************************************************************

    NAME */
#include <utility/tagitem.h>
#include <utility/name.h>
        #include <proto/utility.h>

        LONG SNPrintf (

/*  SYNOPSIS */
        STRPTR              buffer,
        LONG                 buffer_size,
        CONST_STRPTR  format,
        ...                )

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
    LONG retval;

    AROS_SLOWSTACKFORMAT_PRE(format);
    retval = VSNPrintf(buffer, buffer_size, format, AROS_SLOWSTACKFORMAT_ARG(format));
    AROS_SLOWSTACKFORMAT_POST(format);

    return retval;
}
