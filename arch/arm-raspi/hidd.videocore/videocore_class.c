/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Class for VideoCore.
    Lang: English.
*/

#define DEBUG 1
#include <aros/debug.h>

#define __OOP_NOATTRBASES__

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <aros/symbolsets.h>
#include <devices/inputevent.h>
#include <exec/alerts.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <oop/oop.h>
#include <clib/alib_protos.h>
#include <string.h>
#include <stdio.h>

#include "videocore_class.h"
#include "videocore_hardware.h"

#include LC_LIBDEFS_FILE

static OOP_AttrBase HiddBitMapAttrBase;  
static OOP_AttrBase HiddPixFmtAttrBase;
static OOP_AttrBase HiddGfxAttrBase;
static OOP_AttrBase HiddSyncAttrBase;
static OOP_AttrBase HiddVideoCoreAttrBase;
static OOP_AttrBase HiddVideoCoreBitMapAttrBase;

static struct OOP_ABDescr attrbases[] =
{
    {IID_Hidd_BitMap,           &HiddBitMapAttrBase             },
    {IID_Hidd_VideoCoreBitMap,  &HiddVideoCoreBitMapAttrBase    },
    {IID_Hidd_VideoCore,        &HiddVideoCoreAttrBase          },
    {IID_Hidd_PixFmt,           &HiddPixFmtAttrBase             },
    {IID_Hidd_Sync,             &HiddSyncAttrBase               },
    {IID_Hidd_Gfx,              &HiddGfxAttrBase                },
    {NULL,                      NULL                            }
};

STATIC ULONG mask_to_shift(ULONG mask)
{
    ULONG i;

    for (i = 32; mask; i --) {
        mask >>= 1;
    }

    if (mask == 32) {
        i = 0;
    }

    return i;
}

OOP_Object *VideoCore__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct VideoCore_staticdata *xsd = XSD(cl);

    struct TagItem pftags[] =
    {
        {aHidd_PixFmt_RedShift,         0       }, /*  0 */
        {aHidd_PixFmt_GreenShift,       0       }, /*  1 */
        {aHidd_PixFmt_BlueShift,        0       }, /*  2 */
        {aHidd_PixFmt_AlphaShift,       0       }, /*  3 */
        {aHidd_PixFmt_RedMask,          0       }, /*  4 */
        {aHidd_PixFmt_GreenMask,        0       }, /*  5 */
        {aHidd_PixFmt_BlueMask,         0       }, /*  6 */
        {aHidd_PixFmt_AlphaMask,        0       }, /*  7 */
        {aHidd_PixFmt_ColorModel,       0       }, /*  8 */
        {aHidd_PixFmt_Depth,            0       }, /*  9 */
        {aHidd_PixFmt_BytesPerPixel,    0       }, /* 10 */
        {aHidd_PixFmt_BitsPerPixel,     0       }, /* 11 */
        {aHidd_PixFmt_StdPixFmt,        0       }, /* 12 */
        {aHidd_PixFmt_CLUTShift,        0       }, /* 13 */
        {aHidd_PixFmt_CLUTMask,         0x0f    }, /* 14 */
        {aHidd_PixFmt_BitMapType,       0       }, /* 15 */
        {TAG_DONE,                      0UL     }
    };
    struct TagItem vcmodes[] =
    {
        {aHidd_Gfx_ModeTags,    NULL            },
        {TAG_MORE,              msg->attrList   }
    };
    struct pRoot_New vcmsgnew;

    D(bug("[VideoCore] VideoCore__Root__New()\n"));

    vcmsgnew.mID = msg->mID;
    vcmsgnew.attrList = vcmodes;
    msg = &vcmsgnew;

    EnterFunc(bug("VideoCore::New()\n"));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    if (o)
    {
        D(bug("[VideoCore] Got object from super\n"));
        XSD(cl)->videocorehidd = o;
        ReturnPtr("VideoCore::New", OOP_Object *, o);
    }
    ReturnPtr("VideoCore::New", OOP_Object *, NULL);
}

VOID VideoCore__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    if (XSD(cl)->mouse.shape != NULL)
        FreeVec(XSD(cl)->mouse.shape);
    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

VOID VideoCore__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    BOOL found = FALSE;

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
//        switch (idx)
//        {
//        case aoHidd_Gfx_SupportsHWCursor:
//            *msg->storage = (IPTR)TRUE;
//            found = TRUE;
//            break;
//        }
    }
    if (!found)
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

