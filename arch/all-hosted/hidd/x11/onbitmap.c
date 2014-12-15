/*
    Copyright © 1995-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Bitmap class for X11 hidd.
    Lang: English.
*/

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>

#include <aros/debug.h>
#include <exec/memory.h>
#include <exec/lists.h>
#include <graphics/rastport.h>
#include <graphics/gfx.h>
#include <oop/oop.h>
#include <hidd/graphics.h>

#include <proto/oop.h>
#include <proto/utility.h>

#include "x11.h"
#include "bitmap.h"
#include "x11gfx_intern.h"

/****************************************************************************************/

#if ADJUST_XWIN_SIZE
#define MASTERWIN(data) (data)->masterxwindow
#define ROOTWIN(data) (data)->masterxwindow
#else
#define MASTERWIN(data) WINDRAWABLE(data)
#define ROOTWIN(data) rootwin
#endif

/****************************************************************************************/

static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth,
        struct x11_staticdata *xsd);

/****************************************************************************************/

BOOL X11BM_InitFB(OOP_Class *cl, OOP_Object *o, struct TagItem *attrList)
{
    Window rootwin;
    OOP_Object *sync, *pixfmt;
    HIDDT_ModeID modeid;
    IPTR depth;
    XSetWindowAttributes winattr;
    int visualclass;
    unsigned long valuemask;
    struct bitmap_data *data = OOP_INST_DATA(cl, o);
    struct x11_staticdata *xsd = XSD(cl);

    EnterFunc(bug("X11Gfx.BitMap::InitFB()\n"));

    /* stegerg */
    visualclass = GetTagData(aHidd_X11BitMap_VisualClass, TrueColor, attrList);
    if (visualclass == PseudoColor)
    {
        Colormap cm;

        HostLib_Lock();
        cm = XCALL(XCreateColormap, GetSysDisplay(), RootWindow(GetSysDisplay(), GetSysScreen()),
                xsd->vi.visual, AllocAll);
        HostLib_Unlock();

        if (cm)
        {
            data->colmap = cm;
            data->flags |= BMDF_COLORMAP_ALLOCED;
        }
    }
    /* end stegerg */

    /*
     * Get window size from our ModeID.
     * We can't support scrolling in framebuffer mode.
     */
    OOP_GetAttr(o, aHidd_BitMap_ModeID, &modeid);
    OOP_GetAttr(o, aHidd_BitMap_GfxHidd, (IPTR *) &data->gfxhidd);
    D(bug("[X11FB] ModeID 0x%08X, driver 0x%p\n", modeid, data->gfxhidd));

    HIDD_Gfx_GetMode(data->gfxhidd, modeid, &sync, &pixfmt);

    OOP_GetAttr(sync, aHidd_Sync_HDisp, &data->width);
    OOP_GetAttr(sync, aHidd_Sync_VDisp, &data->height);

    /* Open an X window to be used for viewing */
    D(bug("[X11FB] Framebuffer window size %ldx%ld\n", data->width, data->height));

    /* Listen for all sorts of events */
    winattr.event_mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask
            | KeyPressMask | KeyReleaseMask | StructureNotifyMask
            | SubstructureNotifyMask | FocusChangeMask | ExposureMask;

    if (XSD(cl)->options & OPTION_BACKINGSTORE)
    {
        /* Framebuffer needs backing store. (Uses lots of mem) */
        winattr.backing_store = Always;
        winattr.save_under = True;
    }

    winattr.cursor = GetSysCursor();
    winattr.background_pixel = BlackPixel(GetSysDisplay(), GetSysScreen());

    rootwin = DefaultRootWindow(GetSysDisplay());
    D(bug("Creating XWindow: root win=%p\n", rootwin));
    depth = DefaultDepth(GetSysDisplay(), GetSysScreen());

    valuemask = CWCursor | CWEventMask | CWBackPixel;

    if (XSD(cl)->options & OPTION_BACKINGSTORE)
        valuemask |= CWBackingStore | CWSaveUnder;

    if (data->flags & BMDF_COLORMAP_ALLOCED)
    {
        winattr.colormap = data->colmap;
        valuemask |= CWColormap;
    }

    HostLib_Lock();

#if ADJUST_XWIN_SIZE
    {
        XSetWindowAttributes rootattr;
        unsigned long rootmask = 0;

        if (XSD(cl)->options & OPTION_FULLSCREEN)
        {
            rootattr.override_redirect = True;
            rootmask |= CWOverrideRedirect;
        }

        if (data->flags & BMDF_COLORMAP_ALLOCED)
        {
            rootattr.colormap = data->colmap;
            rootmask |= CWColormap;
        }

        MASTERWIN(data) = XCALL(XCreateWindow, GetSysDisplay(), rootwin,
                0, /* leftedge     */
                0, /* topedge    */
                data->width,
                data->height,
                0, /* BorderWidth    */
                depth,
                InputOutput,
                DefaultVisual(GetSysDisplay(), GetSysScreen()),
                rootmask,
                &rootattr);
    }

    if (MASTERWIN(data))
#endif
        WINDRAWABLE(data) = XCALL(XCreateWindow, GetSysDisplay(),
                ROOTWIN(data),
                0, /* leftedge     */
                0, /* topedge    */
                data->width,
                data->height,
                0, /* BorderWidth    */
                depth,
                InputOutput,
                DefaultVisual (GetSysDisplay(), GetSysScreen()),
                valuemask,
                &winattr);
    HostLib_Unlock();

    D(bug("[X11FB] Xwindow: 0x%p\n", WINDRAWABLE(data)));

    if (WINDRAWABLE(data))
    {
        struct MsgPort *port;
        Pixmap icon;
#if !ADJUST_XWIN_SIZE
        XSizeHints sizehint;
#endif
        XClassHint *classhint;

        HostLib_Lock();

        classhint = XCALL(XAllocClassHint);
        classhint->res_name = "AROS";
        classhint->res_class = "AROS";
        XCALL(XSetClassHint, GetSysDisplay(), MASTERWIN(data), classhint);

        XCALL(XStoreName, GetSysDisplay(), MASTERWIN(data), "AROS");
        XCALL(XSetIconName, GetSysDisplay(), MASTERWIN(data), "AROS Screen");

#if !ADJUST_XWIN_SIZE
        sizehint.flags = PMinSize | PMaxSize;
        sizehint.min_width = data->width;
        sizehint.min_height = data->height;
        sizehint.max_width = data->width;
        sizehint.max_height = data->height;

        XCALL(XSetWMNormalHints, GetSysDisplay(), MASTERWIN(data), &sizehint);
#endif

        XCALL(XSetWMProtocols,
                GetSysDisplay(), MASTERWIN(data), &XSD(cl)->delete_win_atom, 1);

        icon = init_icon(GetSysDisplay(), MASTERWIN(data),
                DefaultColormap(GetSysDisplay(), GetSysScreen()), depth, xsd);

        if (icon)
        {
            XWMHints hints;

            hints.icon_pixmap = icon;
            hints.flags = IconPixmapHint;

            XCALL(XSetWMHints, GetSysDisplay(), MASTERWIN(data), &hints);
        }

        if (XSD(cl)->options & OPTION_BACKINGSTORE)
        {
            DRAWABLE(data) = WINDRAWABLE(data);
            data->flags |= BMDF_BACKINGSTORE;
        }
        else
        {
            DRAWABLE(data) = XCALL(XCreatePixmap, data->display, WINDRAWABLE(data), data->width, data->height, depth);
        }

        D(bug("Calling XMapRaised\n"));

        /*
         * stegerg: XMapRaised is now called inside the X11 task when getting
         *          the NOTY_MAPWINDOW message, otherwise the X11 task can
         *        get a "dead" MapNotify event:
         *
         *       XCreateWindow is called here on the app task context.
         *       If we also call XMapRaised here then the X11 task might
         *       get the MapNotify event before he got the NOTY_WINCREATE
         *       message sent from here (see below). So the X11 task
         *       would not know about our window and therefore ignore
         *       the MapNotify event from X.
         *
         *       This caused the freezes which sometimes happened during
         *       startup when the Workbench screen was opened.
         *
         *    XCALL(XMapRaised, GetSysDisplay(), DRAWABLE(data));
         */

        HostLib_Unlock();

        /*
         * Now we need to get some message from the X11 task about when
         * the window has been mapped (ie. MapWindow event).
         * This is because we cannot render into the window until the
         * it has been mapped.kfind &
         */

        port = CreateMsgPort();

        if (NULL != port)
        {
            /* Send a message to the x11 task that the window has been created */
            struct notify_msg msg;

            msg.notify_type = NOTY_WINCREATE;
            msg.xdisplay = GetSysDisplay();
            msg.xwindow = WINDRAWABLE(data);
            msg.masterxwindow = MASTERWIN(data);
            msg.bmobj = o;
            msg.execmsg.mn_ReplyPort = port;

            HostLib_Lock();
            XCALL(XSync, GetSysDisplay(), FALSE);
            HostLib_Unlock();

            X11DoNotify(xsd, &msg);

            if (!(XSD(cl)->options & OPTION_DELAYXWINMAPPING))
            {
                /*
                 * Send a message to the X11 task to ask when the window has been mapped.
                 * We change only notify_type, other fields are already set.
                 */
                msg.notify_type = NOTY_MAPWINDOW;

                HostLib_Lock();
                XCALL(XSync, GetSysDisplay(), FALSE);
                HostLib_Unlock();

                X11DoNotify(xsd, &msg);
                D(kprintf("NOTY_MAPWINDOW request done\n"));
            }

            DeleteMsgPort(port);

            return TRUE;
        } /* if (port) */
    } /* if WINDRAWABLE(data) */

    return FALSE;
}

