;/* simpleimage.c - program to show the use of a simple Intuition Image.
lc -b1 -cfist -v -y -j73 simpleimage.c
blink FROM LIB:c.o+"simpleimage.o" TO "simpleimage" LIB LIB:lc.lib LIB:amiga.lib
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

#define INTUI_V36_NAMES_ONLY

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>

#include <stdio.h>

static const char version[] = "$VER: simpleimage 41.1 (14.3.1997)\n";

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

#define MYIMAGE_LEFT	(0)
#define MYIMAGE_TOP	(0)
#define MYIMAGE_WIDTH	(24)
#define MYIMAGE_HEIGHT	(10)
#define MYIMAGE_DEPTH	(1)

/* This is the image data.  It is a one bit-plane open rectangle which is 24
** pixels wide and 10 high.  Make sure that it is in CHIP memory, or allocate
** a block of chip memory with a call like this: AllocMem(data_size,MEMF_CHIP),
** and then copy the data to that block.  See the Exec chapter on Memory
** Allocation for more information on AllocMem().
*/
UWORD __chip myImageData[] =
    {
    0xFFFF, 0xFF00,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xC000, 0x0300,
    0xFFFF, 0xFF00,
    };

/*
** main routine. Open required library and window and draw the images.
** This routine opens a very simple window with no IDCMP.  See the
** chapters on "Windows" and "Input and Output Methods" for more info.
** Free all resources when done.
*/
int main(int argc, char *argv[])
{
struct Window *win;
struct Image myImage;

IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",37);
if (IntuitionBase != NULL)
    {
#ifdef __AROS__
    if (NULL != (win = OpenWindowTags(NULL,
			WA_Width,	200,
			WA_Height,	100,
			WA_RMBTrap,	TRUE,
			WA_IDCMP,	IDCMP_RAWKEY,
			TAG_END)))
#else
    if (NULL != (win = OpenWindowTags(NULL,
			WA_Width,	200,
			WA_Height,	100,
			WA_RMBTrap,	TRUE,
			TAG_END)))
#endif
	{
	myImage.LeftEdge    = MYIMAGE_LEFT;
	myImage.TopEdge     = MYIMAGE_TOP;
	myImage.Width	    = MYIMAGE_WIDTH;
	myImage.Height	    = MYIMAGE_HEIGHT;
	myImage.Depth	    = MYIMAGE_DEPTH;
	myImage.ImageData   = myImageData;
	myImage.PlanePick   = 0x1;		/* use first bit-plane	   */
	myImage.PlaneOnOff  = 0x0;		/* clear all unused planes */
	myImage.NextImage   = NULL;

	/* Draw the 1 bit-plane image into the first bit-plane (color 1) */
	DrawImage(win->RPort,&myImage,10,10);

	/* Draw the same image at a new location */
	DrawImage(win->RPort,&myImage,100,10);

#ifdef __AROS__
	/* Wait for a keypress */
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
