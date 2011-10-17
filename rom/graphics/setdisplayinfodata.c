/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Store a display mode information in the database
    Lang: english
*/

#include <aros/libcall.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>

/*
 * This function was private in AmigaOS(tm) and is used by display drivers
 * in DEVS:Monitors to populate display mode database.
 * AROS uses its own interface with display drivers and doesn't need this function.
 * It is kept only for binary compatibility with m68k Workbench disks.
 */

AROS_LH5(ULONG, SetDisplayInfoData,
         AROS_LHA(DisplayInfoHandle, handle, A0),
         AROS_LHA(UBYTE *, buf, A1),
         AROS_LHA(ULONG, size, D0),
         AROS_LHA(ULONG, tagID, D1),
         AROS_LHA(ULONG, ID, D2),
         struct GfxBase *, GfxBase, 125, Graphics)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT

}
