/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/monitorclass.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/oop.h>
#include <proto/utility.h>

#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase

#include "intuition_intern.h"
#include "monitorclass_intern.h"
#include "monitorclass_private.h"

/*****************************************************************************************

    NAME
	--background--

    LOCATION
	MONITORCLASS

    NOTES
	In AROS display drivers have associated BOOPSI objects of MONITORCLASS class.
	This class provides information about relative physical placement of monitors
	in user's workspace as well as some additional properties.

	MONITORCLASS is a pseudo name. This class is in fact private to the system and
	does not have a public ID. The user can't create objects of this class manually.

	This class is fully compatible with MorphOS starting from v2.6.

*****************************************************************************************/

Object *DisplayDriverNotify(APTR obj, BOOL add, struct IntuitionBase *IntuitionBase)
{
    if (add)
	return NewObject(GetPrivIBase(IntuitionBase)->monitorclass, NULL, MA_DriverObject, obj, TAG_DONE);
    else {
	DisposeObject(obj);
	return NULL;
    }
}

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase      ((struct IntuitionBase *)(cl->cl_UserData))
#define HiddAttrBase	   (GetPrivIBase(IntuitionBase)->HiddAttrBase)
#define HiddGfxAttrBase    (GetPrivIBase(IntuitionBase)->HiddGfxAttrBase)
#define HiddPixFmtAttrBase (GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase)

/***********************************************************************************/

static void ActivationHandler(Object *mon, OOP_Object *bitmap)
{
    Class *cl = OCLASS(mon);

    /* Experimental: NewMonitor will be picked up by input handler
       when the next event arrives, so no signals etc */
    GetPrivIBase(IntuitionBase)->NewMonitor = mon;
}

/***********************************************************************************/

static BYTE pixelformats[] = {
    -1,
    -1,
    -1,
    PIXFMT_RGB24,
    PIXFMT_BGR24,
    PIXFMT_RGB16,
    PIXFMT_RGB16PC,
    PIXFMT_BGR16,
    PIXFMT_BGR16PC,
    PIXFMT_RGB15,
    PIXFMT_RGB15PC,
    PIXFMT_BGR15,
    PIXFMT_BGR15PC,
    PIXFMT_ARGB32,
    PIXFMT_BGRA32,
    PIXFMT_RGBA32,
    -1,
    PIXFMT_ARGB32,
    PIXFMT_BGRA32,
    PIXFMT_RGBA32,
    -1,
    PIXFMT_LUT8,
    -1
};

Object *MonitorClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    OOP_Object *driver = (OOP_Object *)GetTagData(MA_DriverObject, 0, msg->ops_AttrList);
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    struct MonitorData *data;
    OOP_Object *sync, *pixfmt;
    /* Tags order is important because CallBackData needs to be set before
       function pointer. Otherwise the function can be called with a wrong
       pointer. */
    struct TagItem tags[] = {
	{aHidd_Gfx_ActiveCallBackData, 0                      },
	{aHidd_Gfx_ActiveCallBack    , (IPTR)ActivationHandler},
	{TAG_DONE                    , 0                      }
    };

    D(kprintf("[monitorclass] OM_NEW\n"));

    if (!driver)
	return NULL;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (!o)
	return NULL;

    data = INST_DATA(cl, o);

    data->driver = driver;

    /* We can't list driver's pixelformats, we can list only modes. This does not harm however,
       just some pixelformats will be processed more than once */
    while ((mode = HIDD_Gfx_NextModeID(driver, mode, &sync, &pixfmt)) != vHidd_ModeID_Invalid) {
	IPTR stdpf;
	BYTE cgxpf;

	OOP_GetAttr(pixfmt, aHidd_PixFmt_StdPixFmt, &stdpf);
	cgxpf = pixelformats[stdpf];
	D(bug("[monitorclass] Mode 0x%08lX, StdPixFmt %lu, CGX pixfmt %d\n", mode, stdpf, cgxpf));

	if (cgxpf != -1) {
	    data->pfobjects[cgxpf] = pixfmt;
	    data->pixelformats[cgxpf] = TRUE;
	}
    }

    tags[0].ti_Data = (IPTR)o;
    OOP_SetAttrs(driver, tags);

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);
    AddTail((struct List *)&GetPrivIBase(IntuitionBase)->MonitorList, (struct Node *)data);
    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return o;
}

