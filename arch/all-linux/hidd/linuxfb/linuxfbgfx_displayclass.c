/*
    Copyright (C) 2016-2026, The AROS Development Team. All rights reserved.

    Desc: Display class for Linux fbdev Gfx Hidd.
    Lang: English.
*/

#define DEBUG 0

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <hidd/unixio.h>

#define __OOP_NOATTRBASES__

#include "linuxfbgfx_intern.h"
#include "linuxfbgfx_bitmap.h"
#include "linuxfbgfx_display.h"

#include LC_LIBDEFS_FILE

/* *********************** Pixelformat detection helpers ********************* */

static HIDDT_Pixel bitfield2mask(struct fb_bitfield *bf)
{
     return ((1L << bf->length) - 1) << bf->offset;
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

    default:
        D(kprintf("!!! FB: UNHANDLED GRAPHTYPE :%d !!!\n", fsi->visual));
        return FALSE;
    }

    D(kprintf("FB;  mask: (%p, %p, %p, %p), shift: (%ld, %ld, %ld, %ld)\n",
          (APTR)pftags[4].ti_Data, (APTR)pftags[5].ti_Data, (APTR)pftags[6].ti_Data, (APTR)pftags[7].ti_Data,
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

/* *********************** Display class *********************** */

OOP_Object *LinuxFBDisplay__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
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
        {aHidd_Sync_Description, (IPTR)"FBDev:%hx%v"}, /* 0 */
        {aHidd_Sync_HDisp      , 0                  }, /* 1 */
        {aHidd_Sync_VDisp      , 0                  }, /* 2 */
        {aHidd_Sync_LeftMargin , 0                  }, /* 3 */
        {aHidd_Sync_RightMargin, 0                  }, /* 4 */
        {aHidd_Sync_HSyncLength, 0                  }, /* 5 */
        {aHidd_Sync_UpperMargin, 0                  }, /* 6 */
        {aHidd_Sync_LowerMargin, 0                  }, /* 7 */
        {aHidd_Sync_VSyncLength, 0                  }, /* 8 */
        {aHidd_Sync_PixelTime  , 0                  }, /* 9 */
        {TAG_DONE              , 0                  }
    };

    struct TagItem modetags[] =
    {
        { aHidd_DMEnum_PixFmtTags  , (IPTR)pftags   },
        { aHidd_DMEnum_SyncTags    , (IPTR)synctags },
        { TAG_DONE                 , 0              }
    };

    struct TagItem dispTags[] =
    {
        {aHidd_Display_ModeTags, (IPTR)modetags},
        {TAG_MORE, 0UL}
    };
    struct pRoot_New newdispMsg;
    struct fb_fix_screeninfo *fsi;
    struct fb_var_screeninfo *vsi;

    D(bug("[LinuxFB:Display] %s()\n", __func__));

    /* Do not allow more than one object instance to be created */
    if (LSD(cl)->linuxfbdisplay)
        return NULL;

    fsi = (struct fb_fix_screeninfo *)GetTagData(aHidd_LinuxFB_FSI, (IPTR)NULL, msg->attrList);
    vsi = (struct fb_var_screeninfo *)GetTagData(aHidd_LinuxFB_VSI, (IPTR)NULL, msg->attrList);

    if ((fsi) && (vsi) && (get_pixfmt(pftags, fsi, vsi)))
    {
        /*
         * Set the gfxmode info.
         * Some devices report all zeroes for clock information (LCD on AspireOne).
         * This makes sync class crash with division by zero error, additionally
         * this would create garbage SpecialMonitor structure.
         * Here we attempt to detect such devices by checking pixclock against
         * being zero.
         */
        synctags[1].ti_Data = vsi->xres;
        synctags[2].ti_Data = vsi->yres;

        if (vsi->pixclock)
        {
            synctags[3].ti_Data = vsi->left_margin;
            synctags[4].ti_Data = vsi->right_margin;
            synctags[5].ti_Data = vsi->hsync_len;
            synctags[6].ti_Data = vsi->upper_margin;
            synctags[7].ti_Data = vsi->lower_margin;
            synctags[8].ti_Data = vsi->vsync_len;
            synctags[9].ti_Data = vsi->pixclock;
        }
        else
        {
            /* Do not supply analog signal information, we have no analog signals */
            synctags[3].ti_Tag = TAG_DONE;
        }

        dispTags[1].ti_Data = (IPTR)msg->attrList;
        newdispMsg.mID = msg->mID;
        newdispMsg.attrList = dispTags;

        o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&newdispMsg);
        if (o)
        {
            struct LinuxFBDisplayData *data = OOP_INST_DATA(cl, o);

            data->basebm = OOP_FindClass(CLID_Hidd_BitMap);
            data->fbdevinfo = (struct FBDevInfo *)GetTagData(aHidd_LinuxFB_DevInfo, (IPTR)NULL, msg->attrList);
            data->gamma = (fsi->visual == FB_VISUAL_DIRECTCOLOR) ? TRUE : FALSE;

            /*
             * Record the detected colour model in the shared FBDevInfo so that
             * CreateObject() can tell palette framebuffers apart. (The original
             * branch left this disabled, breaking palette detection.)
             */
            if (data->fbdevinfo)
                data->fbdevinfo->fbtype = pftags[8].ti_Data;

            D(bug("[LinuxFB:Display] Gamma support: %d\n", data->gamma));

            if (data->gamma)
            {
                /*
                 * Some explanations of this magic.
                 * In 16-bit modes we have different number of gradations per component.
                 * For example, R5G6B5 layout assumes that we have 64 gradations for
                 * G and only 32 gradations for R and B.
                 * Consequently, gamma table for G is twice longer. But framebuffer API
                 * allows to load only complete triplets.
                 * Hence we have to scale down our gamma table supplied by the OS, which
                 * always contains 256 values three times, with different scale factor for
                 * each color.
                 */
                UBYTE bits = 0;

                if (vsi->red.length   > bits) bits = vsi->red.length;
                if (vsi->green.length > bits) bits = vsi->green.length;
                if (vsi->blue.length  > bits) bits = vsi->blue.length;

                D(bug("Initializing gamma table scaler down to %d bits\n", bits));
                /* Determine how many triplets we will load (the longest table) */
                data->scale_size = 1 << bits;

                /* And then - scale factor for each component */
                data->r_step = 0x100 >> vsi->red.length;
                data->g_step = 0x100 >> vsi->green.length;
                data->b_step = 0x100 >> vsi->blue.length;

                D(bug("Steps R G B: %d %d %d\n", data->r_step, data->g_step, data->b_step));
            }
        }
    }
    D(bug("[LinuxFB:Display] %s: obj @ 0x%p\n", __func__, o));
    return o;
}

