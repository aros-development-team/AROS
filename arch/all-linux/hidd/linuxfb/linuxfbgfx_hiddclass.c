/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: Linux fbdev Gfx Hidd for AROS.
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
#include <hidd/gfx.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/utility.h>
#include <hidd/unixio.h>

#include "linuxfbgfx_intern.h"
#include "linuxfbgfx_bitmap.h"

#define CURSOR_IMAGE_BPP    (4)

#include LC_LIBDEFS_FILE

static BOOL setup_linuxfb(struct LinuxFB_staticdata *fsd, int fbdev,
              struct fb_fix_screeninfo *fsi, struct fb_var_screeninfo *vsi);
static VOID cleanup_linuxfb(struct LinuxFB_data *data, struct LinuxFB_staticdata *fsd);

static AROS_INTH1(ResetHandler, struct LinuxFB_data *, data)
{
    AROS_INTFUNC_INIT

    if (data->fbdevinfo.confd != -1)
    {
        /* Enable console and restore keyboard mode */
        Hidd_UnixIO_IOControlFile(data->unixio, data->fbdevinfo.confd, KDSETMODE, (void *)KD_TEXT, NULL);
        Hidd_UnixIO_IOControlFile(data->unixio, data->fbdevinfo.confd, KDSKBMODE, (void *)data->fbdevinfo.kbmode, NULL);
    }

    return FALSE;

    AROS_INTFUNC_EXIT
}

/***************** FBGfx::New() ***********************/

OOP_Object *LinuxFBGfx__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct LinuxFB_staticdata *fsd = LSD(cl);
    struct fb_fix_screeninfo fsi;
    struct fb_var_screeninfo vsi;
    int fbdev = GetTagData(aHidd_LinuxFB_File, -1, msg->attrList);
    char *baseaddr = MAP_FAILED;

    if (fbdev == -1)
    {
        D(bug("[LinuxFB] No file descriptor supplied in New()\n"));
        return NULL;
    }

    /* Do GfxHidd initalization here */
    if (setup_linuxfb(LSD(cl), fbdev, &fsi, &vsi))
    {
        /* Memorymap the framebuffer using mmap() */
        baseaddr = Hidd_UnixIO_MemoryMap(fsd->unixio, NULL, fsi.smem_len, PROT_READ | PROT_WRITE, MAP_SHARED, fbdev, 0, NULL);

        D(bug("[LinuxFB] Mapped at 0x%p\n", baseaddr));
        if (baseaddr != MAP_FAILED)
        {
            /*
             * The display class builds the pixelformat/sync mode descriptions from
             * the FSI/VSI we pass below; here we only advertise the framebuffer type.
             */
            struct TagItem mytags[] =
            {
                { aHidd_Gfx_FrameBufferType, vHidd_FrameBuffer_Mirrored },
                { TAG_MORE                 , (IPTR)msg->attrList        }
            };

            struct pRoot_New mymsg =
            {
                msg->mID,
                mytags
            };

            o = (OOP_Object *)OOP_DoSuperMethod(cl, o, &mymsg.mID);
            if (NULL != o)
            {
                struct LinuxFB_data *data = OOP_INST_DATA(cl, o);
                struct TagItem displaytags[] =
                {
                    {aHidd_Display_GfxHidd,     (IPTR)o                 },
                    {aHidd_LinuxFB_DevInfo,     (IPTR)&data->fbdevinfo  },
                    {aHidd_LinuxFB_FSI,         (IPTR)&fsi              },
                    {aHidd_LinuxFB_VSI,         (IPTR)&vsi              },
                    {TAG_DONE,                  0                       }
                };

                data->basebm            = OOP_FindClass(CLID_Hidd_BitMap);
                data->fbdevinfo.fbdev    = fbdev;
                data->fbdevinfo.baseaddr = baseaddr;
                data->fbdevinfo.pitch    = fsi.line_length;
                data->mem_len            = fsi.smem_len;
                data->fbdevinfo.bpp      = ((vsi.bits_per_pixel - 1) / 8) + 1;
                data->fbdevinfo.xres     = vsi.xres;
                data->fbdevinfo.yres     = vsi.yres;
                data->fbdevinfo.confd    = -1;

                data->unixio = fsd->unixio;
                data->gamma = (fsi.visual == FB_VISUAL_DIRECTCOLOR) ? TRUE : FALSE;
                D(bug("[LinuxFB] Gamma support: %d\n", data->gamma));

                /* Create the display object, handing it the framebuffer info */
                LSD(cl)->linuxfbdisplay = OOP_NewObject(LSD(cl)->displayclass, NULL, displaytags);
                if (LSD(cl)->linuxfbdisplay)
                {
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
VOID LinuxFBGfx__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    struct LinuxFB_data *data = OOP_INST_DATA(cl, o);

    RemResetCallback(&data->resetHandler);
    cleanup_linuxfb(data, LSD(cl));

    OOP_DoSuperMethod(cl, o, msg);
}

BOOL LinuxFBGfx__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    Hidd_Gfx_Switch (msg->attrID, idx)
    {
    case aoHidd_Gfx_DisplayDefault:
        *msg->storage = (IPTR)LSD(cl)->linuxfbdisplay;
        return TRUE;
    }

    return OOP_DoSuperMethod(cl, o, &msg->mID);
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