/*****************************************************************************************

    NAME
	MA_MonitorName

    SYNOPSIS
	[..G], STRPTR

    LOCATION
	MONITORCLASS

    FUNCTION
	Query monitor driver name

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_Manufacturer

    SYNOPSIS
	[..G], STRPTR

    LOCATION
	MONITORCLASS

    FUNCTION
	Query video card hardware manufacturer name

    NOTES
	Not all drivers may specify manufacturer string. NULL is a valid return value.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_ManufacturerID

    SYNOPSIS
	[..G], ULONG

    LOCATION
	MONITORCLASS

    FUNCTION
	Query video card hardware's numeric manufacturer ID (which may come from PCI,
	Zorro, etc).

    NOTES
	Not all drivers may have assigned IDs. For example VGA driver and virtual
	hosted drivers do not associate themselves with any IDs.

    EXAMPLE

    BUGS

    SEE ALSO
	MA_ProductID

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_ProductID

    SYNOPSIS
	[..G], ULONG

    LOCATION
	MONITORCLASS

    FUNCTION
	Query video card hardware's numeric product ID (which may come from PCI, Zorro, etc).

    NOTES
	Not all drivers may have assigned IDs. For example VGA driver and virtual
	hosted drivers do not associate themselves with any IDs.

    EXAMPLE

    BUGS

    SEE ALSO
	MA_ManufacturerID

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_MemorySize

    SYNOPSIS
	[..G], ULONG

    LOCATION
	MONITORCLASS

    FUNCTION
	Query total size of video card memory in bytes.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_MemoryClock

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_PixelFormats

    SYNOPSIS
	[..G], ULONG *

    LOCATION
	MONITORCLASS

    FUNCTION
	Query table of supported pixelformats.

	A returned value is a pointer to static array of ULONGs, one ULONG per CyberGraphX
	pixelformat. Values of these ULONGs are actually booleans. TRUE value in the array
	says that the pixelformat is supported,	FALSE means it's not.

    NOTES

    EXAMPLE
	ULONG *pfs;

	GetAttr(MA_PixelFormats, monitor, (IPTR *)&pfs);
	if (pfs[PUXFMT_LUT8])
	    printf("The display driver supports LUT8 format\n");

    BUGS

    SEE ALSO
	MA_ManufacturerID

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_TopLeftMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in top-left diagonal direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor, MA_MiddleRightMonitor,
	MA_BottomLeftMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_TopMiddleMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in top direction relative to the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor, MA_MiddleRightMonitor,
	MA_BottomLeftMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_TopRightMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in top-right diagonal direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor, MA_MiddleRightMonitor,
	MA_BottomLeftMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_MiddleLeftMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in left direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleRightMonitor,
	MA_BottomLeftMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_MiddleRightMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in right direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor,
	MA_BottomLeftMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_BottomLeftMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in bottom-left diagonal direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor,
	MA_MiddleRightMonitor, MA_BottomMiddleMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_BottomMiddleMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in bottom direction relative to the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor,
	MA_MiddleRightMonitor, MA_BottomLeftMonitor, MA_BottomRightMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_BottomRightMonitor

    SYNOPSIS
	[.SG], Object *

    LOCATION
	MONITORCLASS

    FUNCTION
	Get a pointer to a monitor placed in bottom-right diagonal direction relative to
	the current one.

	This attribute is used to describe relative placement of monitors in user's
	physical environment.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_TopLeftMonitor, MA_TopMiddleMonitor, MA_TopRightMonior, MA_MiddleLeftMonitor,
	MA_MiddleRightMonitor, MA_BottomLeftMonitor, MA_BottomMiddleMonitor

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_GammaControl

    SYNOPSIS
	[..G], BOOL

    LOCATION
	MONITORCLASS

    FUNCTION
	Query if the display driver supports gamma control

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_GetDefaultGammaTables, MM_SetDefaultGammaTables

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_PointerType

    SYNOPSIS
	[..G], ULONG

    LOCATION
	MONITORCLASS

    FUNCTION
	Query supported mouse pointer sprite formats.

	The returned value is a combination of the following bit flags:
	  PointerType_3Plus1 - color 0 transparent, 1-3 visible	(Amiga(tm) chipset sprite)
	  PointerType_2Plus1 - color 0 transparent, 1 unsefined (can be for example clear or
			       inverse), 2-3 visible
	  PointerType_ARGB   - Direct color bitmap (hi-color or truecolor, possibly with alpha
			       channel

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_GetPointerBounds

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_DriverName

    SYNOPSIS
	[..G], STRPTR

    LOCATION
	MONITORCLASS

    FUNCTION
	Query CyberGraphX driver name. It is the name which can be given to
	cybergraphics.library/BestCModeIDTagList() as CYBRBIDTG_BoardName value.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_GetPointerBounds

    INTERNALS

*****************************************************************************************/

