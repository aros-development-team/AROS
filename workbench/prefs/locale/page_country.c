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
static WORD domleft, domtop, domwidth, domheight;
static WORD active_country = 0;
static BOOL page_active;

/*********************************************************************************************/

static LONG country_makegadgets(void)
{
    struct NewGadget ng;
    
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

    gad = countrygad = CreateGadget(LISTVIEW_KIND, gad, &ng, GTLV_Labels    	, (IPTR)&country_list,
    	    	    	    	    	    	    	     GTLV_ShowSelected	, NULL	    	     ,
							     GTLV_Selected    	, active_country     ,
							     GTLV_MakeVisible	, active_country     ,
   	    	    	    	    	    	    	     TAG_DONE);
    

    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void country_cleanup(void)
{
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
		    strcpy(localeprefs.lp_CountryName, entry->lve.realname);
		    LoadCountry(localeprefs.lp_CountryName, &localeprefs.lp_CountryData);
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