/****************************************************************************************/

VOID X11BM_DisposeFB(struct bitmap_data *data, struct x11_staticdata *xsd)
{
    EnterFunc(bug("X11Gfx.BitMap::DisposePM()\n"));

    if (WINDRAWABLE(data))
    {
        struct MsgPort *port;
        struct notify_msg msg;

        port = CreateMsgPort();

        if (NULL == port)
        {
            D(kprintf("COULD NOT CREATE PORT OR ALLOCATE MEM IN onbitmap_dispose()\n"));
            return;
        }

        msg.notify_type = NOTY_WINDISPOSE;
        msg.xdisplay = GetSysDisplay();
        msg.xwindow = WINDRAWABLE(data);
        msg.masterxwindow = MASTERWIN(data);
        msg.execmsg.mn_ReplyPort = port;

        X11DoNotify(xsd, &msg);
        DeleteMsgPort(port);

    }

    /* Dispose everything */
    HostLib_Lock();

    if (WINDRAWABLE(data))
        XCALL(XDestroyWindow, GetSysDisplay(), WINDRAWABLE(data));

#if ADJUST_XWIN_SIZE
    if (MASTERWIN(data))
        XCALL(XDestroyWindow, GetSysDisplay(), MASTERWIN(data));
#endif

    if (data->flags & BMDF_COLORMAP_ALLOCED)
        XCALL(XFreeColormap, GetSysDisplay(), data->colmap);

    if (!(data->flags & BMDF_BACKINGSTORE))
        XCALL(XFreePixmap, GetSysDisplay(), DRAWABLE(data));

    XCALL(XFlush, GetSysDisplay());

    HostLib_Unlock();

    ReturnVoid("X11Gfx.BitMap::DisposeFB");
}