OOP_Object *VideoCore__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    BOOL displayable;
    BOOL framebuffer;
    OOP_Class *classptr = NULL;
    struct TagItem tags[2];
    struct pHidd_Gfx_NewBitMap vcmsgnew;

    EnterFunc(bug("VideoCore::NewBitMap()\n"));
    displayable = GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList);
    framebuffer = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);
    if (framebuffer)
        classptr = XSD(cl)->videocoreonbmclass;
    else if (displayable)
        classptr = XSD(cl)->videocoreoffbmclass;
    else
    {
        HIDDT_ModeID modeid;
        modeid = (HIDDT_ModeID)GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);
        if (modeid != vHidd_ModeID_Invalid)
            classptr = XSD(cl)->videocoreoffbmclass;
        else
        {
            HIDDT_StdPixFmt stdpf;
            stdpf = (HIDDT_StdPixFmt)GetTagData(aHidd_BitMap_StdPixFmt, vHidd_StdPixFmt_Unknown, msg->attrList);
            if (stdpf == vHidd_StdPixFmt_Unknown)
            {
                OOP_Object *friend;
                friend = (OOP_Object *)GetTagData(aHidd_BitMap_Friend, (IPTR)NULL, msg->attrList);
                if (friend != NULL)
                {
                    OOP_Class *friend_class = NULL;
                    OOP_GetAttr(friend, aHidd_BitMap_ClassPtr, (IPTR *)&friend_class);
                    if (friend_class == XSD(cl)->videocoreonbmclass)
                    {
                        classptr = XSD(cl)->videocoreoffbmclass;
                    }
                }
            }
        }
    }
    if (classptr != NULL)
    {
        tags[0].ti_Tag = aHidd_BitMap_ClassPtr;
        tags[0].ti_Data = (IPTR)classptr;
        tags[1].ti_Tag = TAG_MORE;
        tags[1].ti_Data = (IPTR)msg->attrList;
        vcmsgnew.mID = msg->mID;
        vcmsgnew.attrList = tags;
        msg = &vcmsgnew;
    }
    ReturnPtr("VideoCore::NewBitMap", OOP_Object *, (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg));
}

