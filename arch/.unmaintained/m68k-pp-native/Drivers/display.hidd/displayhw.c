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
#include <asm/registers.h>

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
    int     	    	    width, height, FBPitch, left, right, i, j, SRCPitch, phase;
    register unsigned long  m;
    unsigned char   	    s1, s2, s3, s4;
    unsigned long   	    *src, *srcPtr;
    unsigned char   	    *dst, *dstPtr;
   
    FBPitch  = RREG_B(LVPW) * 2;
    SRCPitch = bmap->width / 4;

    left = pbox->x1 & ~7;
    right = (pbox->x2 & ~7) + 7;

    while(num--)
    {
        width  = (right - left + 1) >> 3;
        height = pbox->y2 - pbox->y1 + 1;
        src    = (unsigned long*)(bmap->VideoData + (pbox->y1 * SRCPitch * 4) + left); 
        dst    = (unsigned char*)(RREG_L(LSSA) + (pbox->y1 * FBPitch) + (left / 8));

	if((phase = ((long)dst & 3L)))
	{
	    phase = 4 - phase;
	    if(phase > width) phase = width;
	    width -= phase;
	}

        while(height--)
	{
	    dstPtr = dst;
	    srcPtr = src;
	    i = width;
	    j = phase;

//#define MERGE (m >> 24) | (m >> 15) | (m >> 6) | (m << 3)
#define MERGE   (m >> 21) | (m >> 14) | (m >> 7) | m

	    while(j--)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		*dstPtr++ = MERGE;
		srcPtr += 2;
	    }

	    while(i >= 4)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		s1 = MERGE;
		m = (srcPtr[3] & 0x01010101) | ((srcPtr[2] & 0x01010101) << 4);
 		s2 = MERGE;
		m = (srcPtr[5] & 0x01010101) | ((srcPtr[4] & 0x01010101) << 4);
 		s3 = MERGE;
		m = (srcPtr[7] & 0x01010101) | ((srcPtr[6] & 0x01010101) << 4);
 		s4 = MERGE;
		*((unsigned long*)dstPtr) = s4 | (s3 << 8) | (s2 << 16) | (s1 << 24);
		srcPtr += 8;
		dstPtr += 4;
		i -= 4;
	    }

	    while(i--)
	    {
		m = (srcPtr[1] & 0x01010101) | ((srcPtr[0] & 0x01010101) << 4);
 		*dstPtr++ = MERGE;
		srcPtr += 2;
	    }
	    
            dst += FBPitch;
            src += SRCPitch;
	    
        } /* while(height--) */
        pbox++;
	
    } /* while(num--) */ 

} 

/****************************************************************************************/
