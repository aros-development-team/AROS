/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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

        Create a link to a file

    INPUTS

        FROM   --  The name of the link
        TO     --  The name of the file or directory to link to
        HARD   --  If specified, the link will be a hard-link; default is
                   to create a soft-link
        FORCE  --  Allow a hard-link to point to a directory

    RESULT

        Standard DOS error codes.

    NOTES

        Avoid using hard links in FFS disk devices. AROS FFS may not hande
        these in a proper manner.

    EXAMPLE

        Makelink ls c:list
         Creates an 'ls' file with a symlink to the 'list' command in C:.

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/

#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/rdargs.h>
#include <dos/stdio.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <utility/tagitem.h>
#include <aros/debug.h>

const TEXT version[] = "$VER: MakeLink 41.1 (2.6.2000)\n";

enum { ARG_FROM = 0, ARG_TO, ARG_HARD, ARG_FORCE };

int __nocommandline;

int main(void)
{
    int  retval = RETURN_FAIL;
    IPTR args[] = { NULL, NULL, (IPTR)FALSE, (IPTR)FALSE };
    struct RDArgs *rda;
    UBYTE *buffer;
    int bufferincrease = 256;
    int buffersize = bufferincrease;

    rda = ReadArgs("FROM/A,TO/A,HARD/S,FORCE/S", args, NULL);

    if(rda != NULL)
    {
        BPTR lock;

        lock = Lock((STRPTR)args[ARG_TO], SHARED_LOCK);

        if(lock != NULL)
        {
            struct FileInfoBlock *fib = AllocDosObject(DOS_FIB, NULL);

            if(fib != NULL)
            {
                if(Examine(lock, fib) == DOSTRUE)
                {
                    if(args[ARG_HARD])
                    {
                        /* Directories may only be hard-linked to if FORCE is
                           specified */
                        if(fib->fib_DirEntryType >= 0 && !(BOOL)args[ARG_FORCE])
                        {
                            PutStr("Hard-links to directories require the FORCE"
                                "keyword\n");
                        }
                        else
                            /* Check loops? */
                            if(MakeLink((STRPTR)args[ARG_FROM],
                                lock,
                                FALSE) == DOSTRUE)
                                retval = RETURN_OK;
                            else
                                PrintFault(IoErr(), "MakeLink");
                    }
                    else
                    {
                        do
                        {
                            if(!(buffer = AllocVec(buffersize, MEMF_ANY)))
                            {
                                SetIoErr(ERROR_NO_FREE_STORE);
                                PrintFault(IoErr(), "MakeLink");
                                break;
                            }

                            /* Get the full path of oldpath */
                            if(NameFromLock(lock, buffer, buffersize))
                            {
                                if(MakeLink((STRPTR)args[ARG_FROM],
                                    (STRPTR)buffer,
                                    TRUE) == DOSTRUE)
                                    retval = RETURN_OK;
                                else
                                {
                                    PrintFault(IoErr(), "MakeLink");
                                    break;
                                }
                            }
                            else if(IoErr() != ERROR_LINE_TOO_LONG)
                            {
                                PrintFault(IoErr(), "MakeLink");
                                break;
                            }
                            FreeVec(buffer);
                            buffersize += bufferincrease;
                        }
                        while(retval != RETURN_OK);
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
        PrintFault(IoErr(), "MakeLink");
        retval = RETURN_FAIL;
    }

    FreeArgs(rda);

    return retval;
}
