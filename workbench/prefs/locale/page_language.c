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

static struct Gadget *gadlist, *availgad, *prefgad, *cleargad, *gad;
static WORD domleft, domtop, domwidth, domheight;
static WORD minwidth, minheight;
static BOOL page_active;

/*********************************************************************************************/

static LONG language_layout(void)
{
    struct RastPort temprp;
    WORD    	    w, h, maxw = 0;
    
    InitRastPort(&temprp);
    SetFont(&temprp, dri->dri_Font);
    
    maxw = TextLength(&temprp, MSG(MSG_GAD_AVAIL_LANGUAGES), strlen(MSG(MSG_GAD_AVAIL_LANGUAGES)));
    w = TextLength(&temprp, MSG(MSG_GAD_PREF_LANGUAGES), strlen(MSG(MSG_GAD_PREF_LANGUAGES)));
    if (w > maxw) maxw = w;
    
    maxw = maxw * 2 + SPACE_X;
    
    w = TextLength(&temprp, MSG(MSG_GAD_CLEAR_LANGUAGES), strlen(MSG(MSG_GAD_CLEAR_LANGUAGES)));
    w += BUTTON_EXTRAWIDTH;
    if (w > maxw) maxw = w;
    
    minwidth = maxw;
    
    h =  dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    h += SPACE_Y;
    h += dri->dri_Font->tf_YSize * 8;
    
    minheight = h;
    
    DeinitRastPort(&temprp);
    
    return TRUE;
}

/*********************************************************************************************/

static LONG language_makegadgets(void)
{
    struct NewGadget ng;
    WORD labelheight;
    
    gad = CreateContext(&gadlist);
    
    labelheight = dri->dri_Font->tf_YSize + 2;
    
    ng.ng_LeftEdge   = domleft;
    ng.ng_TopEdge    = domtop + labelheight;
    ng.ng_Width      = (domwidth - SPACE_X) / 2;
    ng.ng_Height     = domheight - dri->dri_Font->tf_YSize - BUTTON_EXTRAHEIGHT - SPACE_Y -
    	    	       labelheight;
    ng.ng_GadgetText = MSG(MSG_GAD_AVAIL_LANGUAGES);
    ng.ng_TextAttr   = 0;
    ng.ng_GadgetID   = MSG_GAD_AVAIL_LANGUAGES;
    ng.ng_Flags      = PLACETEXT_ABOVE;
    ng.ng_VisualInfo = vi;
    
    gad = availgad = CreateGadget(LISTVIEW_KIND, gad, &ng, GTLV_Labels, (IPTR)&language_list,
    	    	    	    	    	    	    	   TAG_DONE);
    
    ng.ng_LeftEdge   = domleft + domwidth - ng.ng_Width;
    ng.ng_GadgetText = MSG(MSG_GAD_PREF_LANGUAGES);
    ng.ng_GadgetID   = MSG_GAD_PREF_LANGUAGES;
    
    gad = prefgad = CreateGadget(LISTVIEW_KIND, gad, &ng, GTLV_Labels, (IPTR)&pref_language_list,
    	    	    	    	    	    	    	  TAG_DONE);
    
    ng.ng_LeftEdge   = domleft;
    ng.ng_TopEdge    = ng.ng_TopEdge + ng.ng_Height + SPACE_Y;
    ng.ng_Width      = domwidth;
    ng.ng_Height     = dri->dri_Font->tf_YSize + BUTTON_EXTRAHEIGHT;
    ng.ng_GadgetText = MSG(MSG_GAD_CLEAR_LANGUAGES);
    ng.ng_GadgetID   = MSG_GAD_CLEAR_LANGUAGES;
    ng.ng_Flags      = PLACETEXT_IN;
    
    gad = cleargad = CreateGadgetA(BUTTON_KIND, gad, &ng, NULL);
    
    return gad ? TRUE : FALSE;
}

/*********************************************************************************************/

static void empty_listviews(void)
{
    if (gadlist)
    {
	struct Window *winparam = page_active ? win : NULL;

	GT_SetGadgetAttrs(availgad, winparam, NULL, GTLV_Labels, (IPTR) NULL,
		    	    	    	    	    TAG_DONE);

	GT_SetGadgetAttrs(prefgad, winparam, NULL, GTLV_Labels, (IPTR) NULL,
		    	    	    		   TAG_DONE);
    }
}

