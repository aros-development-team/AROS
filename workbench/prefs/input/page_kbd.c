/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <devices/keymap.h>
#include <aros/asmcall.h>
#include <aros/debug.h>
#include <stdio.h>
#include <string.h>

/*********************************************************************************************/

#define FRAME_FRAMEWIDTH  (FRAMESPACEX * 2)
#define FRAME_FRAMEHEIGHT (FRAMESPACEY + dri->dri_Font->tf_YSize + SPACE_Y)

#define FRAME_OFFX (FRAMESPACEX)
#define FRAME_OFFY (dri->dri_Font->tf_YSize + SPACE_Y)

#define LABELSPACE_X 4

#define REPEAT_DELAY_TICS_MIN 1
#define REPEAT_DELAY_TICS_MAX 75
#define REPEAT_DELAY_TICS_RANGE (REPEAT_DELAY_TICS_MAX - REPEAT_DELAY_TICS_MIN + 1)

#define REPEAT_RATE_TICS_MIN 0
#define REPEAT_RATE_TICS_MAX 12
#define REPEAT_RATE_TICS_RANGE (REPEAT_RATE_TICS_MAX - REPEAT_RATE_TICS_MIN + 1)

#define REVERSE_RATE_SCROLLER 1

/*********************************************************************************************/

static struct Gadget 	*gadlist, *gad, *lvgad, *rategad, *delaygad, *showrategad, *showdelaygad;
static struct Gadget	*testgad;
static BPTR 	    	testkeymap_seg;
static WORD 	    	minwidth, minheight;
static WORD 	    	domleft, domtop, domwidth, domheight;
static WORD 	    	lvwidth, lvheight, lvgroupwidth, lvgroupheight, lvgroupx1, lvgroupy1;
static WORD 	    	ratewidth, rateheight, rategroupwidth, rategroupheight, rategroupx1, rategroupy1;
static WORD 	    	delaywidth, delayheight, delaygroupwidth, delaygroupheight, delaygroupx1, delaygroupy1;
static WORD 	     	testheight, testgroupwidth, testgroupheight, testgroupx1, testgroupy1;
static WORD 	    	showtimewidth;

static BOOL 	    	init_done;

static BOOL 	    	page_active, inputdev_changed;
static UBYTE	    	showratebuf[30], showdelaybuf[30];

/*********************************************************************************************/

static void try_setting_test_keymap(void)
{
    struct KeyMapResource *KeyMapResource;
    struct KeyMapNode	  *kmn = NULL;
    struct Node     	  *node;
    BPTR    	    	   lock, seg, olddir, oldseg = 0;
    
    if ((KeyMapResource = OpenResource("keymap.resource")))
    {
    	Forbid();
	
    	ForeachNode(&KeyMapResource->kr_List, node)
	{
	    if (!stricmp(inputprefs.ip_Keymap, node->ln_Name))
	    {
	    	kmn = (struct KeyMapNode *)node;
		break;
	    }
	}
	
	Permit();
    }
    
    if (!kmn)
    {
	lock = Lock("DEVS:Keymaps", SHARED_LOCK);

	if (lock)
    	{
	    olddir = CurrentDir(lock);
	    
	    
	    if ((seg = LoadSeg(inputprefs.ip_Keymap)))
	    {
	    	kmn = (struct KeyMapNode *) (((UBYTE *)BADDR(seg)) + sizeof(APTR));
		oldseg = testkeymap_seg;
		testkeymap_seg = seg;
	    }
	    
	    CurrentDir(olddir);
	    UnLock(lock);
	}
    }
    
    if (kmn)
    {
    	SetGadgetAttrs(testgad, win, NULL, STRINGA_AltKeyMap, (IPTR)&kmn->kn_KeyMap,
	    	    	    	    	   TAG_DONE);
    }
    
    if (oldseg) UnLoadSeg(oldseg);
    
}

/*********************************************************************************************/

static LONG kbd_init(void)
{
    init_done = TRUE;
    return TRUE;
}

/*********************************************************************************************/

