/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Graphics function InitGels()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <graphics/gels.h>
#include <proto/exec.h>

#include "gels_internal.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH3(void, InitGels,

/*  SYNOPSIS */
	AROS_LHA(struct VSprite *, head, A0),
	AROS_LHA(struct VSprite *, tail, A1),
	AROS_LHA(struct GelsInfo *, GInfo, A2),

/*  LOCATION */
	struct GfxBase *, GfxBase, 20, Graphics)

/*  FUNCTION
	Makes the two VSprites head and tail of the gel list that is connected
	to the GelsInfo structure. The two VSprites are linked together and
	their x and y coordinates are initilized such that the serve as the
	keystones of the list.

    INPUTS
	head  - pointer to the VSprite structure to be used as head of the gel list
	tail  - pointer to the VSprite structure to be used as tail of the gel list
	GInfo - pointer to the GelsInfo structure to be initilized

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics/rastport.h  graphics/gels.h

    INTERNALS

    HISTORY


*****************************************************************************/
{
	AROS_LIBFUNC_INIT
	AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

	/* initilize the head's coordinates with the lowest possible values */
	head -> OldY = 0x8000;
	head -> OldX = 0x8000;
	head -> Y    = 0x8000;
	head -> X    = 0x8000;

	/* initilize the tail's coordinates with the highest possible values */
	tail -> OldY = 0x7FFF;
	tail -> OldX = 0x7FFF;
	tail -> Y    = 0x7FFF;
	tail -> X    = 0x7FFF;

	/* now link it to the gelsinfo and interconnect them */
	GInfo -> gelHead = head;
	GInfo -> gelTail = tail;

	head -> NextVSprite = tail;
	head -> ClearPath   = tail;
	tail -> PrevVSprite = head;

	head -> IntVSprite = NULL;
	tail -> IntVSprite = NULL;

	AROS_LIBFUNC_EXIT
} /* InitGels */
