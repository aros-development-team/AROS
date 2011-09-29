/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Cocoa Touch display HIDD for AROS
    Lang: English.
*/

#define DEBUG 1

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/displayinfo.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <oop/oop.h>

#include "classbase.h"
#include "gfxclass.h"

/****************************************************************************************/

OOP_AttrBase HiddChunkyBMAttrBase;
OOP_AttrBase HiddBitMapAttrBase;  
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_ChunkyBM , &HiddChunkyBMAttrBase },
    { IID_Hidd_BitMap	, &HiddBitMapAttrBase	},
    { IID_Hidd_Sync 	, &HiddSyncAttrBase	},
    { IID_Hidd_PixFmt	, &HiddPixFmtAttrBase	},
    { IID_Hidd_Gfx  	, &HiddGfxAttrBase	},
    { IID_Hidd		, &HiddAttrBase		},
    { NULL  	    	, NULL      	    	}
};

/****************************************************************************************/

OOP_Object *UIKit__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem pftags[] =
    {
        /* Shifts are non-obviously calculated from the MSB, not from the LSB.
           I. e. color value is placed in the most significant byte of the ULONG
           before shifting (cc000000, not 000000cc) */
    	{ aHidd_PixFmt_RedShift     , 24			   },
	{ aHidd_PixFmt_GreenShift   , 16			   },
	{ aHidd_PixFmt_BlueShift    , 8				   },
	{ aHidd_PixFmt_AlphaShift   , 0				   },
	{ aHidd_PixFmt_RedMask	    , 0x000000FF		   },
	{ aHidd_PixFmt_GreenMask    , 0x0000FF00		   },
	{ aHidd_PixFmt_BlueMask     , 0x00FF0000		   },
	{ aHidd_PixFmt_AlphaMask    , 0x00000000		   },
	{ aHidd_PixFmt_ColorModel   , vHidd_ColorModel_TrueColor   },
	{ aHidd_PixFmt_Depth	    , 24			   },
	{ aHidd_PixFmt_BytesPerPixel, 4				   },
	{ aHidd_PixFmt_BitsPerPixel , 24			   },
	{ aHidd_PixFmt_StdPixFmt    , vHidd_StdPixFmt_Native	   },
	{ aHidd_PixFmt_BitMapType   , vHidd_BitMapType_Chunky      },
	{ TAG_DONE  	    	    , 0UL			   } 
    };

    struct TagItem p_synctags[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 },
	{ aHidd_Sync_VDisp  	, 160 },
	{ aHidd_Sync_Description, (IPTR)"iOS: %hx%v Portrait"},
	{ TAG_DONE  	    	, 0   }
    };

    struct TagItem l_synctags[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 },
	{ aHidd_Sync_VDisp  	, 160 },
	{ aHidd_Sync_Description, (IPTR)"iOS: %hx%v Landscape"},
	{ TAG_DONE  	    	, 0UL } 
    };
    
    struct TagItem mode_tags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags		},
	{ aHidd_Sync_HMin	, 112			}, /* In fact these can be even smaller, and */
	{ aHidd_Sync_VMin	, 112			}, /* maximum can be even bigger...	     */
	{ aHidd_Sync_HMax	, 16384			},
	{ aHidd_Sync_VMax	, 16384			},
	{ aHidd_Gfx_SyncTags	, (IPTR)p_synctags	},
	{ aHidd_Gfx_SyncTags	, (IPTR)l_synctags	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem mytags[] =
    {
	{ aHidd_Gfx_ModeTags	, (IPTR)mode_tags	 },
	{ aHidd_Name		, (IPTR)"UIKit"		 },
	{ aHidd_HardwareName	, (IPTR)"iOS Cocoa Touch"},
	{ aHidd_ProducerName	, (IPTR)"Apple Corp."    },
	{ TAG_MORE  	    	, (IPTR)msg->attrList 	 }
    };
    struct pRoot_New mymsg = { msg->mID, mytags };
    APTR display;
    struct UIKitBase *base = cl->UserData;

    EnterFunc(bug("UIKitGfx::New()\n"));

    /* Create a display window. TODO: Support multuple displays. */
    HostLib_Lock();
    display = base->iface->OpenDisplay(0);
    AROS_HOST_BARRIER
    HostLib_Unlock();

    if (!display)
    {
    	D(bug("[UIKitGfx] Failed to create display window\n"));
    	return NULL;
    }

    if (base->metrics.orientation == O_LANDSCAPE)
    {
    	l_synctags[0].ti_Data = base->metrics.width;
    	l_synctags[1].ti_Data = base->metrics.height - base->metrics.screenbar;

    	p_synctags[0].ti_Data = base->metrics.height;
    	p_synctags[1].ti_Data = base->metrics.width - base->metrics.screenbar;
    }
    else
    {
    	p_synctags[0].ti_Data = base->metrics.width;
    	p_synctags[1].ti_Data = base->metrics.height - base->metrics.screenbar;

    	l_synctags[0].ti_Data = base->metrics.height;
    	l_synctags[1].ti_Data = base->metrics.width - base->metrics.screenbar;
    }

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (NULL != o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);

	D(bug("UIKitGfx::New(): Got object from super\n"));
	data->display = display;
    }
    ReturnPtr("UIKitGfx::New", OOP_Object *, o);
}

