/*
    Copyright © 2020, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <exec/rawfmt.h>

/*****************************************************************************

    NAME */
#include <proto/utility.h>

        AROS_LH4(LONG, VSNPrintf,

/*  SYNOPSIS */
        AROS_LHA(STRPTR, buffer, A0),
        AROS_LHA(LONG, buffer_size, D0),
        AROS_LHA(CONST_STRPTR, format, A1),
        AROS_LHA(RAWARG, args, A2),

/*  LOCATION */
        struct UtilityBase *, UtilityBase, 52, Utility)

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

    LONG required_size = 0;
    STRPTR temp_buffer;
    
    RawDoFmt(format, args, RAWFMTFUNC_COUNT, &required_size);
    if (buffer)
    {
        // required_size already contains trailing zero
        temp_buffer = AllocMem(required_size, MEMF_ANY);
        if (temp_buffer)
        {
            RawDoFmt(format, args, RAWFMTFUNC_STRING, temp_buffer);
            Strlcpy(buffer, temp_buffer, buffer_size);
            FreeMem(temp_buffer, required_size);
        }
    }
    return required_size;
    
    AROS_LIBFUNC_EXIT
}

