/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Display Inside...
    Lang: English.
*/

/*
    NOTE: This file is mostly based on XFree86 vgaWH.c sources
*/

/****************************************************************************************/

#include <proto/exec.h>
#include <oop/oop.h>
#include "displayhw.h"
#include "displayclass.h"
#include "bitmap.h"

/****************************************************************************************/

/****************************************************************************************/

#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

/****************************************************************************************/


/*
** Load specified palette, if NULL load default
*/
void DisplayLoadPalette(struct DisplayHWRec *regs, unsigned char *pal)
{
}

/****************************************************************************************/


/*
** displayHWSaveScreen
**      perform a sequencer reset.
*/
void DisplaySaveScreen(int start)
{
}

/****************************************************************************************/

/*
** Blank the screen.
*/
int DisplayBlankScreen(int on)
{
    return TRUE;
}

/****************************************************************************************/

#define DisplayIOBase	0x3d0

/****************************************************************************************/

/*
** displayRestore --
**      restore a video mode
*/
void DisplayRestore(struct displayHWRec *restore, BOOL onlyDac)
{
}

/****************************************************************************************/

/*
** displaySave --
**      save the current video mode
*/
void * DisplaySave(struct displayHWRec *save)
{

	return ((void *) save);
}

/****************************************************************************************/

/*
** Init VGA registers for given displaymode
*/
int DisplayInitMode(struct DisplayModeDesc *mode, struct displayHWRec *regs)
{
    return TRUE;
}

/****************************************************************************************/

void DisplayRefreshArea(struct bitmap_data *bmap, int num, struct Box *pbox)
{
} 

/****************************************************************************************/
