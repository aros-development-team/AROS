;/* filereq.c - Execute me to compile me with SASC 5.10
LC -b1 -cfistq -v -y -j73 filereq.c
Blink FROM LIB:c.o,filereq.o TO filereq LIBRARY LIB:LC.lib,LIB:Amiga.lib
quit
*/

/*
Copyright (c) 1992 Commodore-Amiga, Inc.

This example is provided in electronic form by Commodore-Amiga, Inc. for
use with the "Amiga ROM Kernel Reference Manual: Libraries", 3rd Edition,
published by Addison-Wesley (ISBN 0-201-56774-1).

The "Amiga ROM Kernel Reference Manual: Libraries" contains additional
information on the correct usage of the techniques and operating system
functions presented in these examples.	The source and executable code
of these examples may only be distributed in free electronic form, via
bulletin board or as part of a fully non-commercial and freely
redistributable diskette.  Both the source and executable code (including
comments) must be included, without modification, in any copy.	This
example may not be published in printed form or distributed with any
commercial product.  However, the programming techniques and support
routines set forth in these examples may be used in the development
of original executable software products for Commodore Amiga computers.

All other rights reserved.

This example is provided "as-is" and is subject to change; no
warranties are made.  All use is at your own risk. No liability or
responsibility is assumed.
*/

#include <exec/types.h>
#include <exec/libraries.h>
#include <libraries/asl.h>
#ifndef _AROS
#include <clib/exec_protos.h>
#include <clib/asl_protos.h>
#else
#include <proto/exec.h>
#include <proto/asl.h>
#endif
#include <stdio.h>

#ifdef LATTICE
int CXBRK(void)     { return(0); }  /* Disable Lattice CTRL/C handling */
void chkabort(void) { return; }     /* really */
#endif

UBYTE *vers = "$VER: filereq 37.0";

#define MYLEFTEDGE 0
#define MYTOPEDGE  0
#define MYWIDTH    320
#define MYHEIGHT   400

struct Library *AslBase = NULL;

struct TagItem frtags[] =
{
    { ASL_Hail,	      (ULONG)"The RKM file requester" },
    { ASL_Height,     MYHEIGHT },
    { ASL_Width,      MYWIDTH },
    { ASL_LeftEdge,   MYLEFTEDGE },
    { ASL_TopEdge,    MYTOPEDGE },
    { ASL_OKText,     (ULONG)"OKAY" },
    { ASL_CancelText, (ULONG)"Not OK" },
    { ASL_File,	      (ULONG)"asl.library" },
    { ASL_Dir,	      (ULONG)"libs:" },
    { TAG_DONE,       NULL }
};

int main(int argc, char **argv)
{
    struct FileRequester *fr;

    if ((AslBase = OpenLibrary("asl.library", 37L)))
    {
	if ((fr = (struct FileRequester *)
	    AllocAslRequest(ASL_FileRequest, frtags)))
	{
	    if (AslRequest(fr, NULL))
	    {
		printf("PATH=%s  FILE=%s\n", fr->rf_Dir, fr->rf_File);
		printf("To combine the path and filename, copy the path\n");
		printf("to a buffer, add the filename with Dos AddPart().\n");
	    }
	    FreeAslRequest(fr);
	}
	else printf("User Cancelled\n");

	CloseLibrary(AslBase);
    }
    return 0;
}