static void DrawFrames(void)
{
    DrawFrameWithTitle(win->RPort,
    	    	       lvgroupx1,
		       lvgroupy1,
		       lvgroupx1 + lvgroupwidth - 1,
		       lvgroupy1 + lvgroupheight - 1,
		       MSG(MSG_GAD_KEY_TYPE));

    DrawFrameWithTitle(win->RPort,
    	    	       rategroupx1,
		       rategroupy1,
		       rategroupx1 + rategroupwidth - 1,
		       rategroupy1 + rategroupheight - 1,
		       MSG(MSG_GAD_KEY_REPEAT_RATE));
		       
    DrawFrameWithTitle(win->RPort,
    	    	       delaygroupx1,
		       delaygroupy1,
		       delaygroupx1 + delaygroupwidth - 1,
		       delaygroupy1 + delaygroupheight - 1,
		       MSG(MSG_GAD_KEY_REPEAT_DELAY));

    DrawFrameWithTitle(win->RPort,
    	    	       testgroupx1,
		       testgroupy1,
		       testgroupx1 + testgroupwidth - 1,
		       testgroupy1 + testgroupheight - 1,
		       MSG(MSG_GAD_KEY_TEST));
}

/*********************************************************************************************/

AROS_UFH3S(IPTR, LVRenderFunc,
    AROS_UFHA(struct Hook *,            hook,     	A0),
    AROS_UFHA(struct ListviewEntry *,  	node,           A2),
    AROS_UFHA(struct LVDrawMsg *,	msg,	        A1)
)
{
    AROS_USERFUNC_INIT

    IPTR retval;
    
    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;
    	WORD textoffx;
	
        UWORD erasepen = BACKGROUNDPEN;

    	textoffx = node->modelnode ? 0 : 10;
	
     	SetDrMd(rp, JAM1);
     	       	
     	switch (msg->lvdm_State)
     	{
     	    case LVR_SELECTED:
     	    case LVR_SELECTEDDISABLED:
	    	if (!node->modelnode)
		{
     	    	    /* We must fill the backgound with FILLPEN */
		    erasepen = FILLPEN;
    	    	}
		/* Fall through */
		
     	    case LVR_NORMAL:
     	    case LVR_NORMALDISABLED:
	     {

    	    	WORD numfit;
    	    	struct TextExtent te;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
    	    	numfit = TextFit(rp, node->node.ln_Name, strlen(node->node.ln_Name),
    	    		&te, NULL, 1, max_x - min_x - textoffx + 1, max_y - min_y + 1);

	    	SetAPen(rp, dri->dri_Pens[TEXTPEN]);
	    	
    	    	/* Render text */
    	    	Move(rp, min_x + textoffx, min_y + 2 + rp->Font->tf_Baseline);
    	    	Text(rp, node->node.ln_Name, numfit);
    	    	
		if (node->modelnode)
		{
    	    	    Move(rp, min_x + textoffx - 1, min_y + 2 + rp->Font->tf_Baseline);
    	    	    Text(rp, node->node.ln_Name, numfit);
		}
     	    	
     	    } break;
     	    	
     	}
     	
     	retval = LVCB_OK;
     }
     else
     {
     	retval = LVCB_UNKNOWN;
     }
     	
     return retval;
     
     AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

