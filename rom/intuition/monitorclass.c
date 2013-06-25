/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <cybergraphx/cybergraphics.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <graphics/driver.h>
#include <graphics/sprite.h>
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
    {
	Object *mon = NewObject(GetPrivIBase(IntuitionBase)->monitorclass, NULL, MA_MonitorHandle, obj, TAG_DONE);

	D(bug("[monitorclass] Created monitorclass object 0x%p\n", mon));
	if (mon) {
	    /* Install default mouse pointer on the new monitor */
	    Object *ptr = GetPrivIBase(IntuitionBase)->DefaultPointer;

	    if (ptr) {
		struct SharedPointer *pointer;

		GetAttr(POINTERA_SharedPointer, ptr, (IPTR *)&pointer);
		DoMethod(mon, MM_SetPointerShape, pointer);
	    }
	}

	return mon;
    }
    else
    {
	DisposeObject(obj);
	return NULL;
    }
}

/***********************************************************************************/

static void SetPointerPos(struct IMonitorNode *data, struct IntuitionBase *IntuitionBase)
{
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    ULONG x = data->mouseX;
    ULONG y = data->mouseY;

    DB2(bug("[monitorclass] SetPointerPos(%d, %d), pointer 0x%p\n", x, y, data->pointer));
    if (data->pointer)
    {
	/* Update sprite position, just for backwards compatibility */
	data->pointer->sprite->es_SimpleSprite.x = x;
	data->pointer->sprite->es_SimpleSprite.y = y;
    }

    DB2(bug("[monitorclass] Physical coordinates: (%d, %d)\n", x, y));
    HIDD_Gfx_SetCursorPos(data->handle->gfxhidd, x, y);
}

/***********************************************************************************/

static void ActivationHandler(Object *mon, OOP_Object *bitmap)
{
    Class *cl = OCLASS(mon);
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    
    /* NewMonitor will be picked up by input handler when the next event arrives, so no signals etc */
    GetPrivIBase(IntuitionBase)->NewMonitor = mon;
}

/***********************************************************************************/

static void ResetGamma(struct IMonitorNode *data)
{
    data->active_r = &data->gamma[GAMMA_R];
    data->active_g = &data->gamma[GAMMA_G];
    data->active_b = &data->gamma[GAMMA_B];
}

