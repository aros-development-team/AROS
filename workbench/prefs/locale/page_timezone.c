/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <stdio.h>

#include "earthmap_small_image.c"
#include "timezones_small_image.c"

/*********************************************************************************************/

#define DOUBLE_ARROWS	    	1

#define CONTINENT_RED 	    	18
#define CONTINENT_GREEN     	114
#define CONTINENT_BLUE      	58

#define OCEAN_RED   	    	21
#define OCEAN_GREEN 	    	18
#define OCEAN_BLUE  	    	114

#define SELECTED_INTENSITY_INC	80

#define GAD_DEC     	    	1000
#define GAD_INC     	    	1001
#define GAD_TEXT    	    	1002

/*********************************************************************************************/

#define NUM_TIMEZONES 36

static struct timezone
{
    WORD id;
    WORD minoffset;
    WORD pen;
}
timezone_table[NUM_TIMEZONES] =
{
    {  0,   0	    	}, /* Z :    0:00 */
    {  1,   1 * 60  	}, /* A : +  1:00 */
    {  2,   2 * 60  	}, /* B : +  2:00 */
    {  3,   3 * 60  	}, /* C : +  3:00 */
    {  4,   3 * 60 + 30 }, /* C*: +  3:30 */
    {  5,   4 * 60      }, /* D : +  4:00 */
    {  6,   4 * 60 + 30 }, /* D*: +  4:30 */
    {  7,   5 * 60  	}, /* E : +  5:00 */
    {  8,   5 * 60 + 30 }, /* E*: +  5:30 */
    {  9,   6 * 60  	}, /* F : +  6:00 */
    { 10,   6 * 60 + 30 }, /* F*: +  6:30 */
    { 11,   7 * 60  	}, /* G : +  7:00 */
    { 12,   8 * 60  	}, /* H : +  8:00 */
    { 13,   9 * 60  	}, /* I : +  9:00 */
    { 14,   9 * 60 + 30 }, /* I*: +  9:30 */
    { 15,  10 * 60 	}, /* K : + 10:00 */
    { 16,  10 * 60 + 30 }, /* K*: + 10:30 */
    { 17,  11 * 60 	}, /* L : + 11:00 */
    { 18,  11 * 60 + 30 }, /* L*: + 11:30 */
    { 19,  12 * 60 	}, /* M : + 12:00 */
    { 20,  13 * 60 	}, /* M*: + 13:00 */
    { 35, -12 * 60 	}, /* Y : - 12:00 */
    { 34, -11 * 60 	}, /* X : - 11:00 */
    { 33, -10 * 60 	}, /* W : - 10:00 */
    { 32,  -9 * 60 - 30 }, /* V*: -  9:30 */
    { 31,  -9 * 60 	}, /* V : -  9:00 */
    { 30,  -8 * 60 - 30 }, /* U*: -  8:30 */
    { 29,  -8 * 60 	}, /* U : -  8:00 */
    { 28,  -7 * 60 	}, /* T : -  7:00 */
    { 27,  -6 * 60 	}, /* S : -  6:00 */
    { 26,  -5 * 60 	}, /* R : -  5:00 */
    { 25,  -4 * 60 	}, /* Q : -  4:00 */
    { 24,  -3 * 60 - 30 }, /* P*: -  3:30 */
    { 23,  -3 * 60 	}, /* P : -  3:00 */
    { 22,  -2 * 60 	}, /* O : -  2:00 */
    { 21,  -1 * 60 	}, /* N : -  1:00 */

};

/*********************************************************************************************/

static struct Gadget *gadlist, *textgad, *gad;
static WORD minwidth, minheight;
static WORD domleft, domtop, domwidth, domheight;
static WORD active_timezone;
static BOOL init_done;

static UBYTE timezones_chunky[TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT];
static UBYTE earthmap_chunky[EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT];
static UBYTE *earthmap_chunky_remapped;

static ULONG earthmap_coltab[256];
static WORD  remaptable[256];
static UBYTE timezone_text[256];
static WORD  pen2index[NUM_TIMEZONES];

static BOOL pens_alloced, page_active;

/*********************************************************************************************/

#if EARTHMAP_SMALL_PACKED || TIMEZONES_SMALL_PACKED

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