VOID VideoCore__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    UBYTE *src = NULL;
    UBYTE *dst = NULL;
    HIDDT_DrawMode mode;
    struct Box box;

    EnterFunc(bug("VideoCore.BitMap::CopyBox\n"));
    mode = GC_DRMD(msg->gc);
    OOP_GetAttr(msg->src, aHidd_VideoCoreBitMap_Drawable, (IPTR *)&src);
    OOP_GetAttr(msg->dest, aHidd_VideoCoreBitMap_Drawable, (IPTR *)&dst);
    if (((dst == NULL) || (src == NULL))) /* no videocoregfx bitmap */
    {
        OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else if (dst == src)
    {
        struct BitmapData *data;
        data = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        switch (mode)
        {
/*        case vHidd_GC_DrawMode_Clear:
            clearCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_And:
            andCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_AndReverse:
            andReverseCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Copy:
            copyCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_AndInverted:
            andInvertedCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_NoOp:
            noOpCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Xor:
            xorCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Or:
            orCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Nor:
            norCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Equiv:
            equivCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Invert:
            invertCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_OrReverse:
            orReverseCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_CopyInverted:
            copyInvertedCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_OrInverted:
            orInvertedCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Nand:
            nandCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
        case vHidd_GC_DrawMode_Set:
            setCopyVideoCore(data->data, msg->srcX, msg->srcY, msg->destX, msg->destY, msg->width, msg->height);
            break;
*/
        default:
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
    }
    else
    {
        struct BitmapData *srcbd = OOP_INST_DATA(OOP_OCLASS(msg->src), msg->src);
        struct BitmapData *dstbd = OOP_INST_DATA(OOP_OCLASS(msg->dest), msg->dest);
        UBYTE *sbuffer;
        ULONG srestadd;
        UBYTE *dbuffer;
        ULONG drestadd;
        ULONG ycnt = msg->height;
        ULONG xcnt;
        LONG offset;
        /* get src/dest video data start addresses and skip sizes */
        if (srcbd->VideoData == srcbd->data->vrambase)
        {
            offset = (msg->srcX*srcbd->bytesperpix)+(msg->srcY*srcbd->data->bytesperline);
            srestadd = (srcbd->data->bytesperline - (msg->width*srcbd->bytesperpix));
//            displayCursorVideoCore(&XSD(cl)->data, 0);
            XSD(cl)->mouse.visible = 0;
        }
        else
        {
            offset = (msg->srcX+(msg->srcY*srcbd->width))*srcbd->bytesperpix;
            srestadd = (srcbd->width - msg->width)*srcbd->bytesperpix;
        }
        sbuffer = srcbd->VideoData+offset;
        if (dstbd->VideoData == dstbd->data->vrambase)
        {
            offset = (msg->destX*dstbd->bytesperpix)+(msg->destY*dstbd->data->bytesperline);
            drestadd = (dstbd->data->bytesperline - (msg->width*dstbd->bytesperpix));
//            displayCursorVideoCore(&XSD(cl)->data, 0);
            XSD(cl)->mouse.visible = 0;
        }
        else
        {
            offset = (msg->destX+(msg->destY*dstbd->width))*dstbd->bytesperpix;
            drestadd = (dstbd->width - msg->width)*dstbd->bytesperpix;
        }
        dbuffer = dstbd->VideoData+offset;
        switch (mode)
        {
        case vHidd_GC_DrawMode_Copy:
            while (ycnt--)
            {
                ULONG pixel;
                xcnt = msg->width;
                while (xcnt)
                {
                    /* get pixel from source */
                        switch (srcbd->bytesperpix)
                    {
                        case 1:
                        pixel = (ULONG)*((UBYTE *)sbuffer);
                        sbuffer++;
                        break;
                        case 2:
                        pixel = (ULONG)*((UWORD *)sbuffer);
                        sbuffer += 2;
                        break;
                        case 4:
                        pixel = (ULONG)*((ULONG *)sbuffer);
                        sbuffer += 4;
                        break;
                        default:
                        D(bug("[VideoCore] Copy: Unknown number of bytes per pixel (%d) in source!\n",srcbd->bytesperpix));
                        pixel = 0;
                        break;
                    }
                    /* write pixel to destination */
                    switch (dstbd->bytesperpix)
                    {
                        case 1:
                        *((UBYTE *)dbuffer) = (UBYTE)pixel;
                        dbuffer++;
                        break;
                        case 2:
                        *((UWORD *)dbuffer) = (UWORD)pixel;
                        dbuffer += 2;
                        break;
                        case 4:	
                        *((ULONG *)dbuffer) = (ULONG)pixel;
                        dbuffer += 4;
                        break;
                        default:
                        D(bug("[VideoCore] Copy: Unknown number of bytes per pixel (%d) in destination!\n",dstbd->bytesperpix));
                        break;
                    }
                    xcnt--;
                }
                sbuffer += srestadd;
                dbuffer += drestadd;
            }
            if (dstbd->VideoData == dstbd->data->vrambase)
            {
                box.x1 = msg->destX;
                box.y1 = msg->destY;
                box.x2 = box.x1+msg->width-1;
                box.y2 = box.y1+msg->height-1;
//                refreshAreaVideoCore(dstbd->data, &box);
            }
            break;
        default:
            kprintf("[VideoCore] mode = %ld src=%lx dst=%lx\n", mode, src, dst);
            OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
        }
        if (XSD(cl)->mouse.visible == 0)
        {
//            displayCursorVideoCore(&XSD(cl)->data, 1);
            XSD(cl)->mouse.visible = 1;
        }
    }
    ReturnVoid("VideoCore.BitMap::CopyBox");
}

BOOL VideoCore__Hidd_Gfx__SetCursorShape(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorShape *msg)
{
    struct VideoCore_staticdata *xsd = XSD(cl);

    if (msg->shape == NULL)
    {
//        displayCursorVideoCore(&XSD(cl)->data, 0);
        xsd->mouse.oopshape = NULL;
        if (xsd->mouse.shape != NULL)
            FreeVec(xsd->mouse.shape);
        xsd->mouse.shape = NULL;
        return TRUE;
    }
    else
    {
        OOP_Object *pfmt;
        OOP_Object *colmap;
        HIDDT_StdPixFmt pixfmt;
        HIDDT_Color color;
        IPTR tmp;
        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &tmp);
        xsd->mouse.width = tmp;
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &tmp);
        xsd->mouse.height = tmp;
        OOP_GetAttr(msg->shape, aHidd_BitMap_PixFmt, (IPTR *)&pfmt);
        OOP_GetAttr(pfmt, aHidd_PixFmt_StdPixFmt, (IPTR *)&pixfmt);
        OOP_GetAttr(msg->shape, aHidd_BitMap_ColorMap, (IPTR *)&colmap);
        xsd->mouse.oopshape = msg->shape;
#if 0
        xsd->mouse.shape = cursor_shape;
        xsd->mouse.width = 11;
        xsd->mouse.height = 11;
//        defineCursorVideoCore(&XSD(cl)->data, &xsd->mouse);
        return TRUE;
#else
        /* convert shape to videocore needs */
        if (xsd->mouse.shape != NULL)
            FreeVec(xsd->mouse.shape);
//        xsd->mouse.shape = AllocVec(SVGA_PIXMAP_SIZE(xsd->mouse.width, xsd->mouse.height, VCDATA(xsd)->bitsperpixel)*4, MEMF_PUBLIC);
        if (xsd->mouse.shape != NULL)
        {
            LONG xcnt;
            LONG ycnt;
            LONG linebytes;
            LONG bytecnt;
            LONG pixelbytes;
            UBYTE *shape;
//            linebytes = SVGA_PIXMAP_SCANLINE_SIZE(xsd->mouse.width, VCDATA(xsd)->bitsperpixel)*4;
            pixelbytes = (VCDATA(xsd)->bitsperpixel+3)/8;
            shape = xsd->mouse.shape;
            for (ycnt=0;ycnt<xsd->mouse.height;ycnt++)
            {
                for (xcnt=0;xcnt<xsd->mouse.width;xcnt++)
                {
                    HIDDT_Pixel pixel;
                    pixel = HIDD_BM_GetPixel(msg->shape, xcnt, ycnt);
                    if (pixfmt == vHidd_StdPixFmt_LUT8)
                    {
                        if (HIDD_CM_GetColor(colmap, pixel, &color))
                        {
                            pixel =
                                (
                                    (((color.red  <<16)>>mask_to_shift(VCDATA(xsd)->redmask))   & VCDATA(xsd)->redmask  )+
                                    (((color.green<<16)>>mask_to_shift(VCDATA(xsd)->greenmask)) & VCDATA(xsd)->greenmask)+
                                    (((color.blue <<16)>>mask_to_shift(VCDATA(xsd)->bluemask))  & VCDATA(xsd)->bluemask )
                                );
                        }
                    }
                    if (pixelbytes == 1)
                    {
                        *((UBYTE *)shape) = (UBYTE)pixel;
                        shape++;
                    }
                    else if (pixelbytes == 2)
                    {
                        *((UWORD *)shape) = (UWORD)pixel;
                        shape += 2;
                    }
                    else if (pixelbytes == 4)
                    {
                        *((ULONG *)shape) = (ULONG)pixel;
                        shape += 4;
                    }
                }
                for (bytecnt=linebytes-(xsd->mouse.width*pixelbytes);bytecnt;bytecnt--)
                {
                    *((UBYTE *)shape) = 0; /* fill up to long boundary */
                    shape++;
                }
            }
//            defineCursorVideoCore(&XSD(cl)->data, &xsd->mouse);
            return TRUE;
        }
#endif
    }
    return FALSE;
}

