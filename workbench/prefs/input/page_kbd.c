/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"
#include <stdio.h>

/*********************************************************************************************/

static struct Gadget *gadlist, *gad;
static WORD minwidth, minheight;
static WORD domleft, domtop, domwidth, domheight;
static BOOL init_done;

static BOOL page_active;

/*********************************************************************************************/

static LONG kbd_init(void)
{
    init_done = TRUE;
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
    return TRUE;
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
	    minwidth  = 50;
    	    minheight = 50;
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
	    retval = kbd_makegadgets();
	    break;
	    
	case PAGECMD_ADDGADGETS:
	    if (!page_active)
	    {
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
