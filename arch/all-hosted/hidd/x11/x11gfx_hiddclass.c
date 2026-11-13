/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.

    Desc: X11 gfx HIDD for AROS.
*/

#include "x11_debug.h"

#define __OOP_NOATTRBASES__

#include <proto/utility.h>
#include <graphics/monitor.h>

#include <X11/cursorfont.h>
#include <signal.h>
#include <string.h>

#include "x11_types.h"
#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"
#include "x11_xshm.h"

#define XVIDMODETAGS            11

#define XFLUSH(x) XCALL(XFlush, x)

/****************************************************************************************/

#define IS_X11GFX_ATTR(attr, idx) ( ( (idx) = (attr) - HiddX11GfxAB) < num_Hidd_BitMap_X11_Attrs)

int xshm_major;

/* Some attrbases needed as global vars.
 These are write-once read-many */

OOP_AttrBase HiddBitMapAttrBase;
OOP_AttrBase HiddX11BitMapAB;
OOP_AttrBase HiddSyncAttrBase;
OOP_AttrBase HiddPixFmtAttrBase;
OOP_AttrBase HiddGfxAttrBase;
OOP_AttrBase HiddDisplayAttrBase;
OOP_AttrBase HiddDMEnumAttrBase;
OOP_AttrBase HiddAttrBase;

static const struct OOP_ABDescr attrbases[] =
{
    { IID_Hidd_BitMap       , &HiddBitMapAttrBase   },
    { IID_Hidd_BitMap_X11   , &HiddX11BitMapAB      },
    { IID_Hidd_Sync         , &HiddSyncAttrBase     },
    { IID_Hidd_PixFmt       , &HiddPixFmtAttrBase   },
    { IID_Hidd_Gfx          , &HiddGfxAttrBase      },
    { IID_Hidd_Display      , &HiddDisplayAttrBase  },
    { IID_Hidd_DMEnum       , &HiddDMEnumAttrBase   },
    { IID_Hidd              , &HiddAttrBase         },
    { NULL                  , NULL                  }
};

VOID cleanupx11stuff(struct x11_staticdata *xsd);
BOOL initx11stuff(struct x11_staticdata *xsd);
ULONG mask_to_shift(ULONG mask);

/****************************************************************************************/

/********** GfxHidd::New()  ******************************/
OOP_Object *X11Cl__Root__New(OOP_Class *cl, OOP_Object *o, struct pRoot_New *msg)
{
    struct TagItem new_tags[] =
    {
        { aHidd_Name,           (IPTR)"x11gfx.hidd"                     },
        { aHidd_HardwareName,   (IPTR)"X Window Gfx Host"               },
        { aHidd_ProducerName,   (IPTR)"X.Org Foundation"                },
        { TAG_MORE,             (IPTR)msg->attrList                     }
    };
    struct pRoot_New new_msg =
    {
        .mID      = msg->mID,
        .attrList = new_tags
    };

    D(bug("[X11:Gfx] %s()\n", __func__));

    o = (OOP_Object *)OOP_DoSuperMethod(cl, o, (OOP_Msg)&new_msg);
    if (o)
    {
        struct gfx_data *data = OOP_INST_DATA(cl, o);
        struct TagItem displaytags[] =
        {
            { aHidd_Display_GfxHidd, (IPTR)o },
            { TAG_DONE,             0        }
        };

        D(bug("[X11:Gfx] %s: object @ 0x%p\n", __func__, o));

        /* The display object performs all the X11 setup and owns the modes */
        XSD(cl)->x11display = OOP_NewObject(XSD(cl)->displayclass, NULL, displaytags);
        if (XSD(cl)->x11display)
        {
            D(bug("[X11:Gfx] %s: display @ 0x%p\n", __func__, XSD(cl)->x11display));
            XSD(cl)->gfxhidd = o;

            /* CopyBox uses the shared X display connection */
            data->display = XSD(cl)->display;
        }
        else
        {
            OOP_MethodID dispose_mid = OOP_GetMethodID(IID_Root, moRoot_Dispose);
            OOP_CoerceMethod(cl, o, (OOP_Msg)&dispose_mid);
            o = NULL;
        }
    }

    return o;
}