static LONG kbd_layout(void)
{
    struct RastPort temprp;
    struct ListviewEntry *entry;
    WORD i, w, h, maxw;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);

    /* Keymap listview gadget */
    
    lvgroupwidth  = TextLength(&temprp, MSG(MSG_GAD_KEY_TYPE), strlen(MSG(MSG_GAD_KEY_TYPE)));
    lvgroupwidth += FRAMETITLE_EXTRAWIDTH;
    
    maxw = 0;
    
    ForeachNode(&keymap_list, entry)
    {
    	w = TextLength(&temprp, entry->node.ln_Name, strlen(entry->node.ln_Name));
	if (entry->modelnode) w += 10;
	if (w > maxw) maxw = w;
    }
    
    maxw += 30 + FRAMESPACEX * 2;
    if (maxw < 150) maxw = 150;
        
    if (maxw > lvgroupwidth) lvgroupwidth = maxw;
    
    lvgroupheight = SPACE_Y * 2 + dri->dri_Font->tf_YSize * 12 + FRAME_FRAMEHEIGHT;
       
    /* Calculate width of time display labels */
    
    for(i = REPEAT_RATE_TICS_MIN; i <= REPEAT_RATE_TICS_MAX; i++)
    {
    	LONG secs, micro;
	
	secs = i / 50;
	micro = (i % 50) * 2;
	
    	snprintf(showratebuf, sizeof(showratebuf), MSG(MSG_TIME_FORMAT), secs, micro);
    	w = TextLength(&temprp, showratebuf, strlen(showratebuf));
	if (w > showtimewidth) showtimewidth = w;
    }

    for(i = REPEAT_DELAY_TICS_MIN; i <= REPEAT_DELAY_TICS_MAX; i++)
    {
    	LONG secs, micro;
	
	secs = i / 50;
	micro = (i % 50) * 2;
	
    	snprintf(showdelaybuf, sizeof(showdelaybuf), MSG(MSG_TIME_FORMAT), secs, micro);
    	w = TextLength(&temprp, showdelaybuf, strlen(showdelaybuf));
	if (w > showtimewidth) showtimewidth = w;
    }
    
    /* Repeat Rate gadget */
    
    rategroupwidth  = TextLength(&temprp, MSG(MSG_GAD_KEY_REPEAT_RATE), strlen(MSG(MSG_GAD_KEY_REPEAT_RATE)));
    rategroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (rategroupwidth < 200 + LABELSPACE_X + showtimewidth)
    {
    	rategroupwidth = 200 + LABELSPACE_X + showtimewidth;
    }

    rategroupheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + FRAME_FRAMEHEIGHT;
    
    /* Repeat Delay gadget */
    
    delaygroupwidth  = TextLength(&temprp, MSG(MSG_GAD_KEY_REPEAT_DELAY), strlen(MSG(MSG_GAD_KEY_REPEAT_DELAY)));
    delaygroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (delaygroupwidth < 200 + LABELSPACE_X + showtimewidth)
    {
    	delaygroupwidth = 200 + LABELSPACE_X + showtimewidth;
    }

    delaygroupheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + FRAME_FRAMEHEIGHT;
    
    if (delaygroupwidth > rategroupwidth)
    {
    	rategroupwidth = delaygroupwidth;
    }
    else
    {
    	delaygroupwidth = rategroupwidth;
    }
    
    /* Test gadget */
    
    testgroupwidth  = delaygroupwidth;
    testheight = dri->dri_Font->tf_YSize + STRING_EXTRAHEIGHT;
    testgroupheight = testheight + FRAME_FRAMEHEIGHT;
    
    minwidth = lvgroupwidth + GROUPSPACE_X + delaygroupwidth;
   
    h = delaygroupheight + GROUPSPACE_Y + rategroupheight + GROUPSPACE_Y + testgroupheight;
    
    minheight = (lvgroupheight > h) ? lvgroupheight : h;

    DeinitRastPort(&temprp);
    
    return TRUE;
}

/*********************************************************************************************/

static void update_inputdev(void)
{
    if (InputIO)
    {
    	if (InputIO->tr_node.io_Device)
	{
    	    InputIO->tr_node.io_Command = IND_SETPERIOD;
	    InputIO->tr_time = inputprefs.ip_KeyRptSpeed;
	    DoIO(&InputIO->tr_node);

    	    InputIO->tr_node.io_Command = IND_SETTHRESH;
	    InputIO->tr_time = inputprefs.ip_KeyRptDelay;
	    DoIO(&InputIO->tr_node);	
	    
	    inputdev_changed = TRUE;
 	}
    }
}

/*********************************************************************************************/

static void update_keymap_gad(void)
{
    struct ListviewEntry    *keymapnode;
    struct TagItem  	    lvsettags[] =
    {
    	{GTLV_Selected	,   0	},
	{TAG_DONE   	    	}
    };
    WORD    	    	    active = -1, index = 0;

    ForeachNode(&keymap_list, keymapnode)
    {
    	if (keymapnode->modelnode == FALSE)
	{
	    if (strcmp(keymapnode->realname, inputprefs.ip_Keymap) == 0)
	    {
	    	active = index;
		break;
	    }
	}
	index++;
    }
    
    lvsettags[0].ti_Data = (IPTR)active;
    
    GT_SetGadgetAttrsA(lvgad, win, NULL, lvsettags);
}

/*********************************************************************************************/

static void update_showrate_gad(void)
{
    struct TagItem settags[] =
    {
    	{GTTX_Text, (IPTR)showratebuf},
	{TAG_DONE   	    	     }
    };
    LONG secs, micro;
    
    secs  = inputprefs.ip_KeyRptSpeed.tv_secs;
    micro = inputprefs.ip_KeyRptSpeed.tv_micro / 10000;
    
    snprintf(showratebuf, sizeof(showratebuf), MSG(MSG_TIME_FORMAT), secs, micro);
    
    GT_SetGadgetAttrsA(showrategad, win, NULL, settags);
    
}