/****************************************************************************************/

#if ADJUST_XWIN_SIZE

BOOL X11BM_SetMode(struct bitmap_data *data, HIDDT_ModeID modeid,
        struct x11_staticdata *xsd)
{
    OOP_Object *sync, *pf;

    if (HIDD_Gfx_GetMode(data->gfxhidd, (HIDDT_ModeID) modeid, &sync, &pf))
    {
        struct MsgPort *port;
        IPTR new_width, new_height;

        OOP_GetAttr(sync, aHidd_Sync_HDisp, &new_width);
        OOP_GetAttr(sync, aHidd_Sync_VDisp, &new_height);

        /*
         * Don't do anything if the size actually won't change.
         * Prevents badly looking flashing, at least on Darwin.
         */
        if (!(xsd->options & OPTION_DELAYXWINMAPPING) && (new_width == data->width) && (new_height == data->height))
            return TRUE;

        port = CreateMsgPort();
        if (port)
        {
            struct notify_msg nmsg;

            /* Send resize message to the x11 task */
            nmsg.notify_type = NOTY_RESIZEWINDOW;
            nmsg.xdisplay = data->display;
            nmsg.xwindow = WINDRAWABLE(data);
            nmsg.masterxwindow = data->masterxwindow;
            nmsg.width = new_width;
            nmsg.height = new_height;
            nmsg.execmsg.mn_ReplyPort = port;

            X11DoNotify(xsd, &nmsg);
            DeleteMsgPort(port);

            /* Update cached size */
            data->width = new_width;
            data->height = new_height;

            return TRUE;
        }
    }
    return FALSE;
}

#endif

/****************************************************************************************/

VOID X11BM_ClearFB(struct bitmap_data *data, HIDDT_Pixel bg)
{
    XSetWindowAttributes winattr;

    winattr.background_pixel = bg;

    XCALL(XChangeWindowAttributes, data->display, WINDRAWABLE(data), CWBackPixel, &winattr);
    XCALL(XClearArea, data->display, DRAWABLE(data), 0, 0, data->width, data->height, FALSE);
    X11BM_ExposeFB(data, 0, 0, data->width, data->height);
}

/****************************************************************************************/

VOID X11BM_ExposeFB(struct bitmap_data *data, WORD x, WORD y, WORD width, WORD height)
{
    if (!(data->flags & BMDF_BACKINGSTORE))
    {
        XCALL(XSetFunction, data->display, data->gc, GXcopy);
        XCALL(XCopyArea, data->display, DRAWABLE(data), WINDRAWABLE(data), data->gc,
                x, y, width, height, x, y);
    }
}

