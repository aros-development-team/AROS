/*
    Copyright © 2004, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <utility/tagitem.h>

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>

#include "ati.h"
#include "radeon.h"
#include "radeon_reg.h"
#include "radeon_bios.h"
#include "radeon_macros.h"

#define DEBUG 1
#include <aros/debug.h>

#define sd ((struct ati_staticdata*)SD(cl))

#undef HiddPCIDeviceAttrBase
#undef HiddGfxAttrBase
#undef HiddPixFmtAttrBase
#undef HiddSyncAttrBase
#undef HiddBitMapAttrBase
#define HiddPCIDeviceAttrBase   (sd->pciAttrBase)
#define HiddATIBitMapAttrBase   (sd->atiBitMapAttrBase)
#define HiddBitMapAttrBase  (sd->bitMapAttrBase)
#define HiddPixFmtAttrBase  (sd->pixFmtAttrBase)
#define HiddGfxAttrBase     (sd->gfxAttrBase)
#define HiddSyncAttrBase    (sd->syncAttrBase)

#define ToRGB8888(alp,c) ((c) | ((alp) << 24))

#if AROS_BIG_ENDIAN

#define CURSOR_SWAPPING_DECL_MMIO
#define CURSOR_SWAPPING_START() \
    OUTREG(RADEON_SURFACE_CNTL, \
           (info->ModeReg.surface_cntl | \
            RADEON_NONSURF_AP0_SWP_32BPP) & \
           ~RADEON_NONSURF_AP0_SWP_16BPP)
#define CURSOR_SWAPPING_END()   (OUTREG(RADEON_SURFACE_CNTL, \
                                        info->ModeReg.surface_cntl))

#else

#define CURSOR_SWAPPING_DECL_MMIO
#define CURSOR_SWAPPING_START()
#define CURSOR_SWAPPING_END()

#endif

#define MAKE_SYNC(name,clock,hdisp,hstart,hend,htotal,vdisp,vstart,vend,vtotal,descr)   \
    struct TagItem sync_ ## name[]={            \
        { aHidd_Sync_PixelClock,    clock*1000  },  \
        { aHidd_Sync_HDisp,         hdisp   },  \
        { aHidd_Sync_HSyncStart,    hstart  },  \
        { aHidd_Sync_HSyncEnd,      hend    },  \
        { aHidd_Sync_HTotal,        htotal  },  \
        { aHidd_Sync_VDisp,         vdisp   },  \
        { aHidd_Sync_VSyncStart,    vstart  },  \
        { aHidd_Sync_VSyncEnd,      vend    },  \
        { aHidd_Sync_VTotal,        vtotal  },  \
        { aHidd_Sync_Description,       (IPTR)descr},   \
        { TAG_DONE, 0UL }}


OOP_Object *METHOD(ATI, Hidd_Gfx, Show)
{
    OOP_Object *fb = NULL;
    if (msg->bitMap)
    {
        atiBitMap *bm = OOP_INST_DATA(OOP_OCLASS(msg->bitMap), msg->bitMap);
        if (bm->state)
        {
            /* Suppose bm has properly allocated state structure */
            if (bm->fbgfx)
            {
                bm->usecount++;
                
                LOCK_HW
     
                LoadState(sd, bm->state);
                DPMS(sd, sd->dpms);
        
                fb = bm->BitMap;
                ShowHideCursor(sd, sd->Card.cursorVisible);
                
                UNLOCK_HW
            }
        }
    }
    
    if (!fb)
        fb = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return fb;
}