/*****************************************************************************************

    NAME
	MA_MemoryClock

    SYNOPSIS
	[..G], ULONG

    LOCATION
	MONITORCLASS

    FUNCTION
	Query video card's memory clock in Hz. 0 is a valid value meaning 'unknown'.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MA_MemorySize

    INTERNALS

*****************************************************************************************/

/***********************************************************************************/

IPTR MonitorClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);
    IPTR val;

    D(kprintf("[monitorclass] OM_GET\n"));

    switch (msg->opg_AttrID)
    {
    case MA_MonitorName:
	OOP_GetAttr(data->driver, aHidd_Name, msg->opg_Storage);
	break;

    case MA_Manufacturer:
	OOP_GetAttr(data->driver, aHidd_ProducerName, msg->opg_Storage);
	break;

    case MA_ManufacturerID:
	OOP_GetAttr(data->driver, aHidd_Producer, msg->opg_Storage);
	break;

    case MA_ProductID:
	OOP_GetAttr(data->driver, aHidd_Product, msg->opg_Storage);
	break;

    case MA_MemorySize:
	OOP_GetAttr(data->driver, aHidd_Gfx_MemorySize, msg->opg_Storage);
	break;

    case MA_PixelFormats:
	*msg->opg_Storage = (IPTR)data->pixelformats;
	break;

    case MA_TopLeftMonitor:
	*msg->opg_Storage = (IPTR)data->topleft;
	break;

    case MA_TopMiddleMonitor:
	*msg->opg_Storage = (IPTR)data->topmiddle;
	break;

    case MA_TopRightMonitor:
	*msg->opg_Storage = (IPTR)data->topright;
	break;

    case MA_MiddleLeftMonitor:
	*msg->opg_Storage = (IPTR)data->middleleft;
	break;

    case MA_MiddleRightMonitor:
	*msg->opg_Storage = (IPTR)data->middleright;
	break;

    case MA_BottomLeftMonitor:
	*msg->opg_Storage = (IPTR)data->bottomleft;
	break;

    case MA_BottomMiddleMonitor:
	*msg->opg_Storage = (IPTR)data->bottommiddle;
	break;

    case MA_BottomRightMonitor:
	*msg->opg_Storage = (IPTR)data->bottomright;
	break;

    case MA_GammaControl:
	*msg->opg_Storage = HIDD_Gfx_GetGamma(data->driver, NULL, NULL, NULL);
	break;

    case MA_PointerType:
	OOP_GetAttr(data->driver, aHidd_Gfx_HWSpriteTypes, &val);
	*msg->opg_Storage = val;
	break;

    case MA_DriverName:
	OOP_GetAttr(data->driver, aHidd_Gfx_DriverName, msg->opg_Storage);
	break;

    case MA_MemoryClock:
	OOP_GetAttr(data->driver, aHidd_Gfx_MemoryClock, msg->opg_Storage);
	break;

    default:
	return DoSuperMethodA(cl, o, (Msg)msg);
    }

    return TRUE;
}

/***********************************************************************************/

