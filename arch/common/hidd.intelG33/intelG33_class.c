/*
    Copyright � 2009-2010, The AROS Development Team. All rights reserved.
    $Id$

    Desc: intelG33_class.c
    Lang: English
*/



#define DEBUG 1
#include <aros/debug.h>
#include <aros/libcall.h>
#include <aros/asmcall.h>
#include <aros/symbolsets.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include LC_LIBDEFS_FILE

#include "intelG33_intern.h"
#include "intelG33_regs.h"

#define BASE(lib) ((struct IntelG33Base*)(lib))
#define SD(cl) (&BASE(cl->UserData)->sd)
#define sd ((struct staticdata*)SD(cl))

#define METHOD(base, id, name) \
  base ## __ ## id ## __ ## name (OOP_Class *cl, OOP_Object *o, struct p ## id ## _ ## name *msg)

#define METHOD_NAME(base, id, name) \
  base ## __ ## id ## __ ## name

#define METHOD_NAME_S(base, id, name) \
  # base "__" # id "__" # name

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)	\
    struct TagItem sync_ ## name[]={ \
        { aHidd_Sync_PixelClock,  clock*1000 }, \
        { aHidd_Sync_HDisp,       hdisp }, \
        { aHidd_Sync_HSyncStart,  hstart }, \
        { aHidd_Sync_HSyncEnd,    hend }, \
        { aHidd_Sync_HTotal,      htotal }, \
        { aHidd_Sync_VDisp,       vdisp }, \
        { aHidd_Sync_VSyncStart,  vstart }, \
        { aHidd_Sync_VSyncEnd,    vend }, \
        { aHidd_Sync_VTotal,      vtotal }, \
        { aHidd_Sync_Description, (IPTR)descr }, \
        { TAG_DONE, 0UL }}


OOP_Object *METHOD(IntelG33, Root, New) {
    D(bug("[G33] Root New\n"));

/* TODO: Calculate timings for allowed screen modes based on (E-)EDID information */

    MAKE_SYNC(1024x768_60, 65000,
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
        "IntelG33:1024x768");
 
    struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,	8	}, /* 0 */
        { aHidd_PixFmt_GreenShift,	16	}, /* 1 */
        { aHidd_PixFmt_BlueShift,  	24	}, /* 2 */
        { aHidd_PixFmt_AlphaShift,	0	}, /* 3 */
        { aHidd_PixFmt_RedMask,		0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,	0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,	0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,	0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,	vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,		24	}, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,	4	}, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,	24	}, /* 11 */
        { aHidd_PixFmt_StdPixFmt,	vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,	vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem modetags[] = {
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags_24bpp  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1024x768_60  },
        { TAG_DONE, 0UL }
    };
  
    struct TagItem mytags[] = {
        { aHidd_Gfx_ModeTags, (IPTR)modetags },
        { TAG_MORE, (IPTR)msg->attrList }
    };

    struct pRoot_New mymsg;

    mymsg.mID = msg->mID;
    mymsg.attrList = mytags;

    msg = &mymsg;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    EnterFunc(bug("[G33]   New()=%08x\n",o));
    if (o) {
        sd->IntelG33Object = o;
    }

    return o;

}

/*
  aoHidd_Gfx_IsWindowed
  xxx aoHidd_Gfx_ActiveBMCallBack
  xxx aoHidd_Gfx_ActiveBMCallBackData
  aoHidd_Gfx_DPMSLevel
  aoHidd_Gfx_PixFmtTags
  aoHidd_Gfx_SyncTags
  aoHidd_Gfx_ModeTags
  aoHidd_Gfx_NumSyncs
  aoHidd_Gfx_SupportsHWCursor
*/

