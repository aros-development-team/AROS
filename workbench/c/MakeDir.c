/*
    Copyright © 1995-2004, The AROS Development Team. All rights reserved.
    $Id$

    MakeDir CLI command.
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

#include <stdio.h>

static const char version[] = "$VER: MakeDir 42.4 (17.10.2005)\n";

/******************************************************************************

    NAME

        MakeDir

    SYNOPSIS

        NAME/M,ALL/S

    LOCATION

        Workbench:C

    FUNCTION

        Create new empty directories with specified names.

    INPUTS

        NAME  --  names of the directories that should be created

    RESULT

    NOTES

        MakeDir does not create an icon for a new directory.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/


enum
{
    ARG_NAME = 0,
    ARG_ALL,
    NOOFARGS
};

LONG CreateDirAll(STRPTR name);

int __nocommandline;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR) NULL };
    struct RDArgs *rda;
    
    LONG   error = RETURN_OK;
    BPTR   lock;
    
    rda = ReadArgs("NAME/M,ALL/S", args, NULL);

    if(rda != NULL)
    {
	int      i = 0;
	STRPTR  *name = (STRPTR *)args[ARG_NAME];

	if((name == NULL) || (*name == NULL))
	{
	    PutStr("No name given\n");
	    error = RETURN_FAIL;
	}
	else
	{
	    for(i = 0; name[i] != NULL; i++)
	    {
                if (args[ARG_ALL])
                    lock = CreateDirAll(name[i]);
                else
		    lock = CreateDir(name[i]);

		/* The AmigaDOS semantics are quite strange here. When it is
		   impossible to create a certain directory, MakeDir goes on
		   to try to create the rest of the specified directories and
		   returns the LAST return value for the operation. */
		if(lock != NULL)
		{
		    UnLock(lock);
		    error = RETURN_OK;
		}
		else
		{
		    PutStr("Cannot create directory ");
		    PutStr(name[i]);
		    PutStr("\n");
		    error = RETURN_ERROR;
		}
	    }
	}

	FreeArgs(rda);
    }
    else
	error = RETURN_FAIL;

    if(error != RETURN_OK)
	PrintFault(IoErr(), NULL);

    return error;
}

/* CreateDirAll
 *
 * Walk path from left to right, Lock()ing each element. If locking fails,
 * try CreateDir.
 * This routine is smart enough to try optimize multiple '/'s.
 */

LONG CreateDirAll(STRPTR name)
{
    STRPTR pt = name;
    BOOL first = TRUE;
    BPTR oldcurdir;
    BPTR l, o;
    UBYTE oc = 0;
    int skip = 0;
    UBYTE _fib[sizeof(struct FileInfoBlock) + 3];
    struct FileInfoBlock *fib = (APTR) ((((IPTR) _fib) + 3) & ~3);

    CurrentDir(oldcurdir = CurrentDir(0));

    for (;;)
    {
        UBYTE c = *pt++;

        if (c == ':' || c == '/' || c == '\0')
        {
            if (c == ':')
            {
                skip = 0;

                if (!first)
                {
                    SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
                    break;
                }
                first = FALSE;
                oc = pt[skip];
                pt[skip] = '\0';
                //Printf("Lock \"%s\"\n", (LONG) name);
                l = Lock(name, ACCESS_READ);
            }
            else
            {
                skip = 0;
                if (c == '/')
                {
                    while (pt[skip] == '/')
                    {
                        skip++;
                    }
                }

                oc = pt[skip];
                pt[skip] = '\0';
                //Printf("Lock \"%s\"\n", (LONG) name);
                l = Lock(name, ACCESS_READ);
                if (!l)
                {
                    pt[skip] = oc;
                    skip = *name != '/' && c == '/' ? -1 : 0;
                    oc = pt[skip];
                    pt[skip] = '\0';

                    //Printf("CreateDir \"%s\"\n", (LONG) name);
                    l = name[0] == '/' || name[0] == '\0' ? 0 : CreateDir(name);
                    if (l)
                    {
                        if (!ChangeMode(CHANGE_LOCK, l, ACCESS_READ))
                        {
                            UnLock(l);
                            l = Lock(name, ACCESS_READ);
                        }
                    }
                }
                else
                {
                    LONG res;

                    /* Make sure it's a directory */
                    if (!(res = Examine(l, fib)) || fib->fib_DirEntryType < 0)
                    {
                        UnLock(l);
                        if (res)
                        {
                            SetIoErr(c == '\0' ? ERROR_OBJECT_EXISTS : ERROR_OBJECT_WRONG_TYPE);
                        }
                        break;
                    }
                    pt += skip;
                    skip = 0;
                }
            }

            if (!l)
            {
                break;
            }

            o = CurrentDir(l);
            if (o != oldcurdir)
            {
                UnLock(o);
            }

            pt[skip] = oc;
            name = pt;

            if (c == '\0')
            {
                //Printf("return success\n");
                return CurrentDir(oldcurdir);
            }
        }
    }

    pt[skip] = oc;

    o = CurrentDir(oldcurdir);
    if (o != oldcurdir)
    {
        UnLock(o);
    }

    //Printf("return error\n");
    return 0;
}
