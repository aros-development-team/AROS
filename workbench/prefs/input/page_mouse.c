/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/debug.h>
#include <stdio.h>

#include "computermouse_image.c"

/*********************************************************************************************/

#define FRAME_FRAMEWIDTH  (FRAMESPACEX * 2)
#define FRAME_FRAMEHEIGHT (FRAMESPACEY + dri->dri_Font->tf_YSize + SPACE_Y)

#define FRAME_OFFX (FRAMESPACEX)
#define FRAME_OFFY (dri->dri_Font->tf_YSize + SPACE_Y)

#define LABELSPACE_X 4

#define CYCLE_EXTRAWIDTH (dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + 16)

#define COMPUTERMOUSE_CENTERX (COMPUTERMOUSE_WIDTH / 2)
#define COMPUTERMOUSE_MAPPOSY 45
#define COMPUTERMOUSE_POINTSY 20
#define COMPUTERMOUSE_LEFTPOINT (COMPUTERMOUSE_CENTERX - 18)
#define COMPUTERMOUSE_RIGHTPOINT (COMPUTERMOUSE_CENTERX + 18)

#define MAPGAD_SPACEX 5 /* should be odd */

/*********************************************************************************************/

static struct Gadget *gadlist, *gad, *leftmapgad, *rightmapgad, *midmapgad;
static WORD minwidth, minheight;
static WORD domleft, domtop, domwidth, domheight;
static WORD mapgroupwidth, mapgroupheight, mapgroupx1, mapgroupy1;
static WORD mapgadwidth, computermousex1, computermousey1;
static WORD leftlinex1, leftliney1, rightlinex1, rightliney1, midlinex1, midliney1;
static BOOL init_done;

static UBYTE computermouse_chunky[COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT];
static UBYTE *computermouse_chunky_remapped;

static STRPTR maplabels[4];
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

static void DrawFrames(void)
{
    DrawFrameWithTitle(win->RPort,
    	    	       mapgroupx1,
		       mapgroupy1,
		       mapgroupx1 + mapgroupwidth - 1,
		       mapgroupy1 + mapgroupheight - 1,
		       MSG(MSG_GAD_MOUSE_BUTTON_MAPPING));

}

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

static void DrawDrop(WORD x, WORD y)
{
    RectFill(win->RPort, x - 1, y - 1, x + 1, y + 1);
    Move(win->RPort, x - 2, y); Draw(win->RPort, x + 2, y);
    Move(win->RPort, x, y - 2); Draw(win->RPort, x, y + 2);
    
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
			       computermousex1,
			       computermousey1,
			       COMPUTERMOUSE_WIDTH,
			       COMPUTERMOUSE_HEIGHT,
			       CTABFMT_XRGB8);
	}
	else
	{
	    WriteChunkyPixels(win->RPort,
			      computermousex1,
			      computermousey1,
			      computermousex1 + COMPUTERMOUSE_WIDTH  - 1,
			      computermousey1 + COMPUTERMOUSE_HEIGHT - 1,
			      computermouse_chunky_remapped,
			      COMPUTERMOUSE_WIDTH);
	}

    	SetAPen(win->RPort, dri->dri_Pens[SHADOWPEN]);
	SetDrMd(win->RPort, JAM1);
	
	Move(win->RPort, leftlinex1, leftliney1);
	Draw(win->RPort, leftlinex1, computermousey1 + COMPUTERMOUSE_POINTSY);
	Draw(win->RPort, computermousex1 + COMPUTERMOUSE_LEFTPOINT, computermousey1 + COMPUTERMOUSE_POINTSY);
	DrawDrop(computermousex1 + COMPUTERMOUSE_LEFTPOINT, computermousey1 + COMPUTERMOUSE_POINTSY);
	
	Move(win->RPort, rightlinex1, rightliney1);
	Draw(win->RPort, rightlinex1, computermousey1 + COMPUTERMOUSE_POINTSY);
	Draw(win->RPort, computermousex1 + COMPUTERMOUSE_RIGHTPOINT, computermousey1 + COMPUTERMOUSE_POINTSY);
	DrawDrop(computermousex1 + COMPUTERMOUSE_RIGHTPOINT, computermousey1 + COMPUTERMOUSE_POINTSY);
	
	Move(win->RPort, midlinex1, midliney1);
	Draw(win->RPort, midlinex1, computermousey1 + COMPUTERMOUSE_POINTSY);
	DrawDrop(midlinex1, computermousey1 + COMPUTERMOUSE_POINTSY);
	
    } /* if (page_active) */
}

/*********************************************************************************************/

