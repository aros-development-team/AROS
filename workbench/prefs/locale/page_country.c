/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <string.h>

/*********************************************************************************************/

static struct Gadget *gadlist, *countrygad, *gad;
static struct Hook lvhook;
static WORD domleft, domtop, domwidth, domheight;
static WORD max_flagw, max_flagh;
static WORD active_country = 0;
static BOOL page_active;

/*********************************************************************************************/

static LONG country_init(void)
{
    struct CountryEntry *entry;
    static UBYTE   	 filename[256];
    
    if (!DataTypesBase) return TRUE;
    
    ForeachNode(&country_list, entry)
    {
    	strcpy(filename, "LOCALE:Flags/Countries");
	AddPart(filename, entry->lve.realname, 256);
	
	entry->dto = NewDTObject(filename, DTA_GroupID	    	, GID_PICTURE	    ,
	    	    	    	    	   OBP_Precision    	, PRECISION_IMAGE   ,
					   PDTA_Screen	    	, (IPTR)scr 	    ,
					   PDTA_Remap	    	, TRUE	    	    ,
					   PDTA_FreeSourceBitMap, TRUE	    	    ,
					   PDTA_DestMode    	, PMODE_V43 	    ,
					   PDTA_UseFriendBitMap , TRUE	    	    ,
					   TAG_DONE);
					   
	if (entry->dto)
	{
	    IPTR val;
	    
	    DoMethod(entry->dto, DTM_PROCLAYOUT, (IPTR) NULL, 1);
	    
	    GetDTAttrs(entry->dto, PDTA_DestBitMap, (IPTR) &entry->flagbm, TAG_DONE);
	    if (!entry->flagbm)
	    {
	    	GetDTAttrs(entry->dto, PDTA_BitMap, (IPTR) &entry->flagbm, TAG_DONE);
	    }
	    
	    GetDTAttrs(entry->dto, DTA_NominalHoriz, (IPTR) &val, TAG_DONE);
	    entry->flagw = (WORD)val;
	    
	    GetDTAttrs(entry->dto, DTA_NominalVert, (IPTR) &val, TAG_DONE);
	    entry->flagh = (WORD)val;
	    
	    if (entry->flagbm)
	    {
	    	if (entry->flagw > max_flagw) max_flagw = entry->flagw;
		if (entry->flagh > max_flagh) max_flagh = entry->flagh;
		
		if (GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH) >= 15)
		{
		    struct BitMap *bm;
		    
		    bm = AllocBitMap(entry->flagw,
		    	    	     entry->flagh,
				     GetBitMapAttr(scr->RastPort.BitMap, BMA_DEPTH),
				     BMF_MINPLANES | BMF_INTERLEAVED,
				     scr->RastPort.BitMap);
				     
		    if (bm)
		    {
		    	BltBitMap(entry->flagbm, 0, 0, bm, 0, 0, entry->flagw, entry->flagh, 192, 255, 0);
			DisposeDTObject(entry->dto);
			
			entry->dto = NULL;
			entry->flagbm = bm;
		    }
		}
	    }
	    else
	    {
	    	DisposeDTObject(entry->dto);
		entry->dto = NULL;
	    }
	    
	} /* if (entry->dto) */
	
    } /* ForeachNode(&country_list, entry) */

    return TRUE;
}

/*********************************************************************************************/

static IPTR LVRenderHook(struct Hook *hook, struct Node *node, struct LVDrawMsg *msg)
{
    IPTR retval;
    
    if (msg->lvdm_MethodID == LV_DRAW)
    {
    	struct DrawInfo *dri = msg->lvdm_DrawInfo;
    	struct RastPort *rp  = msg->lvdm_RastPort;
    	
    	WORD min_x = msg->lvdm_Bounds.MinX;
    	WORD min_y = msg->lvdm_Bounds.MinY;
    	WORD max_x = msg->lvdm_Bounds.MaxX;
    	WORD max_y = msg->lvdm_Bounds.MaxY;

        UWORD erasepen = BACKGROUNDPEN;

     	SetDrMd(rp, JAM1);
     	      	
     	switch (msg->lvdm_State)
     	{
     	    case LVR_SELECTED:
     	    case LVR_SELECTEDDISABLED:
 		erasepen = FILLPEN;
		/* Fall through */
		
     	    case LVR_NORMAL:
     	    case LVR_NORMALDISABLED:
	    {

    	    	struct TextExtent   te;
		struct CountryEntry *ce;
    	    	WORD 	    	    numfit;
    	    
		SetAPen(rp, dri->dri_Pens[erasepen]);
     	    	RectFill(rp, min_x, min_y, max_x, max_y);
     	    	
		ce = (struct CountryEntry *)node;
		if (ce->flagbm)
		{
		    BltBitMapRastPort(ce->flagbm,
		    	    	      0,
				      0,
		    	    	      rp,
				      min_x + 1,
				      (min_y + max_y + 1 - ce->flagh) / 2,
				      ce->flagw,
				      ce->flagh,
				      192);
		}
		
		if (max_flagh) min_x += max_flagw + 3;
		
    	    	numfit = TextFit(rp, node->ln_Name, strlen(node->ln_Name),
    	    		&te, NULL, 1, max_x - min_x + 1, max_y - min_y + 1);

	    	SetAPen(rp, dri->dri_Pens[TEXTPEN]);
	    	
    	    	/* Render text */
    	    	Move(rp, min_x, (min_y + max_y + 1 - rp->Font->tf_YSize) / 2 + rp->Font->tf_Baseline);
    	    	Text(rp, node->ln_Name, numfit);
  	    	     	    }
	    break;
     	    	     	
     	}
     	
     	retval = LVCB_OK;
     }
     else
     {
     	retval = LVCB_UNKNOWN;
     }
     	
     return retval;
}

