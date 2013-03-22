/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
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

static AROS_INTH1(ResetHandler, struct LinuxFB_data *, data)
{
    AROS_INTFUNC_INIT

    if (data->confd != -1)
    {
        /* Enable console and restore keyboard mode */
        Hidd_UnixIO_IOControlFile(data->unixio, data->confd, KDSETMODE, (void *)KD_TEXT, NULL);
        Hidd_UnixIO_IOControlFile(data->unixio, data->confd, KDSKBMODE, (void *)data->kbmode, NULL);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

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

                    data->fbdevinfo.fbdev    = fbdev;
                    data->fbdevinfo.fbtype   = pftags[8].ti_Data;
                    data->fbdevinfo.baseaddr = baseaddr;
                    data->fbdevinfo.pitch    = fsi.line_length;
                    data->mem_len            = fsi.smem_len;
                    data->fbdevinfo.bpp      = ((vsi.bits_per_pixel - 1) / 8) + 1;
                    data->fbdevinfo.xres     = vsi.xres;
                    data->fbdevinfo.yres     = vsi.yres;

                    InitSemaphore(&data->framebufferlock);

                    data->confd = -1;
                    data->unixio = fsd->unixio;

                    data->resetHandler.is_Code = (VOID_FUNC)ResetHandler;
                    data->resetHandler.is_Data = data;
                    AddResetCallback(&data->resetHandler);

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
    struct LinuxFB_data *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->resetHandler);
    cleanup_linuxfb(data, LSD(cl));

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
        }
    }
    OOP_DoSuperMethod(cl, o, &msg->mID);
}

OOP_Object *LinuxFB__Hidd_Gfx__Show(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_Show *msg)
{
    struct LinuxFB_data *data = OOP_INST_DATA(cl, o);
    struct TagItem tags[] =
    {
	{aHidd_BitMap_Visible, FALSE},
	{TAG_DONE	     , 0    }
    };

    ObtainSemaphore(&data->framebufferlock);

    if (data->visible)
	OOP_SetAttrs(data->visible, tags);

    if (msg->bitMap)
    {
        if (data->confd == -1)
        {
            /*
             * Switch console into gfx mode, no more console output
             * FIXME: How to determine which console is connected to this framebuffer ?
             */
            data->confd = Hidd_UnixIO_OpenFile(data->unixio, "/dev/tty0", O_RDWR, 0, NULL);
            if (data->confd != -1)
            {
                Hidd_UnixIO_IOControlFile(data->unixio, data->confd, KDGKBMODE, &data->kbmode, NULL);
                Hidd_UnixIO_IOControlFile(data->unixio, data->confd, KDSETMODE, (void *)KD_GRAPHICS, NULL);
                Hidd_UnixIO_IOControlFile(data->unixio, data->confd, KDSKBMODE, K_RAW, NULL);
            }
        }

	tags[0].ti_Data = TRUE;
	OOP_SetAttrs(msg->bitMap, tags);

        if (data->fbdevinfo.fbtype == vHidd_ColorModel_Palette)
        {
            OOP_Object *cm = NULL;

            OOP_GetAttr(msg->bitMap, aHidd_BitMap_ColorMap, (IPTR *)&cm);
            if (cm)
            {
                IPTR n = 0;
                unsigned int i;
                HIDDT_Color col;
                struct fb_cmap fbcol =
                {
                    0, 1,
                    &col.red,
                    &col.green,
                    &col.blue,
                    &col.alpha
                };

                OOP_GetAttr(cm, aHidd_ColorMap_NumEntries, &n);

                for (i = 0; i < n; i++)
                {
                    fbcol.start = i;
                    HIDD_CM_GetColor(cm, i, &col);
                    Hidd_UnixIO_IOControlFile(data->unixio, data->fbdevinfo.fbdev, FBIOPUTCMAP, &fbcol, NULL);
                }
            }
        }
    }
    else
    {
        memset(data->fbdevinfo.baseaddr, 0, data->fbdevinfo.pitch * data->fbdevinfo.yres);
    }

    data->visible = msg->bitMap;
    ReleaseSemaphore(&data->framebufferlock);

    return msg->bitMap;
}


static BOOL setup_linuxfb(struct LinuxFB_staticdata *fsd, int fbdev, struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi)
{
    int r1, r2;

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
    Hidd_UnixIO_CloseFile(fsd->unixio, data->fbdevinfo.fbdev, NULL);
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
        pftags[4 ].ti_Data = bitfield2mask(&vsi->red);    /* Masks: R, G, B, A */
        pftags[5 ].ti_Data = bitfield2mask(&vsi->green);
        pftags[6 ].ti_Data = bitfield2mask(&vsi->blue);
                
        pftags[8 ].ti_Data = vHidd_ColorModel_Palette;
        pftags[13].ti_Data = 0;                /* LUT shift */
        pftags[14].ti_Data = 0xFF;                /* LUT mask  */
        break;
    
     case FB_VISUAL_STATIC_PSEUDOCOLOR:        
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