IPTR MonitorClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;

    tstate = msg->ops_AttrList;
    while((tag = NextTagItem((const struct TagItem **)&tstate))) {
	switch (tag->ti_Tag) {
	case MA_TopLeftMonitor:
	    data->topleft = (Object *)tag->ti_Data;
	    break;

	case MA_TopMiddleMonitor:
	    data->topmiddle = (Object *)tag->ti_Data;
	    break;

	case MA_TopRightMonitor:
	    data->topright = (Object *)tag->ti_Data;
	    break;

	case MA_MiddleLeftMonitor:
	    data->middleleft = (Object *)tag->ti_Data;
	    break;

	case MA_MiddleRightMonitor:
	    data->middleright = (Object *)tag->ti_Data;
	    break;

	case MA_BottomLeftMonitor:
	    data->bottomleft = (Object *)tag->ti_Data;
	    break;

	case MA_BottomMiddleMonitor:
	    data->bottommiddle = (Object *)tag->ti_Data;
	    break;

	case MA_BottomRightMonitor:
	    data->bottomright = (Object *)tag->ti_Data;
	    break;
	
	case MA_PointerVisible:
	    HIDD_Gfx_SetCursorVisible(data->driver, tag->ti_Data);
	    break;
	}
    }
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/***********************************************************************************/

IPTR MonitorClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct MonitorData *data = INST_DATA(cl, o);
    struct TagItem tags[] = {
	{aHidd_Gfx_ActiveCallBack, 0},
	{TAG_DONE                , 0}
    };

    D(kprintf("MonitorClass: OM_DISPOSE\n"));

    /* Disable activation callback */
    OOP_SetAttrs(data->driver, tags);

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);
    Remove((struct Node *)data);

    /* TODO: fix up spatial links */

    /* If an active monitor is being removed, we should activate another one */
    if (GetPrivIBase(IntuitionBase)->ActiveMonitor == o)
	ActivateMonitor((Object *)GetHead(&GetPrivIBase(IntuitionBase)->MonitorList), IntuitionBase);

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    return DoSuperMethodA(cl, o, msg);
}

/************************************************************************************

    NAME
	MM_GetRootBitMap

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, ULONG PixelFormat, struct BitMap **Store);

        DoMethodA(Object *obj, struct msGetRootBitMap *msg);

    LOCATION

    FUNCTION
	This method is provided only for source code compatibility with MorphOS operating
	system.

	Under MorphOS this method returns a pointer to internal root bitmap of the
	display driver corresponding to the specified pixelformat. Displayable bitmaps are
	supposed to be created as friends of the root bitmap.

	In AROS displayable bitmaps need complete display mode information and not
	only pixelformat. So this method will never be implemented and will always return NULL
	pointer. In order to create a displayable RTG bitmap on AROS the user needs to supply
	BMF_SCREEN flag together with display mode ID to AllocBitMap() function.

    INPUTS
	obj         - A monitor object
	MethodID    - MM_GetRootBitMap
	PixelFormat - A CyberGraphX pixelformat code to get root bitmap for
	Store	    - A storage where root bitmap pointer will be placed.

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	graphics.library/AllocBitMap()

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_GetRootBitMap(Class *cl, Object *obj, struct msGetRootBitMap *msg)
{
    *msg->Store = NULL;

    return 0;
}

/************************************************************************************

    NAME
	MM_Query3DSupport

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, ULONG PixelFormat, ULONG *Store);

        DoMethodA(Object *obj, struct msQuery3DSupport *msg);

    LOCATION

    FUNCTION
	Ask the display driver for type of 3D support for the given pixelformat.

	Supplied storage will be filled with one of:
	    MSQUERY3D_UNKNOWN  - Unsupported pixelformat or some internal error
	    MSQUERY3D_NODRIVER - There is no 3D driver for the given pixelformat
	    MSQUERY3D_SWDRIVER - A software 3D support is available for the given
				 pixelformat
	    MSQUERY3D_HWDRIVER - A hardware 3D support is available for the given
				 pixelformat

    INPUTS
	obj         - A monitor object to query
	MethodID    - MM_Query3DSupport
	PixelFormat - A CyberGraphX pixelformat code
	Store	    - A pointer to a storage where return value will be placed

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_Query3DSupport(Class *cl, Object *obj, struct msQuery3DSupport *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);
    OOP_Object *pf = data->pfobjects[msg->PixelFormat];

    if (pf) {
	if (HIDD_Gfx_QueryHardware3D(data->driver, pf))
	    *msg->Store = MSQUERY3D_HWDRIVER;
	else {
	    IPTR depth;

	    OOP_GetAttr(pf, aHidd_PixFmt_Depth, &depth);
	    if (depth > 8)
		*msg->Store = MSQUERY3D_SWDRIVER;
	    else
		*msg->Store = MSQUERY3D_NODRIVER;
	}
    } else
	*msg->Store = MSQUERY3D_UNKNOWN;

    return *msg->Store;
}

/************************************************************************************

    NAME
	MM_GetDefaultGammaTables

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

        DoMethodA(Object *obj, struct msGetDefaultGammaTables *msg);

    LOCATION

    FUNCTION
	Get default gamma correction tables for the monitor

    INPUTS
	obj      - A monitor object to query
	MethodID - MM_GetDefaultGammaTables
	Red	 - A pointer to an array of 256 bytes where gamma correction data for
		   red component will be placed. You may speciy a NULL pointer in order
		   to ignore this component.
	Green	 - A pointer to an array of 256 bytes where gamma correction data for
		   green component will be placed. You may speciy a NULL pointer in order
		   to ignore this component.
	Blue	 - A pointer to an array of 256 bytes where gamma correction data for
		   blue component will be placed. You may speciy a NULL pointer in order
		   to ignore this component.

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_SetDefaultGammaTables

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_GetDefaultGammaTables(Class *cl, Object *obj, struct msGetDefaultGammaTables *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);

    /* Currently we don't use per-screen gamma tables, so we just forward the request
       to the driver.
       If we implement per-screen gamma correction, we'll need more sophisticated
       management here */
    return HIDD_Gfx_GetGamma(data->driver, msg->Red, msg->Green, msg->Blue);
}

