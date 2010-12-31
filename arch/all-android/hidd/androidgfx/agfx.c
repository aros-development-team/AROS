/*
    Copyright  1995-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Android-hosted graphics driver class.
    Lang: English.
*/

#define DEBUG 1

#define __OOP_NOATTRBASES__

#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <graphics/displayinfo.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "agfx.h"
#include "agfx_graphics.h"

OOP_Object *AGFXCl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    jobject display;

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

    struct TagItem sync_tags[] =
    {
    	{ aHidd_Sync_HDisp  	, 160 	    	    	 },
	{ aHidd_Sync_VDisp  	, 160 	    	    	 },
	{ TAG_DONE  	    	, 0UL 	    	    	 }
    };

    struct TagItem mode_tags[] =
    {
	{ aHidd_Gfx_PixFmtTags	, (IPTR)pftags		},
	{ aHidd_Sync_HMin	, 112			}, /* In fact these can be even smaller, and */
	{ aHidd_Sync_VMin	, 112			}, /* maximum can be even bigger...	     */
	{ aHidd_Sync_HMax	, 16384			},
	{ aHidd_Sync_VMax	, 16384			},
	{ aHidd_Sync_Description, (IPTR)"Android: %hx%v"},
	{ aHidd_Gfx_SyncTags	, (IPTR)sync_tags	},
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

    HostLib_Lock();

    /* Get Java display object */
    display = JNI_CallObjectMethod(XSD(cl)->jobj, XSD(cl)->GetDisplay_mID);
    /* Get display size */
    sync_tags[0].ti_Data = JNI_GetIntField(display, XSD(cl)->Width_aID);
    sync_tags[1].ti_Data = JNI_GetIntField(display, XSD(cl)->Height_aID);

    HostLib_Unlock();

    D(bug("[AGFX] Display size: %ux%u\n", sync_tags[0].ti_Data, sync_tags[1].ti_Data));

    /* Register gfxmodes */
    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&mymsg);
    if (o)
    {
	struct gfx_data *data = OOP_INST_DATA(cl, o);

	D(bug("AGFX::New(): Got object from super\n"));

	data->width  = sync_tags[0].ti_Data;
	data->height = sync_tags[1].ti_Data;
    }
    ReturnPtr("AGFXGfx::New", OOP_Object *, o);
}

/****************************************************************************************/

OOP_Object *AGFXCl__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    HIDDT_ModeID modeid;
    struct pHidd_Gfx_NewBitMap p;
    OOP_Object *newbm;
    struct gfx_data *data = OOP_INST_DATA(cl, o);;
    struct TagItem tags[] =
    {
	{TAG_IGNORE, 0			},
	{TAG_MORE  , (IPTR)msg->attrList}
    };

    EnterFunc(bug("AGFX::NewBitMap()\n"));

    /*
     * Having a valid ModeID means that we are asked to create
     * either displayable bitmap or a friend of a displayable bitmap.
     * Create our bitmap only if we have a valid ModeID
     */
    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
    if (modeid != vHidd_ModeID_Invalid)
    {
        tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
	tags[0].ti_Data = (IPTR)XSD(cl)->bmclass;
    }

    p.mID = msg->mID;
    p.attrList = tags;
    newbm = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);

    ReturnPtr("AGFX::NewBitMap", OOP_Object *, newbm);
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
    /* TODO */
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

static const char *host_vars[] = {
    "Java_Env",
    "Java_Class",
    "Java_Object",
    NULL
};

static const STRPTR interfaces[] = {
    IID_Hidd_BitMap,
    IID_Hidd_Sync,
    IID_Hidd_PixFmt,
    IID_Hidd_Gfx,
    IID_Hidd,
    NULL
};

#undef XSD
#define XSD(cl) (&agfxBase->xsd)

static int agfx_init(struct AGFXBase *agfxBase)
{
    struct HostInterface *tmpif;
    ULONG r;
    jclass MainClass = NULL;
    jclass DisplayClass;

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
	return FALSE;

    agfxBase->HostLibHandle = HostLib_Open("libAROSBootstrap.so", NULL);
    if (!agfxBase->HostLibHandle)
	return FALSE;

    tmpif = (struct HostInterface *)HostLib_GetInterface(agfxBase->HostLibHandle, host_vars, &r);
    if (!tmpif)
	return FALSE;

    /*
     * Our interface contains pointers to variables, buf we need values.
     * So we cache them and then just drop the interface.
     */
    if (!r)
    {
	agfxBase->xsd.jni  = *tmpif->jni;
	agfxBase->xsd.jobj = *tmpif->obj;
	MainClass = *tmpif->cl;
    }
    HostLib_DropInterface((APTR *)tmpif);
    if (r)
	return FALSE;

    agfxBase->xsd.AttrBases = AllocAttrBases(interfaces);
    if (!agfxBase->xsd.AttrBases)
	return FALSE;
	
    D(bug("[AGFX] Obtaining Java IDs...\n"));

    HostLib_Lock();

    /* Find DisplayView class */
    DisplayClass = JNI_FindClass("org/aros/bootstrap/DisplayView");
    D(bug("[AGFX] DisplayView class 0x%p\n", DisplayClass));
    if (!DisplayClass)
	return FALSE;

    /*
     * Cache method and property IDs.
     * We don't check for errors here because these functions throw exceptions
     * (read: abort) when they fail. Thanks Sun! :(
     */
    agfxBase->xsd.GetDisplay_mID = JNI_GetMethodID(MainClass, "GetDisplay", "()Lorg/aros/bootstrap/DisplayView;");
    agfxBase->xsd.Width_aID      = JNI_GetFieldID(DisplayClass, "Width", "I");
    agfxBase->xsd.Height_aID     = JNI_GetFieldID(DisplayClass, "Height", "I");

    HostLib_Unlock();

    D(bug("[AGFX] Init OK\n"));
    return TRUE;
}

/****************************************************************************************/

static int agfx_expunge(struct AGFXBase *agfxBase)
{
    D(bug("[AGFX] Expunge\n"));

    if (!HostLibBase)
	return TRUE;

    if (agfxBase->xsd.AttrBases)
	FreeAttrBases(interfaces, agfxBase->xsd.AttrBases);

    if (agfxBase->HostLibHandle)
	HostLib_Close(agfxBase->HostLibHandle, NULL);

    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(agfx_init, 0);
ADD2EXPUNGELIB(agfx_expunge, 0);
