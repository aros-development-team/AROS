/*
    Copyright 2011, The AROS Development Team. All rights reserved.
*/

#include <aros/symbolsets.h>
#include <proto/exec.h>
#include "hostgl_ctx_manager.h"
#include "tls.h"
#include <aros/debug.h>

#include LC_LIBDEFS_FILE

/* AROS is just one process to host, thus it can only have
   one current GLX context. On the other hand, there can
   be many GL apps running under AROS at the same time, each
   with it's own GL context. To solve this, calls to HostGL
   functions are serialized and at each call, it is checked
   if current HostGL GLX context matches current AROS task
   GL context. If no, change is made before executing the call.
   Constant switching of GLX contexts is bad for performance,
   but solves the problem. */

static struct SignalSemaphore global_glx_context_sem;
static volatile GLXContext global_glx_context;
static Display * global_x11_display = NULL;

static struct TaskLocalStorage * ctxtls = NULL;


/* Note: only one AROS task at a time may send commands via the display. Code
   acquiring display must guarantee this. Opening display in the task itself
   won't help - just use the this pointer and serialize calls */
Display * HostGL_GetGlobalX11Display()
{
    return global_x11_display;
}

GLAContext HostGL_GetCurrentContext()
{
    return GetFromTLS(ctxtls);
}

VOID HostGL_SetCurrentContext(GLAContext ctx)
{
    InsertIntoTLS(ctxtls, ctx);
}

/* Note: This lock needs to be obtained before doing ANY X11 or GLX call - remember AROS is one
   process to Linux and different AROS tasks cannot inter-mingle their X11/GLX calls */
VOID HostGL_Lock()
{
    ObtainSemaphore(&global_glx_context_sem);
}

VOID HostGL_UnLock()
{
    ReleaseSemaphore(&global_glx_context_sem);
}

/* This funtion needs to be called while holding the semaphore */
VOID HostGL_UpdateGlobalGLXContext()
{
    struct hostgl_context *cur_ctx = HostGL_GetCurrentContext();
    Display * dsp = HostGL_GetGlobalX11Display();

    if (cur_ctx)
    {
        if (cur_ctx->glXctx != global_glx_context)
        {
            global_glx_context = cur_ctx->glXctx;
            D(bug("[HostGL] TASK: 0x%x, GLX: 0x%x\n",FindTask(NULL), global_glx_context));

#if defined(RENDERER_SEPARATE_X_WINDOW)
            GLXCALL(glXMakeContextCurrent, dsp, cur_ctx->glXWindow, cur_ctx->glXWindow, cur_ctx->glXctx);
#endif
#if defined(RENDERER_PBUFFER_WPA)
            GLXCALL(glXMakeContextCurrent, dsp, cur_ctx->glXPbuffer, cur_ctx->glXPbuffer, cur_ctx->glXctx);
#endif
#if defined(RENDERER_PIXMAP_BLIT)
            GLXCALL(glXMakeContextCurrent, dsp, cur_ctx->glXPixmap, cur_ctx->glXPixmap, cur_ctx->glXctx);
#endif
        }
    }
    else
    {
        global_glx_context = NULL;
        D(bug("[HostGL] TASK: 0x%x, GLX: 0x%x\n",FindTask(NULL), global_glx_context));

        GLXCALL(glXMakeContextCurrent, dsp, None, None, NULL);
    }
}

static int HostGL_Ctx_Manager_Init(LIBBASETYPEPTR LIBBASE)
{
    InitSemaphore(&global_glx_context_sem);
    global_glx_context = NULL;
    global_x11_display = XCALL(XOpenDisplay, NULL);
    ctxtls = CreateTLS();
    return 1;
}

static int HostGL_Ctx_Manager_Expunge(LIBBASETYPEPTR LIBBASE)
{
    if (ctxtls) DestroyTLS(ctxtls);
    if (global_x11_display) XCALL(XCloseDisplay, global_x11_display);
//TODO: destroy global glx context if not null
    return 1;
}

/* These functions need to be at priority 1, so that XCALL/GLXCALL/GLCALL are made workable bofore them */
ADD2INITLIB(HostGL_Ctx_Manager_Init, 1)
ADD2EXPUNGELIB(HostGL_Ctx_Manager_Expunge, 1)