/************************************************************************************

    NAME
	MM_GetDefaultPixelFormat

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, ULONG Depth, ULONG *Store);

        DoMethodA(Object *obj, struct msGetDefaultPixelFormat *msg);

    LOCATION

    FUNCTION
	Get driver's preferred pixelformat for specified bitmap depth.

    INPUTS
	obj      - A monitor object
	MethodID - MM_GetDefaultPixelFormat
	Depth	 - Depth to ask about
	Store	 - A pointer to an ULONG location where CyberGraphX pixelformat
		   number will be placed. -1 means unsupported depth.

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_GetDefaultPixelFormat(Class *cl, Object *obj, struct msGetDefaultPixelFormat *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);
    ULONG i;

    for (i = 0; i < MONITOR_MAXPIXELFORMATS; i++) {
	if (data->pfobjects[i]) {
	    IPTR depth;

	    OOP_GetAttr(data->pfobjects[i], aHidd_PixFmt_Depth, &depth);
	    if (depth == msg->Depth) {
		*msg->Store = i;
		break;
	    }
	}
    }

    if (i == MONITOR_MAXPIXELFORMATS)
        *msg->Store = -1;

    return *msg->Store;
}

/************************************************************************************

    NAME
	MM_GetPointerBounds

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, ULONG PointerType, ULONG *Width, ULONG *Height);

        DoMethodA(Object *obj, struct msGetPointerBounds *msg);

    LOCATION
	monitorclass

    FUNCTION
	Get maximum allowed size of mouse pointer sprite.

    INPUTS
	obj         - A monitor object
	MethodID    - MM_GetPointerBounds
	PointerType - Pointer type (one of PointerType_...)
	Width	    - A pointer to an ULONG location where width will be placed.
	Height	    - A pointer to an ULONG location where height will be placed.

    RESULT
	FALSE is given pointer type is not supported, TRUE otherwise.

    NOTES
	Width and Height are considered undefined if the method returns FALSE.

    EXAMPLE

    BUGS

    SEE ALSO
	MA_PointerType

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_GetPointerBounds(Class *cl, Object *obj, struct msGetPointerBounds *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);

    return HIDD_Gfx_GetMaxSpriteSize(data->driver, msg->PointerType, msg->Width, msg->Height);
}

/************************************************************************************

    NAME
	MM_RunBlanker

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID);

        DoMethodA(Object *obj, Msg *msg);

    LOCATION

    FUNCTION
	Starts screensaver on the monitor.

	At the moment AROS has no integrated screensaver support. The method is
	considered reserved and not implemented.

    INPUTS
	obj         - A monitor object
	MethodID    - MM_RunBlanker

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_EnterPowerSaveMode, MM_ExitBlanker

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_RunBlanker(Class *cl, Object *obj, Msg *msg)
{
    /* We have no integrated screensaver support */

    return FALSE;
}

