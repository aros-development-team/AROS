/*
    (C) 1995-98 AROS - The Amiga Research OS
    $Id$

    Desc: Graphics function AddBob()
    Lang: english
*/
#include <graphics/gels.h>
#include <graphics/rastport.h>
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <proto/graphics.h>

	AROS_LH2(void, AddBob,

/*  SYNOPSIS */
	AROS_LHA(struct Bob *, bob, A0),
	AROS_LHA(struct RastPort *, rp, A1),

/*  LOCATION */
	struct GfxBase *, GfxBase, 16, Graphics)

/*  FUNCTION
	The Bob is linked into the current gel list via AddVSprite.
	The Bob's flags are set up.

    INPUTS
	Bob = pointer to Bob to be added to gel list
	rp  = pointer to RastPort that has an initilized GelsInfo linked
	      to it (see InitGels()).

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	InitGels() AddVSprite() graphics/rastport.h graphics/gels.h

    INTERNALS

    HISTORY

*****************************************************************************/
{
  AROS_LIBFUNC_INIT
  AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)

  bob->Flags &= 0x00FF;
  AddVSprite (bob->BobVSprite, rp);

  AROS_LIBFUNC_EXIT
} /* AddBob */
