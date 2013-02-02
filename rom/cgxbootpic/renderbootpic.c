/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <exec/types.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <proto/utility.h>

#include "cgxbootpic_intern.h"

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
    AROS_LH4(void, RenderBootPic,

/*  SYNOPSIS */

    AROS_LHA(void *, framebuffer, A0),
    AROS_LHA(ULONG, width, D0),
    AROS_LHA(ULONG, height, D1),
    AROS_LHA(ULONG, depth, D2),
/*  LOCATION */

    struct CgxBootPicBase *, CgxBootPicBase, 5, CgxBootPic)

/*  FUNCTION
    This function dumps a boot picture into the specified framebuffer.

    INPUTS

    RESULT

    NOTES
    The Gfx susbsytem opens cgxbootpic.library if it is
    available, and when displays are created, calls this
    function with the displays framebuffer as input to render
    the selected bootpic.

    EXAMPLE

    BUGS
    This implementation is bugged - need to check what the real library
    on m68k expects as params and correct this code appropriately.
    SEE ALSO

    INTERNALS

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    D(bug("[CgxBootPic] %s()\n", __PRETTY_FUNCTION__));

    return;

    AROS_LIBFUNC_EXIT
} /* RenderBootPic */
