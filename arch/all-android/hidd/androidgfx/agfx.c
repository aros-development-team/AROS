/*
    Copyright © 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android-hosted graphics driver class.
    Lang: English.
*/

#define DEBUG 1

#define __OOP_NOATTRBASES__

#include <sys/types.h>
#include <android/configuration.h>
#include <fcntl.h>

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/displayinfo.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <hidd/keyboard.h>
#include <hidd/mouse.h>
#include <hidd/unixio.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include LC_LIBDEFS_FILE

#include "server.h"

OOP_Object *AGFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct QueryRequest query;

    struct TagItem pftags[] =
    {
        /*
	 * Shifts are non-obviously calculated from the MSB, not from the LSB.
         * I. e. color value is placed in the most significant byte of the ULONG
         * before shifting (cc000000, not 000000cc)
	 */
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
	{ TAG_DONE  	    	    , 0				   }
    };

    struct TagItem p_sync_tags[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 	    	    	 },
	{ aHidd_Sync_VDisp  	, 160 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Android: %hx%v Portrait"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem l_sync_tags[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 	    	    	 },
	{ aHidd_Sync_VDisp  	, 160 	    	    	 },
	{ aHidd_Sync_Description, (IPTR)"Android: %hx%v Landscape"},
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem mode_tags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags		},
#ifdef ENABLE_SCROLL
	{ aHidd_Sync_HMax	, 16384			},
	{ aHidd_Sync_VMax	, 16384			},
#endif
	{ aHidd_Gfx_SyncTags	, (IPTR)p_sync_tags	},
	{ aHidd_Gfx_SyncTags	, (IPTR)l_sync_tags	},
	{ TAG_DONE  	    	, 0UL 	    	    	}
    };

    struct TagItem mytags[] =
    {
	{ aHidd_Gfx_ModeTags	, (IPTR)mode_tags	 },
	{ aHidd_Name		, (IPTR)"android.monitor"},
	{ aHidd_HardwareName	, (IPTR)"Android OS"	 },
	{ aHidd_ProducerName	, (IPTR)"Google inc."	 },
	{ TAG_MORE  	    	, (IPTR)msg->attrList 	 }
    };
    struct pRoot_New mymsg = { msg->mID, mytags };

    EnterFunc(bug("AGFX::New()\n"));

    /* Get display size */
    query.req.cmd = cmd_Query;
    query.req.len = 1;
    query.id      = 0;
    DoRequest(&query.req, XSD(cl));

    if (query.req.status != STATUS_ACK)
    {
    	D(bug("[AGFX] Display server communication error\n"));
    	return NULL;
    }

    D(bug("[AGFX] Display size: %ux%u, Titlebar size: %u, Orientation: %u\n", query.width, query.height, query.titlebar, query.orientation));

    if (query.orientation == ACONFIGURATION_ORIENTATION_PORT)
    {
    	p_sync_tags[0].ti_Data = query.width;
    	p_sync_tags[1].ti_Data = query.height - query.titlebar;
    	l_sync_tags[0].ti_Data = query.height;
    	l_sync_tags[1].ti_Data = query.width - query.titlebar;
    }
    else
    {
    	l_sync_tags[0].ti_Data = query.width;
    	l_sync_tags[1].ti_Data = query.height - query.titlebar;
    	p_sync_tags[0].ti_Data = query.height;
    	p_sync_tags[1].ti_Data = query.width - query.titlebar;
    }

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);

	D(bug("AGFX::New(): Got object from super\n"));

	data->width  = query.width;
	data->height = query.height;
    }
    ReturnPtr("AGFXGfx::New", OOP_Object *, o);
}

/****************************************************************************************/

OOP_Object *AGFXCl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL displayable;
    struct pHidd_Gfx_NewBitMap p;
    struct TagItem tags[] =
    {
	{TAG_IGNORE, 0			},
	{TAG_MORE  , (IPTR)msg->attrList}
    };

    /* Here we select a class for the bitmap to create */

    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    if (displayable)
    {
    	/* Displayable bitmaps are bitmaps of our class */
        tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
    }
    else
    {
	/* Non-displayable friends of our bitmaps are plain chunky bitmaps */
    	OOP_Object *friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);

    	if (friend && (OOP_OCLASS(friend) == XSD(cl)->bmclass))
    	{
    	    tags[0].ti_Tag  = aHidd_BitMap_ClassID;
    	    tags[0].ti_Data = (IPTR)CLID_Hidd_ChunkyBM;
    	}
    }

    /* The base class will take care about other cases */

    p.mID = msg->mID;
    p.attrList = tags;
    
    return (OOP_Object *)OOP_DoSuperMethod(cl, o, &p.mID);
}

/****************************************************************************************/

VOID AGFXCl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
    	switch (idx)
	{
	    case aoHidd_Gfx_IsWindowed:
	    case aoHidd_Gfx_NoFrameBuffer:
	    	*msg->storage = TRUE;
		return;

	    case aoHidd_Gfx_DriverName:
		*msg->storage = (IPTR)"Android";
		return;
	}
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/****************************************************************************************/

OOP_Object *AGFXCl__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct gfx_data *data = OOP_INST_DATA(cl, o);

    D(bug("[AGFX] Show(0x%p)\n", msg->bitMap));

    if (data->bitmap)
    	OOP_SetAttrsTags(data->bitmap, aHidd_BitMap_Visible, FALSE, TAG_DONE);

    if (msg->bitMap)
    	OOP_SetAttrsTags(msg->bitMap, aHidd_BitMap_Visible, TRUE, TAG_DONE);
    else
    {
    	/* addr == NULL will clear the screen. Offset and size are ignored. */
	struct ShowRequest show;

	show.req.cmd     = cmd_Show;
	show.req.len     = 8;
	show.displayid   = 0;
	show.orientation = ACONFIGURATION_ORIENTATION_ANY;	/* Do not change the orientation */
	show.addr        = 0;

	DoRequest(&show.req, XSD(cl));
    }

    data->bitmap = msg->bitMap;
    return msg->bitMap;
}

