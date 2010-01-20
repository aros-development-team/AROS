/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function GetColorMap()
    Lang: english
*/
#include "exec/memory.h"
#include "exec/types.h"
#include "proto/exec.h"
#include "graphics/view.h"
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(struct ColorMap *, GetColorMap,

/*  SYNOPSIS */
	AROS_LHA(ULONG, entries, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 95, Graphics)

/*  FUNCTION
	Allocates, initializes a ColorMap structure and passes back the
	pointer. This enables you to do calls to SetRGB4() and LoadRGB4()
	to load colors for a view port.
	The ColorTable pointer in the ColorMap structure points to a hardware
	specific colormap data structure which you should not interpret.

    INPUTS
	entries	- the number of entries for the colormap

    RESULT
	NULL  - not enough memory could be allocated for the necessary
	        data structures
	other - pointer to a initialized ColorMap structure that may be
	        stored into the ViewPort.ColorMap pointer.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	FreeColorMap(), SetRGB4(), graphics/view.h

    INTERNALS
	RGB Colortable with preference values is incomplete.

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct ColorMap * NewCM = (struct ColorMap *)AllocMem(sizeof(struct ColorMap),
                                                          MEMF_PUBLIC|MEMF_CLEAR);
    LONG 	    * ptr1, * ptr2;
							  
#if 0
    /* ColorTable with some preference values; !!! incomplete */
    const WORD 	      RGBColorTable[] = {0x0000,0x0f00,0x00f0,0x0ff0,
				  	 0x000f,0x0f0f,0x00ff,0x0fff}; /* !!!etc. */
#endif


    /* go on if we got the memory for the ColorMap */
    if (NULL != NewCM)
    {
	/* get memory for the ColorTable */
	NewCM -> ColorTable = AllocMem(entries * sizeof(UWORD), MEMF_CLEAR|MEMF_PUBLIC);

	/* get memory for LowColorbits !!!how much memory we need for that?? */
	NewCM -> LowColorBits = AllocMem(entries * sizeof(UWORD), MEMF_CLEAR|MEMF_PUBLIC);

	ptr1 = NewCM -> ColorTable;
	ptr2 = NewCM -> LowColorBits;

	/* did we get all the memory we wanted? */
	if ( (NULL != ptr1) && (NULL != ptr2) )
	{
#if 0
	    ULONG i;
	    LONG  * L_RGBColorTable = (LONG *)&RGBColorTable[0];
#endif

	    /* further init the GetColorMap structure */
	    NewCM->Type             = COLORMAP_TYPE_V39;
	    NewCM->Count            = entries;
	    NewCM->SpriteResolution = SPRITERESN_DEFAULT;
	    NewCM->SpriteResDefault = SPRITERESN_ECS;
	    NewCM->AuxFlags         = CMAF_FULLPALETTE;
	    NewCM->VPModeID         = -1;
	    /* Originally we could always use palette entries 16-19 for
	       sprites, even if the screen has less than 32 colors. AROS may
	       run on hardware that does not allow this (e.g. VGA cards).
	       In this case we have to shift down sprite colors. Currently
	       we use 4 colors before last 4 colors. For example on VGA cards
	       with only 16 colors we use colors 9 - 12. Remember that last 4 colors
	       of the screen are always used by Intuition.
	       Remember that the first color of the sprite is always transparent. So actually
	       we use 3, not 4 colors.
	       Yes, sprites may look not as expected on screens with low color depth, but at
	       least they will be seen. It's better than nothing.

	       FIXME: this mapping scheme assumes that we always have at least 16 colors.
               For current display modes supported by AROS it's always true, but in future
	       we may support more display modes (for example monochrome ones), and we
	       should take into account that on screens with less than 11 colors this simply
	       won't work */
	    if (entries > 24) {
		/* FIXME: Shouldn't these be different? */
	        NewCM->SpriteBase_Even  = 16;
	        NewCM->SpriteBase_Odd   = 16;
	    } else {
		NewCM->SpriteBase_Even  = entries - 8;
		NewCM->SpriteBase_Odd   = entries - 8;
	    }
	    NewCM->Bp_1_base        = 0x0008;

#if 0
	    /* Fill the ColorTable and the LowColorBits with the appropriate Data */

	    /* as we`re clever we`re doing some 32 bit copying with the 16 bit data */
	    for (i = 0; i < (entries >> 1); i++)
	    {
        	LONG ColorValue = L_RGBColorTable[i];
        	*ptr1++ = ColorValue;
        	*ptr2++ = ColorValue;
	    }
	    /* is there one WORD left to copy? */
	    if (1 == (entries & 1) )
	    {
        	WORD ColorValue = RGBColorTable[entries-1];
        	*(WORD *)ptr1 = ColorValue;
        	*(WORD *)ptr2 = ColorValue;
	    }
#endif

	}
	else /* not enough memory for the tables */
	{
	    if (NULL != NewCM -> ColorTable)
        	FreeMem(NewCM -> ColorTable, entries * sizeof(UWORD));
	    if (NULL != NewCM -> LowColorBits)
        	FreeMem(NewCM -> LowColorBits, entries * sizeof(UWORD));

	    FreeMem(NewCM, sizeof(struct ColorMap));
	    /* make return value invalid */
	    NewCM = NULL;
	}
	
    } /* if (NULL != NewCM) */

    return NewCM;

    AROS_LIBFUNC_EXIT
  
} /* GetColorMap */
