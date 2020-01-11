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
        Formatted output to a buffer. Maximal buffer_size characters
        are written including the trainling zero. The string will be
        null-terminated.

    INPUTS
        buffer      - where the string will be written. Might be NULL. In
                      that case the required size will still be returnend.
        
        buffer_size - the size of the buffer. Must be at least 1.
        format      - the format specification
        args        - the arguments which will be filled in

    RESULT
        The number of characters which would have been written without
        the buffer_size limitation. The trailing zero is included.

    NOTES
        The same rules as for RawDoFmt() are valid for format and args.

    EXAMPLE
        TEXT buffer[12];
        IPTR args[2];
        args[0] = (IPTR)"XYZ";
        args[1] = 12345;
        LONG count = VSNPrintf(buffer, sizeof buffer, "ab%scd%ldef", (RAWARG)args);

    BUGS

    SEE ALSO
        exec.library/RawDoFmt()

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

