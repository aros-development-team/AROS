/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc: Addbuffers CLI command
    Lang: english
*/

#include <stdio.h>
#include <proto/dos.h>
#include <dos/dos.h>

#define ARG_STRING "DRIVE/A,BUFFERS/N"
#define ARG_DRIVE 0
#define ARG_BUFFERS 1
#define ARG_COUNT 2

static const char version[] = "$VER: addbuffers 41.1 (18.2.1997)\n";

int main (int argc, char ** argv)
{
    IPTR args[ARG_COUNT] = { 0, 0 };
    struct RDArgs *rda;
    int result;
    int error = RETURN_OK;
    ULONG *bufsptr;
    ULONG buffers = 0;

    rda = ReadArgs(ARG_STRING, args, NULL);
    if (rda != NULL)
    {
        bufsptr = (ULONG *)args[ARG_BUFFERS];
        if (bufsptr != NULL) buffers = *bufsptr;
        result = AddBuffers((char *)args[ARG_DRIVE], buffers);
        if (result != 0)
            Printf("%s has %ld buffers\n", (LONG)(char *)args[ARG_DRIVE], IoErr());
        else
	{
            PrintFault(IoErr(), "AddBuffers");
            error = RETURN_FAIL;
        }
        FreeArgs(rda);
    } else
    {
        PrintFault(IoErr(), "AddBuffers");
	error = RETURN_FAIL;
    }
    return(error);
}
