/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <aros/asmcall.h>
#include <stdio.h>

/*********************************************************************************************/

#define FRAME_FRAMEWIDTH  (FRAMESPACEX * 2)
#define FRAME_FRAMEHEIGHT (FRAMESPACEY + dri->dri_Font->tf_YSize + SPACE_Y)

#define FRAME_OFFX (FRAMESPACEX)
#define FRAME_OFFY (dri->dri_Font->tf_YSize + SPACE_Y)

/*********************************************************************************************/

static struct Gadget 	*gadlist, *gad, *lvgad, *rategad, *delaygad;
static WORD 	    	minwidth, minheight;
static WORD 	    	domleft, domtop, domwidth, domheight;
static WORD 	    	lvwidth, lvheight, lvgroupwidth, lvgroupheight, lvgroupx1, lvgroupy1;
static WORD 	    	ratewidth, rateheight, rategroupwidth, rategroupheight, rategroupx1, rategroupy1;
static WORD 	    	delaywidth, delayheight, delaygroupwidth, delaygroupheight, delaygroupx1, delaygroupy1;

static BOOL 	    	init_done;

static BOOL 	    	page_active;

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
     	
     AROS_USERFUNC_EXIT
}

/*********************************************************************************************/

static LONG kbd_layout(void)
{
    struct RastPort temprp;
    struct ListviewEntry *entry;
    WORD w, h, maxw;
    
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
    
    lvgroupheight = SPACE_Y * 2 + dri->dri_Font->tf_YSize * 8 + FRAME_FRAMEHEIGHT;
    
    DeinitRastPort(&temprp);
   
    /* Repeat Rate gadget */
    
    rategroupwidth  = TextLength(&temprp, MSG(MSG_GAD_KEY_REPEAT_RATE), strlen(MSG(MSG_GAD_KEY_REPEAT_RATE)));
    rategroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (rategroupwidth < 200) rategroupwidth = 200;

    rategroupheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + FRAME_FRAMEHEIGHT;
    
    /* Repeat Delay gadget */
    
    delaygroupwidth  = TextLength(&temprp, MSG(MSG_GAD_KEY_REPEAT_DELAY), strlen(MSG(MSG_GAD_KEY_REPEAT_DELAY)));
    delaygroupwidth += FRAMETITLE_EXTRAWIDTH;
    if (delaygroupwidth < 200) delaygroupwidth = 200;

    delaygroupheight = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT + FRAME_FRAMEHEIGHT;
    
    if (delaygroupwidth > rategroupwidth)
    {
    	rategroupwidth = delaygroupwidth;
    }
    else
    {
    	delaygroupwidth = rategroupwidth;
    }
    
    minwidth = lvgroupwidth + SPACE_X + delaygroupwidth;
   
    h = delaygroupheight + SPACE_Y + rategroupheight;
    
    minheight = (lvgroupheight > h) ? lvgroupheight : h;
    
    return TRUE;
}

/*********************************************************************************************/

static LONG kbd_input(struct IntuiMessage *msg)
{
    LONG retval = FALSE;

    return retval;
}

/*********************************************************************************************/

static void kbd_cleanup(void)
{
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
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
    	    	    	    	    	    	    	TAG_DONE);
	
    ratewidth  = rategroupwidth  - FRAME_FRAMEWIDTH;
    rateheight = rategroupheight - FRAME_FRAMEHEIGHT;
     		
    ng.ng_LeftEdge = rategroupx1 + FRAME_OFFX;
    ng.ng_TopEdge  = rategroupy1 + FRAME_OFFY;
    ng.ng_Width    = ratewidth;
    ng.ng_Height   = rateheight;
    ng.ng_GadgetID = MSG_GAD_KEY_REPEAT_RATE;
    
    gad = rategad = CreateGadget(SCROLLER_KIND, gad, &ng, GTSC_Total, 100,
							  TAG_DONE);
    					      
    delaywidth  = delaygroupwidth  - FRAME_FRAMEWIDTH;
    delayheight = delaygroupheight - FRAME_FRAMEHEIGHT;
     		
    ng.ng_LeftEdge = delaygroupx1 + FRAME_OFFX;
    ng.ng_TopEdge  = delaygroupy1 + FRAME_OFFY;
    ng.ng_Width    = delaywidth;
    ng.ng_Height   = delayheight;
    ng.ng_GadgetID = MSG_GAD_KEY_REPEAT_DELAY;
    
    gad = delaygad = CreateGadget(SCROLLER_KIND, gad, &ng, GTSC_Total, 100,
							  TAG_DONE);

    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void kbd_prefs_changed(void)
{
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
	    delaygroupx1 = rategroupx1 = domleft + lvgroupwidth + SPACE_X;
	    break;
	    
	case PAGECMD_SETDOMTOP:
	    domtop = param;
	    lvgroupy1 = rategroupy1 = param;
	    delaygroupy1 = rategroupy1 + rategroupheight + SPACE_Y;
	    break;
	    
	case PAGECMD_SETDOMWIDTH:
	    domwidth = param;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    lvheight += (param - lvgroupheight);
	    lvgroupheight = param;
	    
	    domheight = param;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	    retval = kbd_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
		AddGList(win, gadlist, -1, -1, NULL);
		GT_RefreshWindow(win, NULL);
		RefreshGList(gadlist, win, NULL, -1);
		DrawFrames();

		page_active = TRUE;
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
