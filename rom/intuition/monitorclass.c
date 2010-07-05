/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
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

#include "intuition_intern.h"
#include "monitorclass_intern.h"

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))
#define HiddAttrBase	(GetPrivIBase(IntuitionBase)->HiddAttrBase)
#define HiddGfxAttrBase (GetPrivIBase(IntuitionBase)->HiddGfxAttrBase)

/***********************************************************************************/

Object *MonitorClass__OM_NEW(Class *cl, Object *o, struct opSet *msg)
{
    OOP_Object *driver = (OOP_Object *)GetTagData(MA_DriverObject, 0, msg->ops_AttrList);
    struct MonitorData *data;

    D(kprintf("[monitorclass] OM_NEW\n"));

    if (!driver)
	return NULL;

    o = (Object *)DoSuperMethodA(cl, o, (Msg)msg);
    if (!o)
	return NULL;

    data = INST_DATA(cl, o);

    data->driver = driver;

    /* TODO: Fill in data->pixelformats */

    return o;
}

/***********************************************************************************/

IPTR MonitorClass__OM_GET(Class *cl, Object *o, struct opGet *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);

    D(kprintf("[monitorclass] OM_GET\n"));

    switch (msg->opg_AttrID)
    {
    case MA_MonitorName:
	OOP_GetAttr(data->driver, aHidd_Name, msg->opg_Storage);
	break;

    case MA_Manufacturer:
	OOP_GetAttr(data->driver, aHidd_Producer, msg->opg_Storage);
	break;

    case MA_ManufacturerID:
	/* TODO: PCI ID ??? */
	*msg->opg_Storage = 0;
	break;

    case MA_ProductID:
	/* TODO: PCI ID ??? */
	*msg->opg_Storage = 0;
	break;

    case MA_MemorySize:
	/* TODO: Add HIDD attribute */
	*msg->opg_Storage = 0;
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
	/* FIXME: Perhaps drivers should really specify this ? */
	*msg->opg_Storage = PointerType_3Plus1|PointerType_ARGB;
	break;

    case MA_DriverName:
	*msg->opg_Storage = (IPTR)(OOP_OCLASS(data->driver)->ClassNode.ln_Name);
	break;

    case MA_MemoryClock:
	/* TODO: Add HIDD attribute */
	*msg->opg_Storage = 0;
	break;

    case MA_DriverObject:
	*msg->opg_Storage = (IPTR)data->driver;
	break;

    default:
	return DoSuperMethodA(cl, o, (Msg)msg);
    }

    return TRUE;
}

/***********************************************************************************/

IPTR MonitorClass__OM_DISPOSE(Class *cl, Object *o, Msg msg)
{   
    D(kprintf("MonitorClass: OM_DISPOSE\n"));

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
	BMF_SCREEN flag togetger with display mode ID to AllocBitMap() function.

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

        DoMethodA(Object *obj, msQuery3DSupport *msg);

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
	At the moment this method is not implemented and always returns
	MSQUERY3D_UNKNOWN

    SEE ALSO
	MM_EnterPowerSaveMode, MM_ExitBlanker

    INTERNALS

************************************************************************************/

IPTR MonitorClass__MM_Query3DSupport(Class *cl, Object *obj, struct msQuery3DSupport *msg)
{
    /* TODO: Add HIDD attribute or something ??? */
    *msg->Store = MSQUERY3D_UNKNOWN;

    return *msg->Store;
}

/************************************************************************************

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

************************************************************************************/

IPTR MonitorClass__MM_GetDefaultPixelFormat(Class *cl, Object *obj, struct msGetDefaultPixelFormat *msg)
{
    /* TODO: Implement this, perhaps again need HIDD API extension */
    *msg->Store = -1;

    return *msg->Store;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_GetPointerBounds(Class *cl, Object *obj, struct msGetPointerBounds *msg)
{
    /* TODO: We really should add some HIDD attributes for this */
    *msg->Width  = 64;
    *msg->Height = 64;

    return TRUE;
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
