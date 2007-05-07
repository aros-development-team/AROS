/*
    Copyright © 2002-2007, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <proto/exec.h>

#include "coolimages_intern.h"

/*****************************************************************************

    NAME */
	AROS_LH2(const struct CoolImage *, COOL_ObtainImageA,

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

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct CoolImage *image = NULL;
    
    switch(imageid)
    {
    	case COOL_SAVEIMAGE_ID:
	    image = &cool_saveimage;
	    break;
	    
    	case COOL_LOADIMAGE_ID:
	    image = &cool_loadimage;
	    break;
	    
    	case COOL_USEIMAGE_ID:
	    image = &cool_useimage;
	    break;
	    
    	case COOL_CANCELIMAGE_ID:
	    image = &cool_cancelimage;
	    break;
	    
    	case COOL_DOTIMAGE_ID:
	    image = &cool_dotimage;
	    break;
	    
    	case COOL_DOTIMAGE2_ID:
	    image = &cool_dotimage2;
	    break;
	    
    	case COOL_WARNIMAGE_ID:
	    image = &cool_warnimage;
	    break;
	    
    	case COOL_DISKIMAGE_ID:
	    image = &cool_diskimage;
	    break;
	    
    	case COOL_SWITCHIMAGE_ID:
	    image = &cool_switchimage;
	    break;
	    
    	case COOL_MONITORIMAGE_ID:
	    image = &cool_monitorimage;
	    break;
	    
    	case COOL_MOUSEIMAGE_ID:
	    image = &cool_mouseimage;
	    break;
	    
    	case COOL_INFOIMAGE_ID:
	    image = &cool_infoimage;
	    break;
	    
    	case COOL_ASKIMAGE_ID:
	    image = &cool_askimage;
	    break;
	    
    	case COOL_KEYIMAGE_ID:
	    image = &cool_keyimage;
	    break;
	    
    	case COOL_CLOCKIMAGE_ID:
	    image = &cool_clockimage;
	    break;
	    
    	case COOL_FLAGIMAGE_ID:
	    image = &cool_flagimage;
	    break;
	    
    	case COOL_HEADIMAGE_ID:
	    image = &cool_headimage;
	    break;
	    
    	case COOL_WINDOWIMAGE_ID:
	    image = &cool_windowimage;
	    break;
	    
    	case COOL_KBDIMAGE_ID:
	    image = &cool_kbdimage;
	    break;
	    
    } /* switch(imageid) */
    
    return image;
    
    AROS_LIBFUNC_EXIT

} /* COOL_ObtainImageA */