/****************************************************************************************/

#if X11SOFTMOUSE

//void init_empty_cursor(Window w, GC gc, struct x11_staticdata *xsd)
VOID X11BM_InitEmptyCursor(struct bitmap_data *data)
{
    Pixmap p, mask;
    int width, height;

    width = height = 1;

    LOCK_X11
    p = XCALL(XCreatePixmap, data->display, WINDRAWABLE(data), width, height, 1);
    UNLOCK_X11

    if (0 != p)
    {
        LOCK_X11
        mask = XCALL(XCreatePixmap, data->display, WINDRAWABLE(data), width, height, 1);
        XCALL(XFlush, data->display);
        UNLOCK_X11

        if (0 != mask)
        {
            /* Define cursor for window */
            XColor fg, bg;
            Cursor c;
            GC gc;

            LOCK_X11
            gc = XCALL(XCreateGC, data->display, WINDRAWABLE(data), 0, 0);
            XCALL(XSetForeground, data->display, gc, 0);
            XCALL(XSetFunction, data->display, gc, GXcopy);
#if 0
            XCALL(XFillRectangle, data->display, p, gc, 1, 1, 1, 1);
            for (y = 0; y < height; y ++)
            {
                for (x = 0; x < width; x ++)
                {
                    XCALL(XDrawPoint, data->display, mask, gc, x, y);
                }
            }
#endif
            UNLOCK_X11

            fg.pixel = BlackPixel(data->display, DefaultScreen(data->display));
            fg.red = 0x0000;
            fg.green = 0x0000;
            fg.blue = 0x0000;
            fg.flags = DoRed | DoGreen | DoBlue;

            bg.pixel = WhitePixel(data->display, DefaultScreen(data->display));
            bg.red = 0xFFFF;
            bg.green = 0xFFFF;
            bg.blue = 0xFFFF;
            bg.flags = DoRed | DoGreen | DoBlue;

            LOCK_X11
            c = XCALL(XCreatePixmapCursor, data->display, p, mask, &fg, &bg, 0, 0);
            UNLOCK_X11

            if (0 != c)
            {
                LOCK_X11
                XCALL(XDefineCursor, data->display, WINDRAWABLE(data), c);
                UNLOCK_X11
            }

            LOCK_X11
            XCALL(XFreePixmap, data->display, mask);
            XCALL(XFreeGC, data->display, gc);
            UNLOCK_X11
        }

        LOCK_X11
        XCALL(XFreePixmap, data->display, p);
        UNLOCK_X11
    }

}

#endif

/****************************************************************************************/

static Pixmap init_icon(Display *d, Window w, Colormap cm, LONG depth,
        struct x11_staticdata *xsd)
{
#include "icon.h"

#define SHIFT_PIX(pix, shift)    \
    (( (shift) < 0) ? (pix) >> (-shift) : (pix) << (shift) )

    Pixmap icon = XCALL(XCreatePixmap, d, w, width, height, depth);
    char *data = header_data;
    LONG red_shift, green_shift, blue_shift;
    GC gc;

    red_shift = 24 - xsd->red_shift;
    green_shift = 24 - xsd->green_shift;
    blue_shift = 24 - xsd->blue_shift;

    if (icon)
    {
        gc = XCALL(XCreateGC, d, icon, 0, 0);

        if (gc)
        {
            WORD x, y;

            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    ULONG rgb[3];
                    ULONG pixel = 0;

                    HEADER_PIXEL(data, rgb);

                    if (xsd->vi.class == TrueColor)
                    {
                        pixel = (SHIFT_PIX(rgb[0] & 0xFF, red_shift)
                                & xsd->vi.red_mask)
                                | (SHIFT_PIX(rgb[1] & 0xFF, green_shift)
                                        & xsd->vi.green_mask)
                                | (SHIFT_PIX(rgb[2] & 0xFF, blue_shift)
                                        & xsd->vi.blue_mask);
                    }
                    else if (xsd->vi.class == PseudoColor)
                    {
                        XColor xcol;

                        xcol.red = (rgb[0] << 8) + rgb[0];
                        xcol.green = (rgb[1] << 8) + rgb[1];
                        xcol.blue = (rgb[2] << 8) + rgb[2];
                        xcol.flags = DoRed | DoGreen | DoBlue;

                        if (XCALL(XAllocColor, d, cm, &xcol))
                        {
                            pixel = xcol.pixel;
                        }
                    }

                    XCALL(XSetForeground, d, gc, pixel);
                    XCALL(XDrawPoint, d, icon, gc, x, y);
                }
            }

            XCALL(XFreeGC, d, gc);

        } /* if (gc) */

    } /* if (icon) */

    return icon;
}

