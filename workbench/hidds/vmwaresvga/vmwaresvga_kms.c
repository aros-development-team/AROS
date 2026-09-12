/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: The hidd's view of the vmwgfx DRM driver: KMS calls through the
          in-process libdrm shim.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <exec/ports.h>
#include <devices/timer.h>

#include <limits.h>
#include <string.h>

#include "vmwaresvga_kms.h"

/* drm-aros/drm_drv.c */
struct pci_dev *vmwgfx_init_findcard(void);
int vmwgfx_init(void);
void vmwgfx_aros_reset(void);
/* drm-aros/vmwgfx_aros.c */
BOOL vmwgfx_aros_has_3d(void);
BOOL vmwgfx_aros_has_mob(void);

#define LOCK_KMS(kms)   ObtainSemaphore(&(kms)->lock)
#define UNLOCK_KMS(kms) ReleaseSemaphore(&(kms)->lock)

/* --- framebuffers --------------------------------------------------------- */

BOOL VMWareSVGA_KMS_CreateFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, ULONG width, ULONG height, ULONG bpp, ULONG depth)
{
    int ret;

    memset(fb, 0, sizeof(*fb));

    LOCK_KMS(kms);
    ret = drmModeCreateDumb(kms->fd, width, height, bpp, &fb->handle, &fb->pitch, &fb->size);
    if (ret)
    {
        UNLOCK_KMS(kms);
        bug("[VMWareSVGA:KMS] %s: no %ux%u buffer (%d)\n", __func__, width, height, ret);
        return FALSE;
    }

    fb->map = drmMMap(kms->fd, fb->handle, NULL, NULL);
    if (!fb->map)
    {
        drmModeDestroyDumb(kms->fd, fb->handle);
        UNLOCK_KMS(kms);
        bug("[VMWareSVGA:KMS] %s: buffer %u cannot be mapped\n", __func__, fb->handle);
        memset(fb, 0, sizeof(*fb));
        return FALSE;
    }

    ret = drmModeAddFB(kms->fd, width, height, depth, bpp, fb->pitch, fb->handle, &fb->fbid);
    if (ret)
    {
        drmMUnmap(kms->fd, fb->handle);
        drmModeDestroyDumb(kms->fd, fb->handle);
        UNLOCK_KMS(kms);
        bug("[VMWareSVGA:KMS] %s: buffer %u refused as a framebuffer (%d)\n", __func__, fb->handle, ret);
        memset(fb, 0, sizeof(*fb));
        return FALSE;
    }
    UNLOCK_KMS(kms);

    fb->width = width;
    fb->height = height;
    fb->bpp = bpp;
    fb->depth = depth;

    D(bug("[VMWareSVGA:KMS] %s: %ux%u@%u fb %u handle %u pitch %u map 0x%p\n", __func__,
        width, height, bpp, fb->fbid, fb->handle, fb->pitch, fb->map));

    return TRUE;
}

VOID VMWareSVGA_KMS_DestroyFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb)
{
    if (!fb->handle)
        return;

    LOCK_KMS(kms);
    if (kms->curfb == fb)
    {
        drmModeSetCrtc(kms->fd, kms->crtc_id, 0, 0, 0, NULL, 0, NULL);
        kms->curfb = NULL;
        kms->curmode = NULL;
    }
    if (fb->fbid)
        drmModeRmFB(kms->fd, fb->fbid);
    drmMUnmap(kms->fd, fb->handle);
    drmModeDestroyDumb(kms->fd, fb->handle);
    UNLOCK_KMS(kms);

    memset(fb, 0, sizeof(*fb));
}

/* --- modes ------------------------------------------------------------------ */

drmModeModeInfoPtr VMWareSVGA_KMS_FindMode(struct VMWareSVGA_KMS *kms, ULONG width, ULONG height)
{
    int i;

    if (!kms->connector)
        return NULL;

    for (i = 0; i < kms->connector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &kms->connector->modes[i];

        if (mode->hdisplay == width && mode->vdisplay == height)
            return mode;
    }
    return NULL;
}

