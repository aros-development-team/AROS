/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Function to replace pixel conversion routine
    Lang: english
*/

#include "graphics_intern.h"
#include <proto/graphics.h>
#include <proto/oop.h>

/*****************************************************************************

    NAME */

	AROS_LH4(APTR, SetRGBConversionFunctionA,

/*  SYNOPSIS */
    	AROS_LHA(ULONG, srcPixFmt, D0),
	AROS_LHA(ULONG, dstPixFmt, D1),
	AROS_LHA(APTR, function, A0),
	AROS_LHA(struct TagItem *, tags, A1),
	
/*  LOCATION */
	struct GfxBase *, GfxBase, 198, Graphics)

/*  FUNCTION
    	Replace RGB pixel conversion routine used for converting from
	srcPixFmt to dstPixFmt.
	
    INPUTS
    	srcPixFmt, dstPixFmt - One of the truecolor vHidd_StdPixFmt_#?
	function - function of type HIDDT_RGBConversionFunction
	tags - none defined yet
	
    RESULT
    	Previous conversion routine or (APTR)-1 on failure (bad value
	for srcPixFmt and/or dstPixFmt)
	
    NOTES
    	The conversion function must return 1 if it did the conversion,
	or 0 if for whatever reason it did not do it or want to do it.
	Then the fallback pixel conversion routine will be used.
	
    EXAMPLE
    	See AROS/test/patchrgbconv.c
	
    BUGS

    SEE ALSO
    	<hidd/graphics.h>
	
    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
	
    (void)tags;
    
    return (APTR)HIDD_BM_SetRGBConversionFunction(SDD(GfxBase)->framebuffer,
    	    	    	    	    	    	  (HIDDT_StdPixFmt)srcPixFmt,
						  (HIDDT_StdPixFmt)dstPixFmt,
						  (HIDDT_RGBConversionFunction)function);
    
    AROS_LIBFUNC_EXIT
	
} /* SetRGBConversionFunctionA */
