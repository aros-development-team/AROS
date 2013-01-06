/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Linux fbdev gfx HIDD for AROS.
    Lang: English.
*/

#define DEBUG 1
#define DEBUG_PF

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <fcntl.h>

#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <aros/debug.h>
#include <oop/oop.h>
#include <hidd/hidd.h>
#include <hidd/graphics.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <hidd/unixio_inline.h>

#include "linuxfb_intern.h"
#include "bitmap.h"

#define CURSOR_IMAGE_BPP    (4)

#include LC_LIBDEFS_FILE

static BOOL setup_linuxfb(struct LinuxFB_staticdata *fsd, int fbdev,
              struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi);
static VOID cleanup_linuxfb(struct LinuxFB_data *data, struct LinuxFB_staticdata *fsd);
static BOOL get_pixfmt(struct TagItem *pftags, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi);

/***************** FBGfx::New() ***********************/

OOP_Object *LinuxFB__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct LinuxFB_staticdata *fsd = LSD(cl);
    struct fb_fix_screeninfo fsi;
    struct fb_var_screeninfo vsi;
    int fbdev = GetTagData(aHidd_LinuxFB_File, -1, msg->attrList);
    char *baseaddr = MAP_FAILED;

    struct TagItem pftags[] =
    {
    { aHidd_PixFmt_RedShift     , 0    }, /* 0 */
    { aHidd_PixFmt_GreenShift   , 0    }, /* 1 */
    { aHidd_PixFmt_BlueShift    , 0    }, /* 2 */
    { aHidd_PixFmt_AlphaShift   , 0    }, /* 3 */
    { aHidd_PixFmt_RedMask      , 0    }, /* 4 */
    { aHidd_PixFmt_GreenMask    , 0    }, /* 5 */
    { aHidd_PixFmt_BlueMask     , 0    }, /* 6 */
    { aHidd_PixFmt_AlphaMask    , 0    }, /* 7 */
    { aHidd_PixFmt_ColorModel   , 0    }, /* 8 */
    { aHidd_PixFmt_Depth        , 0    }, /* 9 */
    { aHidd_PixFmt_BytesPerPixel, 0    }, /* 10 */
    { aHidd_PixFmt_BitsPerPixel , 0    }, /* 11 */
    { aHidd_PixFmt_StdPixFmt    , 0    }, /* 12 */
    { aHidd_PixFmt_CLUTShift    , 0    }, /* 13 */
    { aHidd_PixFmt_CLUTMask     , 0    }, /* 14 */
    { aHidd_PixFmt_BitMapType   , 0    }, /* 15 */
    { TAG_DONE                  , 0    }
    };
        
    struct TagItem synctags[] =
    {
    { aHidd_Sync_PixelTime  , 0 },    /* 0 */
    { aHidd_Sync_HDisp      , 0 },    /* 1 */
    { aHidd_Sync_VDisp      , 0 },    /* 2 */
    { aHidd_Sync_LeftMargin , 0 },    /* 3 */
    { aHidd_Sync_RightMargin, 0 },    /* 4 */
    { aHidd_Sync_HSyncLength, 0 },    /* 5 */
    { aHidd_Sync_UpperMargin, 0 },    /* 6 */
    { aHidd_Sync_LowerMargin, 0 },    /* 7 */
    { aHidd_Sync_VSyncLength, 0 },    /* 8 */
    { aHidd_Sync_Description, 0 },    /* 9 */
    { TAG_DONE              , 0 }
    };
    
    struct TagItem modetags[] =
    {
    { aHidd_Gfx_PixFmtTags  , (IPTR)pftags   },
    { aHidd_Gfx_SyncTags    , (IPTR)synctags },
    { TAG_DONE              , 0              }
    };
    
    struct TagItem mytags[] =
    {
    { aHidd_Gfx_ModeTags, (IPTR)modetags },
    { TAG_MORE          , 0              }
    };

    struct pRoot_New mymsg;

    if (fbdev == -1)
    {
        D(bug("[LinuxFB] No file descriptor supplied in New()\n"));
        return NULL;
    }

    /* Do GfxHidd initalization here */
    if (setup_linuxfb(LSD(cl), fbdev, &fsi, &vsi))
    {
        if (get_pixfmt(pftags, &fsi, &vsi))
        {
            /* Memorymap the framebuffer using mmap() */
            baseaddr = Hidd_UnixIO_MemoryMap(fsd->unixio, NULL, fsi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0, NULL);
    
            D(bug("[LinuxFB] Mapped at 0x%p\n", baseaddr));
            if (baseaddr != MAP_FAILED)
            {
                /* Register gfxmodes */

                /* Set the gfxmode info */
                synctags[0].ti_Data = vsi.pixclock;
                synctags[1].ti_Data = vsi.xres;
                synctags[2].ti_Data = vsi.yres;
                synctags[3].ti_Data = vsi.left_margin;
                synctags[4].ti_Data = vsi.right_margin;
                synctags[5].ti_Data = vsi.hsync_len;
                synctags[6].ti_Data = vsi.upper_margin;
                synctags[7].ti_Data = vsi.lower_margin;
                synctags[8].ti_Data = vsi.vsync_len;
                synctags[9].ti_Data = (IPTR)"FBDev:%hx%v";

                mytags[1].ti_Data = (IPTR)msg->attrList;
                mymsg.mID      = msg->mID;
                mymsg.attrList = mytags;
    
                o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &mymsg.mID);
                if (NULL != o)
                {
                    struct LinuxFB_data *data = OOP_INST_DATA(cl, o);

                    data->fbdev = fbdev;
                    data->fbdevinfo.baseaddr = baseaddr;
                    data->fbdevinfo.pitch = fsi.line_length;
                    data->mem_len  = fsi.smem_len;
                    data->fbdevinfo.bpp = ((vsi.bits_per_pixel - 1) / 8) + 1;
                    data->fbdevinfo.xres = vsi.xres;
                    data->fbdevinfo.yres = vsi.yres;
#if BUFFERED_VRAM
                    InitSemaphore(&data->framebufferlock);
#endif
                    return o;
                }
            }
        }
    }

    if (baseaddr != MAP_FAILED)
        Hidd_UnixIO_MemoryUnMap(fsd->unixio, baseaddr, fsi.smem_len, NULL);
    Hidd_UnixIO_CloseFile(fsd->unixio, fbdev, NULL);

    return NULL;
}