/****************************************************************************************/

/* The following two functions are candidates for inclusion into oop.library */
static void FreeAttrBases(const STRPTR *iftable, OOP_AttrBase *bases)
{
    ULONG i;
    
    for (i = 0; iftable[i]; i++)
    {
	if (bases[i])
	    OOP_ReleaseAttrBase(iftable[i]);
    }

    FreeVec(bases);
}

static OOP_AttrBase *AllocAttrBases(const STRPTR *iftable)
{
    ULONG cnt, i;
    OOP_AttrBase *ret;
    
    for (cnt = 0; iftable[cnt]; cnt++);

    ret = AllocVec(cnt * sizeof(OOP_AttrBase), MEMF_CLEAR);
    if (ret)
    {
	for (i = 0; i < cnt; i++)
	{
	    ret[i] = OOP_ObtainAttrBase(iftable[i]);
	    if (!ret[i])
	    {
		FreeAttrBases(iftable, ret);
		return NULL;
	    }
	}
    }

    return ret;
}

static const STRPTR interfaces[] =
{
    IID_Hidd_ChunkyBM,
    IID_Hidd_BitMap,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_Gfx,
    IID_Hidd_Kbd,
    IID_Hidd_Mouse,
    IID_Hidd,
    NULL
};

static int GetPipe(const char *name, APTR lib, APTR HostLibBase)
{
    int *ptr = HostLib_GetPointer(lib, name, NULL);
    
    if (!ptr)
    {
    	D(bug("[AGFX] Failed to locate symbol %s\n", name));
    	return -1;
    }
    
    return *ptr;
}

#undef XSD
#define XSD(cl) (&agfxBase->xsd)

static int agfx_init(struct AGFXBase *agfxBase)
{
    APTR HostLibBase;
    APTR HostLibHandle;
    int res;
    struct HelloRequest hello;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
	return FALSE;

    HostLibHandle = HostLib_Open("libAROSBootstrap.so", NULL);
    if (!HostLibHandle)
	return FALSE;

    agfxBase->xsd.DisplayPipe = GetPipe("DisplayPipe", HostLibHandle, HostLibBase);
    agfxBase->xsd.InputPipe   = GetPipe("InputPipe", HostLibHandle, HostLibBase);

    D(bug("[AGFX] DisplayPipe %d InputPipe %d\n", agfxBase->xsd.DisplayPipe, agfxBase->xsd.InputPipe));

    HostLib_Close(HostLibHandle, NULL);

    if ((agfxBase->xsd.DisplayPipe == -1) || (agfxBase->xsd.InputPipe == -1))
    	return FALSE;

    agfxBase->xsd.AttrBases = AllocAttrBases(interfaces);
    if (!agfxBase->xsd.AttrBases)
	return FALSE;

    agfxBase->xsd.unixio = OOP_NewObject(NULL, CLID_Hidd_UnixIO, NULL);
    if (!agfxBase->xsd.unixio)
    	return FALSE;

    NEWLIST(&agfxBase->xsd.waitQueue);

    agfxBase->xsd.serverInt.fd          = agfxBase->xsd.InputPipe;
    agfxBase->xsd.serverInt.mode	= vHidd_UnixIO_Read;
    agfxBase->xsd.serverInt.handler     = agfxInt;
    agfxBase->xsd.serverInt.handlerData = &agfxBase->xsd;

    res = Hidd_UnixIO_AddInterrupt(agfxBase->xsd.unixio, &agfxBase->xsd.serverInt);
    if (res)
    {
	Hidd_UnixIO_CloseFile(agfxBase->xsd.unixio, agfxBase->xsd.DisplayPipe, NULL);
    	Hidd_UnixIO_CloseFile(agfxBase->xsd.unixio, agfxBase->xsd.InputPipe, NULL);

	/* We don't need to dispose a unixio v42 object, it's a singletone. */
	agfxBase->xsd.unixio = NULL;
    	return FALSE;
    }

    /* Say hello to our display server */
    hello.req.cmd = cmd_Hello;
    hello.req.len = 1;
    hello.version = PROTOCOL_VERSION;
    DoRequest(&hello.req, &agfxBase->xsd);

    if (hello.req.status != STATUS_ACK)
    {
    	D(bug("[AGFX] Display server version mismatch\n"));
    	return FALSE;
    }

    D(bug("[AGFX] Init OK\n"));

    return TRUE;
}

/****************************************************************************************/

static int agfx_expunge(struct AGFXBase *agfxBase)
{
    D(bug("[AGFX] Expunge\n"));

    if (agfxBase->xsd.unixio)
    {
	Hidd_UnixIO_RemInterrupt(agfxBase->xsd.unixio, &agfxBase->xsd.serverInt);

    	if (agfxBase->xsd.DisplayPipe != -1)
	    Hidd_UnixIO_CloseFile(agfxBase->xsd.unixio, agfxBase->xsd.DisplayPipe, NULL);

	if (agfxBase->xsd.InputPipe != -1)
    	    Hidd_UnixIO_CloseFile(agfxBase->xsd.unixio, agfxBase->xsd.InputPipe, NULL);
    }

    if (agfxBase->xsd.AttrBases)
	FreeAttrBases(interfaces, agfxBase->xsd.AttrBases);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(agfx_init, 0);
ADD2EXPUNGELIB(agfx_expunge, 0);
ADD2LIBS("unixio.hidd", 42, static struct Library *, unixioBase);