/*********************************************************************************************/

static LONG country_makegadgets(void)
{
    struct NewGadget ng;
    WORD    	     itemheight;
    
    gad = CreateContext(&gadlist);
    
    ng.ng_LeftEdge   = domleft;
    ng.ng_TopEdge    = domtop;
    ng.ng_Width      = domwidth;
    ng.ng_Height     = domheight;
    ng.ng_GadgetText = NULL;
    ng.ng_TextAttr   = 0;
    ng.ng_GadgetID   = MSG_GAD_TAB_COUNTRY;
    ng.ng_Flags      = 0;
    ng.ng_VisualInfo = vi;

    itemheight = dri->dri_Font->tf_YSize;
    if (max_flagh > itemheight) itemheight = max_flagh;
    itemheight += 2;
   
    lvhook.h_Entry = HookEntry;
    lvhook.h_SubEntry = LVRenderHook;
    
    gad = countrygad = CreateGadget(LISTVIEW_KIND, gad, &ng, GTLV_Labels    	, (IPTR)&country_list,
    	    	    	    	    	    	    	     GTLV_ShowSelected	, (IPTR) NULL	    	     ,
							     GTLV_Selected    	, active_country     ,
							     GTLV_MakeVisible	, active_country     ,
							     GTLV_ItemHeight    , itemheight  	     ,
							     GTLV_CallBack  	, (IPTR)&lvhook      ,
   	    	    	    	    	    	    	     TAG_DONE);
    

    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void country_cleanup(void)
{
    struct CountryEntry *entry;
    
    ForeachNode(&country_list, entry)
    {
    	if (entry->dto)
	{
	    DisposeDTObject(entry->dto);
	}
	else if (entry->flagbm)
	{
	    FreeBitMap(entry->flagbm);
	}
    }
    
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
}

/*********************************************************************************************/

static LONG country_input(struct IntuiMessage *msg)
{
    struct CountryEntry *entry;    
    LONG    	retval = FALSE;
    
    if (msg->Class == IDCMP_GADGETUP)
    {
    	struct Gadget *gad = (struct Gadget *)msg->IAddress;
	
	switch(gad->GadgetID)
	{
	    case MSG_GAD_TAB_COUNTRY:
	    	if ((entry = (struct CountryEntry *)FindListNode(&country_list, msg->Code)))
		{
		    if (active_country != msg->Code)
		    {
			active_country = msg->Code;
			strcpy(localeprefs.lp_CountryName, entry->lve.realname);
			LoadCountry(localeprefs.lp_CountryName, &localeprefs.lp_CountryData);
		    }
		}
		retval = TRUE;
		break;
		
	} /* switch(gad->GadgetID) */
	
    } /* if (msg->Class == IDCMP_GADGETUP) */
    
    return retval;
}

/*********************************************************************************************/

static void country_prefs_changed(void)
{
    struct CountryEntry *entry;
    WORD                i = 0;
    
    active_country = 0;
    
    ForeachNode(&country_list, entry)
    {
    	if (Stricmp(localeprefs.lp_CountryName, entry->lve.realname) == 0)
	{
	    active_country = i;
	    break;
	}
	i++;
    }
    
    if (gadlist)
    {
    	struct Window *winparam = page_active ? win : NULL;
	
	GT_SetGadgetAttrs(countrygad, winparam, NULL, GTLV_Selected   , active_country,
	    	    	    	    	    	      GTLV_MakeVisible, active_country,
						      TAG_DONE); 
    }
}

/*********************************************************************************************/

LONG page_country_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    retval = country_init();
	    break;
	    
	case PAGECMD_LAYOUT:
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	    retval = 20;
	    break;
	    
	case PAGECMD_GETMINHEIGHT:
	    retval = 20;
	    break;
	    
	case PAGECMD_SETDOMLEFT:
	    domleft = param;
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
	    retval = country_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
		GT_SetGadgetAttrs(countrygad, NULL, NULL, GTLV_MakeVisible, active_country, TAG_DONE);
		AddGList(win, gadlist, -1, -1, NULL);
		GT_RefreshWindow(win, NULL);
		RefreshGList(gadlist, win, NULL, -1);
		
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
	
	case PAGECMD_PREFS_CHANGED:
	    country_prefs_changed();
	    break;
	    
	case PAGECMD_HANDLEINPUT:
	    retval = country_input((struct IntuiMessage *)param);
	    break;
	    
	case PAGECMD_CLEANUP:
	    country_cleanup();
	    break;
    }
    
    return retval;
}