Object *MonitorClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddAttrBase = GetPrivIBase(IntuitionBase)->HiddAttrBase;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    OOP_AttrBase HiddGfxAttrBase = GetPrivIBase(IntuitionBase)->HiddGfxAttrBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct MonitorHandle *handle = (struct MonitorHandle *)GetTagData(MA_MonitorHandle, 0, msg->ops_AttrList);
    HIDDT_ModeID mode = vHidd_ModeID_Invalid;
    struct IMonitorNode *data;
    OOP_Object *sync, *pixfmt;
    /* Tags order is important because CallBackData needs to be set before
       function pointer. Otherwise the function can be called with a wrong
       pointer. */
    struct TagItem tags[] =
    {
	{aHidd_Gfx_ActiveCallBackData, 0                      },
	{aHidd_Gfx_ActiveCallBack    , (IPTR)ActivationHandler},
	{TAG_DONE                    , 0                      }
    };

    D(kprintf("[monitorclass] OM_NEW\n"));

    if (!handle)
	return NULL;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (!o)
	return NULL;

    data = INST_DATA(cl, o);

    data->handle = handle;

    /* We can't list driver's pixelformats, we can list only modes. This does not harm however,
       just some pixelformats will be processed more than once */
    while ((mode = HIDD_Gfx_NextModeID(handle->gfxhidd, mode, &sync, &pixfmt)) != vHidd_ModeID_Invalid)
    {
	IPTR cgxpf;

	OOP_GetAttr(pixfmt, aHidd_PixFmt_CgxPixFmt, &cgxpf);
	D(bug("[monitorclass] Mode 0x%08lX, CGX pixfmt %ld\n", mode, cgxpf));

	if (cgxpf != -1)
        {
	    data->pfobjects[cgxpf] = pixfmt;
	    data->pixelformats[cgxpf] = TRUE;
	}
    }

    if (OOP_GET(data->handle->gfxhidd, aHidd_Gfx_SupportsGamma))
    {
        UWORD i;

        D(bug("[monitorclass] Creating gamma table\n"));

        data->gamma = AllocMem(256 * 3, MEMF_ANY);
        if (!data->gamma)
        {
            DoSuperMethod(cl, o, OM_DISPOSE);
            return NULL;
        }

        /* Create default gamma table */
        for (i = 0; i < 256; i++)
        {
            data->gamma[i + GAMMA_R] = i;
            data->gamma[i + GAMMA_G] = i;
            data->gamma[i + GAMMA_B] = i;
        }
        ResetGamma(data);
    }

    OOP_GetAttr(handle->gfxhidd, aHidd_Name, (IPTR *)&data->MonitorName);

    tags[0].ti_Data = (IPTR)o;
    OOP_SetAttrs(handle->gfxhidd, tags);

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
	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
    	In MorphOS up to v2.5 this attribute returns monitor ID, not a pointer to a
	monitor object.

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
	  PointerType_2Plus1 - color 0 transparent, 1 undefined (can be for example clear or
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

/*****************************************************************************************

    NAME
	MA_Windowed

    SYNOPSIS
	[..G], BOOL

    LOCATION
	MONITORCLASS

    FUNCTION
	Check if this monitor is a window on hosted OS desktop.

	This means that the host OS is responsible for handling mouse input and display
	activation. Monitors with this attribute set to TRUE should be ignored by
	multi-display desktop configuration software.

	These monitors should have no spatial links to other monitors. Activation of these
	monitors is done by clicking on their windows on the host's desktop and is handled
	by the driver itself.

    NOTES
	This attribute is AROS-specific, it does not exist in MorphOS.

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

*****************************************************************************************/

/***********************************************************************************/

IPTR MonitorClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddAttrBase = GetPrivIBase(IntuitionBase)->HiddAttrBase;
    OOP_AttrBase HiddGfxAttrBase = GetPrivIBase(IntuitionBase)->HiddGfxAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, o);

    D(kprintf("[monitorclass] OM_GET\n"));

    switch (msg->opg_AttrID)
    {
    case MA_MonitorName:
	*msg->opg_Storage = (IPTR)data->MonitorName;
	break;

    case MA_Manufacturer:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_ProducerName, msg->opg_Storage);
	break;

    case MA_ManufacturerID:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Producer, msg->opg_Storage);
	break;

    case MA_ProductID:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Product, msg->opg_Storage);
	break;

    case MA_MemorySize:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Gfx_MemorySize, msg->opg_Storage);
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
	*msg->opg_Storage = data->gamma ? TRUE : FALSE;
	break;

    case MA_PointerType:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Gfx_HWSpriteTypes, msg->opg_Storage);
	break;

    case MA_DriverName:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Gfx_DriverName, msg->opg_Storage);
	break;

    case MA_MemoryClock:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Gfx_MemoryClock, msg->opg_Storage);
	break;

    case MA_Windowed:
	OOP_GetAttr(data->handle->gfxhidd, aHidd_Gfx_IsWindowed, msg->opg_Storage);
	break;

    case MA_MonitorID:
        *msg->opg_Storage = data->handle->id;
        break;

    default:
	return DoSuperMethodA(cl, o, (Msg)msg);
    }

    return TRUE;
}

/***********************************************************************************/

IPTR MonitorClass__OM_SET(Class *cl, Object *o, struct opSet *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *UtilityBase = GetPrivIBase(IntuitionBase)->UtilityBase;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    struct IMonitorNode *data = INST_DATA(cl, o);
    struct TagItem  *tag, *tstate;

    tstate = msg->ops_AttrList;
    while((tag = NextTagItem(&tstate))) {
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
	    HIDD_Gfx_SetCursorVisible(data->handle->gfxhidd, tag->ti_Data);
	    break;
	}
    }
    return DoSuperMethodA(cl, o, (Msg)msg);
}

/***********************************************************************************/

#define Relink(nextAttr, prev, prevAttr, next)		\
    if (prev)						\
	SetAttrs(prev, nextAttr, next, TAG_DONE);	\
    if (next)						\
	SetAttrs(next, prevAttr, prev, TAG_DONE)

