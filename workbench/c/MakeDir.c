/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$

    MakeDir CLI command.
*/

#include <exec/memory.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <utility/tagitem.h>

const TEXT version[] = "$VER: MakeDir 42.6 (3.4.2014)\n";

/******************************************************************************

    NAME

        MakeDir

    SYNOPSIS

        NAME/M/A,ALL/S

    LOCATION

        C:

    FUNCTION

        Create new empty directories with specified names.

    INPUTS

        NAME  --  names of the directories that should be created
        ALL   --  creates intermediate directories

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

BPTR CreateDirAll(STRPTR name);

int __nocommandline;

int main(void)
{
    IPTR           args[NOOFARGS] = { (IPTR) NULL };
    struct RDArgs *rda;
    
    LONG   result = RETURN_OK;
    LONG   error = 0;
    BPTR   lock;
    
    rda = ReadArgs("NAME/M/A,ALL/S", args, NULL);

    if(rda != NULL)
    {
	int      i = 0;
	STRPTR  *name = (STRPTR *)args[ARG_NAME];

	if((name == NULL) || (*name == NULL))
	{
	    error = ERROR_REQUIRED_ARG_MISSING;
	    result = RETURN_FAIL;
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
		if(lock != BNULL)
		{
		    UnLock(lock);
		    result = RETURN_OK;
		}
		else
		{
		    error = IoErr();
		    PutStr("Cannot create directory ");
		    PutStr(name[i]);
		    PutStr("\n");
		    result = RETURN_ERROR;
		}
	    }
	}

	FreeArgs(rda);
    }
    else
	result = RETURN_FAIL;

    if(result != RETURN_OK)
	PrintFault(error, "MakeDir");

    return result;
}

/* CreateDirAll
 *
 * Walk path from left to right, Lock()ing each element. If locking fails,
 * try CreateDir.
 * This routine is smart enough to try optimize multiple '/'s.
 */

BPTR CreateDirAll(STRPTR name)
{
    STRPTR pt = name;
    BOOL first = TRUE;
    BPTR oldcurdir;
    BPTR l, o;
    UBYTE oc = 0;
    int skip = 0;
    UBYTE _fib[sizeof(struct FileInfoBlock) + 3];
    struct FileInfoBlock *fib = (APTR) ((((IPTR) _fib) + 3) & ~3);
    LONG error = 0;

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
                    error = ERROR_DEVICE_NOT_MOUNTED;
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
                            error = (c == '\0' ? ERROR_OBJECT_EXISTS : ERROR_OBJECT_WRONG_TYPE);
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
    SetIoErr(error);
    return BNULL;
}
