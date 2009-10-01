/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id: CopyToPAR.c 31627 2009-07-26 12:26:01Z mazze $

    Desc: Change some internal options of graphics.library.
    Lang: English
*/
/*****************************************************************************

    NAME

        GfxControl

    SYNOPSIS

        PREVENT_DIRECT_BITMAP_ACCESS=PDBA/S,
	ALLOW_DIRECT_BITMAP_ACCESS=ADBA/S,
	DUMP/S

    LOCATION

        C:

    FUNCTION

        Change some internal options of graphics.library
        
    INPUTS

        PREVENT_DIRECT_BITMAP_ACCESS   --  Causes LockBitMapTagList() calls to
	                                   always fail

        ALLOW_DIRECT_BITMAP_ACCESS     --  Allow LocKBitMapTagList() to go to
	                                   gfx driver which may or may not
					   support it. (default)

    	DUMP	    	    	       --  Show current settings
	
    RESULT

        Standard DOS return codes.

    NOTES
    	By default 
    BUGS

    INTERNALS

******************************************************************************/
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

#include <stdlib.h>
#include <stdio.h>

/****************************************************************************************/

#define ARG_TEMPLATE 	    "PREVENT_DIRECT_BITMAP_ACCESS=PDBA/S,ALLOW_DIRECT_BITMAP_ACCESS=ADBA/S,DUMP/S"
#define ARG_PDBA   	    0
#define ARG_ADBA   	    1
#define ARG_DUMP            2
#define NUM_ARGS    	    3

/****************************************************************************************/

struct RDArgs 	*myargs;
IPTR	         args[NUM_ARGS];
UBYTE	         s[256];

/****************************************************************************************/

static void cleanup(char *msg, ULONG retcode)
{
    if (msg) 
    {
    	fprintf(stderr, "GfxControl: %s\n", msg);
    }
    
    if (myargs) FreeArgs(myargs);

    exit(retcode);
}

/****************************************************************************************/

static void getarguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	cleanup(s, RETURN_FAIL);
    }
}

/****************************************************************************************/

/* See rom graphics/graphics_intern.h */

#define GFXFLAG_PREVENT_DIRECT_BITMAP_ACCESS 0x8000

static void action(void)
{
    if (args[ARG_PDBA])
    {
    	Forbid();
    	GfxBase->GfxFlags |= GFXFLAG_PREVENT_DIRECT_BITMAP_ACCESS;
	Permit();
    }

    if (args[ARG_ADBA])
    {
    	Forbid();
    	GfxBase->GfxFlags &= ~GFXFLAG_PREVENT_DIRECT_BITMAP_ACCESS;
	Permit();
    }
    
    if (args[ARG_DUMP])
    {
    	printf("Prevent Direct BitMap Access: %s\n",
	       (GfxBase->GfxFlags & GFXFLAG_PREVENT_DIRECT_BITMAP_ACCESS) ? "YES" : "NO");
    }
    
}

/****************************************************************************************/

int main(void)
{
    getarguments();
    action();
    cleanup(NULL, 0);
    
    return 0;
}