IPTR MonitorClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddGfxAttrBase = GetPrivIBase(IntuitionBase)->HiddGfxAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, o);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_ActiveCallBack, 0},
	{TAG_DONE                , 0}
    };

    D(kprintf("MonitorClass: OM_DISPOSE\n"));

    /* Disable activation callback */
    OOP_SetAttrs(data->handle->gfxhidd, tags);

    ObtainSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);
    Remove((struct Node *)data);

    /* Remove this monitor from spatial links */
    Relink(MA_BottomRightMonitor, data->topleft, MA_TopLeftMonitor, data->bottomright);
    Relink(MA_BottomMiddleMonitor, data->topmiddle, MA_TopMiddleMonitor, data->bottommiddle);
    Relink(MA_BottomLeftMonitor, data->topright, MA_TopRightMonitor, data->bottomleft);
    Relink(MA_MiddleLeftMonitor, data->middleright, MA_MiddleRightMonitor, data->middleleft);

    /* If an active monitor is being removed, we should activate another one */
    if (GetPrivIBase(IntuitionBase)->ActiveMonitor == o)
	ActivateMonitor((Object *)GetHead(&GetPrivIBase(IntuitionBase)->MonitorList), -1, -1, IntuitionBase);

    ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->MonitorListSem);

    if (data->gamma)
        FreeMem(data->gamma, 256 * 3);

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
	a taglist with BMATags_DisplayID specification to AllocBitMap() function.

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
	    MSQUERY3D_NODRIVER - There is no 3D support for the given pixelformat
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    OOP_Object *pf = data->pfobjects[msg->PixelFormat];

    if (pf) {
	if (HIDD_Gfx_QueryHardware3D(data->handle->gfxhidd, pf))
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

void MonitorClass__MM_GetDefaultGammaTables(Class *cl, Object *obj, struct msGetDefaultGammaTables *msg)
{
    struct IMonitorNode *data = INST_DATA(cl, obj);

    if (data->gamma)
    {
        if (msg->Red)
            CopyMem(&data->gamma[GAMMA_R], msg->Red, 256);
        if (msg->Green)
            CopyMem(&data->gamma[GAMMA_G], msg->Green, 256);
        if (msg->Blue)
            CopyMem(&data->gamma[GAMMA_B], msg->Blue, 256);
     }
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);

    return HIDD_Gfx_GetMaxSpriteSize(data->handle->gfxhidd, msg->PointerType, msg->Width, msg->Height);
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddGfxAttrBase = GetPrivIBase(IntuitionBase)->HiddGfxAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_DPMSLevel, vHidd_Gfx_DPMSLevel_Off},
	{TAG_DONE	    , 0			     }    
    };
    
    return OOP_SetAttrs(data->handle->gfxhidd, tags);
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_AttrBase HiddGfxAttrBase = GetPrivIBase(IntuitionBase)->HiddGfxAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_DPMSLevel, vHidd_Gfx_DPMSLevel_On},
	{TAG_DONE	    , 0			    }    
    };
    
    return OOP_SetAttrs(data->handle->gfxhidd, tags);
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
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct IMonitorNode *data = INST_DATA(cl, obj);

    if (data->gamma)
    {
        IPTR ret;

        if (msg->Red)
            CopyMem(msg->Red  , &data->gamma[GAMMA_R], 256);
        if (msg->Green)
            CopyMem(msg->Green, &data->gamma[GAMMA_G], 256);
        if (msg->Blue)
            CopyMem(msg->Blue , &data->gamma[GAMMA_B], 256);

        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->ViewLordLock);

        if (data->screenGamma == 3)
        {
            /*
             * All three components are determined by screen.
             * Do not disturb.
             */
            ret = TRUE;
        }
        else
        {
            /*
             * This monitor currently uses default gamma table.
             * Update it.
             */
            OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;

            ret = HIDD_Gfx_SetGamma(data->handle->gfxhidd, data->active_r, data->active_g, data->active_b);
        }

        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ViewLordLock);
        return ret;
    }

    return FALSE;
}

/************************************************************************************/

ULONG MonitorClass__MM_GetCompositionFlags(Class *cl, Object *obj, struct msGetCompositionFlags *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    struct HIDD_ModeProperties modeprops;

    HIDD_Gfx_ModeProperties(data->handle->gfxhidd, msg->ModeID & (!data->handle->mask),
			    &modeprops, sizeof(modeprops));
    return modeprops.CompositionFlags;
}

/************************************************************************************/

void MonitorClass__MM_SetPointerPos(Class *cl, Object *obj, struct msSetPointerPos *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct IMonitorNode *data = INST_DATA(cl, obj);

    data->mouseX = msg->x;
    data->mouseY = msg->y;
    SetPointerPos(data, IntuitionBase);
}

/************************************************************************************/

IPTR MonitorClass__MM_CheckID(Class *cl, Object *obj, struct msGetCompositionFlags *msg)
{
    struct IMonitorNode *data = INST_DATA(cl, obj);

    return ((msg->ModeID & data->handle->mask) == data->handle->id);
}

/************************************************************************************/