BOOL VMWareSVGA_KMS_SetMode(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, drmModeModeInfoPtr mode)
{
    uint32_t connector_id;
    int ret;

    if (!kms->connector || !fb || !fb->fbid || !mode)
        return FALSE;

    connector_id = kms->connector->connector_id;

    LOCK_KMS(kms);
    ret = drmModeSetCrtc(kms->fd, kms->crtc_id, fb->fbid, 0, 0, &connector_id, 1, mode);
    if (ret == 0)
    {
        /*
         * Becoming the scanout buffer may have moved the buffer (into
         * video RAM on a host without guest backed objects), and a move
         * retires the CPU view it had; take the current one.
         */
        APTR map = drmMMap(kms->fd, fb->handle, NULL, NULL);

        if (map)
            fb->map = map;
        kms->curfb = fb;
        kms->curmode = mode;
    }
    UNLOCK_KMS(kms);

    D(bug("[VMWareSVGA:KMS] %s: crtc %u fb %u mode %s: %d\n", __func__, kms->crtc_id, fb->fbid, mode->name, ret));

    return (ret == 0);
}

/* --- front buffer flushing --------------------------------------------------- */

VOID VMWareSVGA_KMS_DirtyFB(struct VMWareSVGA_KMS *kms, struct VMWareSVGA_FB *fb, struct Box *box)
{
    drmModeClip clip;

    if (!fb || !fb->fbid)
        return;

    if (box)
    {
        if (box->x1 < 0) box->x1 = 0;
        if (box->y1 < 0) box->y1 = 0;
        if (box->x2 >= (int)fb->width) box->x2 = fb->width - 1;
        if (box->y2 >= (int)fb->height) box->y2 = fb->height - 1;
        if (box->x2 < box->x1 || box->y2 < box->y1)
            return;
        clip.x1 = box->x1;
        clip.y1 = box->y1;
        clip.x2 = box->x2 + 1;
        clip.y2 = box->y2 + 1;
    }
    else
    {
        clip.x1 = 0;
        clip.y1 = 0;
        clip.x2 = fb->width;
        clip.y2 = fb->height;
    }

    LOCK_KMS(kms);
    if (drmModeDirtyFB(kms->fd, fb->fbid, &clip, 1) != 0)
        D(bug("[VMWareSVGA:KMS] %s: fb %u (%u,%u)-(%u,%u) not flushed\n", __func__, fb->fbid, clip.x1, clip.y1, clip.x2, clip.y2));
    UNLOCK_KMS(kms);
}

static VOID VMWareSVGA_KMS_DamageReset(struct VMWareSVGA_KMS *kms)
{
    kms->damage.x1 = INT_MAX;
    kms->damage.y1 = INT_MAX;
    kms->damage.x2 = INT_MIN;
    kms->damage.y2 = INT_MIN;
    kms->damage_pending = FALSE;
}

VOID VMWareSVGA_KMS_DamageAdd(struct VMWareSVGA_KMS *kms, struct Box *box)
{
    int tmp;

    if (box->x1 > box->x2) { tmp = box->x2; box->x2 = box->x1; box->x1 = tmp; }
    if (box->y1 > box->y2) { tmp = box->y2; box->y2 = box->y1; box->y1 = tmp; }

    ObtainSemaphore(&kms->damage_lock);
    if (box->x1 < kms->damage.x1) kms->damage.x1 = box->x1;
    if (box->y1 < kms->damage.y1) kms->damage.y1 = box->y1;
    if (box->x2 > kms->damage.x2) kms->damage.x2 = box->x2;
    if (box->y2 > kms->damage.y2) kms->damage.y2 = box->y2;
    kms->damage_pending = TRUE;
    ReleaseSemaphore(&kms->damage_lock);
}

VOID VMWareSVGA_KMS_DamageFlush(struct VMWareSVGA_KMS *kms)
{
    struct Box box;
    BOOL pending;

    ObtainSemaphore(&kms->damage_lock);
    pending = kms->damage_pending;
    box = kms->damage;
    VMWareSVGA_KMS_DamageReset(kms);
    ReleaseSemaphore(&kms->damage_lock);

    if (pending && kms->curfb)
        VMWareSVGA_KMS_DirtyFB(kms, kms->curfb, &box);
}

/*
 * Drawing marks the front buffer dirty; this task hands the accumulated
 * rectangle to the device a few times a second, so that a burst of small
 * updates costs one host round trip.
 */
