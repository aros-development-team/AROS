/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: autoinit library - support function to get the program's name
    Lang: english
*/

#include <proto/exec.h>
#include <proto/dos.h>

#include <exec/memory.h>
#include <dos/dos.h>

#include <workbench/startup.h>

/* Make the programs which don't link against the startup code happy */
struct WBStartup *WBenchMsg __attribute__((weak)) = NULL;

/* Unfortunately we can't rely on the libc's strlen here, since this is the last library
   to be linked in the program.  */
static ULONG strlen(char *str)
{
    char *ptr = str;
    while (*ptr) ptr++;

    return (IPTR)ptr - (IPTR)str;
}

/* Unfortunately we can't rely on the libamiga's StrDup here, since this is the last
   library to be linked in the program.  */
static char *StrDup(char *str)
{
    STRPTR dup;
    ULONG  len;

    if (str == NULL) return NULL;

    len = strlen(str);
    dup = AllocVec(len + 1, MEMF_PUBLIC);
    if (dup != NULL) CopyMem(str, dup, len + 1);

    return dup;

}

char *__getprogramname(void)
{
    char *name   = NULL;

    /* Have we been started from WB? */
    if (WBenchMsg && WBenchMsg->sm_NumArgs)
    {
        name = StrDup(WBenchMsg->sm_ArgList[0].wa_Name);

        if (!name)
	    SetIoErr(ERROR_NO_FREE_STORE);
    }
    /* Otherwise, is this a cli process? */
    else if (0 && Cli() != NULL)
    {
        LONG  namlen = 64;
        int   done   = 0;

        do
        {
	    if (!(name = AllocVec(namlen, MEMF_ANY)))
	    {
	        SetIoErr(ERROR_NO_FREE_STORE);
	        return NULL;
	    }

	    if (!(GetProgramName(name, namlen)))
	    {
	        if (IoErr() == ERROR_LINE_TOO_LONG)
	        {
		    namlen *= 2;
		    FreeVec(name);
	        }
	        else
		    return NULL;
	    }
	    else
	        done = 1;
        } while (!done);

    }
    /* The last resort: use the Task's name.  */
    else
    {
        name = StrDup(FindTask(NULL)->tc_Node.ln_Name);
    }

    return name;
}