VOID LinuxFBDisplay__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    struct LinuxFBDisplayData *data = OOP_INST_DATA(cl, o);
    ULONG idx;

    Hidd_Display_Switch (msg->attrID, idx)
    {
    case aoHidd_Display_SupportsGamma:
        *msg->storage = (IPTR)data->gamma;
        return;
    }

    OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
}

/********** Display::CreateObject()  ****************************/

OOP_Object *LinuxFBDisplay__Hidd_Display__CreateObject(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_CreateObject *msg)
{
    struct LinuxFBDisplayData *data = OOP_INST_DATA(cl, o);
    OOP_Object      *object = NULL;

    if (msg->cl == data->basebm)
    {
        BOOL fb = GetTagData(aHidd_BitMap_FrameBuffer, FALSE, msg->attrList);

        D(bug("[LinuxFB:Display] %s: framebuffer=%d\n", __func__, fb));
        /*
         * Our framebuffer is a chunky bitmap at predetermined address.
         * We don't care about friends etc here, because even if our
         * class is selected for friend bitmap, it's completely safe
         * because it will not get FBDevInfo. Storage overhead per
         * bitmap is extremely small here (just 8 bytes).
         */
        if (fb)
        {
            struct TagItem tags[] =
            {
                {aHidd_BitMap_ClassPtr        , (IPTR)LSD(cl)->bmclass         },
                {aHidd_BitMap_BytesPerRow     , data->fbdevinfo->pitch         },
                {aHidd_ChunkyBM_Buffer        , (IPTR)data->fbdevinfo->baseaddr},
                {aHidd_LinuxFBBitmap_FBDevInfo, -1                             },
                {TAG_MORE                     , (IPTR)msg->attrList            }
            };
            struct pHidd_Display_CreateObject p =
            {
                msg->mID,
                msg->cl,
                tags
            };

            if (data->fbdevinfo->fbtype == vHidd_ColorModel_Palette)
            {
                tags[3].ti_Data = data->fbdevinfo->fbdev;
            }

            if (data->fbdevinfo->confd == -1)
            {
                /*
                 * Switch console into gfx mode, no more console output
                 * FIXME: How to determine which console is connected to this framebuffer ?
                 */
                data->fbdevinfo->confd = Hidd_UnixIO_OpenFile(LSD(cl)->unixio, "/dev/tty0", O_RDWR, 0, NULL);
                if (data->fbdevinfo->confd != -1)
                {
                    Hidd_UnixIO_IOControlFile(LSD(cl)->unixio, data->fbdevinfo->confd, KDGKBMODE, &data->fbdevinfo->kbmode, NULL);
                    Hidd_UnixIO_IOControlFile(LSD(cl)->unixio, data->fbdevinfo->confd, KDSETMODE, (void *)KD_GRAPHICS, NULL);
                    Hidd_UnixIO_IOControlFile(LSD(cl)->unixio, data->fbdevinfo->confd, KDSKBMODE, K_RAW, NULL);
                }
            }

            object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&p);
        }
        else
            object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);
    }
    else
        object = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)msg);

    return object;
}

