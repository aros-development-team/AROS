/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: Ask CLI command
    Lang: english
*/

#include <stdio.h>
#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos/dos.h>
#include <proto/utility.h>

#include "shcommands.h"

static int stripwhites(char * buffer);
static char * skipwhites(char * buffer);

AROS_SH1(Ask, 41.3,
AROS_SHA(STRPTR, ,PROMPT,/A,NULL))
{
    AROS_SHCOMMAND_INIT

    char buffer[100];
    int ready = 0;
    int error = RETURN_OK;

    struct UtilityBase *UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if (!UtilityBase)
        return RETURN_FAIL;

    while (ready == 0)
    {
	VPrintf("%s ", (IPTR *)&SHArg(PROMPT));
	Flush(Output());
        if (FGets(Input(), buffer, 100) == (STRPTR)buffer)
	{
            char * tmpbuf;
            int tmplen;
            tmpbuf = skipwhites(buffer);
            tmplen = stripwhites(tmpbuf);
            if (tmplen == 0)
                ready = 1;
            else if (tmplen == 1)
	    {
                if (Strnicmp(tmpbuf, "y", 1) == 0)
		{
                    error = RETURN_WARN;
                    ready = 1;
		}
		else
		if (Strnicmp(tmpbuf, "n", 1) == 0)
  		    ready = 1;
 	    }
	    else
	    if (tmplen == 2)
	    {
                if (Strnicmp(tmpbuf, "no", 2) == 0)
                    ready = 1;
     	    }
	    else
	    if (tmplen == 3)
	    {
                if (Strnicmp(tmpbuf, "yes", 3) == 0)
		{
                    error = RETURN_WARN;
                    ready = 1;
                }
	    }
	}
	else
	    ready = 1;
    }

    CloseLibrary((struct Library *)UtilityBase);

    return error;

    AROS_SHCOMMAND_EXIT
}

static int stripwhites(char * buffer)
{
    int len;
    len = strlen(buffer);
    while ((len != 0) &&
           (buffer[len - 1] == ' ' ||
            buffer[len - 1] == '\t' ||
            buffer[len - 1] == '\n'))
        len--;
    return len;
}

static char * skipwhites(char * buffer)
{
    while (buffer[0] == ' ' || buffer[0] == '\t')
        buffer++;
    return buffer;
}
