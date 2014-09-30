/*
    Copyright © 1995-2014, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <aros/config.h>

#include "gdi.h"

#include <aros/symbolsets.h>

#include <proto/hostlib.h>

#include LC_LIBDEFS_FILE

#define DEBUG 0
#include <aros/debug.h>

void *gdi_handle = NULL;
void *user_handle = NULL;
void *native_handle = NULL;

struct gdi_func *gdi_func;
struct user_func *user_func;
struct native_func *native_func;

static const char *gdi_func_names[] = {
    "CreateDCA",
    "CreateCompatibleDC",
    "DeleteDC",
    "GetDeviceCaps",
    "CreateBitmapIndirect",
    "CreateCompatibleBitmap",
    "CreateSolidBrush",
    "DeleteObject",
    "SelectObject",
    "SetBkColor",
    "SetTextColor",
    "SetROP2",
    "BitBlt",
    "PatBlt",
    "GetPixel",
    "SetPixel",
    "GetDIBits",
    "StretchDIBits",
    NULL
};

static const char *user_func_names[] = {
    "CreateIconIndirect",
    "DestroyIcon",
    "SetCursor",
    "SetWindowPos",
    "RedrawWindow",
     NULL
};

static const char *native_func_names[] = {
    "GDI_Init",
    "GDI_Shutdown",
    "GDI_PutMsg",
    "GDI_KbdAck",
    "GDI_MouseAck",
    NULL
};

void *KernelBase;
void *HostLibBase;

APTR *gdi_hostlib_load_so(const char *sofile, const char **names, void **handle)
{
    APTR *funcptr;
    ULONG i;

    D(bug("[gdi] loading functions from %s\n", sofile));

    if ((*handle = HostLib_Open(sofile, NULL)) == NULL) {
        D(kprintf("[gdi] couldn't open '%s'\n", sofile));
        return NULL;
    }

    funcptr = HostLib_GetInterface(*handle, names, &i);
    if (funcptr)
    {
        if (!i)
        {
            D(bug("[gdi] done\n"));
            return funcptr;
        }
        D(kprintf("[gdi] couldn't get %lu symbols from '%s'\n", i, sofile));
        HostLib_DropInterface(funcptr);
    }
    HostLib_Close(*handle, NULL);
    return NULL;
}

static int gdi_hostlib_init(LIBBASETYPEPTR LIBBASE) {
    D(bug("[gdi] hostlib init\n"));

    if ((KernelBase = OpenResource("kernel.resource")) == NULL) {
        kprintf("[gdi] couldn't open kernel.resource");
        return FALSE;
    }
    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL) {
        kprintf("[gdi] couldn't open hostlib.resource");
        return FALSE;
    }

    gdi_func = (struct gdi_func *)gdi_hostlib_load_so(GDI_SOFILE, gdi_func_names, &gdi_handle);
    if (!gdi_func)
        return FALSE;
    user_func = (struct user_func *)gdi_hostlib_load_so(USER_SOFILE, user_func_names, &user_handle);
    if (user_func) {
        native_func = (struct native_func *)gdi_hostlib_load_so(NATIVE_SOFILE, native_func_names, &native_handle);
        if (native_func)
            return TRUE;
        HostLib_DropInterface((APTR *)user_func);
    	HostLib_Close(user_handle, NULL);
    }
    HostLib_DropInterface((APTR *)gdi_func);
    HostLib_Close(gdi_handle, NULL);
    return FALSE;
}

static int gdi_hostlib_expunge(LIBBASETYPEPTR LIBBASE) {
    D(bug("[gdi] hostlib expunge\n"));

    if (gdi_func != NULL)
        HostLib_DropInterface((APTR *)gdi_func);
    if (gdi_handle != NULL)
        HostLib_Close(gdi_handle, NULL);

    if (native_func != NULL)
        HostLib_DropInterface((APTR *)native_func);
    if (native_handle != NULL)
        HostLib_Close(native_handle, NULL);

    return TRUE;
}

ADD2INITLIB(gdi_hostlib_init, 0)
ADD2EXPUNGELIB(gdi_hostlib_expunge, 0)

