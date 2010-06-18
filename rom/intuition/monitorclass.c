/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/debug.h>
#include <hidd/graphics.h>
#include <hidd/hidd.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/monitorclass.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "intuition_intern.h"
#include "monitorclass_intern.h"

/***********************************************************************************/

#undef IntuitionBase
#define IntuitionBase   ((struct IntuitionBase *)(cl->cl_UserData))

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
	*msg->opg_Storage = data->pixelformats;
	break;

    case MA_TopLeftMonitor:
	*msg->opg_Storage = data->topleft;
	break;

    case MA_TopMiddleMonitor:
	*msg->opg_Storage = data->topmiddle;
	break;

    case MA_TopRightMonitor:
	*msg->opg_Storage = data->topright;
	break;

    case MA_MiddleLeftMonitor:
	*msg->opg_Storage = data->middleleft;
	break;

    case MA_MiddleRightMonitor:
	*msg->opg_Storage = data->middleright;
	break;

    case MA_BottomLeftMonitor:
	*msg->opg_Storage = data->bottomleft;
	break;

    case MA_BottomMiddleMonitor:
	*msg->opg_Storage = data->bottommiddle;
	break;

    case MA_BottomRightMonitor:
	*msg->opg_Storage = data->bottomright;
	break;

    case MA_GammaControl:
	*msg->opg_Storage = HIDD_Gfx_GetGamma(data->driver, NULL, NULL, NULL);
	break;

    case MA_PointerType:
	/* FIXME: Perhaps drivers should really specify this ? */
	*msg->opg_Storage = PointerType_3Plus1|PointerType_ARGB;
	break;

    case MA_DriverName:
	*msg->opg_Storage = OOP_OCLASS(data->driver)->ClassNode.ln_Name;
	break;

    case MA_MemoryClock:
	/* TODO: Add HIDD attribute */
	*msg->opg_Storage = 0;
	break;

    case MA_DriverObject:
	*msg->opg_Storage = data->driver;
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

************************************************************************************/

IPTR MonitorClass__MM_GetRootBitMap(Class *cl, Object *obj, struct msGetRootBitMap *msg)
{
    msg->Store = NULL;

    return 0;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_Query3DSupport(Class *cl, Object *obj, struct msQuery3DSupport *msg)
{
    /* TODO: Add HIDD attribute or something ??? */
    msg->Store = MSQUERY3D_UNKNOWN;

    return msg->Store;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_GetDefaultGammaTables(Class *cl, Object *obj, struct msGetDefaultGammaTables *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);

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
    msg->Store = -1;

    return msg->Store;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_GetPointerBounds(Class *cl, Object *obj, struct msGetPointerBounds *msg)
{
    /* TODO: We really should add some HIDD attributes for this */
    msg->Width  = 64;
    msg->Height = 64;

    return TRUE;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_RunBlanker(Class *cl, Object *obj, Msg *msg)
{
    /* We have no integrated screensaver support */

    return FALSE;
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_EnterPowerSaveMode(Class *cl, Object *obj, Msg *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);
    struct TagItem tags[] =
    {
	{aHidd_Gfx_DPMSLevel, vHidd_Gfx_DPMSLevel_Off},
	{TAG_DONE	    , 0			     }    
    };
    
    return OOP_SetAttrs(data->driver, tags);
}

/************************************************************************************

************************************************************************************/

IPTR MonitorClass__MM_ExitBlanker(Class *cl, Object *obj, Msg *msg)
{
    struct MonitorData *data = INST_DATA(cl, o);
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
    struct MonitorData *data = INST_DATA(cl, o);

    /* Currently we don't use per-screen gamma tables, so we just forward the request
       to the driver.
       If we implement per-screen gamma correction, we'll need more sophisticated
       management here */
    return HIDD_Gfx_SetGamma(data->driver, msg->Red, msg->Green, msg->Blue);
}
