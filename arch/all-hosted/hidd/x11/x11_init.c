/*
    Copyright � 1995-2017, The AROS Development Team. All rights reserved.
    $Id$

    Desc: X11 hidd initialization code.
    Lang: English.
*/

#include "x11_debug.h"

#define __OOP_NOATTRBASES__

#include <aros/bootloader.h>
#include <proto/bootloader.h>

#include "x11_types.h"
#include LC_LIBDEFS_FILE
#include "x11_hostlib.h"
#include "x11gfx_fullscreen.h"

/****************************************************************************************/

#undef XSD

/****************************************************************************************/

static BOOL initclasses(struct x11_staticdata *xsd);
static VOID freeclasses(struct x11_staticdata *xsd);
struct Task *create_x11task(struct x11task_params *params);
VOID x11task_entry(struct x11task_params *xtp);

/****************************************************************************************/

static OOP_AttrBase HiddPixFmtAttrBase;

static struct OOP_ABDescr abd[] =
{
    { IID_Hidd_PixFmt   , &HiddPixFmtAttrBase   },
    { NULL              , NULL                  }
};

/****************************************************************************************/

static BOOL initclasses(struct x11_staticdata *xsd)
{
    D(bug("[X11] %s()\n", __PRETTY_FUNCTION__));

    /* Get some attrbases */

    if (!OOP_ObtainAttrBases(abd))
        goto failure;

    return TRUE;

    failure: freeclasses(xsd);

    return FALSE;
}

/****************************************************************************************/

static VOID freeclasses(struct x11_staticdata *xsd)
{
    D(bug("[X11] %s()\n", __PRETTY_FUNCTION__));

    OOP_ReleaseAttrBases(abd);
}

/****************************************************************************************/

static int MyErrorHandler(Display * display, XErrorEvent * errevent)
{
    char buffer[256];

#if USE_XSHM
    extern int xshm_major;

    if ((xshm_major != 0) && (errevent->request_code == xshm_major) &&
            (errevent->minor_code == 4) && /* XShmGetImage */
            (errevent->error_code == BadMatch))
    {
        /* Ignore this error */

        return 0;
    }
#endif

    Disable();
    XCALL(XGetErrorText, display, errevent->error_code, buffer, sizeof (buffer));
    Enable();

    bug("XError %d (Major=%d, Minor=%d) task = %s\n%s\n", errevent->error_code, errevent->request_code,
            errevent->minor_code, FindTask(0)->tc_Node.ln_Name, buffer);

    return 0;
}

/****************************************************************************************/

static int MySysErrorHandler(Display * display)
{
    /*  This should have been host's perror(), not AROS perror()
     perror ("X11-Error"); */
    bug("X11 system error!\n");

    return 0;
}

/****************************************************************************************/

int X11_Init(struct x11_staticdata *xsd)
{
    D(bug("[X11] %s()\n", __PRETTY_FUNCTION__));

    if (xsd->display)
    {
        D(bug("[X11] %s: already initialized\n", __PRETTY_FUNCTION__));
        return TRUE;
    }

    /* Do not need to single-thread this since no other tasks are using X
       currently */

    xsd->display = XCALL(XOpenDisplay, NULL);
    D(bug("[X11] %s: X display @ 0x%p\n", __PRETTY_FUNCTION__, xsd->display));

    if (xsd->display)
    {
        struct x11task_params xtp;
        struct Task *x11task;
        APTR BootLoaderBase;

        xsd->options = 0L;

#if DEBUG_X11_SYNCHRON
        XCALL(XSynchronize, xsd->display, True);
#endif
        XCALL(XSetErrorHandler, MyErrorHandler);
        XCALL(XSetIOErrorHandler, MySysErrorHandler);

#if defined(HOST_OS_darwin) || defined(__arm__)
        /*
         * Neither ARM targets nor XQuartz like operations on unmapped window, strange effects occur (bootmenu breaks, for example).
         * X11 driver needs serious rewrite. For now i hope this will do.
         */
#else
        /* Do not map (show) X window as long as there's no screen */
        xsd->options |= OPTION_DELAYXWINMAPPING;
#endif

        BootLoaderBase = OpenResource("bootloader.resource");

        /*
         *    Argument parsing from bootloader.resource 
         */

        if (BootLoaderBase)
        {
            struct List *args;
            struct Node *n;
            args = GetBootInfo(BL_Args);
            if (args)
            {
                for (n = args->lh_Head; n->ln_Succ; n = n->ln_Succ)
                {
                    /* do we have fullscreen flag ? */
                    if (!strcmp("--fullscreen", n->ln_Name))
                    {
                        if (x11_fullscreen_supported(xsd->display))
                            xsd->options |= OPTION_FULLSCREEN;
                    }

                    if (strcmp("--backingstore", n->ln_Name) == 0)
                    {
                        xsd->options |= OPTION_BACKINGSTORE;
                    }

                    if (strcmp("--forcestdmodes", n->ln_Name) == 0)
                    {
                        xsd->options |= OPTION_FORCESTDMODES;
                    }

                    if (strcmp("--forcedelayxwinmapping", n->ln_Name) == 0)
                    {
                        xsd->options |= OPTION_DELAYXWINMAPPING;
                    }

                }
            }
        }

        D(
            if (xsd->options & OPTION_DELAYXWINMAPPING)
            {
                D(bug("[X11] %s: option DELAYXWINMAPPING\n", __PRETTY_FUNCTION__));
            }
        )

        xsd->delete_win_atom            = XCALL(XInternAtom, xsd->display, "WM_DELETE_WINDOW", FALSE);
        xsd->clipboard_atom             = XCALL(XInternAtom, xsd->display, "CLIPBOARD", FALSE);
        xsd->clipboard_property_atom    = XCALL(XInternAtom, xsd->display, "AROS_HOSTCLIP", FALSE);
        xsd->clipboard_incr_atom        = XCALL(XInternAtom, xsd->display, "INCR", FALSE);
        xsd->clipboard_targets_atom     = XCALL(XInternAtom, xsd->display, "TARGETS", FALSE);

        xtp.parent      = FindTask(NULL);
        xtp.ok_signal   = SIGBREAKF_CTRL_E;
        xtp.fail_signal = SIGBREAKF_CTRL_F;
        xtp.kill_signal = SIGBREAKF_CTRL_C;
        xtp.xsd = xsd;

        if ((x11task = create_x11task(&xtp)))
        {
            if (initclasses(xsd))
            {
                D(bug("[X11] %s: task & classes initialized\n", __PRETTY_FUNCTION__));
                return TRUE;
            }

            Signal(x11task, xtp.kill_signal);
        }

        XCALL(XCloseDisplay, xsd->display);

    }

   D(bug("[X11] %s: failed to initialize\n", __PRETTY_FUNCTION__));

    return FALSE;
}

/****************************************************************************************/
