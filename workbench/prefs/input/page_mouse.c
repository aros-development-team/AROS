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
#include <string.h>

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

#define DOUBLECLICK_DELAY_TICS_MIN 1
#define DOUBLECLICK_DELAY_TICS_MAX 200
#define DOUBLECLICK_DELAY_TICS_RANGE (DOUBLECLICK_DELAY_TICS_MAX - DOUBLECLICK_DELAY_TICS_MIN + 1)

/*********************************************************************************************/

static struct Gadget *gadlist, *gad, *leftmapgad, *rightmapgad, *midmapgad;
static struct Gadget *doublegad, *showdoublegad, *speedgad, *accelgad;
static WORD minwidth, minheight;
static WORD domleft, domtop, domwidth, domheight;
static WORD mapgroupwidth, mapgroupheight, mapgroupx1, mapgroupy1;
static WORD doublegroupwidth, doublegroupheight;
static WORD doublegroupx1, doublegroupy1;
static WORD mapgadwidth, computermousex1, computermousey1;
static WORD speedgroupwidth, speedgroupheight;
static WORD speedgroupx1, speedgroupy1, speedgadwidth;
static WORD leftlinex1, leftliney1, rightlinex1, rightliney1, midlinex1, midliney1;
static WORD showtimewidth;
static BOOL init_done;

static UBYTE computermouse_chunky[COMPUTERMOUSE_WIDTH * COMPUTERMOUSE_HEIGHT];
static UBYTE *computermouse_chunky_remapped;

static CONST_STRPTR maplabels[4];
static CONST_STRPTR speedlabels[4];
static ULONG computermouse_coltab[256];
static WORD  remaptable[256];
static UBYTE showdoubleclickbuf[30];

static BOOL pens_alloced, page_active, intuiprefs_changed;

static struct Preferences restore_intui_prefs;

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

static void try_setting_mousespeed(void)
{
    struct Preferences p;
    
    GetPrefs(&p, sizeof(p));
    p.PointerTicks = inputprefs.ip_PointerTicks;
    p.DoubleClick  = inputprefs.ip_DoubleClick;
    p.KeyRptDelay  = inputprefs.ip_KeyRptDelay;
    p.KeyRptSpeed  = inputprefs.ip_KeyRptSpeed;
    if (inputprefs.ip_MouseAccel)
    {
    	p.EnableCLI |= MOUSE_ACCEL;
    }
    else
    {
    	p.EnableCLI &= ~MOUSE_ACCEL;
    }
    
    SetPrefs(&p, sizeof(p), FALSE);
    
    intuiprefs_changed = TRUE;
}

/*********************************************************************************************/

static void DrawFrames(void)
{
    DrawFrameWithTitle(win->RPort,
    	    	       mapgroupx1,
		       mapgroupy1,
		       mapgroupx1 + mapgroupwidth - 1,
		       mapgroupy1 + mapgroupheight - 1,
		       MSG(MSG_GAD_MOUSE_BUTTON_MAPPING));
 
    DrawFrameWithTitle(win->RPort,
    	    	       doublegroupx1,
		       doublegroupy1,
		       doublegroupx1 + doublegroupwidth - 1,
		       doublegroupy1 + doublegroupheight - 1,
		       MSG(MSG_GAD_MOUSE_DOUBLECLICK_DELAY));

    DrawFrameWithTitle(win->RPort,
    	    	       speedgroupx1,
		       speedgroupy1,
		       speedgroupx1 + speedgroupwidth - 1,
		       speedgroupy1 + speedgroupheight - 1,
		       MSG(MSG_GAD_MOUSE_SPEED));

}

/*********************************************************************************************/

