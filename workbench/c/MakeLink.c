/*
    Copyright © 1995-2008, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        MakeLink

    SYNOPSIS

        FROM/A, TO/A, HARD/S, FORCE/S

    LOCATION

        C:

    FUNCTION

        Create a link to a file.

    INPUTS

        FROM   --  The name of the link
        TO     --  The name of the file or directory to link to
        HARD   --  If specified, the link will be a hard link; default is
                   to create a soft link
        FORCE  --  Allow a hard-link to point to a directory

    RESULT

        Standard DOS error codes.

    NOTES

        Not all file systems support links.

    EXAMPLE

        Makelink ls C:List
         Creates an 'ls' file with a soft link to the 'List' command in C:.

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <aros/debug.h>

const TEXT version[] = "$VER: MakeLink 41.1 (2.6.2000)\n";

enum { ARG_FROM = 0, ARG_TO, ARG_HARD, ARG_FORCE };

int __nocommandline;

int main(void)
{
    int  retval = RETURN_FAIL;
    IPTR args[] = { (IPTR)NULL, (IPTR)NULL, (IPTR)FALSE, (IPTR)FALSE };
    struct RDArgs *rda;

    rda = ReadArgs("FROM/A,TO/A,HARD/S,FORCE/S", args, NULL);

    if(rda != NULL)
    {
        if(args[ARG_HARD])
        {
            BPTR lock;

            lock = Lock((STRPTR)args[ARG_TO], SHARED_LOCK);

            if(lock != BNULL)
            {
                struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

                if(fib != NULL)
                {
                    if(Examine(lock, fib))
                    {
                        /* Directories may only be hard-linked to if FORCE is
                           specified */
                        if(fib->fib_DirEntryType >= 0 && !(BOOL)args[ARG_FORCE])
                        {
                            PutStr("Hard links to directories require the"
                                " FORCE keyword\n");
                        }
                        else
                        {
                            /* Check loops? */
                            if(MakeLink((STRPTR)args[ARG_FROM], (SIPTR)lock, FALSE))
                                retval = RETURN_OK;
                            else
                                PrintFault(IoErr(), "MakeLink");
                        }
                    }

                    FreeDosObject(DOS_FIB, fib);
                }

                UnLock(lock);
            }
            else
            {
                PutStr((STRPTR)args[ARG_TO]);
                PrintFault(IoErr(), "");
            }
        }
        else
        {
            if(MakeLink((STRPTR)args[ARG_FROM], (SIPTR)args[ARG_TO], TRUE))
                retval = RETURN_OK;
        }
    }
    else
    {
        PrintFault(IoErr(), "MakeLink");
        retval = RETURN_FAIL;
    }

    FreeArgs(rda);

    return retval;
}