static LONG timezone_init(void)
{
    WORD i;

    if (!truecolor)
    {
    	earthmap_chunky_remapped = AllocVec(EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT, MEMF_PUBLIC);
	if (!earthmap_chunky_remapped) return FALSE;
    }
    
#if EARTHMAP_SMALL_PACKED
    unpack_byterun1(earthmap_small_data, earthmap_chunky, EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT);
#endif
#if TIMEZONES_SMALL_PACKED
    unpack_byterun1(timezones_small_data, timezones_chunky, TIMEZONES_SMALL_WIDTH * TIMEZONES_SMALL_HEIGHT);
#endif

    
    for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
    {
    	static struct TagItem 	obp_tags[] =
	{
	    {OBP_Precision, PRECISION_IMAGE },
	    {OBP_FailIfBad, FALSE   	    },
	    {TAG_DONE	    	    	    }
	};
    	ULONG 	    	    	rgb = earthmap_small_pal[i];
	ULONG 	    	    	r = (rgb & 0xFF0000) >> 16;
	ULONG 	    	    	g = (rgb & 0x00FF00) >> 8;
	ULONG 	    	    	b = (rgb & 0x0000FF);
	ULONG 	    	    	a = (r + g + b) / 3;
	
	r = (a * OCEAN_RED   + (255 - a) * CONTINENT_RED  ) / 255;
	g = (a * OCEAN_GREEN + (255 - a) * CONTINENT_GREEN) / 255;
	b = (a * OCEAN_BLUE  + (255 - a) * CONTINENT_BLUE ) / 255;
	
	rgb = (r << 16) + (g << 8) + b;
	
	earthmap_coltab[i] = rgb;
	
	if (!truecolor)
	{
	    remaptable[i] = ObtainBestPenA(scr->ViewPort.ColorMap,
	    	    	    	    	   r * 0x01010101,
					   g * 0x01010101,
					   b * 0x01010101,
					   obp_tags);
	}

	r += SELECTED_INTENSITY_INC; if (r > 255) r = 255;
	g += SELECTED_INTENSITY_INC; if (g > 255) g = 255;
	b += SELECTED_INTENSITY_INC; if (b > 255) b = 255;
	
	rgb = (r << 16) + (g << 8) + b;
	
	earthmap_coltab[128 + i] = rgb;

	if (!truecolor)
	{
	    remaptable[128 + i] = ObtainBestPenA(scr->ViewPort.ColorMap,
	    	    	    	    	    	 r * 0x01010101,
					    	 g * 0x01010101,
					    	 b * 0x01010101,
					    	 obp_tags);
	}
	
    }
    
    if (!truecolor) pens_alloced = TRUE;
    
    /* For each timezone find out, with which pen (index) it is
       represented in the timezones image */
       
    for(i = 0; i < NUM_TIMEZONES; i++)
    {
	WORD id = timezone_table[i].id;
    	WORD i2;
    	ULONG r = ((id & 0x30) >> 4) * 64;
	ULONG g = ((id & 0x0C) >> 2) * 64;
	ULONG b = ((id & 0x03)     ) * 64;
	ULONG rgb = (r << 16) + (g << 8) + b;
	
    	timezone_table[i].pen = -1;
	for(i2 = 0; i2 < TIMEZONES_SMALL_COLORS; i2++)
	{
	    if (timezones_small_pal[i2] == rgb)
	    {
	    	timezone_table[i].pen = i2;
		break;
	    }
	}
	
	pen2index[id] = i;
    }
    
    init_done = TRUE;
    
    return TRUE;
}

/*********************************************************************************************/

static void ClearEarthmapSelection(void)
{
    LONG l;
    
    for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
    {
    	earthmap_chunky[l] &= 127;
    }
}

/*********************************************************************************************/

static void SetEarthmapSelection(UBYTE timezonespen)
{
    LONG l;
    
    for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
    {
    	if (timezones_chunky[l] == timezonespen) earthmap_chunky[l] |= 128;
    }
}

/*********************************************************************************************/

static void RepaintEarthmap(void)
{
    if (page_active)
    {
    	CONST_STRPTR fmt;
	WORD   minoffset;
	
	minoffset = timezone_table[active_timezone].minoffset;
	if (minoffset < 0) minoffset = -minoffset;
	
	if (truecolor)
	{
	    WriteLUTPixelArray(earthmap_chunky,
		    	       0,
			       0,
			       EARTHMAP_SMALL_WIDTH,
			       win->RPort,
			       earthmap_coltab,
			       domleft + 1,
			       domtop  + 1,
			       EARTHMAP_SMALL_WIDTH,
			       EARTHMAP_SMALL_HEIGHT,
			       CTABFMT_XRGB8);
	}
	else
	{
    	    LONG l;

	    for(l = 0; l < EARTHMAP_SMALL_WIDTH * EARTHMAP_SMALL_HEIGHT; l++)
	    {
		earthmap_chunky_remapped[l] = remaptable[earthmap_chunky[l]];
	    }

	    WriteChunkyPixels(win->RPort,
	    	    	      domleft + 1,
			      domtop  + 1,
			      domleft + 1 + EARTHMAP_SMALL_WIDTH  - 1,
			      domtop  + 1 + EARTHMAP_SMALL_HEIGHT - 1,
			      earthmap_chunky_remapped,
			      EARTHMAP_SMALL_WIDTH);
	}

	if (minoffset == 60)
	{
	    fmt = MSG(MSG_TIMEZONE_1HOUR);
	}
	else if (minoffset % 60)
	{
	    fmt = MSG(MSG_TIMEZONE_HOURSMINS);
	}
	else
	{
	    fmt = MSG(MSG_TIMEZONE_HOURS);
	}
	
	sprintf(timezone_text, fmt, minoffset / 60, minoffset % 60);
	
	GT_SetGadgetAttrs(textgad, win, NULL, GTTX_Text, (IPTR)timezone_text,
	    	    	    	    	      TAG_DONE);
					       
    } /* if (page_active) */
}