/********** GfxHidd::Dispose()  ******************************/
VOID UIKit__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct UIKitBase *base = cl->UserData;
    struct gfx_data *data  = OOP_INST_DATA(cl, o);

    HostLib_Lock();
    base->iface->CloseDisplay(data->display);
    AROS_HOST_BARRIER
    HostLib_Unlock();

    D(bug("UIKitGfx::Dispose: calling super\n"));    
    OOP_DoSuperMethod(cl, o, msg);
}

/****************************************************************************************/

OOP_Object *UIKit__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    struct UIKitBase *base = cl->UserData;
    HIDDT_ModeID modeid;
    struct pHidd_Gfx_NewBitMap p;
    struct TagItem tags[] =
    {
	{TAG_IGNORE, 0			},
	{TAG_MORE  , (IPTR)msg->attrList}
    };

    /* Here we select a class for the bitmap to create */
    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    if (modeid != vHidd_ModeID_Invalid)
    {
        tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
        tags[0].ti_Data = (IPTR)base->bmclass;

	D(bug("[UIKitGfx] ModeID: 0x%08lX, ClassPtr: 0x%p\n", modeid, tags[0].ti_Data));
    }

    p.mID = msg->mID;
    p.attrList = tags;

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &p.mID);
}

/****************************************************************************************/

VOID UIKit__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG   	     idx;
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    case aoHidd_Gfx_NoFrameBuffer:
	    	*msg->storage = TRUE;
		return;

	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)"CocoaTouch";
		return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

ULONG UIKit__Hidd_Gfx__ShowViewPorts(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ShowViewPorts *msg)
{
    /* TODO */
    return TRUE;
}

/****************************************************************************************/

VOID UIKit__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    /* TODO */
}

/****************************************************************************************/

/* This is simple - all modes have the same properties */
static struct HIDD_ModeProperties mode_props =
{
    0,	/* Ooops, we have no sprites */
    0,
    COMPF_ABOVE|COMPF_BELOW|COMPF_LEFT|COMPF_RIGHT
};

ULONG UIKit__Hidd_Gfx__ModeProperties(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_ModeProperties *msg)
{
    ULONG len = msg->propsLen;

    if (len > sizeof(mode_props))
        len = sizeof(mode_props);
    CopyMem(&mode_props, msg->props, len);

    return len;
}

/****************************************************************************************/

static const char *symbols[] =
{
    "GetMetrics",
    "OpenDisplay",
    "CloseDisplay",
    "NewBitMap",
    "DisposeBitMap",
    NULL
};

static int UIKit_Init(struct UIKitBase *base) 
{
    if (!OOP_ObtainAttrBases(attrbases))
    	return FALSE;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
    	return FALSE;

    base->hostlib = HostLib_Open("Libs/Host/uikit_hidd.dylib", NULL);
    if (!base->hostlib)
    	return FALSE;

    base->iface = (struct UIKitInterface *)HostLib_GetInterface(base->hostlib, symbols, NULL);
    if (!base->iface)
    	return FALSE;

    D(bug("[UIKit] Native library loaded succesfully\n"));

    HostLib_Lock();

    /* Cache screen metrics. Status bar is always the same anyway. */
    base->iface->GetMetrics(&base->metrics);
    AROS_HOST_BARRIER

    HostLib_Unlock();

    D(bug("[UIKit] Display %u x %u points, screenbar size %u\n", base->metrics.width, base->metrics.height, base->metrics.screenbar));

    return TRUE;
}

/****************************************************************************************/

static int UIKit_Expunge(struct UIKitBase *base)
{
    OOP_ReleaseAttrBases(attrbases);

    if (!HostLibBase)
    	return TRUE;

    if (base->iface)
	HostLib_DropInterface((void **)base->iface);
    
    if (base->hostlib)
    	HostLib_Close(base->hostlib, NULL);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(UIKit_Init, 0);
ADD2EXPUNGELIB(UIKit_Expunge, 0);
