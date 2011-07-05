/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Touch CLI command. 
    Lang: english
*/

#include <dos/dos.h>
#include <proto/dos.h>
#include <dos/filesystem.h>

int __nocommandline;

int main(void)
{
    IPTR           args[1] = {NULL};
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
