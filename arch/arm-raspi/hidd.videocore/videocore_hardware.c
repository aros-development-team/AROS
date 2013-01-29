/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$
 
    Desc: VideoCore4 hardware functions
    Lang: English
*/

#define DEBUG 1 /* no SysBase */
#include <aros/debug.h>

#include <asm/io.h>

#include "videocore_hardware.h"
#include "videocore_class.h"

#undef SysBase
extern struct ExecBase *SysBase;

ULONG getVideoCoreID(struct HWData *data)
{

}

VOID initVideoCoreFIFO(struct HWData *data)
{

}

VOID syncVideoCoreFIFO(struct HWData *data)
{

}

VOID writeVideoCoreFIFO(struct HWData *data, ULONG val)
{

    }

BOOL initVideoCoreHW(struct HWData *data, OOP_Object *device)
{
    return TRUE;
}

VOID setModeVideoCore(struct HWData *data, ULONG width, ULONG height)
{
    D(bug("[VideoCore] SetMode: %dx%d\n", width, height));

}

VOID refreshAreaVideoCore(struct HWData *data, struct Box *box)
{

}

VOID rectFillVideoCore(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height)
{

}

VOID ropFillVideoCore(struct HWData *data, ULONG color, LONG x, LONG y, LONG width, LONG height, ULONG mode)
{

}

VOID ropCopyVideoCore(struct HWData *data, LONG sx, LONG sy, LONG dx, LONG dy, ULONG width, ULONG height, ULONG mode)
{

}

VOID defineCursorVideoCore(struct HWData *data, struct MouseData *mouse)
{

}

VOID displayCursorVideoCore(struct HWData *data, LONG mode)
{

}

VOID moveCursorVideoCore(struct HWData *data, LONG x, LONG y)
{

}