OOP_Object *METHOD(ATI, Hidd_Gfx, NewBitMap)
{
    BOOL displayable, framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem mytags[2];
    struct pHidd_Gfx_NewBitMap mymsg; 
    
    /* Displayable bitmap ? */
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

    if (framebuffer)
    {
        /* If the user asks for a framebuffer map we must ALLWAYS supply a class */ 
        classptr = sd->OnBMClass;
    }
    else if (displayable)
    {
        classptr = sd->OnBMClass;   //offbmclass;
    }
    else
    {
        HIDDT_ModeID modeid;
        /* 
            For the non-displayable case we can either supply a class ourselves
            if we can optimize a certain type of non-displayable bitmaps. Or we
            can let the superclass create on for us.
           
            The attributes that might come from the user deciding the bitmap
            pixel format are:
            - aHidd_BitMap_ModeID:  a modeid. create a nondisplayable
                bitmap with the size  and pixelformat of a gfxmode.
            - aHidd_BitMap_StdPixFmt: a standard pixelformat as described in
                hidd/graphics.h
            - aHidd_BitMap_Friend: if this is supplied and none of the two above
                are supplied, then the pixel format of the created bitmap
                will be the same as the one of the friend bitmap.
                
            These tags are listed in prioritized order, so if
            the user supplied a ModeID tag, then you should not care about StdPixFmt
            or Friend. If there is no ModeID, but a StdPixFmt tag supplied,
            then you should not care about Friend because you have to
            create the correct pixelformat. And as said above, if only Friend
            is supplied, you can create a bitmap with same pixelformat as Frien
        */
        
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        
        if (vHidd_ModeID_Invalid != modeid)
        {
            /* User supplied a valid modeid. We can use our offscreen class */
            classptr = sd->OnBMClass;
        }
        else
        {
            /* 
               We may create an offscreen bitmap if the user supplied a friend
               bitmap. But we need to check that he did not supplied a StdPixFmt
            */
            HIDDT_StdPixFmt stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);

            if (vHidd_StdPixFmt_Plane == stdpf)
            {
                classptr = sd->PlanarBMClass;
            }
            else if (vHidd_StdPixFmt_Unknown == stdpf)
            {
                /* No std pixfmt supplied */
                OOP_Object *friend;

                /* Did the user supply a friend bitmap ? */
                friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, 0, msg->attrList);
                if (NULL != friend)
                {
                    OOP_Object * gfxhidd;
                    /* User supplied friend bitmap. Is the friend bitmap a Ati Gfx hidd bitmap ? */
                    OOP_GetAttr(friend, aHidd_BitMap_GfxHidd, (APTR)&gfxhidd);
                    if (gfxhidd == o)
                    {
                        /* Friend was NVidia hidd bitmap. Now we can supply our own class */
                        classptr = sd->OffBMClass;          
                    }
                }
            }
        }
    }
   
    D(bug("[ATI] classptr = %p\n", classptr));
    
    /* Do we supply our own class ? */
    if (NULL != classptr)
    {
        /* Yes. We must let the superclass not that we do this. This is
           done through adding a tag in the frot of the taglist */
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

void METHOD(ATI, Hidd_Gfx, CopyBox)
{
    ULONG mode = GC_DRMD(msg->gc);
    IPTR src=0, dst=0;

    /* Check whether we can get Drawable attribute of our nVidia class */
    OOP_GetAttr(msg->src,   aHidd_ATIBitMap_Drawable,   &src);
    OOP_GetAttr(msg->dest,  aHidd_ATIBitMap_Drawable,   &dst);

    if (!dst || !src)
    {
        /* No. One of the bitmaps is not nVidia bitmap */
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
    {

        /* Yes. Get the instance data of both bitmaps */
        atiBitMap *bm_src = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        atiBitMap *bm_dst = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);

#warning TODO: Implement native ATI::CopyBox();
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

        D(bug("[ATI] CopyBox(src(%p,%d:%d@%d),dst(%p,%d:%d@%d),%d:%d\n",
                bm_src->framebuffer,msg->srcX,msg->srcY,bm_src->depth,
                bm_dst->framebuffer,msg->destX,msg->destY,bm_dst->depth,
                msg->width, msg->height));
    
        bm_src->usecount++;
        bm_dst->usecount++;
    }
}

BOOL METHOD(ATI, Hidd_Gfx, SetCursorShape)
{
    D(bug("[ATI] Set cursor shape %08x\n", msg->shape));

    if (msg->shape == NULL)
    {
        ShowHideCursor(sd, 0);
        sd->Card.cursorVisible = 0;
    }
    else
    {
        OOP_Object  *pfmt;
        OOP_Object  *colormap;
        HIDDT_StdPixFmt pixfmt;
        HIDDT_Color color;

        ULONG       width, height, x, y;
        ULONG       maxw,maxh;
        ULONG       save1=0, save2=0;

        ULONG       *curimg = (ULONG*)((IPTR)sd->Card.CursorStart + (IPTR)sd->Card.FrameBuffer);

        struct pHidd_BitMap_GetPixel __gp = {
            mID:    OOP_GetMethodID((STRPTR)CLID_Hidd_BitMap, moHidd_BitMap_GetPixel)
        }, *getpixel = &__gp;

        struct pHidd_ColorMap_GetColor __gc = {
            mID:        OOP_GetMethodID((STRPTR)CLID_Hidd_ColorMap, moHidd_ColorMap_GetColor),
            colorReturn:    &color,
        }, *getcolor = &__gc;

        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);
        OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (APTR)&pfmt);
        OOP_GetAttr(pfmt, aHidd_PixFmt_StdPixFmt, &pixfmt);
        OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (APTR)&colormap);

        if (width > 64) width = 64;
        if (height > 64) height = 64;
            
        maxw = 64;
        maxh = 64;
    
        LOCK_HW

        if (!sd->Card.IsSecondary) {
            save1 = INREG(RADEON_CRTC_GEN_CNTL) & ~(ULONG)(RADEON_CRTC_ARGB_EN|RADEON_CRTC_ARGB_EN);
            save1 |= RADEON_CRTC_ARGB_EN;
            OUTREG(RADEON_CRTC_GEN_CNTL, save1 & (ULONG)~RADEON_CRTC_CUR_EN);
        }
    
        if (sd->Card.IsSecondary) {
            save2 = INREG(RADEON_CRTC_GEN_CNTL) & ~(ULONG)(RADEON_CRTC_ARGB_EN|RADEON_CRTC_ARGB_EN);
            save2 |= RADEON_CRTC_ARGB_EN;
            OUTREG(RADEON_CRTC2_GEN_CNTL, save2 & (ULONG)~RADEON_CRTC2_CUR_EN);
        }

        CURSOR_SWAPPING_START();

        for (x = 0; x < maxw*maxh; x++)
           curimg[x] = 0;
    
        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                HIDDT_Pixel pixel;
                getpixel->x = x;
                getpixel->y = y;
                pixel = OOP_DoMethod(msg->shape, (OOP_Msg)getpixel);
                
                if (pixfmt == vHidd_StdPixFmt_LUT8)
                {
                    getcolor->colorNo = pixel;
                    OOP_DoMethod(colormap, (OOP_Msg)getcolor);
                    pixel = ((color.red << 8) & 0xff0000) |
                        ((color.green) & 0x00ff00)    |
                        ((color.blue >> 8) & 0x0000ff);
                    
                    if (pixel)
                        *curimg++ = ToRGB8888(0xe0, pixel);
                    else curimg++;
                }
            }
            for (x=width; x < maxw; x++, curimg++)
                *curimg = 0;
        }
    
        for (y=height; y < maxh; y++)
            for (x=0; x < maxw; x++)
                { if (*curimg!=0x50000000) *curimg = 0; curimg++; }

        CURSOR_SWAPPING_END();
        
        if (!sd->Card.IsSecondary)
            OUTREG(RADEON_CRTC_GEN_CNTL, save1);

        if (sd->Card.IsSecondary)
            OUTREG(RADEON_CRTC2_GEN_CNTL, save2);
        
        UNLOCK_HW
    }

    return TRUE;
}

