/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: english
*/

#include <intuition/classusr.h>
#include <clib/alib_protos.h>
#ifdef _AROS
#include <proto/muimaster.h>
#endif

#include "mui.h"
#include "muimaster_intern.h"

/*****************************************************************************

    NAME */
#ifndef _AROS
__asm BOOL MUI_Layout(register __a0 Object *obj,register __d1 LONG left,register __d2 LONG top,register __d3 LONG width,register __d4 LONG height, register __d5 ULONG flags)
#else
	AROS_LH6(BOOL, MUI_Layout,

/*  SYNOPSIS */
	AROS_LHA(Object *, obj, A0),
	AROS_LHA(ULONG, left, D0),
	AROS_LHA(ULONG, top, D1),
	AROS_LHA(ULONG, width, D2),
	AROS_LHA(LONG, height, D3),
	AROS_LHA(ULONG, two, D4),

/*  LOCATION */
	struct Library *, MUIMasterBase, 21, MUIMaster)
#endif
/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS
	The function itself is a bug ;-) Remove it!

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct MUIMasterBase *,MUIMasterBase)

    static const struct MUIP_Layout method = { MUIM_Layout };
    Object *parent = _parent(obj);

/*
 * Called only by groups, never by windows
 */
//    ASSERT(parent != NULL);

    _left(obj) = left + _mleft(parent);
    _top(obj) = top + _mtop(parent);
    _width(obj) = width;
    _height(obj) = height;

    DoMethodA(obj, (Msg)&method);
    return TRUE;

    AROS_LIBFUNC_EXIT

} /* MUIA_Layout */