/********** Display::SetGamma()  ****************************/

BOOL LinuxFBDisplay__Hidd_Display__SetGamma(OOP_Class *cl, OOP_Object *o, struct pHidd_Display_GetGamma *msg)
{
    struct LinuxFBDisplayData *data = OOP_INST_DATA(cl, o);

    if (data->gamma)
    {
        struct LinuxFB_staticdata *fsd = LSD(cl);
        UWORD r, g, b, i;
        struct fb_cmap col =
        {
            0, 1, &r, &g, &b, NULL
        };
        UBYTE ri = data->r_step - 1;
        UBYTE gi = data->g_step - 1;
        UBYTE bi = data->b_step - 1;

        D(bug("[LinuxFB:Display]  N  R  G  B gamma tables:\n"));
        for (i = 0; i < data->scale_size; i++)
        {
            col.start = i;

            r = msg->Red[ri]   << 8;
            g = msg->Green[gi] << 8;
            b = msg->Blue[bi]  << 8;
            D(bug("[LinuxFB:Display] %02X %02X %02X %02X\n", i, msg->Red[ri], msg->Green[gi], msg->Blue[bi]));

            /*
             * Wraparound here is intentional. It prevents from buffer overflow.
             * Example: RGB 565 format. In this case we load 64 triplets. But
             * only G channel will actually use 64 values, R and B will use only
             * 32 values. So, we scale down 256 values to 32, but the rest 32
             * unused values also need to be picked up from somewhere.
             * Alternatively we could add some conditions, but it would be slower.
             */
            ri += data->r_step;
            gi += data->g_step;
            bi += data->b_step;

            Hidd_UnixIO_IOControlFile(fsd->unixio, data->fbdevinfo->fbdev, FBIOPUTCMAP, &col, NULL);
        }
        return TRUE;
    }

    return FALSE;
}
