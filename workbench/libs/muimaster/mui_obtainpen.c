/*
    Copyright © 2002, The AROS Development Team. 
    All rights reserved.
    
    $Id$
*/

#include <exec/types.h>
#include <proto/graphics.h>
#ifdef __AROS__
#include <proto/muimaster.h>
#endif

#include <string.h>
#include <stdlib.h>

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef __AROS__
__asm LONG MUI_ObtainPen(register __a0 struct MUI_RenderInfo *mri, register __a1 struct MUI_PenSpec *spec, register __d0 ULONG flags)
#else
	AROS_LH3(LONG, MUI_ObtainPen,

/*  SYNOPSIS */
	AROS_LHA(struct MUI_RenderInfo *, mri, A0),
	AROS_LHA(struct MUI_PenSpec *, spec, A1),
	AROS_LHA(ULONG, flags, D0),

/*  LOCATION */
	struct Library *, MUIMasterBase, 22, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    LONG retval = -1;
    
    if (!spec || !mri || !mri->mri_Colormap) return -1;

    switch(spec->ps_buf[0])
    {
    	case PST_MUI:
	    {
	    	LONG pen;
		
		StrToLong(spec->ps_buf + 1, &pen);
		
		if ((pen >= 0) && (pen < MPEN_COUNT))
		{
		    retval = mri->mri_Pens[pen];
		}
    	    }
	    break;
	    
	case PST_CMAP:
	    {
	    	LONG pen;
		
		StrToLong(spec->ps_buf + 1, &pen);
		
		if (pen < 0) pen = mri->mri_Colormap->Count + pen;
		if ((pen >= 0) && (pen < mri->mri_Colormap->Count))
		{
		    retval = pen;
		}
	    }
	    break;
	    
    	case PST_RGB:
	    {
	    	struct TagItem obp_tags[] =
		{
		    { OBP_FailIfBad, FALSE  },
		    { TAG_DONE	    	    }
		};
		STRPTR s = spec->ps_buf + 1;
	    	ULONG r, g, b;
		
		r = strtoul(s, (char **)&s, 16);
		s++;
		g = strtoul(s, (char **)&s, 16);
		s++;
		b = strtoul(s, (char **)&s, 16);
		
		retval = ObtainBestPenA(mri->mri_Colormap, r, g, b, obp_tags);
			
		if (retval != -1)
		{
		    /* flag to indicate that ReleasePen() needs to be called
		       in MUI_ReleasePen() */
		       
		    retval |= 0x10000;
		}
	    }
	    break;
	    	    
    } /* switch(spec->ps_buf[0]) */
    
    return retval;

    AROS_LIBFUNC_EXIT

} /* MUIA_ObtainPen */
