/*
    (C) 1997 AROS - The Amiga Replacement OS
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

static const char version[] = "$VER: Ask 1.0 (19.2.1997)\n";

static struct UtilityBase *UtilityBase;

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
	        printf("%s ", args[0]);
                if (FGets(Input(), buffer, 100) == (STRPTR)buffer)
	        {
                    if (Stricmp(buffer, "\n") == 0)
                        ready = 1;
                    else
		    {
                        STRPTR pos;
                        printf("string: %s ($%02X.%02X.%02X.%02X)", buffer, buffer[0], buffer[1], buffer[2], buffer[3]);
                        pos = strchr(buffer, 0x0a);
                        if (pos != NULL)
                            pos[0] = 0x00;
                        if (Stricmp(buffer, "y") == 0 || Stricmp(buffer, "yes") == 0)
                        {
                            error = RETURN_WARN;
                            ready = 1;
                        } else if (Stricmp(buffer, "n") == 0 || Stricmp(buffer, "no") == 0)
                            ready = 1;
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

    printf("err: %d\n", error); /* !!! */
    return(error);
}