/****************************************************************************************/

/********** GfxHidd::Dispose()  ******************************/
VOID X11Cl__Root__Dispose(OOP_Class *cl, OOP_Object *o, OOP_Msg msg)
{
    D(bug("[X11:Gfx] %s(0x%p)\n", __func__, o));

    cleanupx11stuff(XSD(cl));

    D(bug("X11Gfx::Dispose: calling super\n"));
    OOP_DoSuperMethod(cl, o, msg);

}

/****************************************************************************************/

VOID X11Cl__Root__Get(OOP_Class *cl, OOP_Object *o, struct pRoot_Get *msg)
{
    ULONG idx;

    D(bug("[X11:Gfx] %s()\n", __func__));

    if (IS_GFX_ATTR(msg->attrID, idx))
    {
        switch (idx)
        {
        case aoHidd_Gfx_IsWindowed:
            *msg->storage = TRUE;
            return;

        case aoHidd_Gfx_SupportsHWCursor:
#if X11SOFTMOUSE
            *msg->storage = FALSE;
#else
            *msg->storage = TRUE;
#endif
            return;

        case aoHidd_Gfx_DriverName:
            *msg->storage = (IPTR) "X11";
            return;

        case aoHidd_Gfx_DisplayDefault:
            *msg->storage = (IPTR)XSD(cl)->x11display;
            return;
        }
    }
    OOP_DoSuperMethod(cl, o, (OOP_Msg) msg);
}

/****************************************************************************************/

VOID X11Cl__Root__Set(OOP_Class *cl, OOP_Object *obj, struct pRoot_Set *msg)
{
    struct TagItem *tag, *tstate;
    ULONG idx;
    struct x11_staticdata *data = XSD(cl);

    D(bug("[X11:Gfx] %s()\n", __func__));

    tstate = msg->attrList;
    while ((tag = NextTagItem(&tstate)))
    {
        if (IS_GFX_ATTR(tag->ti_Tag, idx))
        {
            switch (idx)
            {
            case aoHidd_Gfx_ActiveCallBack:
                data->activecallback = (void *) tag->ti_Data;
                break;

            case aoHidd_Gfx_ActiveCallBackData:
                data->callbackdata = (void *) tag->ti_Data;
                break;
            }
        }
    }
    OOP_DoSuperMethod(cl, obj, &msg->mID);
}

/****************************************************************************************/

VOID X11Cl__Hidd_Gfx__CopyBox(OOP_Class *cl, OOP_Object *o, struct pHidd_Gfx_CopyBox *msg)
{
    ULONG mode;
    Drawable src = 0, dest = 0;
    GC gc = 0;

    struct gfx_data *data = OOP_INST_DATA(cl, o);

    D(bug("[X11:Gfx] %s()\n", __func__));

    mode = GC_DRMD(msg->gc);

    OOP_GetAttr(msg->src, aHidd_BitMap_X11_Drawable, (IPTR *) &src);
    OOP_GetAttr(msg->dest, aHidd_BitMap_X11_Drawable, (IPTR *) &dest);

    if (0 == dest || 0 == src)
    {
        /*
         * One of objects is not an X11 bitmap.
         * Let the superclass do the copying in a more general way
         */
        OOP_DoSuperMethod(cl, o, &msg->mID);
        return;
    }

    OOP_GetAttr(msg->src, aHidd_BitMap_X11_GC, (IPTR *) &gc);

    HostLib_Lock();

    XCALL(XSetFunction, data->display, gc, mode);
    XCALL(XCopyArea,
            data->display, src, dest, gc, msg->srcX, msg->srcY, msg->width, msg->height, msg->destX, msg->destY);

    HostLib_Unlock();
}

/****************************************************************************************/

/****************************************************************************************/

ULONG mask_to_shift(ULONG mask)
{
    ULONG i;

    for (i = 32; mask; i--)
    {
        mask >>= 1;
    }

    if (mask == 32)
    {
        i = 0;
    }

    return i;
}

/****************************************************************************************/

#undef XSD
#define XSD(cl) xsd

/*
 Inits sysdisplay, sysscreen, colormap, etc.. */
