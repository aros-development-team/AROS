/*
    Copyright � 1995-2001, The AROS Development Team. All rights reserved.
    $Id$	 $Log

    Desc: Graphics function TextLength()
    Lang: english
*/
#include "graphics_intern.h"

#undef NUMCHARS
#define NUMCHARS(tf) ((tf->tf_HiChar - tf->tf_LoChar) + 2)

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <proto/graphics.h>

	AROS_LH3(WORD, TextLength,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),
	AROS_LHA(STRPTR           , string, A0),
	AROS_LHA(ULONG            , count, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 9, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

    struct TextFont *tf = rp->Font;
    WORD    	    strlen;

    if (tf->tf_Flags & FPF_PROPORTIONAL)
    {
    	WORD  idx;
	WORD  defaultidx = NUMCHARS(tf) - 1; /* Last glyph is the default glyph */
	UBYTE c;
	
	for(strlen = 0; count; count--)
	{
	    c = *string++;
	    
	    if ( c < tf->tf_LoChar || c > tf->tf_HiChar)
	    {
		idx = defaultidx;
	    }
	    else
	    {
		idx = c - tf->tf_LoChar;
	    }
	    	    
   	    strlen += ((WORD *)tf->tf_CharKern)[idx];
	    strlen += ((WORD *)tf->tf_CharSpace)[idx];
	}
    }
    else
    {
    	strlen = count * tf->tf_XSize;
    }

    return strlen;
    
    AROS_LIBFUNC_EXIT
    
} /* TextLength */