BOOL VideoCore__Hidd_Gfx__SetCursorPos(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorPos *msg)
{
    XSD(cl)->mouse.x = msg->x;
    XSD(cl)->mouse.y = msg->y;
    if (XSD(cl)->mouse.x<0)
        XSD(cl)->mouse.x=0;
    if (XSD(cl)->mouse.y<0)
        XSD(cl)->mouse.y=0;
    /* TODO: check visible width/height */
//    moveCursorVideoCore(&XSD(cl)->data, msg->x, msg->y);
    return TRUE;
}

VOID VideoCore__Hidd_Gfx__SetCursorVisible(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_SetCursorVisible *msg)
{
    XSD(cl)->mouse.visible = msg->visible;
//    displayCursorVideoCore(&XSD(cl)->data, msg->visible ? 1 : 0);
}


static int VideoCore_InitStatic(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("[VideoCore] VideoCore_InitStatic()\n"));

    LIBBASE->vsd.mouse.x=0;
    LIBBASE->vsd.mouse.y=0;
    LIBBASE->vsd.mouse.shape = NULL;

    if (!OOP_ObtainAttrBases(attrbases))
    {
        D(bug("[VideoCore] VideoCore_InitStatic: attrbases init failed\n"));
        return FALSE;
    }
    
    D(bug("[VideoCore] VideoCore_InitStatic: ok\n"));

    ReturnInt("VideoCore_InitStatic", int, TRUE);
}

static int VideoCore_ExpungeStatic(LIBBASETYPEPTR LIBBASE)
{
    EnterFunc(bug("[VideoCore] VideoCore_ExpungeStatic()\n"));

    OOP_ReleaseAttrBases(attrbases);
    ReturnInt("VideoCore_ExpungeStatic", int, TRUE);
}

ADD2INITLIB(VideoCore_InitStatic, 0)
ADD2EXPUNGELIB(VideoCore_ExpungeStatic, 0)
