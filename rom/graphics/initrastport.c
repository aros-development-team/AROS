/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$	$Log

    Desc: Graphics function InitRastPort()
    Lang: english
*/
#include "graphics_intern.h"
#include <graphics/rastport.h>
#include <proto/exec.h>

#include "gfxfuncsupport.h"

static const struct RastPort defaultRastPort =
{
    NULL, /* Layer */
    NULL, /* BitMap */
    NULL, /* AreaPtrn */
    NULL, /* TmpRas */
    NULL, /* AreaInfo */
    NULL, /* GelsInfo */
    ~0, /* Mask */
    ~0, /* FgPen */
    0, /* BgPen */
    ~0, /* AOlPen */
    JAM2, /* DrawMode */
    0, /* AreaPtSz */
    0, /* linpatcnt */
    0, /* dummy */
    0, /* Flags */
    ~0, /* LinePtrn */
    0,0, /* cp_x, cp_y */
    { 0,0,0,0, 0,0,0,0 }, /* minterms[] */
    0, /* PenWidth */
    0, /* PenHeight */
    NULL, /* Font */
    0, /* AlgoStyle */
    0, /* TxFlags */
    0, /* TxHeight */
    0, /* TxWidth */
    0, /* TxBaseline */
    0, /* TxSpacing */
    NULL, /* RP_User */
    { 0,0 }, /* longreserved */
    { 0,0,0,0, 0,0,0 }, /* wordreserved */
    { 0,0,0,0, 0,0,0,0 }, /* reserved */
};

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH1(void, InitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 33, Graphics)

/*  FUNCTION
	Initializes a RastPort structure.

    INPUTS
	rp - The RastPort to initialize.

    RESULT
	all entries in RastPort get zeroed out, with the following exceptions:

	    Mask, FgPen, AOLPen, and LinePtrn are set to -1.
	    The DrawMode is set to JAM2
	    The font is set to the standard system font

    NOTES
	You must call DeinitRastPort() before you free the structure.

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

    CopyMem ((UBYTE *)&defaultRastPort, rp, sizeof (struct RastPort));
    RP_BACKPOINTER(rp) = rp; /* Mark rastport as valid (no manual clone) */
    
    SetFont (rp, GfxBase->DefaultFont);

    AROS_LIBFUNC_EXIT
    
} /* InitRastPort */
