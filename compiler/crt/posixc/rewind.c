/*
    Copyright (C) 1995-2025, The AROS Development Team. All rights reserved.

    Change the position in a stream.
*/

/*****************************************************************************

    NAME */
#include <stdio.h>

        void rewind (

/*  SYNOPSIS */
        FILE * stream)

/*  FUNCTION
        Sets the file position indicator for the given stream to the beginning
        of the file. Any error and end-of-file indicators for the stream are
        cleared.

    INPUTS
        stream - Pointer to a FILE object that identifies the stream to rewind.

    RESULT
        None (void function).

    NOTES
        - Unlike `fseek()`, `rewind()` does not return a value.
        - The function clears the error and EOF indicators for the stream,
          allowing further I/O operations without error states.

    EXAMPLE
        FILE *fp = fopen("example.txt", "r");
        if (fp) {
            // Read some data...
            rewind(fp);  // Reset to start of file
            // Read again from beginning...
            fclose(fp);
        }

    BUGS
        None known.

    SEE ALSO
        fopen(), fwrite(), fseek(), clearerr()

    INTERNALS
        Implemented as a call to `fseek(stream, 0L, SEEK_SET)` followed by
        `clearerr(stream)` to reset stream state indicators.

******************************************************************************/
{
    fseek (stream, 0L, SEEK_SET);
    clearerr (stream);
} /* rewind */