/*********************************************************************************************/

static void scroll_timezone(WORD delta)
{
    active_timezone += delta;
    
    if (active_timezone < 0)
    {
	active_timezone = NUM_TIMEZONES - 1;
    }
    else if (active_timezone >= NUM_TIMEZONES)
    {
	active_timezone = 0;
    }
    
    ClearEarthmapSelection();
    SetEarthmapSelection(timezone_table[active_timezone].pen);
    RepaintEarthmap();
}

/*********************************************************************************************/

static LONG timezone_input(struct IntuiMessage *msg)
{
    LONG retval = FALSE;
    
    if ((msg->Class == IDCMP_MOUSEBUTTONS) && (msg->Code == SELECTDOWN))
    {
    	WORD x = msg->MouseX - (domleft + 1);
	WORD y = msg->MouseY - (domtop + 1);
	
    	if ((x >= 0) &&
	    (y >= 0) &&
	    (x <= domwidth - 2) &&
	    (y <= domheight - 2))
	{
	    ULONG timezonergb;
	    LONG timezoneid;
	    UBYTE timezonepen;
	    
	    retval = TRUE;

	    timezonepen = timezones_chunky[y * TIMEZONES_SMALL_WIDTH + x];
	    timezonergb = timezones_small_pal[timezonepen];
	    timezoneid =  (timezonergb & 0xC00000) >> (16 + 2);
	    timezoneid += (timezonergb & 0x00C000) >> (8 + 4);
	    timezoneid += (timezonergb & 0x0000C0) >> (0 + 6);
	    	    
	    if ((timezoneid >= 0) && (timezoneid < NUM_TIMEZONES))
	    {
	    	active_timezone = pen2index[timezoneid];
		
		/* AmigaOS seems to have the sign the other way round, therefore the "-" */

		localeprefs.lp_GMTOffset = -timezone_table[active_timezone].minoffset;

		ClearEarthmapSelection();
		SetEarthmapSelection(timezonepen);
		RepaintEarthmap();
		
	    }
	    
	}

    }
    else if ((msg->Class == IDCMP_RAWKEY))
    {
    	switch(msg->Code)
	{
	    case CURSORLEFT:
	    	scroll_timezone(-1);
	    	retval = TRUE;
		break;
		
	    case CURSORRIGHT:
	    	scroll_timezone(1);
	    	retval = TRUE;
		break;
		
	}
    }
    else if ((msg->Class == IDCMP_GADGETUP))
    {
    	struct Gadget *gad = (struct Gadget *)msg->IAddress;
	
	switch(gad->GadgetID)
	{
	    case GAD_DEC:
	    	scroll_timezone(-1);
		retval = TRUE;
		break;
		
	    case GAD_INC:
	    	scroll_timezone(1);
		retval = TRUE;
		break;
	}
    }
        
    return retval;
}

/*********************************************************************************************/

static void timezone_cleanup(void)
{
    WORD i;
    
    if (earthmap_chunky_remapped)
    {
    	FreeVec(earthmap_chunky_remapped);
	earthmap_chunky_remapped = NULL;
    }
    
    if (pens_alloced)
    {
    	for(i = 0; i < EARTHMAP_SMALL_COLORS; i++)
	{
	    if (remaptable[i]       != -1) ReleasePen(scr->ViewPort.ColorMap, remaptable[i]);
	    if (remaptable[128 + i] != -1) ReleasePen(scr->ViewPort.ColorMap, remaptable[128 + i]);
	}
	pens_alloced = FALSE;
    }
    
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
}

/*********************************************************************************************/