/********** FBGfx::Dispose()  ******************************/
VOID LinuxFB__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    cleanup_linuxfb(OOP_INST_DATA(cl, o), LSD(cl));

    OOP_DoSuperMethod(cl, o, msg);
}

/********** FBGfx::NewBitMap()  ****************************/
OOP_Object *LinuxFB__Hidd_Gfx__NewBitMap(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_NewBitMap *msg)
{
    struct TagItem tags[3];
    struct pHidd_Gfx_NewBitMap p;
    HIDDT_ModeID modeid;

    modeid = GetTagData(aHidd_BitMap_ModeID, vHidd_ModeID_Invalid, msg->attrList);

    if (modeid != vHidd_ModeID_Invalid)
    {
        tags[0].ti_Tag  = aHidd_BitMap_ClassPtr;
        tags[0].ti_Data = (IPTR)LSD(cl)->bmclass;
        tags[1].ti_Tag  = TAG_IGNORE;
        tags[1].ti_Data = TAG_IGNORE;
        tags[2].ti_Tag  = TAG_MORE;
        tags[2].ti_Data = (IPTR)msg->attrList;

        if (GetTagData(aHidd_BitMap_Displayable, FALSE, msg->attrList))
        {
            struct LinuxFB_data *data = OOP_INST_DATA(cl, o);

            tags[1].ti_Tag  = aHidd_LinuxFBBitmap_FBDevInfo;
            tags[1].ti_Data = (IPTR)&data->fbdevinfo;
        }

        p.mID = msg->mID;
        p.attrList = tags;

        msg = &p;
    }

    return (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/******* FBGfx::Set()  ********************************************/
VOID LinuxFB__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;
    
    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_NoFrameBuffer:
            *msg->storage = TRUE;
            return;
        case aoHidd_Gfx_SupportsHWCursor:
            *msg->storage = (IPTR)TRUE;
            return;
        case aoHidd_Gfx_HWSpriteTypes:
            *msg->storage = vHidd_SpriteType_DirectColor;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

static BOOL setup_linuxfb(struct LinuxFB_staticdata *fsd, int fbdev, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi)
{
    int r1, r2;
    int fd;

    fd = Hidd_UnixIO_OpenFile(fsd->unixio, "/dev/tty0", O_RDWR, 0, NULL);
    if (fd != -1)
    {
        /* Switch console into gfx mode, no more console output */
        Hidd_UnixIO_IOControlFile(fsd->unixio, fd, KDSETMODE, (void *)KD_GRAPHICS, NULL);
        Hidd_UnixIO_IOControlFile(fsd->unixio, fd, KDSKBMODE, K_RAW, NULL);
        Hidd_UnixIO_CloseFile(fsd->unixio, fd, NULL);
    }

    r1 = Hidd_UnixIO_IOControlFile(fsd->unixio, fbdev, FBIOGET_FSCREENINFO, fsi, NULL);
    r2 = Hidd_UnixIO_IOControlFile(fsd->unixio, fbdev, FBIOGET_VSCREENINFO, vsi, NULL);

    if (r1 == -1)
    {
        D(kprintf("!!! COULD NOT GET FIXED SCREEN INFO !!!\n"));
        return FALSE;
    }

    if (r2 == -1)
    {
        D(kprintf("!!! COULD NOT GET FIXED SCREEN INFO !!!\n"));
        return FALSE;
    }

    D(kprintf("FB: Width: %d, height: %d, line length=%d\n",
              vsi->xres, vsi->yres, fsi->line_length));

    return TRUE;
}

static VOID cleanup_linuxfb(struct LinuxFB_data *data, struct LinuxFB_staticdata *fsd)
{
    Hidd_UnixIO_MemoryUnMap(fsd->unixio, data->fbdevinfo.baseaddr, data->mem_len, NULL);
    Hidd_UnixIO_CloseFile(fsd->unixio, data->fbdev, NULL);
}

static HIDDT_Pixel bitfield2mask(struct fb_bitfield *bf)
{
#if 0
     return ((1L << (bf->offset)) - 1)  - ((1L << (bf->offset - bf->length)) - 1);
#else
     return ((1L << bf->length) - 1) << bf->offset;
#endif

}

static ULONG bitfield2shift(struct fb_bitfield *bf)
{
     int shift;
     
     shift = 32 - (bf->offset + bf->length);
     if (shift == 32)
         shift = 0;
     
     return shift;
}

#ifdef DEBUG_PF

static void print_bitfield(const char *color, struct fb_bitfield *bf)
{
    kprintf("FB: Bitfield %s: %d, %d, %d\n"
    , color, bf->offset, bf->length, bf->msb_right);
}

#else

#define print_bitfield(color, bf)

#endif

static BOOL get_pixfmt(struct TagItem *pftags, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi)
{   
    BOOL success = TRUE;

    pftags[9 ].ti_Data = vsi->bits_per_pixel;            /* Depth        */
    pftags[10].ti_Data = ((vsi->bits_per_pixel - 1) / 8) + 1;    /* Bytes per pixel    */
    pftags[11].ti_Data = vsi->bits_per_pixel;            /* Size            */

    print_bitfield("red",    &vsi->red);
    print_bitfield("green",  &vsi->green);
    print_bitfield("blue",   &vsi->blue);
    print_bitfield("transp", &vsi->transp);

    switch (fsi->visual)
    {
    case FB_VISUAL_TRUECOLOR:
    case FB_VISUAL_DIRECTCOLOR:
        pftags[0].ti_Data = bitfield2shift(&vsi->red);    /* Shifts: R, G, B, A */
        pftags[1].ti_Data = bitfield2shift(&vsi->green);
        pftags[2].ti_Data = bitfield2shift(&vsi->blue);
        pftags[3].ti_Data = bitfield2shift(&vsi->transp);

        pftags[4].ti_Data = bitfield2mask(&vsi->red);    /* Masks: R, G, B, A */
        pftags[5].ti_Data = bitfield2mask(&vsi->green);
        pftags[6].ti_Data = bitfield2mask(&vsi->blue);
        pftags[7].ti_Data = bitfield2mask(&vsi->transp);

        pftags[8].ti_Data = vHidd_ColorModel_TrueColor;
        break;
    
    case FB_VISUAL_PSEUDOCOLOR:
#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
        /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
                    screens expect DisplayInfo->redbits/greenbits/bluebits to have
            correct values (they are calculated based on the red/green/blue masks)
            which reflect the "size" of the palette (16M, 4096, 262144) */
        
        pftags[4 ].ti_Data = bitfield2mask(&vsi->red);    /* Masks: R, G, B, A */
        pftags[5 ].ti_Data = bitfield2mask(&vsi->green);
        pftags[6 ].ti_Data = bitfield2mask(&vsi->blue);
                
        pftags[8 ].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = 0;                /* LUT shift */
        pftags[14].ti_Data = 0xFF;                /* LUT mask  */
        break;
    
     case FB_VISUAL_STATIC_PSEUDOCOLOR:
#warning "also pseudocolor pixelformats need red/green/blue masks now. Is the calc. correct here!?"
        /* stegerg: apps when using GetDisplayInfoData(DTA_DISP) even on 8 bit palettized
                    screens expect DisplayInfo->redbits/greenbits/bluebits to have
            correct values (they are calculated based on the red/green/blue masks)
            which reflect the "size" of the palette (16M, 4096, 262144) */
        
        pftags[4 ].ti_Data = bitfield2mask(&vsi->red);    /* Masks: R, G, B, A */
        pftags[5 ].ti_Data = bitfield2mask(&vsi->green);
        pftags[6 ].ti_Data = bitfield2mask(&vsi->blue);
        pftags[8 ].ti_Data = vHidd_ColorModel_StaticPalette;
        pftags[13].ti_Data = 0;                /* LUT shift */
        pftags[14].ti_Data = 0xFF;                /* LUT mask  */
        break;
    
/*        case FB_VISUAL_MONO01:
        case FB_VISUAL_MONO10: */
    default:
        D(kprintf("!!! FB: UNHANDLED GRAPHTYPE :%d !!!\n", fsi->visual));
        return FALSE;
    }

    D(kprintf("FB;  mask: (%p, %p, %p, %p), shift: (%ld, %ld, %ld, %ld)\n",
          pftags[4].ti_Data, pftags[5].ti_Data, pftags[6].ti_Data, pftags[7].ti_Data,
          pftags[0].ti_Data, pftags[1].ti_Data, pftags[2].ti_Data, pftags[3].ti_Data));

    switch (fsi->type)
    {
    case FB_TYPE_PACKED_PIXELS:
        pftags[15].ti_Data = vHidd_BitMapType_Chunky;
        break;

    case FB_TYPE_PLANES:
        pftags[15].ti_Data = vHidd_BitMapType_Planar;
        break;

    case FB_TYPE_INTERLEAVED_PLANES:
        pftags[15].ti_Data = vHidd_BitMapType_InterleavedPlanar;
        break;

    default:
        D(kprintf("!!! UNSUPPORTED FRAMEBUFFER TYPE: %d !!!\n", fsi->type));
        success = FALSE;
        break;
    }

    return success;    
}

#define writel(val, addr)           (*(volatile ULONG*)(addr) = (val))
#define writeb(val, addr)           (*(volatile UBYTE*)(addr) = (val))
#define readl(addr)                 (*(volatile ULONG*)(addr))
#define readb(addr)                 (*(volatile UBYTE*)(addr))

static inline int do_alpha(int a, int v)
{
    int tmp = a*v;
    return ((tmp << 8) + tmp + 32768) >> 16;
}

static VOID HIDDLInuxFBPutAlphaImage32ToFBDev(APTR srcbuff, ULONG srcpitch, struct FBDevInfo * fbdevinfo,
        LONG destX, LONG destY, LONG width, LONG height)
{
    /* TODO: what about 16bpp modes ? */

    LONG x,y;
    APTR dstbuff    = fbdevinfo->baseaddr;
    ULONG dstpitch  = fbdevinfo->pitch;
    UBYTE dstbpp    = fbdevinfo->bpp;

    for(y = 0; y < height; y++)
    {
        /* Calculate line start addresses */
        IPTR srcaddr = (srcpitch * y) + (IPTR)srcbuff;
        IPTR destaddr = (destX * dstbpp) + (dstpitch * (destY + y)) + (IPTR)dstbuff;

        for (x = 0; x < width; x++)
        {
            ULONG       srcpix;
            LONG        src_red, src_green, src_blue, src_alpha;
            LONG        dst_red, dst_green, dst_blue;

            /* Read RGBA pixel from input array */
            srcpix = *(ULONG *)srcaddr;
#if AROS_BIG_ENDIAN
            src_red   = (srcpix & 0x00FF0000) >> 16;
            src_green = (srcpix & 0x0000FF00) >> 8;
            src_blue  = (srcpix & 0x000000FF);
            src_alpha = (srcpix & 0xFF000000) >> 24;
#else
            src_red   = (srcpix & 0x0000FF00) >> 8;
            src_green = (srcpix & 0x00FF0000) >> 16;
            src_blue  = (srcpix & 0xFF000000) >> 24;
            src_alpha = (srcpix & 0x000000FF);
#endif

            /*
            * If alpha=0, do not change the destination pixel at all.
            * This saves us unnecessary reads and writes to VRAM.
            */
            if (src_alpha != 0)
            {
                /*
                * Full opacity. Do not read the destination pixel, as
                * it's value does not matter anyway.
                */
                if (src_alpha == 0xff)
                {
                    dst_red = src_red;
                    dst_green = src_green;
                    dst_blue = src_blue;
                }
                else
                {
                    /*
                    * Alpha blending with source and destination pixels.
                    * Get destination.
                    */

                    if (dstbpp == 3)
                    {
                        dst_blue  = readb(destaddr);
                        dst_green = readb(destaddr + 1);
                        dst_red   = readb(destaddr + 2);
                    }


                    if (dstbpp == 4)
                    {
                        ULONG destpix = readl(destaddr);
                        dst_red   = (destpix & 0x00FF0000) >> 16;
                        dst_green = (destpix & 0x0000FF00) >> 8;
                        dst_blue  = (destpix & 0x000000FF);
                    }

                    dst_red   += do_alpha(src_alpha, src_red - dst_red);
                    dst_green += do_alpha(src_alpha, src_green - dst_green);
                    dst_blue  += do_alpha(src_alpha, src_blue - dst_blue);
                }

                /* Store the new pixel */

                if (dstbpp == 3)
                {
                    writeb(dst_blue , destaddr);
                    writeb(dst_green, destaddr + 1);
                    writeb(dst_red  , destaddr + 2);
                }

                if (dstbpp == 4)
                {
                    ULONG destpix = (dst_red << 16) + (dst_green << 8) + (dst_blue);
                    writel(destpix, destaddr);
                }
            }



            /* Advance pointers */
            srcaddr += CURSOR_IMAGE_BPP;
            destaddr += dstbpp;
        }
    }
}

#define X_RES (data->fbdevinfo.xres)
#define Y_RES (data->fbdevinfo.yres)

static VOID FBImage_FillFromMemory(struct FBImage * fbimage, APTR src, LONG srcpitch, LONG x, LONG y,
        LONG width, LONG height)
{
    LONG yi;
    src = src + y * srcpitch + x * fbimage->bpp;
    APTR dst = fbimage->buffer;

    for (yi = 0; yi < height; yi++)
    {
        CopyMem(src, dst, width * fbimage->bpp);

        src += srcpitch;
        dst += fbimage->width * fbimage->bpp;
    }

}

static VOID FBImage_FillFromFBDev(struct FBImage * fbimage, struct FBDevInfo * fbdevinfo, LONG x, LONG y,
        LONG width, LONG height)
{
    FBImage_FillFromMemory(fbimage, fbdevinfo->baseaddr, fbdevinfo->pitch, x, y, width, height);
}

static VOID FBImage_CopyToFBDev(struct FBImage * fbimage, struct FBDevInfo * fbdevinfo, LONG x, LONG y,
        LONG width, LONG height)
{
    LONG yi;
    APTR src = fbimage->buffer;
    APTR dst = fbdevinfo->baseaddr + y * fbdevinfo->pitch + x * fbimage->bpp;

    for (yi = 0; yi < height; yi++)
    {
        CopyMem(src, dst, width * fbimage->bpp);

        src += fbimage->width * fbimage->bpp;
        dst += fbdevinfo->pitch;
    }
}

static VOID FBImage_FreeData(struct FBImage * fbimage)
{
    FreeVec(fbimage->buffer);
    fbimage->width  = -1;
    fbimage->height = -1;
}

static VOID FBImage_Recreate(struct FBImage * fbimage, LONG width, LONG height, BYTE bpp)
{
    if ((!fbimage->buffer) || (fbimage->width != width) || (fbimage->height != height) || (fbimage->bpp != bpp))
    {
        FreeVec(fbimage->buffer);
        fbimage->buffer = AllocVec(width * height * CURSOR_IMAGE_BPP, MEMF_PUBLIC);
        fbimage->width  = width;
        fbimage->height = height;
        fbimage->bpp    = bpp;
    }
}

static void HIDDLinuxFBRedrawCursorAt(OOP_Object * gfx, LONG x, LONG y)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct LinuxFB_data * data = OOP_INST_DATA(cl, gfx);

    LONG width = data->cinfo.img.width;
    LONG height = data->cinfo.img.height;

    /* Clip drawing to screen size */
    if (x >= X_RES) return;
    if (y >= Y_RES) return;
    if (x + width >= X_RES) width = X_RES - x;
    if (y + height >= Y_RES) height = Y_RES - y;

    /* FIXME: the redraw code will crash for nagative values */
    if (x < 0) x = 0;
    if (y < 0) y = 0;

    /* Restore any saved fb part */
    if (data->sfb.active)
    {
        FBImage_CopyToFBDev(&data->sfb.img, &data->fbdevinfo, data->sfb.x, data->sfb.y, data->sfb.width,
                data->sfb.height);
        data->sfb.active = FALSE;
    }

    /* If cursor is visible and there is cursor image, store fb part and draw cursor */
    if (data->cinfo.visible && data->cinfo.img.buffer)
    {
        /* Store fb part */
        FBImage_Recreate(&data->sfb.img, data->cinfo.img.width, data->cinfo.img.height, data->fbdevinfo.bpp);
        FBImage_FillFromFBDev(&data->sfb.img, &data->fbdevinfo, x, y, width, height);
        data->sfb.x         = x;
        data->sfb.y         = y;
        data->sfb.width     = width;
        data->sfb.height    = height;
        data->sfb.active    = TRUE;

        /* Draw cursor */
        HIDDLInuxFBPutAlphaImage32ToFBDev(data->cinfo.img.buffer, data->cinfo.img.width * CURSOR_IMAGE_BPP,
                &data->fbdevinfo, x, y, width, height);
    }
}

static void HIDDLinuxFBShowCursor(OOP_Object * gfx, BOOL visible)
{
    OOP_Class * cl = OOP_OCLASS(gfx);
    struct LinuxFB_data * gfxdata = OOP_INST_DATA(cl, gfx);

    gfxdata->cinfo.visible = visible;

    HIDDLinuxFBRedrawCursorAt(gfx, gfxdata->cinfo.currentx, gfxdata->cinfo.currenty);
}


#if AROS_BIG_ENDIAN
#define Machine_BGRA32 vHidd_StdPixFmt_BGRA32
#else
#define Machine_BGRA32 vHidd_StdPixFmt_ARGB32
#endif

BOOL METHOD(LinuxFB, Hidd_Gfx, SetCursorShape)
{
    struct LinuxFB_data * gfxdata = OOP_INST_DATA(cl, o);

    if (msg->shape == NULL)
    {
        /* Hide cursor */
        HIDDLinuxFBShowCursor(o, FALSE);
        FBImage_FreeData(&gfxdata->cinfo.img);
    }
    else
    {
        IPTR width, height;

        OOP_GetAttr(msg->shape, aHidd_BitMap_Width, &width);
        OOP_GetAttr(msg->shape, aHidd_BitMap_Height, &height);

        if (width > 64) width = 64;
        if (height > 64) height = 64;

        FBImage_Recreate(&gfxdata->cinfo.img, width, height, CURSOR_IMAGE_BPP);

        /* Get data from the bitmap */
        HIDD_BM_GetImage(msg->shape, (UBYTE *)gfxdata->cinfo.img.buffer, width * CURSOR_IMAGE_BPP, 0, 0,
            width, height, Machine_BGRA32);

        /* Show updated cursor */
        HIDDLinuxFBShowCursor(o, TRUE);
    }

    return TRUE;
}

BOOL METHOD(LinuxFB, Hidd_Gfx, SetCursorPos)
{
    struct LinuxFB_data * gfxdata = OOP_INST_DATA(cl, o);

    HIDDLinuxFBRedrawCursorAt(o, msg->x, msg->y);

    gfxdata->cinfo.currentx = msg->x;
    gfxdata->cinfo.currenty = msg->y;

    return TRUE;
}

VOID METHOD(LinuxFB, Hidd_Gfx, SetCursorVisible)
{
    HIDDLinuxFBShowCursor(o, msg->visible);
}

/* This method is a hack to allow refreshing "saved" area when it changed, coordinates are in screen, not bitmap space */
VOID METHOD(LinuxFB, Hidd_LinuxFB, FBChanged)
{
    struct LinuxFB_data * gfxdata = OOP_INST_DATA(cl, o);

    struct SavedFB * sfb = &gfxdata->sfb;

    /* Check if change overlapped with saved fb part, if yes re-read it from source (not FB) and redraw cursor */
    if (sfb->x + sfb->width < msg->x)
        return;
    if (sfb->x > msg->x + msg->width)
        return;
    if (sfb->y + sfb->height < msg->y)
        return;
    if (sfb->y > msg->y + msg->height)
        return;

    if (sfb->active)
        FBImage_FillFromMemory(&sfb->img, msg->src, msg->srcpitch, sfb->x, sfb->y, sfb->width, sfb->height);

    HIDDLinuxFBRedrawCursorAt(o, gfxdata->cinfo.currentx, gfxdata->cinfo.currenty);
}
