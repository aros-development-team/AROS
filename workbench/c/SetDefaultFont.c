/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/******************************************************************************

    NAME

        SetDefaultFont

    SYNOPSIS

        FONTNAME/A,FONTSIZE/N/A,SCREEN/S

    LOCATION

       C:

    FUNCTION

        Set the default system/screen Font
	
    INPUTS

        FONTNAME  --  the name of the font
	FONTSIZE  --  the size of the font
    	SCREEN    --  if specified set the default screen font otherwise
	              set the default system font.

    RESULT

    NOTES
    	The default system font must be mono spaced (non-proportional)

    EXAMPLE

        SetDefaultFont ttcourier 12

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

******************************************************************************/


#include <exec/exec.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/diskfont.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const TEXT version[] = "$VER: SetDefaultFont 41.1 (1.2.2001)\n";

#define ARG_TEMPLATE "FONTNAME/A,FONTSIZE/N/A,SCREEN/S"

enum
{
    ARG_FONTNAME = 0,
    ARG_FONTSIZE,
    ARG_SCREEN,
    NOOFARGS
};

static struct RDArgs 	*myargs;
static IPTR 	    	args[NOOFARGS];
static char 	    	s[256];
static char 	    	*fontname;
static LONG 	    	fontsize;
static BOOL 	    	screenfont;

static void Cleanup(char *msg, WORD rc)
{
    if (msg)
    {
    	printf("SetDefaultFont: %s\n",msg);
    }
    
    if (myargs)
    {
	FreeArgs(myargs);
    }

    exit(rc);
}

int GfxBase_version = 0;
int IntuitionBase_version = 0;
int DiskFontBase_version = 0;

static void GetArguments(void)
{
    if (!(myargs = ReadArgs(ARG_TEMPLATE, args, 0)))
    {
    	Fault(IoErr(), 0, s, 255);
	Cleanup(s, RETURN_FAIL);
    }
    
    fontname   = (char *)args[ARG_FONTNAME];
    fontsize   = *(IPTR *)args[ARG_FONTSIZE];
    screenfont = args[ARG_SCREEN] ? TRUE : FALSE;
}

static void Action(void)
{
    struct TextAttr ta;
    struct TextFont *font;
    
    strcpy(s, fontname);
    if (!strstr(fontname, ".font")) strcat(s, ".font");
    
    ta.ta_Name  = s;
    ta.ta_YSize = fontsize;
    ta.ta_Style = 0;
    ta.ta_Flags = 0;

    font = OpenDiskFont(&ta);
    if (!font)
    {
    	Cleanup("Can't open font!", RETURN_FAIL);
    }
    
    if (screenfont)
    {
    	SetDefaultScreenFont(font);
    }
    else
    {
   	if (font->tf_Flags & FPF_PROPORTIONAL)
	{
	    CloseFont(font);
	    Cleanup("The font must be mono spaced (non-proportional)!",
		    RETURN_ERROR);
	}
	
	Forbid();
    	GfxBase->DefaultFont = font;
	Permit();
    }
}

int main(void)
{
    GetArguments();    
    Action();
    Cleanup(0, RETURN_OK);
    
    return 0;
}