static LONG timezone_makegadgets(void)
{
    struct NewGadget ng;
    WORD w;
    
    w = dri->dri_Font->tf_XSize + 8;
    
    gad = CreateContext(&gadlist);

    ng.ng_LeftEdge   = domleft;
    ng.ng_TopEdge    = domtop + EARTHMAP_SMALL_HEIGHT + 2 + SPACE_Y;
    ng.ng_Width      = w;
    ng.ng_Height     = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    ng.ng_GadgetText = "<";
    ng.ng_TextAttr   = 0;
    ng.ng_GadgetID   = GAD_DEC;
    ng.ng_Flags      = 0;
    ng.ng_VisualInfo = vi;
   
    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);

#if DOUBLE_ARROWS
    ng.ng_LeftEdge   = domleft + domwidth - w - w;
    
    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);
#endif
    
    ng.ng_LeftEdge   = domleft + domwidth - w;
    ng.ng_GadgetText = ">";
    ng.ng_GadgetID   = GAD_INC;
    
    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);

#if DOUBLE_ARROWS
    ng.ng_LeftEdge   = domleft + w;
    
    gad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);
#endif

    
#if DOUBLE_ARROWS
    ng.ng_LeftEdge   = domleft + w * 2 + SPACE_X;
    ng.ng_Width      = domwidth - w * 4 - SPACE_X * 2;
#else
    ng.ng_LeftEdge   = domleft + w + SPACE_X;
    ng.ng_Width      = domwidth - w * 2 - SPACE_X * 2;
#endif
    ng.ng_GadgetID   = GAD_TEXT;
    ng.ng_GadgetText = NULL;
    
    gad = textgad = CreateGadget(TEXT_KIND, gad, &ng, GTTX_Border   	, TRUE	    	,
    	    	    	    	    	    	      GTTX_Clipped  	, TRUE	    	,
						      GTTX_Text     	, (IPTR)"hello" ,
						      GTTX_Justification, GTJ_CENTER	,
						      TAG_DONE);
    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void timezone_prefs_changed(void)
{
    WORD i;
    
    for(i = 0; i < NUM_TIMEZONES; i++)
    {
    	/* AmigaOS seems to have the sign the other way round, therefore the "-" */
	
    	if (-localeprefs.lp_GMTOffset == timezone_table[i].minoffset)
	{
	    active_timezone = i;
	    break;
	}
    }
    
    if (init_done)
    {
    	ClearEarthmapSelection();
	SetEarthmapSelection(timezone_table[active_timezone].pen);
	RepaintEarthmap();
    }
    
}

/*********************************************************************************************/

static void DrawEarthmapFrame(void)
{
    SetDrMd(win->RPort, JAM1);

    SetAPen(win->RPort, dri->dri_Pens[SHADOWPEN]);

    RectFill(win->RPort, domleft,
		    	 domtop,
			 domleft + domwidth - 2,
			 domtop);

    RectFill(win->RPort, domleft,
		    	 domtop + 1,
			 domleft,
			 domtop + EARTHMAP_SMALL_HEIGHT + 2 - 1);

    SetAPen(win->RPort, dri->dri_Pens[SHINEPEN]);

    RectFill(win->RPort, domleft + domwidth - 1,
		    	 domtop,
			 domleft + domwidth - 1,
			 domtop + EARTHMAP_SMALL_HEIGHT + 2 - 1);

    RectFill(win->RPort, domleft + 1,
		    	 domtop + EARTHMAP_SMALL_HEIGHT + 2 - 1,
			 domleft + domwidth - 2,
			 domtop + EARTHMAP_SMALL_HEIGHT + 2 - 1);
}

/*********************************************************************************************/

LONG page_timezone_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    retval = timezone_init();
	    break;
	    
	case PAGECMD_LAYOUT:
	    minwidth  = EARTHMAP_SMALL_WIDTH + 2;
    	    minheight = EARTHMAP_SMALL_HEIGHT + 2 + dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + SPACE_Y;
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
	    retval = timezone_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
	    	DrawEarthmapFrame();

		AddGList(win, gadlist, -1, -1, NULL);
		GT_RefreshWindow(win, NULL);
		RefreshGList(gadlist, win, NULL, -1);

    	    	ClearEarthmapSelection();
		SetEarthmapSelection(timezone_table[active_timezone].pen);
		
		page_active = TRUE;

		RepaintEarthmap();		
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
	    DrawEarthmapFrame();
	    RepaintEarthmap();
	    break;
	
	case PAGECMD_HANDLEINPUT:
	    retval = timezone_input((struct IntuiMessage *)param);
	    break;
	
	case PAGECMD_PREFS_CHANGED:
	    timezone_prefs_changed();
	    break;
	    
	case PAGECMD_CLEANUP:
	    timezone_cleanup();
	    break;
    }
    
    return retval;
    
}
