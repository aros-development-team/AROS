;/* intuitext.c - Execute me to compile me with SAS C 5.10
LC -b1 -cfistq -v -y -j73 intuitext.c
Blink FROM LIB:c.o,intuitext.o TO intuitext LIBRARY LIB:LC.lib,LIB:Amiga.lib
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


/*
** intuitext.c - program to show the use of an Intuition IntuiText object.
*/
#define INTUI_V36_NAMES_ONLY

#include <exec/types.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdio.h>

static const char version[] = "$VER: intuitext 41.1 (14.3.1997)\n";

#ifdef __AROS__
#ifdef __chip
#undef __chip
#endif
#define __chip
#include <proto/alib.h>
#endif

#ifdef LATTICE
int CXBRK(void)    { return(0); }  /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */
#endif

struct IntuitionBase *IntuitionBase = NULL;

#define MYTEXT_LEFT (0)
#define MYTEXT_TOP  (0)

/*
** main routine. Open required library and window and draw the images.
** This routine opens a very simple window with no IDCMP.  See the
** chapters on "Windows" and "Input and Output Methods" for more info.
** Free all resources when done.
*/
int main(int argc, char **argv)
{
struct Screen	 *screen;
struct DrawInfo  *drawinfo;
struct Window	 *win;
struct IntuiText  myIText;
struct TextAttr   myTextAttr;

ULONG myTEXTPEN;
ULONG myBACKGROUNDPEN;

IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37);
if (IntuitionBase)
    {
#ifdef __AROS__
    if ((screen = LockPubScreen(NULL)))
#else
    if (screen = LockPubScreen(NULL))
#endif
	{
#ifdef __AROS__
	if ((drawinfo = GetScreenDrawInfo(screen)))
#else
	if (drawinfo = GetScreenDrawInfo(screen))
#endif
	    {
	    /* Get a copy of the correct pens for the screen.
	    ** This is very important in case the user or the
	    ** application has the pens set in a unusual way.
	    */
	    myTEXTPEN = drawinfo->dri_Pens[TEXTPEN];
	    myBACKGROUNDPEN  = drawinfo->dri_Pens[BACKGROUNDPEN];

	    /* create a TextAttr that matches the specified font. */
	    myTextAttr.ta_Name	= drawinfo->dri_Font->tf_Message.mn_Node.ln_Name;
	    myTextAttr.ta_YSize = drawinfo->dri_Font->tf_YSize;
	    myTextAttr.ta_Style = drawinfo->dri_Font->tf_Style;
	    myTextAttr.ta_Flags = drawinfo->dri_Font->tf_Flags;

	    /* open a simple window on the workbench screen for displaying
	    ** a text string.  An application would probably never use such a
	    ** window, but it is useful for demonstrating graphics...
	    */
#ifdef __AROS__
	    if ((win = OpenWindowTags(NULL,
				WA_PubScreen,	 (IPTR)screen,
				WA_RMBTrap,	 TRUE,
				WA_IDCMP,	 IDCMP_RAWKEY,
				TAG_END)))
#else
	    if (win = OpenWindowTags(NULL,
				WA_PubScreen,	 screen,
				WA_RMBTrap,	 TRUE,
				TAG_END))
#endif
		{
		myIText.FrontPen    = myTEXTPEN;
		myIText.BackPen     = myBACKGROUNDPEN;
		myIText.DrawMode    = JAM2;
		myIText.LeftEdge    = MYTEXT_LEFT;
		myIText.TopEdge     = MYTEXT_TOP;
		myIText.ITextFont   = &myTextAttr;
		myIText.IText	    = "Hello, World.  ;-)";
		myIText.NextText    = NULL;

		/* Draw the text string at 10,10 */
		PrintIText(win->RPort,&myIText,10,10);

#ifdef __AROS__
		/* Wait for keypress */
		Wait (1L << win->UserPort->mp_SigBit);
#else
		/* Wait a bit, then quit.
		** In a real application, this would be an event loop,
		** like the one described in the Intuition Input and
		** Output Methods chapter.
		*/
		Delay(200);
#endif

		CloseWindow(win);
		}
	    FreeScreenDrawInfo(screen,drawinfo);
	    }
	UnlockPubScreen(NULL,screen);
	}
    CloseLibrary((struct Library *)IntuitionBase);
    }
    return 0;
}