/*********************************************************************************************/

static void update_rate_gad(void)
{
    struct TagItem scsettags[] =
    {
    	{GTSC_Top, 0},
	{TAG_DONE   }
    };
    LONG rate;
    
    rate = inputprefs.ip_KeyRptSpeed.tv_secs * 50;
    rate += inputprefs.ip_KeyRptSpeed.tv_micro / (1000000 / 50);
    
    if (rate < REPEAT_RATE_TICS_MIN)
    {
    	rate = REPEAT_RATE_TICS_MIN;
    }
    else if (rate > REPEAT_RATE_TICS_MAX)
    {
    	rate = REPEAT_RATE_TICS_MAX;
    }
    
    rate -= REPEAT_RATE_TICS_MIN;
    
#if REVERSE_RATE_SCROLLER
    rate = REPEAT_RATE_TICS_RANGE - 1 - rate;
#endif
    scsettags[0].ti_Data = rate;
    
    GT_SetGadgetAttrsA(rategad, win, NULL, scsettags);
    
    update_showrate_gad();
}

/*********************************************************************************************/

static void update_showdelay_gad(void)
{
    struct TagItem settags[] =
    {
    	{GTTX_Text, (IPTR)showdelaybuf},
	{TAG_DONE   	    	      }
    };
    LONG secs, micro;
    
    secs  = inputprefs.ip_KeyRptDelay.tv_secs;
    micro = inputprefs.ip_KeyRptDelay.tv_micro / 10000;
    
    snprintf(showdelaybuf, sizeof(showdelaybuf), MSG(MSG_TIME_FORMAT), secs, micro);
    
    GT_SetGadgetAttrsA(showdelaygad, win, NULL, settags);
}

/*********************************************************************************************/

static void update_delay_gad(void)
{
    struct TagItem scsettags[] =
    {
    	{GTSC_Top, 0},
	{TAG_DONE   }
    };
    LONG delay;
    
    delay = inputprefs.ip_KeyRptDelay.tv_secs * 50;
    delay += inputprefs.ip_KeyRptDelay.tv_micro / (1000000 / 50);
    
    if (delay < REPEAT_DELAY_TICS_MIN)
    {
    	delay = REPEAT_DELAY_TICS_MIN;
    }
    else if (delay > REPEAT_DELAY_TICS_MAX)
    {
    	delay = REPEAT_DELAY_TICS_MAX;
    }
    
    scsettags[0].ti_Data = delay - REPEAT_DELAY_TICS_MIN;
    
    GT_SetGadgetAttrsA(delaygad, win, NULL, scsettags);
    
    update_showdelay_gad();
}

/*********************************************************************************************/

static LONG kbd_input(struct IntuiMessage *msg)
{
    struct ListviewEntry    *keymapnode;
    LONG    	    	    retval = FALSE;
    
    if (msg->Class == IDCMP_GADGETUP)
    {
    	struct Gadget *gad = (struct Gadget *)msg->IAddress;
	
	switch(gad->GadgetID)
	{
	    case MSG_GAD_KEY_TYPE:
	    	if ((keymapnode = (struct ListviewEntry *)FindListNode(&keymap_list, msg->Code)))
		{
		    if ((keymapnode->modelnode))
		    {
		    	update_keymap_gad();
		    }
		    else
		    {
		    	strncpy(inputprefs.ip_Keymap, keymapnode->realname, sizeof(inputprefs.ip_Keymap));
			try_setting_test_keymap();
		    }
		}

	    	SetGadgetAttrs(testgad, win, NULL, GTST_String, (IPTR)"", TAG_DONE);
		ActivateGadget(testgad, win, NULL);

		retval = TRUE;
	    	break;
		
		
	} /* switch(gad->GadgetID) */
	
    } /* if (msg->Class == IDCMP_GADGETUP) */
    
    if (!retval)
    {
    	struct Gadget *gad = (struct Gadget *)msg->IAddress;
	LONG	       top;
	
    	switch(msg->Class)
	{
	    case IDCMP_GADGETUP:
	    case IDCMP_GADGETDOWN:
	    case IDCMP_MOUSEMOVE:
	    	if (gad == rategad)
		{
		    top = msg->Code;
		 #if REVERSE_RATE_SCROLLER
    	    	    top = REPEAT_RATE_TICS_RANGE - 1 - top;
		 #endif
		    top += REPEAT_RATE_TICS_MIN;
		    
		    inputprefs.ip_KeyRptSpeed.tv_secs = top / 50;
		    inputprefs.ip_KeyRptSpeed.tv_micro = (top % 50) * (1000000 / 50);
		    update_showrate_gad();
		    update_inputdev();
		    
		    if (msg->Class == IDCMP_GADGETUP)
		    {
		    	SetGadgetAttrs(testgad, win, NULL, GTST_String, (IPTR)"", TAG_DONE);
		    	ActivateGadget(testgad, win, NULL);
    	    	    }

		    retval = TRUE;
		}
		else if (gad == delaygad)
		{
		    top = msg->Code + REPEAT_DELAY_TICS_MIN;
		    
		    inputprefs.ip_KeyRptDelay.tv_secs = top / 50;
		    inputprefs.ip_KeyRptDelay.tv_micro = (top % 50) * (1000000 / 50);
		    update_showdelay_gad();
		    update_inputdev();
		    
		    if (msg->Class == IDCMP_GADGETUP)
		    {
		    	SetGadgetAttrs(testgad, win, NULL, GTST_String, (IPTR)"", TAG_DONE);
		    	ActivateGadget(testgad, win, NULL);
    	    	    }
		    
		    retval = TRUE;
		}
	    	break;
		
	} /* switch(msg->Class) */
	
    } /* if (!retval) */
    
    return retval;
}

