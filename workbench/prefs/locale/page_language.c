/*
    (C) 2001 AROS - The Amiga Research OS
    $Id$

    Desc:
    Lang: English
*/

/*********************************************************************************************/

#include "global.h"

LONG page_language_handler(LONG cmd, IPTR param)
{
    LONG retval = TRUE;
    
    switch(cmd)
    {
    	case PAGECMD_INIT:
	    break;
	    
	case PAGECMD_LAYOUT:
	    break;
	    
	case PAGECMD_GETMINWIDTH:
	case PAGECMD_GETMINHEIGHT:
	case PAGECMD_SETDOMLEFT:
	case PAGECMD_SETDOMTOP:
	case PAGECMD_SETDOMWIDTH:
	case PAGECMD_SETDOMHEIGHT:
	case PAGECMD_MAKEGADGETS:
	case PAGECMD_ADDGADGETS:
	case PAGECMD_REMGADGETS:
	    break;
	    
	case PAGECMD_HANDLEINPUT:
	    retval = FALSE;
	    break;
	    
	case PAGECMD_CLEANUP:
	    break;
    }
    
    return retval;
}