void METHOD(IntelG33, Root, Get) {
    D(bug("[G33] Root Get\n"));

    ULONG idx;
    BOOL found = FALSE;
    if (IS_GFX_ATTR(msg->attrID, idx)) {
	    switch (idx) {
            case aoHidd_Gfx_SupportsHWCursor:
                D(bug("      ...HWCursor\n"));
                *msg->storage = (IPTR)TRUE;
                found = TRUE;
                break;
            case aoHidd_Gfx_DPMSLevel:
                D(bug("      ...DPMSLevel\n"));
                switch(((G33_RD_REGW(MMADR, ADPA)>>10)&0x3)) {
                    case 0:
                        *msg->storage = vHidd_Gfx_DPMSLevel_On;
                        break;
                    case 1:
                        *msg->storage = vHidd_Gfx_DPMSLevel_Suspend;
                        break;
                    case 2:
                        *msg->storage = vHidd_Gfx_DPMSLevel_Standby;
                        break;
                    case 3:
                        *msg->storage = vHidd_Gfx_DPMSLevel_Off;
                        break;
                }
                found = TRUE;
                break;
            default:
                D(bug("      ...ID = %d\n",idx));
                break;
        }
    }else{
        D(bug("      ...Not gfx attribute\n"));
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        
    return;
}

void METHOD(IntelG33, Root, Set) {
    D(bug("[G33] Root Set\n"));

    ULONG idx;
    const struct TagItem *tags, *tag;

    tags = msg->attrList;

    while ((tag = NextTagItem(&tags))) {
        if (IS_GFX_ATTR(tag->ti_Tag, idx)) {
            switch(idx) {
                case aoHidd_Gfx_DPMSLevel:
                    D(bug("      ...DPMSLevel\n"));
                    ObtainSemaphore(&sd->Chipset.CSLock);
                    switch(tag->ti_Data) {
                        case vHidd_Gfx_DPMSLevel_On:
                            G33_WRM_REGW(MMADR, ADPA, 0x0000, DPMSMASK);
                            break;
                        case vHidd_Gfx_DPMSLevel_Suspend:
                            G33_WRM_REGW(MMADR, ADPA, 0x0400, DPMSMASK);
                            break;
                        case vHidd_Gfx_DPMSLevel_Standby:
                            G33_WRM_REGW(MMADR, ADPA, 0x0800, DPMSMASK);
                            break;
                        case vHidd_Gfx_DPMSLevel_Off:
                            G33_WRM_REGW(MMADR, ADPA, 0x0c00, DPMSMASK);
                            break;
                    }
                    ReleaseSemaphore(&sd->Chipset.CSLock);
                    break;
                default:
                    D(bug("      ...ID = %d\n",idx));
                    break;
            }
        }else{
            D(bug("      ...Not gfx attribute\n"));
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *METHOD(IntelG33, Hidd_Gfx, NewBitMap) {
    D(bug("[G33] Hidd_Gfx NewBitMap\n"));

    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg;

    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    if (framebuffer) {
        D(bug("      ...Asked for framebuffer\n"));
        /* If the user asks for a framebuffer map we must ALLWAYS supply a class */ 
        classptr = sd->OnBMClass;
    }else if (displayable) {
        D(bug("      ...Asked for displayable\n"));
        classptr = sd->OnBMClass;
    }else {
        HIDDT_ModeID modeid;
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        
        if (vHidd_ModeID_Invalid != modeid) {
            /* User supplied a valid modeid. We can use our offscreen class */
            classptr = sd->OnBMClass;
        } else {
            /*
              We may create an offscreen bitmap if the user supplied a friend
              bitmap. But we need to check that he did not supplied a StdPixFmt
    	    */
            HIDDT_StdPixFmt stdpf;
            stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

            if (vHidd_StdPixFmt_Plane == stdpf) {
                classptr = sd->PlanarBMClass;
            } else if (vHidd_StdPixFmt_Unknown == stdpf) {
                /* No std pixfmt supplied */
                OOP_Object *friend;

                /* Did the user supply a friend bitmap ? */
                friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
                if (NULL != friend) {
                    /* User supplied friend bitmap. Is the friend bitmap a G33 Gfx hidd bitmap ? */
                    OOP_Object *gfxhidd;
                    OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
                    if (gfxhidd == o) {
                        /* Friend was G33 hidd bitmap. Now we can supply our own class */
                        classptr = sd->OffBMClass;
                    }
                }
	        }
        }
    }

    if (NULL != classptr) {
        /* Yes. We must let the superclass note that we do this. This is
           done through adding a tag in front of the taglist */
        mytags[0].ti_Tag    = aHidd_BitMap_ClassPtr;
        mytags[0].ti_Data   = (IPTR)classptr;
        mytags[1].ti_Tag    = TAG_MORE;
        mytags[1].ti_Data   = (IPTR)msg->attrList;
        
        /* Like in Gfx::New() we init a new message struct */
        mymsg.mID       = msg->mID;
        mymsg.attrList  = mytags;
        
        /* Pass the new message to the superclass */
        msg = &mymsg;
    }

    return (OOP_Object*)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg); 
}