void METHOD(ATI, Hidd_Gfx, SetCursorVisible)
{
    ShowHideCursor(sd, msg->visible);
    sd->Card.cursorVisible = msg->visible;
}

void METHOD(ATI, Hidd_Gfx, SetCursorPos)
{
    D(bug("[ATI] Set cursor pos %d:%d\n", msg->x, msg->y));
    
    if (!sd->Card.IsSecondary) {
        OUTREG(RADEON_CUR_HORZ_VERT_OFF,  RADEON_CUR_LOCK);
        OUTREG(RADEON_CUR_HORZ_VERT_POSN, (RADEON_CUR_LOCK
                                           | (msg->x << 16)
                                           | (msg->y & 0xffff)));
        OUTREG(RADEON_CUR_OFFSET, sd->Card.CursorStart);

    D(bug("[ATI] OFF=%08x, HV_OFF=%08x, HV_POSN=%08x\n",
        INREG(RADEON_CUR_OFFSET),
        INREG(RADEON_CUR_HORZ_VERT_OFF),        
        INREG(RADEON_CUR_HORZ_VERT_POSN)));
        
    } else {
        OUTREG(RADEON_CUR2_HORZ_VERT_OFF,  RADEON_CUR2_LOCK);
        OUTREG(RADEON_CUR2_HORZ_VERT_POSN, (RADEON_CUR2_LOCK
                                           | (msg->x << 16)
                                           | (msg->y & 0xffff)));
        OUTREG(RADEON_CUR2_OFFSET, sd->Card.CursorStart /* + addend???? */);
    }
}