BOOL initx11stuff(struct x11_staticdata *xsd)
{
    /*    XColor fg, bg; */
    BOOL ok = TRUE;
    XVisualInfo template;
    XVisualInfo *visinfo;
    int template_mask;
    int numvisuals;

    D(bug("[X11:Gfx] %s(0x%p)\n", __func__, xsd);)

    if (!X11_Init(xsd))
        return FALSE;

    LOCK_X11

    /* Get some info on the display */
    template.visualid = XCALL(XVisualIDFromVisual, DefaultVisual(xsd->display, DefaultScreen(xsd->display)));
    template_mask = VisualIDMask;

    visinfo = XCALL(XGetVisualInfo, xsd->display, template_mask, &template, &numvisuals);

    if (numvisuals > 1)
    {
        D(bug("[X11:Gfx] %s: got %d visualinfo from X\n", __func__, numvisuals));

//            CCALL(raise, SIGSTOP);
    }

    if (NULL == visinfo)
    {
        D(bug("[X11:Gfx] %s: no visualinfo available!\n", __func__));

        CCALL(raise, SIGSTOP);

        ok = FALSE;
    }
    else
    {
        XPixmapFormatValues *pmf;
        int i, n;

        /* Store the visual info structure */
        xsd->vi = AllocMem(sizeof(XVisualInfo), MEMF_ANY);
        memcpy(xsd->vi, visinfo, sizeof(XVisualInfo));

        XCALL(XFree, visinfo);

        visinfo = xsd->vi;

        /* We only support TrueColor for now */

        switch (visinfo->class)
        {
        case TrueColor:
            /* Get the pixel masks */
            xsd->red_shift = mask_to_shift(xsd->vi->red_mask);
            xsd->green_shift = mask_to_shift(xsd->vi->green_mask);
            xsd->blue_shift = mask_to_shift(xsd->vi->blue_mask);
            break;

        case PseudoColor:
            /* stegerg */
            xsd->vi->red_mask = ((1 << xsd->vi->bits_per_rgb) - 1) << (xsd->vi->bits_per_rgb * 2);
            xsd->vi->green_mask = ((1 << xsd->vi->bits_per_rgb) - 1) << (xsd->vi->bits_per_rgb * 1);
            xsd->vi->blue_mask = ((1 << xsd->vi->bits_per_rgb) - 1);
            xsd->red_shift = mask_to_shift(xsd->vi->red_mask);
            xsd->green_shift = mask_to_shift(xsd->vi->green_mask);
            xsd->blue_shift = mask_to_shift(xsd->vi->blue_mask);
            /* end stegerg */
            break;

        default:
            D(bug("[X11:Gfx] %s: unsupported display mode!\n", __func__));

            CCALL(raise, SIGSTOP);
        }

        xsd->depth = 0;

        /* stegerg: based on xwininfo source */

        {
            XWindowAttributes win_attributes;

            if (!XCALL(XGetWindowAttributes, xsd->display,
                    RootWindow(xsd->display, DefaultScreen(xsd->display)),
                    &win_attributes))
            {
                D(bug("[X11:Gfx] %s: failed to obtain bits per pixel\n", __func__));

                CCALL(raise, SIGSTOP);
            }
            xsd->depth = win_attributes.depth;
            D(
                bug("\n");
                bug("[X11:Gfx] %s: Display Depth = %dbit (Default = %dbit)\n", __func__, DisplayPlanes(xsd->display, DefaultScreen(xsd->display)), DefaultDepth(xsd->display, DefaultScreen(xsd->display)));

            )
        }

        xsd->bytes_per_pixel = 0;
        pmf = XCALL(XListPixmapFormats, xsd->display, &n);
        if (pmf) {
            D(bug("[X11:Gfx] %s: checking pixmapformats for depth bytes per pixel\n", __func__);)

            for (i = 0; i < n; i++) {
                D(bug("[X11:Gfx] %s:     depth %d, bits_per_pixel %d, scanline_pad %d\n", 
                    __func__,
                    pmf[i].depth, pmf[i].bits_per_pixel, pmf[i].scanline_pad);)

                if (pmf[i].depth == DefaultDepth(xsd->display, DefaultScreen(xsd->display)))
                    xsd->bytes_per_pixel = (pmf[i].bits_per_pixel) >> 3;
            }
            XCALL(XFree, (char *) pmf);
        }

        if (xsd->bytes_per_pixel == 0)
        {
            XImage *testimage;

            D(bug("[X11:Gfx] %s: attempting to use test image to obtain bytes per pixel\n", __func__);)

            /* Create a dummy X image to get bits per pixel */
            testimage = XCALL(XGetImage, xsd->display, RootWindow(xsd->display,
                            DefaultScreen(xsd->display)), 0, 0, 1, 1,
                    AllPlanes, ZPixmap);

            if (NULL != testimage)
            {
                xsd->bytes_per_pixel = (testimage->bits_per_pixel + 7) >> 3;
                XDestroyImage(testimage);
            }
            else
            {
                D(bug("[X11:Gfx] %s: failed to create query image\n", __func__));
                CCALL(raise, SIGSTOP);
            }
        }
        
        D(bug("[X11:Gfx] %s: %d Bytes per Pixel\n", __func__, xsd->bytes_per_pixel);)
        
        if (PseudoColor == xsd->vi->class)
        {
            xsd->clut_mask = (1L << xsd->depth) - 1;
            xsd->clut_shift = 0;
        }
    }

    /* Create a dummy window for pixmaps */
    xsd->dummy_window_for_creating_pixmaps = XCALL(XCreateSimpleWindow, xsd->display,
            DefaultRootWindow(xsd->display),
            0, 0, 100, 100,
            0,
            BlackPixel(xsd->display, DefaultScreen(xsd->display)),
            BlackPixel(xsd->display, DefaultScreen(xsd->display)));
    if (0 == xsd->dummy_window_for_creating_pixmaps)
    {
        D(bug("[X11:Gfx] %s: failed to create pixmap window\n", __func__));
        ok = FALSE;
    }

#if USE_XSHM
    {
        char *displayname = XCALL(XDisplayName, NULL);

        if ((strncmp(displayname, ":", 1) == 0) ||
                (strncmp(displayname, "unix:", 5) == 0))
        {
            /* Display is local, not remote. XSHM is possible */

            /* Do we have Xshm support ? */
            xsd->xshm_info = init_shared_mem(xsd->display);

            if (NULL == xsd->xshm_info)
            {
                /* ok = FALSE; */
                D(bug("INITIALIZATION OF XSHM FAILED !!\n"));
            }
            else
            {
                int a, b;

                InitSemaphore(&xsd->shm_sema);
                xsd->use_xshm = TRUE;

                XCALL(XQueryExtension, xsd->display, "MIT-SHM", &xshm_major, &a, &b);
            }
        }
    }
#endif

    UNLOCK_X11

    ReturnBool("initx11stuff", ok);

}

