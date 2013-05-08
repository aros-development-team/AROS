/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Touch CLI command. 
    Lang: english
*/

/******************************************************************************


    NAME

        Touch

    SYNOPSIS

        NAME/A

    LOCATION

        C:

    FUNCTION

        Sets the time stamp of the file to the current time. If the file doesn't
        exist it will be created.

    INPUTS

        NAME -- The name of the file to be touched.

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/

#include <dos/dos.h>
#include <proto/dos.h>

const TEXT version[] = "$VER: Touch 41.1 (7.9.1999)";

int __nocommandline;

int main(void)
{
    IPTR           args[1] = { 0 };
    struct RDArgs *rda     = ReadArgs("NAME/A", args, NULL);

    if (rda)
    {
        struct DateStamp ds;

        /* Attempt to update the file's date stamp */
        if (SetFileDate((CONST_STRPTR)args[0], DateStamp(&ds))) {
            return RETURN_OK;
        } else {
            /* Attempt to create the file, if needed */
            BPTR fh = Open((STRPTR)args[0], MODE_NEWFILE);
            if (fh) {
                Close(fh);
                return RETURN_OK;
            }
        }

        PrintFault(IoErr(), NULL);
   }
   return RETURN_FAIL;
}