static LONG mouse_layout(void)
{
    static LONG butmapids[] =
    {
    	MSG_GAD_MOUSE_MAPPING_SELECT,
	MSG_GAD_MOUSE_MAPPING_MENU,
	MSG_GAD_MOUSE_MAPPING_THIRD
    };
    struct RastPort temprp;
    WORD    	    i, w, biggestw;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    biggestw = 0;
    for(i = 0; i < 3; i++)
    {
    	maplabels[i] = MSG(butmapids[i]);
	w = TextLength(&temprp, maplabels[i], strlen(maplabels[i]));
	if (w > biggestw) biggestw = w;
    }
    
    mapgadwidth = biggestw + CYCLE_EXTRAWIDTH;
    
    biggestw = mapgadwidth * 2 + MAPGAD_SPACEX + FRAME_FRAMEWIDTH;
    
    w = TextLength(&temprp, MSG(MSG_GAD_MOUSE_BUTTON_MAPPING), strlen(MSG(MSG_GAD_MOUSE_BUTTON_MAPPING)));
    w += FRAMETITLE_EXTRAWIDTH;
    
    if (w > biggestw) biggestw = w;
    
    w = COMPUTERMOUSE_WIDTH + FRAME_FRAMEWIDTH;
    if (w > biggestw) biggestw = w;
    
    mapgroupwidth  = biggestw;
    mapgroupheight = COMPUTERMOUSE_HEIGHT + FRAME_FRAMEHEIGHT;
           
    minwidth  = mapgroupwidth;
    minheight = mapgroupheight;

    DeinitRastPort(&temprp);
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
    struct NewGadget ng;
    
    gad = CreateContext(&gadlist);

    mapgroupx1 = domleft;
    mapgroupy1 = domtop;

    computermousex1 = (mapgroupwidth - FRAME_FRAMEWIDTH - COMPUTERMOUSE_WIDTH) / 2 + mapgroupx1 + FRAME_OFFX;
    computermousey1 = (mapgroupheight - FRAME_FRAMEHEIGHT - COMPUTERMOUSE_HEIGHT) / 2 + mapgroupy1 + FRAME_OFFY;
     
    ng.ng_LeftEdge   = computermousex1 + COMPUTERMOUSE_CENTERX - (MAPGAD_SPACEX / 2) - mapgadwidth ;
    ng.ng_TopEdge    = computermousey1 + COMPUTERMOUSE_MAPPOSY;
    ng.ng_Width      = mapgadwidth;
    ng.ng_Height     = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    ng.ng_GadgetText = NULL;
    ng.ng_TextAttr   = 0;
    ng.ng_GadgetID   = MSG_GAD_MOUSE_MAPPING_SELECT;
    ng.ng_Flags      = 0;
    ng.ng_VisualInfo = vi;
     
    leftliney1 = rightliney1 = ng.ng_TopEdge - 1;
    leftlinex1 = ng.ng_LeftEdge + (mapgadwidth / 2);
    
    gad = leftmapgad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR)maplabels,
    	    	    	    	    	    	    	  TAG_DONE);
		
    ng.ng_LeftEdge  += MAPGAD_SPACEX + mapgadwidth;
    ng.ng_GadgetID  =  MSG_GAD_MOUSE_MAPPING_MENU;
    
    rightlinex1 = ng.ng_LeftEdge + (mapgadwidth / 2);
    
    gad = rightmapgad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR)maplabels,
    	    	    	    	    	    	    	   TAG_DONE);
	
    ng.ng_LeftEdge =  computermousex1 + COMPUTERMOUSE_CENTERX - (mapgadwidth / 2);
    ng.ng_TopEdge  += ng.ng_Height + SPACE_Y;
    ng.ng_GadgetID =  MSG_GAD_MOUSE_MAPPING_THIRD;
    	
    midlinex1 = computermousex1 + COMPUTERMOUSE_CENTERX;
    midliney1 = ng.ng_TopEdge - 1;
    					   
    gad = midmapgad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR)maplabels,
    	    	    	    	    	    	    	   TAG_DONE);
    
    return gad ? TRUE : FALSE;
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
	    retval = mouse_layout();
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	    retval = minwidth;
	    break;
	    
	case PAGECMD_GETMINHEIGHT:
	    retval = minheight;
	    break;
	    
	case PAGECMD_SETDOMLEFT:
	    domleft = param;
	    kprintf("setdomleft. domleft = %d\n", domleft);
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    domwidth = param;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    domheight = param;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	    retval = mouse_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
		page_active = TRUE;

	    	RepaintComputerMouse();

		AddGList(win, gadlist, -1, -1, NULL);
		GT_RefreshWindow(win, NULL);
		RefreshGList(gadlist, win, NULL, -1);
		DrawFrames();
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
	    RefreshGList(leftmapgad, win, NULL, 1);
	    RefreshGList(rightmapgad, win, NULL, 1);	    
	    RefreshGList(midmapgad, win, NULL, 1);
	    DrawFrames();
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