/****************************************************************************************/

VOID cleanupx11stuff(struct x11_staticdata *xsd)
{
    D(bug("[X11:Gfx] %s()\n", __func__));

    LOCK_X11

    /* Do nothing for now */
    if (0 != xsd->dummy_window_for_creating_pixmaps)
    {
        XCALL(XDestroyWindow, xsd->display, xsd->dummy_window_for_creating_pixmaps);
    }

#if USE_XSHM
    cleanup_shared_mem(xsd->display, xsd->xshm_info);
#endif
    FreeMem(xsd->vi, sizeof(XVisualInfo));
    xsd->vi = NULL;

    UNLOCK_X11
}

/****************************************************************************************/

//#define xsd (&LIBBASE->xsd)
/****************************************************************************************/

static int x11gfx_init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[X11:Gfx] %s: initialising semaphore @ 0x%p\n", __func__, &LIBBASE->xsd.sema));

    InitSemaphore(&LIBBASE->xsd.sema);

    return OOP_ObtainAttrBases(attrbases);
}

/****************************************************************************************/

static int x11gfx_expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[X11:Gfx] %s()\n", __func__));

    OOP_ReleaseAttrBases(attrbases);
    return TRUE;
}

/****************************************************************************************/

ADD2INITLIB(x11gfx_init, 0);
ADD2EXPUNGELIB(x11gfx_expunge, 0);

/****************************************************************************************/