/*********************************************************************************************/

static void reset_listviews(void)
{
    if (gadlist)
    {
	struct Window *winparam = page_active ? win : NULL;

	GT_SetGadgetAttrs(availgad, winparam, NULL, GTLV_Labels, (IPTR)&language_list,
		    	    	    	    	    TAG_DONE);

	GT_SetGadgetAttrs(prefgad, winparam, NULL, GTLV_Labels, (IPTR)&pref_language_list,
		    	    	    		   TAG_DONE);
    }	    
}

/*********************************************************************************************/

static void clear_languages(void)
{
    struct Node *node, *node2;
    
    ForeachNodeSafe(&pref_language_list, node, node2)
    {
	Remove(node);
	SortInNode(&language_list, node);
    }
}

/*********************************************************************************************/

static void update_prefs(void)
{
    struct LanguageEntry *entry;
    WORD    	    	 i = 0;
    
    ForeachNode(&pref_language_list, entry)
    {
    	strcpy(localeprefs.lp_PreferredLanguages[i], entry->lve.realname);
	i++;
	if (i == 10) break;	       
    }
    
    if (i < 10) localeprefs.lp_PreferredLanguages[i][0] = '\0';
}

/*********************************************************************************************/

static LONG language_input(struct IntuiMessage *msg)
{
    struct Node *node;
    LONG    	retval = FALSE;
    
    if (msg->Class == IDCMP_GADGETUP)
    {
    	struct Gadget *gad = (struct Gadget *)msg->IAddress;
	
	switch(gad->GadgetID)
	{
	    case MSG_GAD_AVAIL_LANGUAGES:
	    	if ((node = FindListNode(&language_list, msg->Code)))
		{
		    empty_listviews();
		    
		    Remove(node);
		    AddTail(&pref_language_list, node);

    	    	    reset_listviews();
		    
		    update_prefs();
		}
		retval = TRUE;
	    	break;

	    case MSG_GAD_PREF_LANGUAGES:
	    	if ((node = FindListNode(&pref_language_list, msg->Code)))
		{
		    empty_listviews();
		    
		    Remove(node);
		    SortInNode(&language_list, node);

    	    	    reset_listviews();

		    update_prefs();
		}
		retval = TRUE;
	    	break;
		
	    case MSG_GAD_CLEAR_LANGUAGES:
	    	empty_listviews();
	    	clear_languages();
		reset_listviews();

		update_prefs();
		retval = TRUE;
	    	break;
	}
    }
    
    return retval;
}

/*********************************************************************************************/

static void language_cleanup(void)
{
    if (gadlist) FreeGadgets(gadlist);
    gadlist = NULL;
}

/*********************************************************************************************/

static void language_prefs_changed(void)
{
    struct LanguageEntry *entry, *entry2;
    WORD    	    	 i;
    
    empty_listviews();
    
    clear_languages();
    
    for(i = 0; i < 10; i++)
    {
    	if (localeprefs.lp_PreferredLanguages[i][0] == '\0') break;
	
    	ForeachNodeSafe(&language_list, entry, entry2)
	{
	    if (Stricmp(localeprefs.lp_PreferredLanguages[i], entry->lve.realname) == 0)
	    {
	    	Remove(&entry->lve.node);
		AddTail(&pref_language_list, &entry->lve.node);
	    }
	}
    }
    
    reset_listviews();

}

/*********************************************************************************************/

LONG page_language_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:	    
	    break;
	    
	case PAGECMD_LAYOUT:
	    retval = language_layout();
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
	    domwidth = param;
	    break;
	    
	case PAGECMD_SETDOMHEIGHT:
	    domheight = param;
	    break;
	    
	case PAGECMD_MAKEGADGETS:
	    retval = language_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
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
	    
	case PAGECMD_HANDLEINPUT:
	    retval = language_input((struct IntuiMessage *)param);
	    break;
	
	case PAGECMD_PREFS_CHANGING:
	    break;
	
	case PAGECMD_PREFS_CHANGED:
	    language_prefs_changed();
	    break;
	    
	case PAGECMD_CLEANUP:
	    language_cleanup();
	    break;
    }
    
    return retval;
}
