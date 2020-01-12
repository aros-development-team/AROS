;/* shadowborder.c - Execute me to compile me with SAS C 5.10
LC -b1 -cfistq -v -y -j73 shadowborder.c
Blink FROM LIB:c.o,shadowborder.o TO shadowborder LIBRARY LIB:LC.lib,LIB:Amiga.lib
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
** shadowborder.c - program to show the use of an Intuition Border.
*/
#define INTUI_V36_NAMES_ONLY

#include <exec/types.h>
#include <intuition/intuition.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdio.h>

static const char version[] __attribute__((used)) = "$VER: shadowborder 41.1 (14.3.1997)\n";

#ifdef __AROS__
#include <proto/alib.h>
#endif

#ifdef LATTICE
int CXBRK(void)    { return(0); }  /* Disable Lattice CTRL/C handling */
int chkabort(void) { return(0); }  /* really */
#endif

struct IntuitionBase *IntuitionBase = NULL;

#define MYBORDER_LEFT	(0)
#define MYBORDER_TOP	(0)

/* This is the border data. */
WORD myBorderData[] =
    {
    0,0, 50,0, 50,30, 0,30, 0,0,
    };


/*
** main routine. Open required library and window and draw the images.
** This routine opens a very simple window with no IDCMP.  See the
** chapters on "Windows" and "Input and Output Methods" for more info.
** Free all resources when done.
*/
int main(int argc, char **argv)
{
struct Screen	*screen;
struct DrawInfo *drawinfo;
struct Window	*win;
struct Border	 shineBorder;
struct Border	 shadowBorder;

ULONG mySHADOWPEN = 1;	/* set default values for pens */
ULONG mySHINEPEN  = 2;	/* in case can't get info...   */

IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37);
if (IntuitionBase)
    {
    if ((screen = LockPubScreen(NULL)))
	{
	if ((drawinfo = GetScreenDrawInfo(screen)))
	    {
	    /* Get a copy of the correct pens for the screen.
	    ** This is very important in case the user or the
	    ** application has the pens set in a unusual way.
	    */
	    mySHADOWPEN = drawinfo->dri_Pens[SHADOWPEN];
	    mySHINEPEN	= drawinfo->dri_Pens[SHINEPEN];

	    FreeScreenDrawInfo(screen,drawinfo);
	    }
	UnlockPubScreen(NULL,screen);
	}

    /* open a simple window on the workbench screen for displaying
    ** a border.  An application would probably never use such a
    ** window, but it is useful for demonstrating graphics...
    */
#ifdef __AROS__
    if ((win = OpenWindowTags(NULL,
			WA_PubScreen,  (IPTR)screen,
			WA_RMBTrap,	 TRUE,
			WA_IDCMP,	IDCMP_RAWKEY,
			TAG_END)))
#else
    if ((win = OpenWindowTags(NULL,
			WA_PubScreen,  screen,
			WA_RMBTrap,	 TRUE,
			TAG_END)))
#endif
	{
	/* set information specific to the shadow component of the border */
	shadowBorder.LeftEdge	= MYBORDER_LEFT + 1;
	shadowBorder.TopEdge	= MYBORDER_TOP + 1;
	shadowBorder.FrontPen	= mySHADOWPEN;
	shadowBorder.NextBorder = &shineBorder;

	/* set information specific to the shine component of the border */
	shineBorder.LeftEdge	= MYBORDER_LEFT;
	shineBorder.TopEdge	= MYBORDER_TOP;
	shineBorder.FrontPen	= mySHINEPEN;
	shineBorder.NextBorder	= NULL;

	/* the following attributes are the same for both borders. */
	shadowBorder.BackPen	= shineBorder.BackPen	= 0;
	shadowBorder.DrawMode	= shineBorder.DrawMode	= JAM1;
	shadowBorder.Count	= shineBorder.Count	= 5;
	shadowBorder.XY 	= shineBorder.XY	= myBorderData;

	/* Draw the border at 10,10 */
	DrawBorder(win->RPort,&shadowBorder,20,50);

	/* Draw the border again at 100,10 */
	DrawBorder(win->RPort,&shadowBorder,100,50);

#ifdef __AROS__
	Wait (1L << win->UserPort->mp_SigBit);
#else
	/* Wait a bit, then quit.
	** In a real application, this would be an event loop, like the
	** one described in the Intuition Input and Output Methods chapter.
	*/
	Delay(200);
#endif

	CloseWindow(win);
	}
    CloseLibrary((struct Library *)IntuitionBase);
    }
    return 0;
}