OOP_Object *METHOD(ATI, Root, New)
{
    struct TagItem pftags_24bpp[] = {
        { aHidd_PixFmt_RedShift,    8   }, /* 0 */
        { aHidd_PixFmt_GreenShift,  16  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   24  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00ff0000 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x0000ff00 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x000000ff }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       32  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   4   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    24  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_BGR032 }, /* 12 Native */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };
   
    struct TagItem pftags_16bpp[] = {
        { aHidd_PixFmt_RedShift,    16  }, /* 0 */
        { aHidd_PixFmt_GreenShift,  21  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   27  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x0000f800 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x000007e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       16  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   2   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    16  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_RGB16_LE }, /* 12 */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    struct TagItem pftags_15bpp[] = {
        { aHidd_PixFmt_RedShift,    17  }, /* 0 */
        { aHidd_PixFmt_GreenShift,  22  }, /* 1 */
        { aHidd_PixFmt_BlueShift,   27  }, /* 2 */
        { aHidd_PixFmt_AlphaShift,  0   }, /* 3 */
        { aHidd_PixFmt_RedMask,     0x00007c00 }, /* 4 */
        { aHidd_PixFmt_GreenMask,   0x000003e0 }, /* 5 */
        { aHidd_PixFmt_BlueMask,    0x0000001f }, /* 6 */
        { aHidd_PixFmt_AlphaMask,   0x00000000 }, /* 7 */
        { aHidd_PixFmt_ColorModel,  vHidd_ColorModel_TrueColor }, /* 8 */
        { aHidd_PixFmt_Depth,       15  }, /* 9 */
        { aHidd_PixFmt_BytesPerPixel,   2   }, /* 10 */
        { aHidd_PixFmt_BitsPerPixel,    15  }, /* 11 */
        { aHidd_PixFmt_StdPixFmt,   vHidd_StdPixFmt_RGB15_LE }, /* 12 */
        { aHidd_PixFmt_BitMapType,  vHidd_BitMapType_Chunky }, /* 15 */
        { TAG_DONE, 0UL }
    };

    MAKE_SYNC(640x480_60,   25174,
         640,  656,  752,  800,
         480,  490,  492,  525,
         "ATI:640x480");

    MAKE_SYNC(800x600_56,   36000,  // 36000
         800,  824,  896, 1024,
         600,  601,  603,  625,
         "ATI:800x600");

    MAKE_SYNC(1024x768_60, 65000,   //78654=60kHz, 75Hz. 65000=50kHz,62Hz
        1024, 1048, 1184, 1344,
         768,  771,  777,  806,
        "ATI:1024x768");
     
    MAKE_SYNC(1152x864_60, 80000,
        1152, 1216, 1328, 1456,
         864,  870,  875,  916,
        "ATI:1152x864");

    MAKE_SYNC(1280x1024_60, 107991,
        1280, 1328, 1440, 1688,
        1024, 1025, 1028, 1066,
        "ATI:1280x1024");    
    
    MAKE_SYNC(1600x1200_60, 155982,
        1600, 1632, 1792, 2048,
        1200, 1210, 1218, 1270,
        "ATI:1600x1200");
    
    struct TagItem modetags[] = {
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags_24bpp  },
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags_16bpp  },
        { aHidd_Gfx_PixFmtTags, (IPTR)pftags_15bpp  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_640x480_60   },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_800x600_56   },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1024x768_60  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1152x864_60  },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1280x1024_60 },
        { aHidd_Gfx_SyncTags,   (IPTR)sync_1600x1200_60 },
        
        { TAG_DONE, 0UL }
    };
    
    struct TagItem mytags[] = {
        { aHidd_Gfx_ModeTags,   (IPTR)modetags  },
        { TAG_MORE, (IPTR)msg->attrList }
    };

    struct pRoot_New mymsg;

    mymsg.mID = msg->mID;
    mymsg.attrList = mytags;

    msg = &mymsg;

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        sd->AtiObject = o;
    }

    EnterFunc(bug("[ATI] RadeonDriver::New()=%08x\n",o));

    return o;
}

void METHOD(ATI, Root, Get)
{
    ULONG idx;
    BOOL found = FALSE;
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
            case aoHidd_Gfx_SupportsHWCursor:
                *msg->storage = (IPTR)TRUE;
                found = TRUE;
                break;
    
            case aoHidd_Gfx_DPMSLevel:
                *msg->storage = sd->dpms;
                found = TRUE;
                break;
        }
    }

    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return;
}

void METHOD(ATI, Root, Set)
{
    ULONG idx;
    struct TagItem *tag;
    const struct TagItem *tags = msg->attrList;

    while ((tag = NextTagItem(&tags)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx))
        {
            switch(idx)
            {
                case aoHidd_Gfx_DPMSLevel:
                    LOCK_HW
                    
                    DPMS(sd, tag->ti_Data);
                    sd->dpms = tag->ti_Data;
                    
                    UNLOCK_HW
                    break;
            }
       }
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}