static VOID VMWareSVGA_KMS_RenderTask(struct VMWareSVGA_KMS *kms)
{
    struct MsgPort *port;
    struct timerequest *timer_request;

    port = CreateMsgPort();
    timer_request = (struct timerequest *)CreateIORequest(port, sizeof(struct timerequest));
    if (!timer_request || OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer_request, 0) != 0)
    {
        bug("[VMWareSVGA:KMS] %s: no timer\n", __func__);
        return;
    }

    for (;;)
    {
        timer_request->tr_node.io_Command = TR_ADDREQUEST;
        timer_request->tr_time.tv_secs = 0;
        timer_request->tr_time.tv_micro = 20000;
        DoIO((struct IORequest *)timer_request);

        if (kms->damage_pending)
            VMWareSVGA_KMS_DamageFlush(kms);
    }
}

/* --- cursor -------------------------------------------------------------------- */

BOOL VMWareSVGA_KMS_SetCursorShape(struct VMWareSVGA_KMS *kms, ULONG *argb, ULONG width, ULONG height)
{
    ULONG *dst;
    ULONG x, y;
    int ret;

    if (!kms->cursor.handle)
    {
        /* the cursor buffer is a plain dumb buffer; it is never a scanout fb */
        LOCK_KMS(kms);
        ret = drmModeCreateDumb(kms->fd, VMWSVGA_CURSOR_SIZE, VMWSVGA_CURSOR_SIZE, 32,
                                &kms->cursor.handle, &kms->cursor.pitch, &kms->cursor.size);
        if (ret == 0)
            kms->cursor.map = drmMMap(kms->fd, kms->cursor.handle, NULL, NULL);
        UNLOCK_KMS(kms);
        if (ret || !kms->cursor.map)
        {
            bug("[VMWareSVGA:KMS] %s: no cursor buffer (%d)\n", __func__, ret);
            kms->cursor.handle = 0;
            return FALSE;
        }
        kms->cursor.width = kms->cursor.height = VMWSVGA_CURSOR_SIZE;
    }

    if (width > VMWSVGA_CURSOR_SIZE)
        width = VMWSVGA_CURSOR_SIZE;
    if (height > VMWSVGA_CURSOR_SIZE)
        height = VMWSVGA_CURSOR_SIZE;

    dst = kms->cursor.map;
    for (y = 0; y < VMWSVGA_CURSOR_SIZE; y++)
    {
        ULONG *line = (ULONG *)((UBYTE *)dst + y * kms->cursor.pitch);

        for (x = 0; x < VMWSVGA_CURSOR_SIZE; x++)
            line[x] = (x < width && y < height) ? argb[y * width + x] : 0;
    }

    kms->cursor_ok = TRUE;
    if (kms->cursor_shown)
        return VMWareSVGA_KMS_ShowCursor(kms, TRUE);

    return TRUE;
}

BOOL VMWareSVGA_KMS_ShowCursor(struct VMWareSVGA_KMS *kms, BOOL show)
{
    int ret;

    if (!kms->cursor.handle)
        return FALSE;

    LOCK_KMS(kms);
    if (show)
        ret = drmModeSetCursor2(kms->fd, kms->crtc_id, kms->cursor.handle,
                                VMWSVGA_CURSOR_SIZE, VMWSVGA_CURSOR_SIZE, 0, 0);
    else
        ret = drmModeSetCursor(kms->fd, kms->crtc_id, 0, VMWSVGA_CURSOR_SIZE, VMWSVGA_CURSOR_SIZE);
    UNLOCK_KMS(kms);

    if (ret)
    {
        D(bug("[VMWareSVGA:KMS] %s: the device refused the cursor (%d)\n", __func__, ret));
        kms->cursor_ok = FALSE;
        return FALSE;
    }
    kms->cursor_shown = show;
    return TRUE;
}

BOOL VMWareSVGA_KMS_MoveCursor(struct VMWareSVGA_KMS *kms, LONG x, LONG y)
{
    int ret;

    if (!kms->cursor_ok)
        return FALSE;

    LOCK_KMS(kms);
    ret = drmModeMoveCursor(kms->fd, kms->crtc_id, x, y);
    UNLOCK_KMS(kms);

    return (ret == 0);
}

/* --- bring-up ----------------------------------------------------------------- */