/************************************************************************************

    NAME
	MM_EnterPowerSaveMode

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID);

        DoMethodA(Object *obj, Msg *msg);

    LOCATION

    FUNCTION
	Starts power saving mode on the monitor.

    INPUTS
	obj         - A monitor object
	MethodID    - MM_EnterPowerSaveMode

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_RunBlanker, MM_ExitBlanker

    INTERNALS
	Current implementation just immediately sets DMPS level to "Off" for the
	monitor.

************************************************************************************/

IPTR MonitorClass__MM_EnterPowerSaveMode(Class *cl, Object *obj, Msg *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_DPMSLevel, vHidd_Gfx_DPMSLevel_Off},
	{TAG_DONE	    , 0			     }    
    };
    
    return OOP_SetAttrs(data->driver, tags);
}

/************************************************************************************

    NAME
	MM_ExitBlanker

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID);

        DoMethodA(Object *obj, Msg *msg);

    LOCATION

    FUNCTION
	Stops screensaver and/or power saving mode on the monitor.

    INPUTS
	obj         - A monitor object
	MethodID    - MM_ExitBlanker

    RESULT
	Undefined.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
	MM_EnterPowerSaveMode, MM_RunBlanker

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_ExitBlanker(Class *cl, Object *obj, Msg *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_DPMSLevel, vHidd_Gfx_DPMSLevel_On},
	{TAG_DONE	    , 0			    }    
    };
    
    return OOP_SetAttrs(data->driver, tags);
}

/************************************************************************************

    NAME
	MM_SetDefaultGammaTables

    SYNOPSIS
        DoMethod(Object *obj, ULONG MethodID, UBYTE *Red, UBYTE *Green, UBYTE *Blue);

        DoMethodA(Object *obj, struct msSetDefaultGammaTables *msg);

    LOCATION

    FUNCTION
	Set default gamma correction tables for the monitor

    INPUTS
	obj      - A monitor object to query
	MethodID - MM_GetDefaultGammaTables
	Red	 - A pointer to an array of 256 bytes where gamma correction data for
		   red component is placed. You may speciy a NULL pointer in order
		   to ignore this component.
	Green	 - A pointer to an array of 256 bytes where gamma correction data for
		   green component is placed. You may speciy a NULL pointer in order
		   to ignore this component.
	Blue	 - A pointer to an array of 256 bytes where gamma correction data for
		   blue component is placed. You may speciy a NULL pointer in order
		   to ignore this component.

    RESULT
	Undefined.

    NOTES
	This method is AROS-specific.

    EXAMPLE

    BUGS

    SEE ALSO
	MM_GetDefaultGammaTables

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_SetDefaultGammaTables(Class *cl, Object *obj, struct msSetDefaultGammaTables *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);

    /* Currently we don't use per-screen gamma tables, so we just forward the request
       to the driver.
       If we implement per-screen gamma correction, we'll need more sophisticated
       management here */
    return HIDD_Gfx_SetGamma(data->driver, msg->Red, msg->Green, msg->Blue);
}

/************************************************************************************/

ULONG MonitorClass__MM_GetCompositionFlags(Class *cl, Object *obj, struct msGetCompositionFlags *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);
    struct HIDD_ModeProperties modeprops;

    HIDD_Gfx_ModeProperties(data->driver, msg->ModeID, &modeprops, sizeof(modeprops));
    return modeprops.CompositionFlags;
}

/************************************************************************************/

void MonitorClass__MM_SetPointerPos(Class *cl, Object *obj, struct msSetPointerPos *msg)
{
    struct MonitorData *data = INST_DATA(cl, obj);

    HIDD_Gfx_SetCursorPos(data->driver, msg->x, msg->y);
}