/*********************************************************************************************/

static void kbd_cleanup(void)
{
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
    
    if (inputdev_changed)
    {
    	InputIO->tr_node.io_Command = IND_SETPERIOD;
	InputIO->tr_time = restore_prefs.ip_KeyRptSpeed;
	DoIO(&InputIO->tr_node);
	
    	InputIO->tr_node.io_Command = IND_SETTHRESH;
	InputIO->tr_time = restore_prefs.ip_KeyRptDelay;
	DoIO(&InputIO->tr_node);	
    }
    
    if (testkeymap_seg)
    {
    	UnLoadSeg(testkeymap_seg);
    }
}

/*********************************************************************************************/

static LONG kbd_makegadgets(void)
{
    static struct Hook lvrenderhook;
    struct NewGadget ng;
  
    lvrenderhook.h_Entry = (APTR) AROS_ASMSYMNAME(LVRenderFunc);
  
    gad = CreateContext(&gadlist);
    
    lvwidth  = lvgroupwidth - FRAME_FRAMEWIDTH;
    lvheight = lvgroupheight - FRAME_FRAMEHEIGHT;
    
    ng.ng_LeftEdge   = lvgroupx1 + FRAME_OFFX;
    ng.ng_TopEdge    = lvgroupy1 + FRAME_OFFY;
    ng.ng_Width      = lvwidth;
    ng.ng_Height     = lvheight;
    ng.ng_GadgetText = NULL;
    ng.ng_TextAttr   = 0;
    ng.ng_GadgetID   = MSG_GAD_KEY_TYPE;
    ng.ng_Flags      = 0;
    ng.ng_VisualInfo = vi;
    
    gad = lvgad = CreateGadget(LISTVIEW_KIND, gad, &ng, GTLV_Labels, (IPTR)&keymap_list,
    	    	    	    	    	    	    	GTLV_ItemHeight, dri->dri_Font->tf_YSize + 4,
							GTLV_CallBack, (IPTR)&lvrenderhook,
							GTLV_ShowSelected, 0,
    	    	    	    	    	    	    	TAG_DONE);
	
    ratewidth  = rategroupwidth  - FRAME_FRAMEWIDTH - LABELSPACE_X - showtimewidth;
    rateheight = rategroupheight - FRAME_FRAMEHEIGHT;
     		
    ng.ng_LeftEdge = rategroupx1 + FRAME_OFFX + showtimewidth + LABELSPACE_X;
    ng.ng_TopEdge  = rategroupy1 + FRAME_OFFY;
    ng.ng_Width    = ratewidth;
    ng.ng_Height   = rateheight;
    ng.ng_GadgetID = MSG_GAD_KEY_REPEAT_RATE;
    
    gad = rategad = CreateGadget(SCROLLER_KIND, gad, &ng, GTSC_Total, REPEAT_RATE_TICS_RANGE + 1,
							  TAG_DONE);
    					      
    delaywidth  = delaygroupwidth  - FRAME_FRAMEWIDTH - LABELSPACE_X - showtimewidth;
    delayheight = delaygroupheight - FRAME_FRAMEHEIGHT;
     		
    ng.ng_LeftEdge = delaygroupx1 + FRAME_OFFX + showtimewidth + LABELSPACE_X;
    ng.ng_TopEdge  = delaygroupy1 + FRAME_OFFY;
    ng.ng_Width    = delaywidth;
    ng.ng_Height   = delayheight;
    ng.ng_GadgetID = MSG_GAD_KEY_REPEAT_DELAY;
    
    gad = delaygad = CreateGadget(SCROLLER_KIND, gad, &ng, GTSC_Total, REPEAT_DELAY_TICS_RANGE + 9,
    	    	    	    	    	    	    	   GTSC_Visible, 10, 
							   TAG_DONE);

    ng.ng_LeftEdge   = rategroupx1 + FRAME_OFFX;
    ng.ng_TopEdge    = rategroupy1 + FRAME_OFFY;
    ng.ng_Width      = showtimewidth;
    ng.ng_Height     = rateheight;
    ng.ng_GadgetID   = 0;
    
    gad = showrategad = CreateGadget(TEXT_KIND, gad, &ng, GTTX_Text, (IPTR)showratebuf,
    	    	    	    	    	    	    	  GTTX_Justification, GTJ_RIGHT,
							  TAG_DONE);
	
    ng.ng_LeftEdge   = delaygroupx1 + FRAME_OFFX;
    ng.ng_TopEdge    = delaygroupy1 + FRAME_OFFY;
    ng.ng_Width      = showtimewidth;
    ng.ng_Height     = rateheight;
    ng.ng_GadgetID   = 0;
    						  
    gad = showdelaygad = CreateGadget(TEXT_KIND, gad, &ng, GTTX_Text, (IPTR)showdelaybuf,
    	    	    	    	    	    	    	   GTTX_Justification, GTJ_RIGHT,
							   TAG_DONE);

    ng.ng_LeftEdge  = testgroupx1 + FRAME_OFFX;
    ng.ng_TopEdge   = testgroupy1 + FRAME_OFFY + (testgroupheight - FRAME_FRAMEHEIGHT - testheight) / 2;
    ng.ng_Width     = testgroupwidth - FRAME_FRAMEWIDTH;
    ng.ng_Height    = testheight;
    ng.ng_GadgetID  = 0;
    
    gad = testgad = CreateGadget(STRING_KIND, gad, &ng, GTST_MaxChars, 255,
    	    	    	    	    	    	    	TAG_DONE);
							  
    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void kbd_prefs_changed(void)
{
    if (page_active)
    {
    	update_keymap_gad();
    	update_rate_gad();
    	update_delay_gad();

	try_setting_test_keymap();
    }
    
    update_inputdev();
}

/*********************************************************************************************/

LONG page_kbd_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    retval = kbd_init();
	    break;
	    
	case PAGECMD_LAYOUT:
	    retval = kbd_layout();
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	    retval = minwidth;
	    break;
	    
	case PAGECMD_GETMINHEIGHT:
	    retval = minheight;
	    break;
	    
	case PAGECMD_SETDOMLEFT:
	    domleft = param;
	    lvgroupx1 = domleft;
	    delaygroupx1 = rategroupx1 = testgroupx1 = domleft + lvgroupwidth + GROUPSPACE_X;
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
	    lvgroupy1 = rategroupy1 = param;
	    delaygroupy1 = rategroupy1 + rategroupheight + GROUPSPACE_Y;
	    testgroupy1 = delaygroupy1 + delaygroupheight + GROUPSPACE_Y;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    domwidth = param;
	    rategroupwidth  = domleft + domwidth - rategroupx1;
	    delaygroupwidth = domleft + domwidth - delaygroupx1;
	    testgroupwidth  = domleft + domwidth - testgroupx1;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    domheight = param;
	    lvheight += (param - lvgroupheight);
	    lvgroupheight = param;
	    testgroupheight = domtop + domheight - testgroupy1;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	    retval = kbd_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
		page_active = TRUE;

	    	kbd_prefs_changed();
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
	    DrawFrames();
	    break;
	
	case PAGECMD_HANDLEINPUT:
	    retval = kbd_input((struct IntuiMessage *)param);
	    break;
	
	case PAGECMD_PREFS_CHANGED:
	    kbd_prefs_changed();
	    break;
	    
	case PAGECMD_CLEANUP:
	    kbd_cleanup();
	    break;
    }
    
    return retval;
    
}