static BOOL VMWareSVGA_KMS_SelectDisplay(struct VMWareSVGA_KMS *kms)
{
    drmModeResPtr res;
    int i;

    res = drmModeGetResources(kms->fd);
    if (!res)
    {
        bug("[VMWareSVGA:KMS] %s: no mode resources\n", __func__);
        return FALSE;
    }

    for (i = 0; i < res->count_connectors; i++)
    {
        drmModeConnectorPtr connector = drmModeGetConnector(kms->fd, res->connectors[i]);

        if (!connector)
            continue;

        D(bug("[VMWareSVGA:KMS] connector %u type %u status %u modes %d\n", connector->connector_id,
            connector->connector_type, connector->connection, connector->count_modes));

        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0)
        {
            kms->connector = connector;
            break;
        }
        drmModeFreeConnector(connector);
    }

    if (!kms->connector)
    {
        bug("[VMWareSVGA:KMS] %s: no connected display (%d connectors)\n", __func__, res->count_connectors);
        drmModeFreeResources(res);
        return FALSE;
    }

    /* the first crtc; the device pairs its display units in order */
    if (res->count_crtcs > 0)
    {
        drmModeEncoderPtr encoder = NULL;

        if (kms->connector->encoder_id)
            encoder = drmModeGetEncoder(kms->fd, kms->connector->encoder_id);

        kms->crtc_id = res->crtcs[0];
        if (encoder)
        {
            if (encoder->crtc_id)
                kms->crtc_id = encoder->crtc_id;
            else
            {
                for (i = 0; i < res->count_crtcs; i++)
                {
                    if (encoder->possible_crtcs & (1 << i))
                    {
                        kms->crtc_id = res->crtcs[i];
                        break;
                    }
                }
            }
            drmModeFreeEncoder(encoder);
        }
    }

    for (i = 0; i < kms->connector->count_modes; i++)
    {
        drmModeModeInfoPtr mode = &kms->connector->modes[i];

        if (mode->hdisplay > kms->max_width)
            kms->max_width = mode->hdisplay;
        if (mode->vdisplay > kms->max_height)
            kms->max_height = mode->vdisplay;
    }

    drmModeFreeResources(res);
    return (kms->crtc_id != 0);
}

BOOL VMWareSVGA_KMS_Init(struct VMWareSVGA_KMS *kms)
{
    memset(kms, 0, sizeof(*kms));
    InitSemaphore(&kms->lock);
    InitSemaphore(&kms->damage_lock);
    VMWareSVGA_KMS_DamageReset(kms);

    /*
     * Once the card is found the driver's helper tasks are running and
     * the module must stay resident; the caller learns that from 'started'.
     */
    if (!vmwgfx_init_findcard())
        return FALSE;
    kms->started = TRUE;
    if (vmwgfx_init() != 0)
    {
        bug("[VMWareSVGA:KMS] %s: the vmwgfx driver did not start\n", __func__);
        return FALSE;
    }

    kms->fd = drmOpen(NULL, NULL);
    if (kms->fd < 0)
    {
        bug("[VMWareSVGA:KMS] %s: drmOpen failed (%d)\n", __func__, kms->fd);
        return FALSE;
    }

    if (!VMWareSVGA_KMS_SelectDisplay(kms))
        return FALSE;

    kms->has3d = vmwgfx_aros_has_3d();
    kms->cursor_ok = vmwgfx_aros_has_mob();

    kms->render_task = NewCreateTask(TASKTAG_PC,    VMWareSVGA_KMS_RenderTask,
                                     TASKTAG_NAME,  "VMWare Render Task",
                                     TASKTAG_PRI,   1,
                                     TASKTAG_ARG1,  kms,
                                     TAG_DONE);

    kms->ready = TRUE;
    /* code anchors: a fault address minus these gives the offset in the module's .text */
    D(bug("[VMWareSVGA:KMS] code: VMWareSVGA_KMS_Init 0x%p VMWareSVGA_KMS_SetMode 0x%p\n",
        VMWareSVGA_KMS_Init, VMWareSVGA_KMS_SetMode));
    D(bug("[VMWareSVGA:KMS] %s: connector %u crtc %u, %d modes up to %ux%u, 3D %s\n", __func__,
        kms->connector->connector_id, kms->crtc_id, kms->connector->count_modes,
        kms->max_width, kms->max_height, kms->has3d ? "yes" : "no"));

    return TRUE;
}

VOID VMWareSVGA_KMS_Shutdown(struct VMWareSVGA_KMS *kms)
{
    vmwgfx_aros_reset();
}