static LONG mouse_init(void)
{
    WORD i;

    GetPrefs(&restore_intui_prefs, sizeof(restore_intui_prefs));
    
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
    static LONG speedids[] =
    {
    	MSG_GAD_MOUSE_SPEED_SLOW,
	MSG_GAD_MOUSE_SPEED_NORMAL,
	MSG_GAD_MOUSE_SPEED_FAST
    };
    struct RastPort temprp;
    WORD    	    i, w, biggestw;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    /* Button mapping gadgets */
    
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
           
    /* Double-Click Delay gadget */
    
    for(i = DOUBLECLICK_DELAY_TICS_MIN; i <= DOUBLECLICK_DELAY_TICS_MAX; i++)
    {
    	LONG secs, micro;
	
	secs = i / 50;
	micro = (i % 50) * 2;
	
    	snprintf(showdoubleclickbuf, sizeof(showdoubleclickbuf), MSG(MSG_TIME_FORMAT), secs, micro);
    	w = TextLength(&temprp, showdoubleclickbuf, strlen(showdoubleclickbuf));
	if (w > showtimewidth) showtimewidth = w;
    }
    
    doublegroupwidth  = TextLength(&temprp,
    	    	    	    	   MSG(MSG_GAD_MOUSE_DOUBLECLICK_DELAY),
				   strlen(MSG(MSG_GAD_MOUSE_DOUBLECLICK_DELAY)));

    doublegroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (doublegroupwidth < 200 + LABELSPACE_X + showtimewidth)
    {
    	doublegroupwidth = 200 + LABELSPACE_X + showtimewidth;
    }

    doublegroupheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + FRAME_FRAMEHEIGHT;

    /* Speed gadgets */
    
    biggestw = 0;
    for(i = 0; i < 3; i++)
    {
    	speedlabels[i] = MSG(speedids[i]);
	w = TextLength(&temprp, speedlabels[i], strlen(speedlabels[i]));
	if (w > biggestw) biggestw = w;
    }

    speedgadwidth = biggestw + CYCLE_EXTRAWIDTH;
    speedgroupwidth = TextLength(&temprp,
    	    	    	    	 MSG(MSG_GAD_MOUSE_SPEED),
				 strlen(MSG(MSG_GAD_MOUSE_SPEED)));
    speedgroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (speedgroupwidth < speedgadwidth + LABELSPACE_X)
    {
    	speedgroupwidth = speedgadwidth + LABELSPACE_X;
    }
    speedgroupwidth += FRAME_FRAMEWIDTH;
    speedgroupheight = (dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT) * 2 + GROUPSPACE_X + FRAME_FRAMEHEIGHT;
    
    minwidth  = mapgroupwidth + GROUPSPACE_X + speedgroupwidth;
    minheight = mapgroupheight + GROUPSPACE_Y + doublegroupheight;

    DeinitRastPort(&temprp);
    
    return TRUE;
}

/*********************************************************************************************/

static void update_showdouble_gad(void)
{
    struct TagItem settags[] =
    {
    	{GTTX_Text, (IPTR)showdoubleclickbuf},
	{TAG_DONE   	    	      	    }
    };
    LONG secs, micro;
    
    secs  = inputprefs.ip_DoubleClick.tv_secs;
    micro = inputprefs.ip_DoubleClick.tv_micro / 10000;
    
    snprintf(showdoubleclickbuf, sizeof(showdoubleclickbuf), MSG(MSG_TIME_FORMAT), secs, micro);
    
    GT_SetGadgetAttrsA(showdoublegad, win, NULL, settags);
}

/*********************************************************************************************/

static void update_double_gad(void)
{
    struct TagItem scsettags[] =
    {
    	{GTSC_Top, 0},
	{TAG_DONE   }
    };
    LONG delay;
    
    delay = inputprefs.ip_DoubleClick.tv_secs * 50;
    delay += inputprefs.ip_DoubleClick.tv_micro / (1000000 / 50);
    
    if (delay < DOUBLECLICK_DELAY_TICS_MIN)
    {
    	delay = DOUBLECLICK_DELAY_TICS_MIN;
    }
    else if (delay > DOUBLECLICK_DELAY_TICS_MAX)
    {
    	delay = DOUBLECLICK_DELAY_TICS_MAX;
    }
    
    scsettags[0].ti_Data = delay - DOUBLECLICK_DELAY_TICS_MIN;
    
    GT_SetGadgetAttrsA(doublegad, win, NULL, scsettags);
    
    update_showdouble_gad();
}

/*********************************************************************************************/

static void update_speed_gad(void)
{
    struct TagItem cysettags[] =
    {
    	{GTCY_Active, 0 },
	{TAG_DONE   	}
    };
    LONG active;
    
    switch(inputprefs.ip_PointerTicks)
    {	    
	case 3:
	case 4:
	    active = 0;
	    break;

	case 2:
	    active = 1;
	    break;
	    	    
	case 1:
	default:
	    active = 2;
	    break;

    }

    cysettags[0].ti_Data = active;
    
    GT_SetGadgetAttrsA(speedgad, win, NULL, cysettags);
}

/*********************************************************************************************/

static void update_accel_gad(void)
{
    struct TagItem cbsettags[] =
    {
    	{GTCB_Checked, 0},
	{TAG_DONE   	}
    };
    
    cbsettags[0].ti_Data = inputprefs.ip_MouseAccel ? TRUE : FALSE;    

    GT_SetGadgetAttrsA(accelgad, win, NULL, cbsettags);
}


