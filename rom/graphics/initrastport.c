/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
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

	AROS_LH1(BOOL, InitRastPort,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 33, Graphics)

/*  FUNCTION
	Initializes a RastPort structure.

    INPUTS
	rp - The RastPort to initialize.

    RESULT
	TRUE if the RastPort was successfuly initialized and FALSE
	otherwise.

    NOTES
	You must call DeinitRastPort() before you free the structure.

	AROS defines this function with a return value which you should
	check.

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

    CopyMem ((UBYTE *)&defaultRastPort, rp, sizeof (struct RastPort));
    RP_BACKPOINTER(rp) = rp; /* Mark rastport as valid (no manual clone) */
    
    SetFont (rp, GfxBase->DefaultFont);

    return TRUE;

    AROS_LIBFUNC_EXIT
    
} /* InitRastPort */
