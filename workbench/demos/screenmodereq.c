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

struct TagItem smtags[] =
{
    { ASLSM_TitleText,	        (IPTR)"Custom Positive and Negative text" },
    { ASLSM_PositiveText,       (IPTR)"Use Screenmode" },
    { ASLSM_NegativeText,       (IPTR)"Forget it" },
    { TAG_DONE,       	        NULL }
};

struct TagItem smtags2[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoOverscanType" },
    { ASLSM_DoOverscanType,     TRUE},
    { TAG_DONE,       	        NULL }
};

struct TagItem smtags3[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoWidth" },
    { ASLSM_DoWidth,            TRUE},
    { TAG_DONE,       	        NULL }
};

struct TagItem smtags4[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoHeight" },
    { ASLSM_DoHeight,		TRUE},
    { TAG_DONE,       	        NULL }
};

struct TagItem smtags5[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoDepth" },
    { ASLSM_DoDepth,		TRUE},
    { TAG_DONE,       	        NULL }
};

struct TagItem smtags6[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoAutoScroll" },
    { ASLSM_DoAutoScroll,	TRUE},
    { TAG_DONE,       	        NULL }
};


struct TagItem smtags7[] =
{
    { ASLSM_TitleText,	        (IPTR)"DoAll" },
    { ASLSM_DoOverscanType,	TRUE},
    { ASLSM_DoWidth,		TRUE},
    { ASLSM_DoHeight,		TRUE},
    { ASLSM_DoDepth,		TRUE},
    { ASLSM_DoAutoScroll,	TRUE},
    { TAG_DONE,       	        NULL }
};

static void showrequester(char *msg, struct TagItem *tags)
{
    struct ScreenModeRequester *sm;

    printf("\n%s:\n",msg ? msg : "");
    
    if ((sm = (struct ScreenModeRequester *)AllocAslRequest(ASL_ScreenModeRequest, tags)))
    {
	if (AslRequest(sm, NULL))
	{
	    printf("\n-------------------------------------------------------\n\n");
	} else printf("Requester was aborted\n");
	FreeAslRequest(sm);
    }
    else printf("Could not alloc FileRequester\n");
}

int main(int argc, char **argv)
{
    if ((AslBase = OpenLibrary("asl.library", 37L)))
    {
	showrequester("Default requester with no tags", NULL);
	showrequester(NULL, smtags);
	showrequester(NULL, smtags2);
	showrequester(NULL, smtags3);
	showrequester(NULL, smtags4);
	showrequester(NULL, smtags5);
	showrequester(NULL, smtags6);
	showrequester(NULL, smtags7);

	CloseLibrary(AslBase);
    } else {
        puts("Could not open asl.library!\n");
    }
    return 0;
}
