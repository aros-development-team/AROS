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

static const char version[] = "$VER: ask 41.3 (4.7.1997)\n";

static struct UtilityBase *UtilityBase;

char * skipwhites(char * buffer)
{
    while (buffer[0] == ' ' || buffer[0] == 0x09)
        buffer++;
    return(buffer);
}

int stripwhites(char * buffer)
{
    int len;
    len = strlen(buffer);
    while ((len != 0) && 
           (buffer[len - 1] == ' ' || 
            buffer[len - 1] == 0x09 || 
            buffer[len - 1] == 0x0a))
        len--;
    return(len);
}

int main(int argc, char **argv)
{
    int error = RETURN_OK;
    STRPTR args[1] = { NULL };
    struct RDArgs *rda;
    char buffer[100];

    UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library", 37);
    if (UtilityBase != NULL)
    {
        rda = ReadArgs("PROMPT/A", (IPTR *)args, NULL);
        if (rda != NULL)
        {
            int ready = 0;
            while (ready == 0)
            {
	        VPrintf("%s ", (IPTR *)&args[0]);
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
                        } else if (Strnicmp(tmpbuf, "n", 1) == 0)
                            ready = 1;
                    } else if (tmplen == 2)
		    {
                        if (Strnicmp(tmpbuf, "no", 2) == 0)
                            ready = 1;
                    } else if (tmplen == 3)
		    {
                        if (Strnicmp(tmpbuf, "yes", 3) == 0)
			{
                            error = RETURN_WARN;
                            ready = 1;
                        }
                    }
                } else
	            ready = 1;
            }
            FreeArgs(rda);
        } else
        {
            PrintFault(IoErr(), "Ask");
            error = RETURN_FAIL;
        }
        CloseLibrary((struct Library *)UtilityBase);
    } else
    {
        PrintFault(ERROR_OBJECT_NOT_FOUND, "Ask");
        error = RETURN_FAIL;
    }

    return(error);
}