/*********************************************************************************************/

static LONG mouse_input(struct IntuiMessage *msg)
{
    struct Gadget   *gad;
    LONG    	    retval = FALSE;
    LONG    	    top;

    gad =(struct Gadget *)msg->IAddress;

    switch(msg->Class)
    {
	case IDCMP_GADGETUP:
	    if (gad == speedgad)
	    {
	    	inputprefs.ip_PointerTicks = 1 << (2 - msg->Code);
		try_setting_mousespeed();
		break;
	    }
	    else if (gad == accelgad)
	    {
	    	inputprefs.ip_MouseAccel = msg->Code ? 1 : 0;
		try_setting_mousespeed();
	    	break;
	    }
	    
	    /* Fall through */
	    
	case IDCMP_GADGETDOWN:
	case IDCMP_MOUSEMOVE:
    	    if (gad == doublegad)
	    {
		top = msg->Code + DOUBLECLICK_DELAY_TICS_MIN;

		inputprefs.ip_DoubleClick.tv_secs = top / 50;
		inputprefs.ip_DoubleClick.tv_micro = (top % 50) * (1000000 / 50);
		update_showdouble_gad();

		retval = TRUE;
	    }
	    break;

    } /* switch(msg->Class) */
    
    return retval;
     
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
    
    if (intuiprefs_changed)
    {
    	SetPrefs(&restore_intui_prefs, sizeof(restore_intui_prefs), FALSE);
    }

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
    
    doublegroupx1 = domleft;
    doublegroupy1 = mapgroupy1 + mapgroupheight + GROUPSPACE_Y;
    
    ng.ng_LeftEdge = doublegroupx1 + FRAME_OFFX;
    ng.ng_TopEdge  = doublegroupy1 + FRAME_OFFY;
    ng.ng_Width    = showtimewidth;
    ng.ng_Height   = doublegroupheight - FRAME_FRAMEHEIGHT;
    ng.ng_GadgetID = 0;
    
    gad = showdoublegad = CreateGadget(TEXT_KIND, gad, &ng, GTTX_Text, (IPTR)showdoubleclickbuf,
    	    	    	    	    	    	    	    GTTX_Justification, GTJ_RIGHT,
							    TAG_DONE);
    
    ng.ng_LeftEdge += showtimewidth + LABELSPACE_X;
    ng.ng_Width    = doublegroupwidth - FRAME_FRAMEWIDTH - LABELSPACE_X - showtimewidth;
    ng.ng_GadgetID = MSG_GAD_MOUSE_DOUBLECLICK_DELAY;
    
    gad = doublegad = CreateGadget(SCROLLER_KIND, gad, &ng, GTSC_Total, DOUBLECLICK_DELAY_TICS_RANGE + 9,
    	    	    	    	    	    	    	    GTSC_Visible, 10,
							    TAG_DONE);
			    			
    ng.ng_LeftEdge = speedgroupx1 + FRAME_OFFX;
    ng.ng_TopEdge  = speedgroupy1 + FRAME_OFFY;
    ng.ng_Width    = speedgroupwidth - FRAME_FRAMEWIDTH;
    ng.ng_Height   = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    ng.ng_GadgetID = MSG_GAD_MOUSE_SPEED;
    
    gad = speedgad = CreateGadget(CYCLE_KIND, gad, &ng, GTCY_Labels, (IPTR)speedlabels,
    	    	    	    	    	    	    	    	     TAG_DONE);

    ng.ng_TopEdge    += ng.ng_Height + GROUPSPACE_X;
    ng.ng_Width      = ng.ng_Height;
    ng.ng_GadgetID   = MSG_GAD_MOUSE_ACCELERATED;
    ng.ng_GadgetText = MSG(MSG_GAD_MOUSE_ACCELERATED);
    ng.ng_Flags      = PLACETEXT_RIGHT;
    
    gad = accelgad = CreateGadget(CHECKBOX_KIND, gad, &ng, GTCB_Scaled, TRUE, TAG_DONE);
            							    
    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void mouse_prefs_changed(void)
{
    if (page_active)
    {
    	update_double_gad();
	update_speed_gad();
	update_accel_gad();
    }
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
	    //kprintf("setdomleft. domleft = %d\n", domleft);
    	    speedgroupx1 = domleft + mapgroupwidth + GROUPSPACE_X;
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
    	    speedgroupy1 = domtop;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    domwidth = param;
	    speedgroupwidth  = domleft + domwidth - speedgroupx1;
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

    	    	mouse_prefs_changed();
		
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
