/*
    Copyright 2011-2025, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>

#include "hostgl_renderer_config.h"
#include "x11_hostlib.h"

#ifndef __HOSTLIB_NOLIBBASE__
#define __HOSTLIB_NOLIBBASE__
#endif
#include <proto/hostlib.h>

#include LC_LIBDEFS_FILE

void *x11_handle = NULL;
struct x11_func x11_func;

static const char *x11_func_names[] =
{
    "XOpenDisplay",
    "XCloseDisplay",
    "XFree",
#if defined(RENDERER_SEPARATE_X_WINDOW)
    "XCreateColormap",
    "XCreateWindow",
    "XDestroyWindow",
    "XFlush",
    "XMapWindow",
#endif
    NULL
};

static APTR HostLibBase;

void *x11_hostlib_load_so(const char *sofile, const char **names, void **funcptr)
{
    void *handle;
    char *err;
    const char *name;
    int i = 0;

    D(bug("[HostGL:X11] loading functions from %s\n", sofile));

    if ((handle = HostLib_Open(sofile, &err)) == NULL)
    {
        kprintf("[HostGL:X11] couldn't open '%s': %s\n", sofile, err);
        return NULL;
    }

    while((name = names[i]) != NULL)
    {
        funcptr[i] = HostLib_GetPointer(handle, name, &err);
        D(bug("%s(%x)\n", names[i], funcptr[i]));
        if (err != NULL)
        {
            kprintf("[HostGL:X11] couldn't get symbol '%s' from '%s': %s\n", names[i], sofile, err);
            HostLib_Close(handle, NULL);
            return NULL;
        }
        i++;
    }

    D(bug("[x11] done\n"));

    return handle;
}

static int x11_hostlib_init(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[HostGL:X11] X11 hostlib init\n"));

    if ((HostLibBase = OpenResource("hostlib.resource")) == NULL)
    {
        kprintf("[HostGL:X11] couldn't open hostlib.resource\n");
        return FALSE;
    }

    if ((x11_handle = x11_hostlib_load_so(X11_SOFILE, x11_func_names, (void **) &x11_func)) == NULL)
    {
        HostLib_Close(x11_handle, NULL);
        return FALSE;
    }

    return TRUE;
}

static int x11_hostlib_expunge(LIBBASETYPEPTR LIBBASE)
{
    D(bug("[HostGL:X11] X11 hostlib expunge\n"));

    if (x11_handle != NULL)
        HostLib_Close(x11_handle, NULL);

    return TRUE;
}

ADD2INITLIB(x11_hostlib_init, 0)
ADD2EXPUNGELIB(x11_hostlib_expunge, 0)

