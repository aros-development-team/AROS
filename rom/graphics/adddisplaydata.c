/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Store a display mode information in the database
    Lang: english
*/

#include <aros/libcall.h>
#include <graphics/gfxbase.h>

/*
 * This is a private AmigaOS(tm) functions which could be used by display drivers
 * in DEVS:Monitors. This is kept only for binary compatibility with m68k Workbench disks.
 */

AROS_LH1(ULONG, AddDisplayData,
         AROS_LHA(APTR, displayInfoRecord, A0),
         struct GfxBase *, GfxBase, 123, Graphics)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT

}
