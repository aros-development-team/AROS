/*
    Copyright (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <stdio.h>

#include "computermouse_image.c"

/*********************************************************************************************/

static struct Gadget *gadlist, *textgad, *gad;
static WORD minwidth, minheight;
static WORD domleft, domtop, domwidth, domheight;
static BOOL init_done;

static UBYTE computermouse_chunky[COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT];
static UBYTE *computermouse_chunky_remapped;

static ULONG computermouse_coltab[256];
static WORD  remaptable[256];

static BOOL pens_alloced, page_active;

/*********************************************************************************************/

#if COMPUTERMOUSE_PACKED

static UBYTE *unpack_byterun1(UBYTE *source, UBYTE *dest, LONG unpackedsize)
{
    UBYTE r;
    BYTE c;
    
    for(;;)
    {
	c = (BYTE)(*source++);
	if (c >= 0)
	{
    	    while(c-- >= 0)
	    {
		*dest++ = *source++;
		if (--unpackedsize <= 0) return source;
	    }
	}
	else if (c != -128)
	{
    	    c = -c;
	    r = *source++;

	    while(c-- >= 0)
	    {
		*dest++ = r;
		if (--unpackedsize <= 0) return source;
	    }
	}
    }
    
}

#endif

/*********************************************************************************************/

static LONG mouse_init(void)
{
    WORD i;

    if (!truecolor)
    {
    	computermouse_chunky_remapped = AllocVec(COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT, MEMF_PUBLIC);
	if (!computermouse_chunky_remapped) return FALSE;
    }
    
#if COMPUTERMOUSE_PACKED
    unpack_byterun1(computermouse_data, computermouse_chunky, COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT);
#endif
    
    for(i = 1; i < COMPUTERMOUSE_COLORS; i++)
    {
    	static struct TagItem 	obp_tags[] =
	{
	    {OBP_Precision, PRECISION_IMAGE },
	    {OBP_FailIfBad, FALSE   	    },
	    {TAG_DONE	    	    	    }
	};
    	ULONG 	    	    	rgb = computermouse_pal[i];
	ULONG 	    	    	r = (rgb & 0xFF0000) >> 16;
	ULONG 	    	    	g = (rgb & 0x00FF00) >> 8;
	ULONG 	    	    	b = (rgb & 0x0000FF);

	computermouse_coltab[i] = rgb;
	
	if (!truecolor)
	{
	    remaptable[i] = ObtainBestPenA(scr->ViewPort.ColorMap,
	    	    	    	    	   r * 0x01010101,
					   g * 0x01010101,
					   b * 0x01010101,
					   obp_tags);
	}

    }
    
    if (!truecolor)
    {
    	LONG l;

    	remaptable[0] = dri->dri_Pens[BACKGROUNDPEN];
	
	for(l = 0; l < COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT; l++)
	{
	    computermouse_chunky_remapped[l] = remaptable[computermouse_chunky[l]];
	}
    }
    else
    {
	ULONG col[3];

	GetRGB32(scr->ViewPort.ColorMap,
	    	 dri->dri_Pens[BACKGROUNDPEN],
		 1,
		 col);

	computermouse_coltab[0] = ((col[0] & 0xFF000000) >> 8) +
	    	    	    	  ((col[1] & 0xFF000000) >> 16) +
		    	    	  ((col[2] & 0xFF000000) >> 24);

    }
    
    init_done = TRUE;
    
    return TRUE;
}

/*********************************************************************************************/

static void RepaintComputerMouse(void)
{
    if (page_active)
    {
	if (truecolor)
	{
	    WriteLUTPixelArray(computermouse_chunky,
		    	       0,
			       0,
			       COMPUTERMOUSE_WIDTH,
			       win->RPort,
			       computermouse_coltab,
			       domleft + 1,
			       domtop  + 1,
			       COMPUTERMOUSE_WIDTH,
			       COMPUTERMOUSE_HEIGHT,
			       CTABFMT_XRGB8);
	}
	else
	{
	    WriteChunkyPixels(win->RPort,
	    	    	      domleft + 1,
			      domtop  + 1,
			      domleft + 1 + COMPUTERMOUSE_WIDTH  - 1,
			      domtop  + 1 + COMPUTERMOUSE_HEIGHT - 1,
			      computermouse_chunky_remapped,
			      COMPUTERMOUSE_WIDTH);
	}

    } /* if (page_active) */
}

/*********************************************************************************************/

static LONG mouse_input(struct IntuiMessage *msg)
{
    return FALSE;
}

/*********************************************************************************************/

static void mouse_cleanup(void)
{
    WORD i;
    
    if (computermouse_chunky_remapped)
    {
    	FreeVec(computermouse_chunky_remapped);
	computermouse_chunky_remapped = NULL;
    }
    
    if (pens_alloced)
    {
    	for(i = 1; i < COMPUTERMOUSE_COLORS; i++)
	{
	    if (remaptable[i] != -1) ReleasePen(scr->ViewPort.ColorMap, remaptable[i]);
	}
	pens_alloced = FALSE;
    }
    
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
}

/*********************************************************************************************/

static LONG mouse_makegadgets(void)
{
    return TRUE;
}

/*********************************************************************************************/

static void mouse_prefs_changed(void)
{

}

/*********************************************************************************************/

LONG page_mouse_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    retval = mouse_init();
	    break;
	    
	case PAGECMD_LAYOUT:
	    minwidth  = COMPUTERMOUSE_WIDTH * 4 + 2;
    	    minheight = COMPUTERMOUSE_HEIGHT * 2 + 2;
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	    retval = minwidth;
	    break;
	    
	case PAGECMD_GETMINHEIGHT:
	    retval = minheight;
	    break;
	    
	case PAGECMD_SETDOMLEFT:
	    domleft = param;
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    /* domwidth = param; */
	    domwidth = minwidth;
	    domleft += (param - minwidth ) /2;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    /* domheight = param; */
	    domheight = minheight;
	    domtop += (param - minheight) / 2;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	    retval = mouse_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
		page_active = TRUE;

	    	RepaintComputerMouse();
	    }
	    break;
	    
	case PAGECMD_REMGADGETS:
	    if (page_active)
	    {
		if (gadlist) RemoveGList(win, gadlist, -1);
		
	    	page_active = FALSE;
	    }
	    break;
	
	case PAGECMD_REFRESH:
	    RepaintComputerMouse();
	    break;
	
	case PAGECMD_HANDLEINPUT:
	    retval = mouse_input((struct IntuiMessage *)param);
	    break;
	
	case PAGECMD_PREFS_CHANGED:
	    mouse_prefs_changed();
	    break;
	    
	case PAGECMD_CLEANUP:
	    mouse_cleanup();
	    break;
    }
    
    return retval;
    
}
