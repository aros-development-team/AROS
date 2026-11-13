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
#ifndef __AROS__
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

struct TextAttr mytextattr = 
{
    "arial.font", 13, 0, 0
};

struct TagItem frtags[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"Custom Positive and Negative text" },
    { ASLFR_PositiveText,       (IPTR)"Load File" },
    { ASLFR_NegativeText,       (IPTR)"Forget it" },
    { ASLFR_InitialShowVolumes, TRUE		  },
    { ASLFR_SetSortBy,		ASLFRSORTBY_Size  },
    { ASLFR_SetSortOrder,	ASLFRSORTORDER_Descend },
    { ASLFR_SetSortDrawers,	ASLFRSORTDRAWERS_Mix	},
    { TAG_DONE,       	        0 }
};

struct TagItem frtags2[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"Save mode" },
    { ASLFR_DoSaveMode,         TRUE},
    { TAG_DONE,       	        0 }
};

struct TagItem frtags3[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"DoPatterns" },
    { ASLFR_DoSaveMode,         TRUE},
    { ASLFR_DoPatterns,		TRUE},
    { TAG_DONE,       	        0 }
};

struct TagItem frtags4[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"Drawers Only" },
    { ASLFR_DrawersOnly,	TRUE},
    { TAG_DONE,       	        0 }
};

struct TagItem frtags5[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"Drawers Only + DoPatterns (pattern hasn't any effect like on AmigaoS)" },
    { ASLFR_DrawersOnly,	TRUE},
    { ASLFR_DoPatterns,		TRUE},
    { TAG_DONE,       	        0 }
};

struct TagItem frtags6[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"Multiselection (use SHIFT)" },
    { ASLFR_DoMultiSelect,	TRUE},
    { ASLFR_InitialDrawer,      (IPTR)"Libs:"},
    { ASLFR_InitialFile,	(IPTR)"Initial file"},
    { ASLFR_DoPatterns,		TRUE},
    { TAG_DONE,       	        0 }
};


struct TagItem frtags_[] =
{
    { ASLFR_TextAttr, (IPTR)&mytextattr},
    { ASLFR_TitleText,	        (IPTR)"The RKM file requester" },
    { ASLFR_InitialHeight,      MYHEIGHT },
    { ASLFR_InitialWidth,       MYWIDTH },
    { ASLFR_InitialLeftEdge,    MYLEFTEDGE },
    { ASLFR_InitialTopEdge,     MYTOPEDGE },
    { ASLFR_PositiveText,       (IPTR)"OKAY" },
    { ASLFR_NegativeText,       (IPTR)"Not OK" },
    { ASLFR_InitialFile,        (IPTR)"asl.library" },
    { ASLFR_InitialDrawer,      (IPTR)"libs:" },
    { ASLFR_DoSaveMode,         TRUE},
    { ASLFR_DoPatterns,		TRUE},
    { ASLFR_DoMultiSelect,      TRUE},
    { TAG_DONE,       	        0 }
};

static void showrequester(char *msg, struct TagItem *tags)
{
    struct FileRequester *fr;

    printf("\n%s:\n",msg ? msg : "");
    
    if ((fr = (struct FileRequester *)AllocAslRequest(ASL_FileRequest, tags)))
    {
	if (AslRequest(fr, NULL))
	{
	    printf("\n-------------------------------------------------------\n\n");
	    printf("PATH=\"%s\"  FILE=\"%s\"\n", fr->rf_Dir, fr->rf_File ? fr->rf_File : (STRPTR)"<NOFILE>");
	    printf("To combine the path and filename, copy the path\n");
	    printf("to a buffer, add the filename with Dos AddPart().\n\n");

	    if(fr->fr_NumArgs > 0)
	    {
		struct WBArg *wbarg = fr->fr_ArgList;
		WORD i;

		printf("MULTI SELECTION:\n"
		       "----------------\n");

		for(i = 1; i <= fr->fr_NumArgs; i++)
		{
		    printf("%3ld: %s\n", (long)i, wbarg->wa_Name);
		    wbarg++;
		}
	    }
	} else printf("\nRequester was aborted\n");
	FreeAslRequest(fr);
    }
    else printf("Could not alloc FileRequester\n");
}

int main(int argc, char **argv)
{
    if ((AslBase = OpenLibrary("asl.library", 37L)))
    {
	showrequester("Default requester with no tags", NULL);
	showrequester(NULL, frtags);
	showrequester(NULL, frtags2);
	showrequester(NULL, frtags3);
	showrequester(NULL, frtags4);
	showrequester(NULL, frtags5);
	showrequester(NULL, frtags6);
	showrequester(NULL, frtags_);

	CloseLibrary(AslBase);
    } else {
        puts("Could not open asl.library!\n");
    }
    return 0;
}