IPTR MonitorClass__MM_SetPointerShape(Class *cl, Object *obj, struct msSetPointerShape *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    struct BitMap *bm;
    BOOL res;

    D(bug("[monitorclass] SetPointerShape(0x%p), old pointer 0x%p\n", msg->pointer, data->pointer));
    /* Don't do anything if already set */
    if (data->pointer == msg->pointer)
	return TRUE;

    bm = msg->pointer->sprite->es_BitMap;
    /* Currently we don't work with non-hidd sprites */
    if (!IS_HIDD_BM(bm))
        return FALSE;

    res = HIDD_Gfx_SetCursorShape(data->handle->gfxhidd, HIDD_BM_OBJ(bm), msg->pointer->xoffset, msg->pointer->yoffset);
    D(bug("[monitorclass] SetPointerShape() returned %d\n", res));
    if (res) {
	data->pointer = msg->pointer;
	/* This will fix up sprite position if hotspot changed */
	SetPointerPos(data, IntuitionBase);
    }

    return res;
}

void MonitorClass__MM_SetScreenGamma(Class *cl, Object *obj, struct msSetScreenGamma *msg)
{
    struct IMonitorNode *data = INST_DATA(cl, obj);

    if (data->gamma)
    {
        struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
        OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
        struct GammaControl *gamma = msg->gamma;

        ObtainSemaphore(&GetPrivIBase(IntuitionBase)->ViewLordLock);

        if (gamma->Active)
        {
            UBYTE screengamma = 0;

            ResetGamma(data);
            if (gamma->UseGammaControl)
            {
                if (gamma->GammaTableR)
                {
                    data->active_r = gamma->GammaTableR;
                    screengamma++;
                }
                if (gamma->GammaTableB)
                {
                    data->active_b = gamma->GammaTableB;
                    screengamma++;
                }
                if (gamma->GammaTableG);
                {
                    data->active_g = gamma->GammaTableG;
                    screengamma++;
                }
            }

            /*
             * The condition here reflects the following variants:
             * 1. We used default gamma and switched to custom one.
             * 2. We used custom gamma and switched to default one.
             * 3. We changed custom gamma.
             */
            if (data->screenGamma || screengamma || msg->force)
            {
                data->screenGamma = screengamma;
                HIDD_Gfx_SetGamma(data->handle->gfxhidd, data->active_r, data->active_g, data->active_b);
            }
        }

        ReleaseSemaphore(&GetPrivIBase(IntuitionBase)->ViewLordLock);
    }
}

/*
 * Find the best (closest to the given) depth with 3D support.
 * This method will give the priority to hardware 3D.
 */
ULONG MonitorClass__MM_FindBest3dDepth(Class *cl, Object *obj, struct msFindBest3dDepth *msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    ULONG swdepth = -1;
    ULONG i;

    for (i = 0; i < MONITOR_MAXPIXELFORMATS; i++)
    {
	if (data->pfobjects[i])
        {
	    IPTR depth;

	    OOP_GetAttr(data->pfobjects[i], aHidd_PixFmt_Depth, &depth);

	    if (depth < msg->depth)
            {
                /* Skip all pixelformats with depth less than requested */
                continue;
            }

            if (HIDD_Gfx_QueryHardware3D(data->handle->gfxhidd, data->pfobjects[i]))
            {
                /* Found hardware 3D ? Good. */
                return depth;
            }

            if ((depth > 8) && (swdepth == -1))
            {
                /* This remembers the first depth suitable for software 3D */
                swdepth = depth;
            }
	}
    }

    return swdepth;
}

/*
 * Calculate 3D capability index.
 * Used for finding the best 3D monitor.
 */
ULONG MonitorClass__MM_Calc3dCapability(Class *cl, Object *obj, Msg msg)
{
    struct IntuitionBase *IntuitionBase = (struct IntuitionBase *)cl->cl_UserData;
    struct Library *OOPBase = GetPrivIBase(IntuitionBase)->OOPBase;
    OOP_MethodID HiddGfxBase = GetPrivIBase(IntuitionBase)->ib_HiddGfxBase;
    OOP_AttrBase HiddPixFmtAttrBase = GetPrivIBase(IntuitionBase)->HiddPixFmtAttrBase;
    struct IMonitorNode *data = INST_DATA(cl, obj);
    ULONG idx = 0;
    ULONG i;

    for (i = 0; i < MONITOR_MAXPIXELFORMATS; i++)
    {
	if (data->pfobjects[i])
        {
            if (HIDD_Gfx_QueryHardware3D(data->handle->gfxhidd, data->pfobjects[i]))
            {
                /*
                 * Each HW 3D mode scores 2.
                 * As a result, monitor with one HW 3D mode will beat SW-only 3D.
                 */
                idx += 2;
            }
            else if (OOP_GET(data->pfobjects[i], aHidd_PixFmt_Depth) > 8)
            {
                /* Any number of SW 3D modes scores 1 */
                idx |= 1;
            }
	}
    }

    return idx;
}
