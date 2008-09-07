/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitArea()
    Lang: english
*/
#include <exec/types.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, InitArea,

/*  SYNOPSIS */
	AROS_LHA(struct AreaInfo *, areainfo  , A0),
	AROS_LHA(void *           , buffer    , A1),
	AROS_LHA(WORD             , maxvectors, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 47, Graphics)

/*  FUNCTION
	This function initializes an areainfo structure. The size of the
	passed pointer to the buffer should be 5 times as large as
	maxvectors (in bytes).

    INPUTS
	areainfo   - pointer to AreaInfo structure to be initialized
	buffer     - pointer to free memory to collect vectors
	maxvectors - maximum number of vectors the buffer can hold.

    RESULT
	Areainfo structure initialized such that it will hold the vectors
	created by AreaMove, AreaDraw and AreaEllipse (AreaCircle).

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	AreaDraw(), AreaMove(), AreaEllipse(), AreaCircle(), graphics/rastport.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT

  areainfo->VctrTbl  = buffer;
  areainfo->VctrPtr  = buffer;
  areainfo->FlagTbl  = (BYTE *)(((IPTR)buffer)+(2*sizeof(WORD)*maxvectors));
  areainfo->FlagPtr  = (BYTE *)(((IPTR)buffer)+(2*sizeof(WORD)*maxvectors));
  areainfo->Count    = 0;
  areainfo->MaxCount = maxvectors;

  AROS_LIBFUNC_EXIT
} /* InitArea */
