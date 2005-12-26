/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitMasks()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitMasks,

/*  SYNOPSIS */
	AROS_LHA(struct VSprite *, vs, A0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 21, Graphics)

/*  FUNCTION
	Creates the standard BorderLine and CollMask masks of the VSprite.
	VSprites and Bobs are treated accordingly.

    INPUTS
        vs = pointer to VSprite structure

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() InitGMasks() graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  long depth, WordsPerPlane, WordsPerLine, count;
  /* is this a Bob or a VSprite? */
  if (0 != (vs -> Flags & VSPRITE))
  {
    /* assumption: vs -> Widtht = 16 (or 32 on a better chipset??)*/
    WordsPerLine = vs -> Width >> 4; /* Width:16 */
    depth = 2; /* standard */
  }
  else
  {
    WordsPerLine = vs -> Width;
    depth = vs -> Depth;
  }
  WordsPerPlane = (vs -> Height) * WordsPerLine;


  /* create the standard collision mask by or'ing all bitplanes */
  for (count = 0; count < WordsPerPlane; count++)
  {
    WORD * PlaneData = vs -> ImageData;
    WORD Data = PlaneData[count];
    long z;
    for (z = 1; z < depth; z++)
    	Data |= PlaneData[count + z*WordsPerPlane];
    (vs -> CollMask)[count] = Data;
  }

  /* create the standard BorderLine from the collision mask by or'ing all
     lines */
  for (count = 0 ; count < WordsPerLine; count++)
  {
  	WORD * CollMask = vs -> CollMask;
  	WORD Data = CollMask[count];
  	long z;
  	for (z = 1; z < vs-> Height; z++)
  	  Data |= CollMask[count + z*WordsPerLine];
  	(vs -> BorderLine)[count] = Data;
  }

  AROS_LIBFUNC_EXIT
} /* InitMasks */
