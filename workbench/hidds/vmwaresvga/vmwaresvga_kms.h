#ifndef _VMWARESVGA_KMS_H
#define _VMWARESVGA_KMS_H

/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: What the hidd needs from the vmwgfx DRM driver: a display to
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
struct VMWareSVGA_FB
{
    ULONG                       handle;         /* GEM handle of the buffer */
    ULONG                       fbid;           /* KMS framebuffer id, 0 when not registered */
    ULONG                       pitch;          /* bytes per line */
    UQUAD                       size;
    APTR                        map;            /* CPU view of the buffer */
    ULONG                       width, height;
    ULONG                       bpp, depth;
};

struct VMWareSVGA_KMS
{
    int                         fd;             /* the hidd's drm file */
    struct SignalSemaphore      lock;           /* serialises drm calls */

    drmModeConnectorPtr         connector;      /* the display we drive */
    ULONG                       crtc_id;
    drmModeModeInfoPtr          curmode;        /* mode set on the crtc, or NULL */
    struct VMWareSVGA_FB        *curfb;         /* framebuffer shown, or NULL */

    BOOL                        started;        /* the driver's tasks are running */
    BOOL                        ready;          /* the display can be driven */
    ULONG                       max_width;      /* largest mode offered */
    ULONG                       max_height;
    BOOL                        has3d;

    /* cursor */
    struct VMWareSVGA_FB        cursor;         /* 64x64 ARGB buffer */
    BOOL                        cursor_ok;      /* the device took our cursor */
    BOOL                        cursor_shown;

    /* deferred front buffer flushing */
    struct SignalSemaphore      damage_lock;
    struct Box                  damage;
    BOOL                        damage_pending;
    struct Task                 *render_task;
};

#define VMWSVGA_CURSOR_SIZE     64

BOOL VMWareSVGA_KMS_Init(struct VMWareSVGA_KMS *kms);
VOID VMWareSVGA_KMS_Shutdown(struct VMWareSVGA_KMS *kms);

BOOL VMWareSVGA_KMS_CreateFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, ULONG width, ULONG height, ULONG bpp, ULONG depth);
VOID VMWareSVGA_KMS_DestroyFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb);

drmModeModeInfoPtr VMWareSVGA_KMS_FindMode(struct VMWareSVGA_KMS *kms, ULONG width, ULONG height);
BOOL VMWareSVGA_KMS_SetMode(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, drmModeModeInfoPtr mode);

VOID VMWareSVGA_KMS_DirtyFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, struct Box *box);
VOID VMWareSVGA_KMS_DamageAdd(struct VMWareSVGA_KMS *kms, struct Box *box);
VOID VMWareSVGA_KMS_DamageFlush(struct VMWareSVGA_KMS *kms);

BOOL VMWareSVGA_KMS_SetCursorShape(struct VMWareSVGA_KMS *kms, ULONG *argb, ULONG width, ULONG height);
BOOL VMWareSVGA_KMS_ShowCursor(struct VMWareSVGA_KMS *kms, BOOL show);
BOOL VMWareSVGA_KMS_MoveCursor(struct VMWareSVGA_KMS *kms, LONG x, LONG y);

#endif /* _VMWARESVGA_KMS_H */
