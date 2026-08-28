/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#include <stdarg.h>
#include <stdio.h>

/* These macros are defined in both UNIX and AROS headers. Get rid of warnings. */
#undef __pure
#undef __const
#undef __pure2
#undef __deprecated

#include <aros/config.h>
#include <aros/kernel.h>
#include <exec/lists.h>
#include <exec/resident.h>
#include <dos/bptr.h>

#include "hostinterface.h"

#include "hostlib.h"
#include "shutdown.h"

#if AROS_MODULES_DEBUG
/* gdb hooks from which it obtains modules list */

/* This is needed in order to bring in definition of struct segment */
#include "../../../rom/debug/debug_intern.h"

APTR AbsExecBase = NULL;
struct segment *seg = NULL;
struct Resident *res = NULL;
struct MinList *Debug_ModList = NULL;
#endif

#if defined(__APPLE__) && defined(__aarch64__)
extern void pthread_jit_write_protect_np(int enabled);
static void Host_JIT_WriteProtect(int enabled)
{
    pthread_jit_write_protect_np(enabled);
}

/* Cocoa display C API (implemented in cocoa_display.m) */
extern void *cocoa_display_init(int width, int height);
extern int   cocoa_display_get_pitch(void);
extern void  cocoa_display_refresh(void);
extern void  cocoa_runloop_step(void);
#endif

/*
 * Some helpful functions that link us to the underlying host OS.
 * Without them we would not be able to estabilish any interaction with it.
 */
static struct HostInterface _HostIFace =
{
    AROS_ARCHITECTURE,
    HOSTINTERFACE_VERSION,

    Host_HostLib_Open,
    Host_HostLib_Close,
    Host_HostLib_GetPointer,
    KPutC,
    Host_HostLib_GetTime,
#if AROS_MODULES_DEBUG
    &Debug_ModList,
#else
    NULL,
#endif
#if defined(__APPLE__) && defined(__aarch64__)
    Host_JIT_WriteProtect,
    cocoa_display_init,
    cocoa_display_get_pitch,
    cocoa_display_refresh,
    cocoa_runloop_step,
    NULL,  /* cocoa_fb_base - set at runtime */
    0,     /* cocoa_fb_width */
    0,     /* cocoa_fb_height */
    0,     /* cocoa_fb_pitch */
    0,     /* cocoa_event_write */
    0,     /* cocoa_event_read */
    {{0}}  /* cocoa_events */
#else
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
#endif
};

void *HostIFace = &_HostIFace;
