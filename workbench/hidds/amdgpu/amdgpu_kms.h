#ifndef _AMDGPU_KMS_H
#define _AMDGPU_KMS_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: What the hidd needs from the amdgpu DRM driver: a display to
          mode-set, framebuffers it can draw into, and a cursor.
*/

#include <exec/types.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>

#include <libdrm/arosdrm.h>
#include <libdrm/arosdrmmode.h>

struct Box
{
    int x1, y1;
    int x2, y2;
};

/* A dumb buffer registered as a scanout framebuffer, mapped for the CPU */
struct Amdgpu_FB
{
    ULONG                       handle;         /* GEM handle of the buffer */
    ULONG                       fbid;           /* KMS framebuffer id, 0 when not registered */
    ULONG                       pitch;          /* bytes per line */
    UQUAD                       size;
    APTR                        map;            /* CPU view of the buffer */
    ULONG                       width, height;
    ULONG                       bpp, depth;
};

struct Amdgpu_KMS
{
    int                         fd;             /* the hidd's drm file */
    struct SignalSemaphore      lock;           /* serialises drm calls */

    drmModeConnectorPtr         connector;      /* the display we drive */
    ULONG                       crtc_id;
    drmModeModeInfoPtr          curmode;        /* mode set on the crtc, or NULL */
    struct Amdgpu_FB        *curfb;         /* framebuffer shown, or NULL */

    BOOL                        started;        /* the driver's tasks are running */
    BOOL                        ready;          /* the display can be driven */
    ULONG                       max_width;      /* largest mode offered */
    ULONG                       max_height;
    BOOL                        has3d;
    BOOL                    no_dirtyfb;

    /* cursor */
    struct Amdgpu_FB        cursor;         /* 64x64 ARGB buffer */
    BOOL                        cursor_ok;      /* the device took our cursor */
    BOOL                        cursor_shown;

    /* deferred front buffer flushing */
    struct SignalSemaphore      damage_lock;
    struct Box                  damage;
    BOOL                        damage_pending;
    struct Task                 *render_task;
};

#define AMDGPU_CURSOR_SIZE     64

BOOL Amdgpu_KMS_Init(struct Amdgpu_KMS *kms);
VOID Amdgpu_KMS_Shutdown(struct Amdgpu_KMS *kms);

BOOL Amdgpu_KMS_CreateFB(struct Amdgpu_KMS *kms, struct Amdgpu_FB *fb, ULONG width, ULONG height, ULONG bpp, ULONG depth);
VOID Amdgpu_KMS_DestroyFB(struct Amdgpu_KMS *kms, struct Amdgpu_FB *fb);

drmModeModeInfoPtr Amdgpu_KMS_FindMode(struct Amdgpu_KMS *kms, ULONG width, ULONG height);
BOOL Amdgpu_KMS_SetMode(struct Amdgpu_KMS *kms, struct Amdgpu_FB *fb, drmModeModeInfoPtr mode);

VOID Amdgpu_KMS_DirtyFB(struct Amdgpu_KMS *kms, struct Amdgpu_FB *fb, struct Box *box);
VOID Amdgpu_KMS_DamageAdd(struct Amdgpu_KMS *kms, struct Box *box);
VOID Amdgpu_KMS_DamageFlush(struct Amdgpu_KMS *kms);

BOOL Amdgpu_KMS_SetCursorShape(struct Amdgpu_KMS *kms, ULONG *argb, ULONG width, ULONG height);
BOOL Amdgpu_KMS_ShowCursor(struct Amdgpu_KMS *kms, BOOL show);
BOOL Amdgpu_KMS_MoveCursor(struct Amdgpu_KMS *kms, LONG x, LONG y);

#endif /* _AMDGPU_KMS_H */
