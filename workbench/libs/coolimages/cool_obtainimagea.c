/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <proto/exec.h>

#include "coolimages_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(struct CoolImage *, COOL_ObtainImageA,

/*  SYNOPSIS */
	AROS_LHA(ULONG, imageid, D0),
	AROS_LHA(struct TagItem *, tags, A0),

/*  LOCATION */
	struct Library *, CoolImagesBase, 5, CoolImages)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct Library *,CoolImagesBase)

    struct CoolImage *retval = 0;
    
    switch(imageid)
    {
    	case COOL_SAVEIMAGE_ID:
	    retval = &cool_saveimage;
	    break;
	    
    	case COOL_LOADIMAGE_ID:
	    retval = &cool_loadimage;
	    break;
	    
    	case COOL_USEIMAGE_ID:
	    retval = &cool_useimage;
	    break;
	    
    	case COOL_CANCELIMAGE_ID:
	    retval = &cool_cancelimage;
	    break;
	    
    	case COOL_DOTIMAGE_ID:
	    retval = &cool_dotimage;
	    break;
	    
    	case COOL_DOTIMAGE2_ID:
	    retval = &cool_dotimage2;
	    break;
	    
    	case COOL_WARNIMAGE_ID:
	    retval = &cool_warnimage;
	    break;
	    
    	case COOL_DISKIMAGE_ID:
	    retval = &cool_diskimage;
	    break;
	    
    	case COOL_SWITCHIMAGE_ID:
	    retval = &cool_switchimage;
	    break;
	    
    	case COOL_MONITORIMAGE_ID:
	    retval = &cool_monitorimage;
	    break;
	    
    	case COOL_MOUSEIMAGE_ID:
	    retval = &cool_mouseimage;
	    break;
	    
    	case COOL_INFOIMAGE_ID:
	    retval = &cool_infoimage;
	    break;
	    
    	case COOL_ASKIMAGE_ID:
	    retval = &cool_askimage;
	    break;
	    
    	case COOL_KEYIMAGE_ID:
	    retval = &cool_keyimage;
	    break;
	    
    	case COOL_CLOCKIMAGE_ID:
	    retval = &cool_clockimage;
	    break;
	    
    	case COOL_FLAGIMAGE_ID:
	    retval = &cool_flagimage;
	    break;
	    
    	case COOL_HEADIMAGE_ID:
	    retval = &cool_headimage;
	    break;
	    
    	case COOL_WINDOWIMAGE_ID:
	    retval = &cool_windowimage;
	    break;
	    
    	case COOL_KBDIMAGE_ID:
	    retval = &cool_kbdimage;
	    break;
	    
    } /* switch(imageid) */
    
    return retval;
    
    AROS_LIBFUNC_EXIT

} /* COOL_ObtainImageA */
