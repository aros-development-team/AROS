/*
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 *
 * Finite x86 bring-up probe for a dedicated non-VGA virtio-gpu PCI device.
 * Default: PCI discovery only. RUN: negotiate, query, reset and release.
 * RENDER: finite offscreen VirGL clear/readback; never changes a scanout.
 * Protocol: OASIS VirtIO 1.3, section 5.7 (GPU Device).
 */
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/rdargs.h>
#include <utility/hooks.h>
#include <hidd/pci.h>
#include <hardware/virtio.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/oop.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "virtiogpu_protocol.h"

struct Library *OOPBase;
OOP_AttrBase HiddPCIDeviceAttrBase;

#define GPU_ID 0x1050
#define PCI_COMMAND 4
#define PCI_MEMORY 2
#define PCI_MASTER 4
#define PCI_INTX_DISABLE 0x400
#define QUEUE_SIZE 8
#define RING_BYTES 4096
#define REQUEST_OFFSET 8192
#define RESPONSE_OFFSET 12288
#define RESPONSE_BYTES 8192
#define DMA_BYTES (RESPONSE_OFFSET + RESPONSE_BYTES)
#define POLL_TICKS 150

struct window { UBYTE bar; ULONG offset, length; BOOL found; };
struct state {
    OOP_Object *device, *driver;
    unsigned matches;
    APTR bars[6];
    ULONG bar_sizes[6];
    struct window windows[5];
    ULONG notify_multiplier;
    volatile struct virtio_pci_common_cfg *common;
    volatile UBYTE *config;
    volatile UBYTE *isr;
    volatile UWORD *notify;
    APTR dma, pixels, copy_pixels;
    STRPTR owner_name;
    UQUAD fence;
    volatile struct virtq_desc *desc;
    volatile struct virtq_avail *avail;
    volatile struct virtq_used *used;
    UWORD last_used, command;
    BOOL owned, changed;
};

_Static_assert(offsetof(struct virtio_pci_common_cfg, queue_desc) == 32, "PCI common layout");

static IPTR attr(OOP_Object *device, OOP_AttrID id)
{
    IPTR value = 0;
    OOP_GetAttr(device, id, &value);
    return value;
}

AROS_UFH3(void, enumerate_gpu,
    AROS_UFHA(struct Hook *, hook, A0),
    AROS_UFHA(OOP_Object *, device, A2),
    AROS_UFHA(APTR, message, A1))
{
    AROS_USERFUNC_INIT
    struct state *s = hook->h_Data;
    (void)message;
    if (attr(device, aHidd_PCIDevice_VendorID) == VIRTIO_PCI_VENDOR_ID &&
        attr(device, aHidd_PCIDevice_ProductID) == GPU_ID &&
        attr(device, aHidd_PCIDevice_Class) == 3 &&
        attr(device, aHidd_PCIDevice_SubClass) == 0x80) {
        s->device = device;
        s->matches++;
    }
    AROS_USERFUNC_EXIT
}

static BOOL capabilities(struct state *s)
{
    UBYTE seen[256] = {0};
    ULONG cur = HIDD_PCIDevice_ReadConfigByte(s->device, 0x34);
    unsigned count = 0;
    while (cur) {
        UBYTE next, id, length, type, bar;
        if (cur < 0x40 || cur > 0xfc || (cur & 3) || seen[cur] || ++count > 48)
            return FALSE;
        seen[cur] = 1;
        id = HIDD_PCIDevice_ReadConfigByte(s->device, cur);
        next = HIDD_PCIDevice_ReadConfigByte(s->device, cur + 1);
        if (id == VIRTIO_PCI_CAP_VENDOR_CFG) {
            length = HIDD_PCIDevice_ReadConfigByte(s->device, cur + 2);
            if (length < 16 || cur + length > 256) return FALSE;
            type = HIDD_PCIDevice_ReadConfigByte(s->device, cur + 3);
            bar = HIDD_PCIDevice_ReadConfigByte(s->device, cur + 4);
            if (type >= 1 && type <= 4) {
                struct window *w = &s->windows[type];
                if (bar > 5 || w->found) return FALSE;
                w->bar = bar;
                w->offset = HIDD_PCIDevice_ReadConfigLong(s->device, cur + 8);
                w->length = HIDD_PCIDevice_ReadConfigLong(s->device, cur + 12);
                w->found = TRUE;
                printf("cap type=%u bar=%u offset=%lu length=%lu\n", type, bar,
                    (unsigned long)w->offset, (unsigned long)w->length);
                if (type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
                    if (length < 20) return FALSE;
                    s->notify_multiplier = HIDD_PCIDevice_ReadConfigLong(s->device, cur + 16);
                }
            }
        }
        cur = next;
    }
    return s->windows[1].found && s->windows[2].found &&
           s->windows[3].found && s->windows[4].found;
}

static BOOL map_windows(struct state *s)
{
    static const ULONG bases[] = { aoHidd_PCIDevice_Base0, aoHidd_PCIDevice_Base1,
        aoHidd_PCIDevice_Base2, aoHidd_PCIDevice_Base3, aoHidd_PCIDevice_Base4,
        aoHidd_PCIDevice_Base5 };
    static const ULONG sizes[] = { aoHidd_PCIDevice_Size0, aoHidd_PCIDevice_Size1,
        aoHidd_PCIDevice_Size2, aoHidd_PCIDevice_Size3, aoHidd_PCIDevice_Size4,
        aoHidd_PCIDevice_Size5 };
    unsigned i;
    for (i = 1; i <= 4; i++) {
        struct window *w = &s->windows[i];
        IPTR base = attr(s->device, HiddPCIDeviceAttrBase + bases[w->bar]);
        IPTR size = attr(s->device, HiddPCIDeviceAttrBase + sizes[w->bar]);
        ULONG raw = HIDD_PCIDevice_ReadConfigLong(s->device, 0x10 + 4 * w->bar);
        if (!base || (raw & 1) || !size || size > 16 * 1024 * 1024 ||
            w->offset > size || w->length > size - w->offset) return FALSE;
        if (!s->bars[w->bar]) {
            APTR mapped = HIDD_PCIDriver_MapPCI(s->driver, (APTR)(base & ~(IPTR)15), size);
            if (!mapped || mapped == (APTR)-1) return FALSE;
            s->bars[w->bar] = mapped;
            s->bar_sizes[w->bar] = size;
        }
    }
    if (s->windows[1].length < sizeof(*s->common) || s->windows[3].length < 1 ||
        s->windows[4].length < 16 || (s->windows[1].offset & 3) ||
        (s->windows[4].offset & 3)) return FALSE;
    s->common = (APTR)((UBYTE *)s->bars[s->windows[1].bar] + s->windows[1].offset);
    s->config = (APTR)((UBYTE *)s->bars[s->windows[4].bar] + s->windows[4].offset);
    s->isr = (APTR)((UBYTE *)s->bars[s->windows[3].bar] + s->windows[3].offset);
    return TRUE;
}

static BOOL reset(struct state *s)
{
    unsigned i;
    s->common->device_status = 0;
    __sync_synchronize();
    for (i = 0; i < POLL_TICKS; i++) {
        if (s->common->device_status == 0) return TRUE;
        Delay(1);
    }
    return FALSE;
}

static UQUAD dma_address(struct state *s, APTR ptr)
{
    APTR address = HIDD_PCIDriver_CPUtoPCI(s->driver, ptr);
    return !address || address == (APTR)-1 ? ~(UQUAD)0 : (UQUAD)(IPTR)address;
}

static BOOL setup_queue(struct state *s, UWORD id)
{
    UBYTE *base = (UBYTE *)s->dma + id * RING_BYTES;
    volatile struct virtq_avail *avail = (APTR)(base + 256);
    struct window *w = &s->windows[2];
    UQUAD offset, desc = dma_address(s, base), avail_dma = dma_address(s, base + 256);
    UQUAD used = dma_address(s, base + 512);
    if (!desc || desc == ~(UQUAD)0 || !avail_dma || avail_dma == ~(UQUAD)0 ||
        !used || used == ~(UQUAD)0) return FALSE;
    s->common->queue_select = id;
    __sync_synchronize();
    if (s->common->queue_size < QUEUE_SIZE || s->common->queue_enable) return FALSE;
    s->common->queue_size = QUEUE_SIZE;
    s->common->queue_msix_vector = 0xffff;
    s->common->queue_desc = desc;
    s->common->queue_driver = avail_dma;
    s->common->queue_device = used;
    avail->flags = VIRTQ_AVAIL_F_NO_INTERRUPT;
    offset = (UQUAD)s->common->queue_notify_off * s->notify_multiplier;
    if (((w->offset + offset) & 1) || offset > w->length || w->length - offset < sizeof(UWORD)) return FALSE;
    if (!id) s->notify = (APTR)((UBYTE *)s->bars[w->bar] + w->offset + offset);
    __sync_synchronize();
    s->common->queue_enable = 1;
    return TRUE;
}

static BOOL command(struct state *s, const void *input, ULONG req_len,
                    ULONG expected, ULONG minimum, ULONG *received, BOOL fenced)
{
    struct gpu_header *req = (APTR)((UBYTE *)s->dma + REQUEST_OFFSET);
    struct gpu_header *resp = (APTR)((UBYTE *)s->dma + RESPONSE_OFFSET);
    ULONG type, len, i;
    UWORD index;
    if (req_len < sizeof(*req) || req_len > RESPONSE_OFFSET - REQUEST_OFFSET)
        return FALSE;
    memcpy(req, input, req_len);
    memset(resp, 0, RESPONSE_BYTES);
    type = req->type;
    if (fenced) { req->flags = GPU_FLAG_FENCE; req->fence = ++s->fence; }
    len = req_len;
    if (!CachePreDMA(req, &len, DMA_ReadFromRAM) || len < req_len) return FALSE;
    len = RESPONSE_BYTES;
    if (!CachePreDMA(resp, &len, 0) || len < RESPONSE_BYTES) return FALSE;
    s->desc[0].addr = dma_address(s, req);
    s->desc[0].len = req_len;
    s->desc[0].flags = VIRTQ_DESC_F_NEXT;
    s->desc[0].next = 1;
    s->desc[1].addr = dma_address(s, resp);
    s->desc[1].len = RESPONSE_BYTES;
    s->desc[1].flags = VIRTQ_DESC_F_WRITE;
    s->desc[1].next = 0;
    index = s->avail->idx;
    s->avail->ring[index % QUEUE_SIZE] = 0;
    __sync_synchronize();
    s->avail->idx = index + 1;
    __sync_synchronize();
    *s->notify = 0;
    for (i = 0; i < POLL_TICKS; i++) {
        __sync_synchronize();
        if (s->used->idx != s->last_used) break;
        if (SetSignal(0, 0) & SIGBREAKF_CTRL_C) break;
        Delay(1);
    }
    if (s->used->idx == s->last_used) {
        printf("FAIL: command 0x%lx timed out\n", (unsigned long)type);
        return FALSE;
    }
    __sync_synchronize();
    *received = s->used->ring[s->last_used % QUEUE_SIZE].len;
    if (s->used->idx != (UWORD)(s->last_used + 1) ||
        s->used->ring[s->last_used % QUEUE_SIZE].id != 0) return FALSE;
    s->last_used++;
    len = RESPONSE_BYTES; CachePostDMA(resp, &len, 0);
    len = req_len; CachePostDMA(req, &len, DMA_ReadFromRAM);
    (void)*s->isr;
    if (*received < minimum || *received > RESPONSE_BYTES || resp->type != expected) {
        printf("FAIL: command 0x%lx response=0x%lx bytes=%lu\n",
            (unsigned long)type, (unsigned long)resp->type, (unsigned long)*received);
        return FALSE;
    }
    if (fenced && (!(resp->flags & GPU_FLAG_FENCE) || resp->fence != s->fence)) {
        puts("FAIL: fence completion mismatch"); return FALSE;
    }
    return TRUE;
}

static BOOL query(struct state *s, ULONG type, ULONG a, ULONG b,
                  ULONG expected, ULONG minimum, ULONG *received)
{
    struct capset_request req = {0};
    req.header.type = type; req.id = a; req.version = b;
    return command(s, &req, type == GPU_GET_DISPLAY ? sizeof(req.header) : sizeof(req),
                   expected, minimum, received, FALSE);
}

/* Wire layouts: VirtIO GPU 1.3; VirGL words: Mesa virgl_protocol.h.
 * Minimal offscreen experiment, not a command encoder for application use. */
#define PIXEL_WIDTH 16
#define PIXEL_BYTES (PIXEL_WIDTH * PIXEL_WIDTH * 4)
#define VIRGL_WORD(cmd, obj, count) ((cmd) | ((obj) << 8) | ((count) << 16))

static BOOL render_command(struct state *s, const void *req, ULONG len, BOOL fenced)
{
    ULONG received;
    return command(s, req, len, GPU_OK_NODATA, sizeof(struct gpu_header), &received, fenced);
}

static BOOL render_probe(struct state *s)
{
    struct render_context ctx = { .header.type = GPU_CTX_CREATE, .header.context = 1,
        .length = 15, .name = "AROS clear test" };
    struct render_create res = { .header.type = GPU_RESOURCE_CREATE_3D, .resource = 1,
        .target = VIRGL_TEXTURE_2D, .format = VIRGL_FORMAT_RGBA8,
        .bind = VIRGL_BIND_RENDER_TARGET, .width = PIXEL_WIDTH,
        .height = PIXEL_WIDTH, .depth = 1, .array_size = 1 };
    struct render_resource ref = { .header.type = GPU_CTX_ATTACH_RESOURCE, .header.context = 1,
        .resource = 1 };
    struct render_backing backing = { .header.type = GPU_ATTACH_BACKING, .resource = 1,
        .count = 1, .length = PIXEL_BYTES };
    struct render_transfer transfer = { .header.type = GPU_TRANSFER_FROM_HOST_3D, .header.context = 1,
        .width = PIXEL_WIDTH, .height = PIXEL_WIDTH, .depth = 1,
        .resource = 1, .stride = PIXEL_WIDTH * 4, .layer_stride = PIXEL_BYTES };
    struct render_submit submit = { .header.type = GPU_SUBMIT_3D, .header.context = 1 };
    UBYTE *pixels;
    ULONG pass, i, n, len;
    s->pixels = HIDD_PCIDriver_AllocPCIMem(s->driver, PIXEL_BYTES);
    if (!s->pixels) return FALSE;
    pixels = s->pixels;
    backing.address = dma_address(s, pixels);
    if (!backing.address || backing.address == ~(UQUAD)0) return FALSE;
    if (!render_command(s, &ctx, sizeof(ctx), FALSE) ||
        !render_command(s, &res, sizeof(res), FALSE) ||
        !render_command(s, &ref, sizeof(ref), FALSE) ||
        !render_command(s, &backing, sizeof(backing), FALSE)) return FALSE;
    for (pass = 0; pass < 2; pass++) {
        UBYTE expected[4] = {pass ? 0 : 255, pass ? 255 : 0, 0, 255};
        n = 0;
        if (!pass) {
            /* Create surface handle 1 on resource 1, RGBA8, level/layer zero. */
            submit.words[n++] = VIRGL_WORD(VIRGL_CREATE_OBJECT, VIRGL_OBJECT_SURFACE, 5);
            submit.words[n++] = 1; submit.words[n++] = 1; submit.words[n++] = VIRGL_FORMAT_RGBA8;
            submit.words[n++] = 0; submit.words[n++] = 0;
            /* Bind one color surface, no depth surface. */
            submit.words[n++] = VIRGL_WORD(VIRGL_SET_FRAMEBUFFER, 0, 3);
            submit.words[n++] = 1; submit.words[n++] = 0; submit.words[n++] = 1;
        }
        /* Clear COLOR0; float colors, double depth zero, stencil zero. */
        submit.words[n++] = VIRGL_WORD(VIRGL_CLEAR, 0, 8);
        submit.words[n++] = VIRGL_CLEAR_COLOR0;
        submit.words[n++] = pass ? 0 : FLOAT_ONE_BITS;
        submit.words[n++] = pass ? FLOAT_ONE_BITS : 0;
        submit.words[n++] = 0; submit.words[n++] = FLOAT_ONE_BITS;
        submit.words[n++] = 0; submit.words[n++] = 0; submit.words[n++] = 0;
        submit.size = n * sizeof(ULONG);
        memset(pixels, 0xa5, PIXEL_BYTES);
        len = PIXEL_BYTES;
        if (!CachePreDMA(pixels, &len, 0) || len < PIXEL_BYTES) return FALSE;
        if (!render_command(s, &submit, offsetof(struct render_submit, words) + submit.size, TRUE) ||
            !render_command(s, &transfer, sizeof(transfer), TRUE)) return FALSE;
        len = PIXEL_BYTES; CachePostDMA(pixels, &len, 0);
        __sync_synchronize();
        for (i = 0; i < PIXEL_BYTES; i++) {
            if (pixels[i] != expected[i % 4]) {
                printf("FAIL: clear %lu byte %lu expected=%u actual=%u\n",
                    (unsigned long)pass, (unsigned long)i, expected[i % 4], pixels[i]);
                return FALSE;
            }
        }
        printf("VirGL clear %lu: %ux%u RGBA=%u,%u,%u,%u; all pixels correct, fence=%llu\n",
            (unsigned long)pass, PIXEL_WIDTH, PIXEL_WIDTH,
            pixels[0], pixels[1], pixels[2], pixels[3], (unsigned long long)s->fence);
    }
    /* Copy into a second texture, preserving a blue border around each region. */
    s->copy_pixels = HIDD_PCIDriver_AllocPCIMem(s->driver, PIXEL_BYTES);
    if (!s->copy_pixels) return FALSE;
    pixels = s->copy_pixels;
    res.resource = ref.resource = backing.resource = transfer.resource = 2;
    backing.address = dma_address(s, pixels);
    if (!backing.address || backing.address == ~(UQUAD)0) return FALSE;
    if (!render_command(s, &res, sizeof(res), FALSE) ||
        !render_command(s, &ref, sizeof(ref), FALSE) ||
        !render_command(s, &backing, sizeof(backing), FALSE)) return FALSE;
    for (i = 0; i < PIXEL_BYTES; i += 4) {
        pixels[i] = 0; pixels[i + 1] = 0; pixels[i + 2] = 255; pixels[i + 3] = 255;
    }
    len = PIXEL_BYTES;
    if (!CachePreDMA(pixels, &len, DMA_ReadFromRAM) || len < PIXEL_BYTES) return FALSE;
    transfer.header.type = GPU_TRANSFER_TO_HOST_3D;
    if (!render_command(s, &transfer, sizeof(transfer), TRUE)) return FALSE;
    len = PIXEL_BYTES; CachePostDMA(pixels, &len, DMA_ReadFromRAM);
    transfer.header.type = GPU_TRANSFER_FROM_HOST_3D;
    for (pass = 0; pass < 2; pass++) {
        memset(submit.words, 0, sizeof(submit.words));
        if (!pass) {
            /* RESOURCE_COPY_REGION: dst2@(3,5), source1@(0,0), box4x6x1. */
            submit.words[0] = VIRGL_WORD(VIRGL_COPY_REGION, 0, 13);
            submit.words[1] = 2; submit.words[3] = 3; submit.words[4] = 5;
            submit.words[6] = 1; submit.words[11] = 4;
            submit.words[12] = 6; submit.words[13] = 1;
            submit.size = 14 * sizeof(ULONG);
        } else {
            /* BLIT: nearest RGBA, source16x16 -> destination3x2@(10,1). */
            submit.words[0] = VIRGL_WORD(VIRGL_BLIT, 0, 21);
            submit.words[1] = VIRGL_MASK_RGBA;
            submit.words[4] = 2; submit.words[6] = VIRGL_FORMAT_RGBA8;
            submit.words[7] = 10; submit.words[8] = 1;
            submit.words[10] = 3; submit.words[11] = 2; submit.words[12] = 1;
            submit.words[13] = 1; submit.words[15] = VIRGL_FORMAT_RGBA8;
            submit.words[19] = PIXEL_WIDTH; submit.words[20] = PIXEL_WIDTH;
            submit.words[21] = 1;
            submit.size = 22 * sizeof(ULONG);
        }
        memset(pixels, 0xa5, PIXEL_BYTES);
        len = PIXEL_BYTES;
        if (!CachePreDMA(pixels, &len, 0) || len < PIXEL_BYTES) return FALSE;
        if (!render_command(s, &submit, offsetof(struct render_submit, words) + submit.size, TRUE) ||
            !render_command(s, &transfer, sizeof(transfer), TRUE)) return FALSE;
        len = PIXEL_BYTES; CachePostDMA(pixels, &len, 0);
        __sync_synchronize();
        for (i = 0; i < PIXEL_BYTES; i++) {
            ULONG x = (i / 4) % PIXEL_WIDTH, y = (i / 4) / PIXEL_WIDTH;
            BOOL green = (x >= 3 && x < 7 && y >= 5 && y < 11) ||
                (pass && x >= 10 && x < 13 && y >= 1 && y < 3);
            UBYTE expected = i % 4 == 3 ? 255 :
                (i % 4 == (green ? 1 : 2) ? 255 : 0);
            if (pixels[i] != expected) {
                printf("FAIL: copy/blit %lu pixel %lu,%lu channel %lu expected=%u actual=%u\n",
                    (unsigned long)pass, (unsigned long)x, (unsigned long)y,
                    (unsigned long)(i % 4), expected, pixels[i]);
                return FALSE;
            }
        }
        printf("VirGL %s: all destination and border pixels correct, fence=%llu\n",
            pass ? "scaled blit 16x16 -> 3x2" : "copy region 4x6",
            (unsigned long long)s->fence);
    }
    /* Release destination, then source, while the context still exists. */
    ref.header.type = GPU_CTX_DETACH_RESOURCE;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ref.header.context = 0; ref.header.type = GPU_DETACH_BACKING;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ref.header.type = GPU_RESOURCE_UNREF;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ref.header.context = 1; ref.resource = 1;
    /* Destroy surface; detach and release all host objects before reset. */
    submit.words[0] = VIRGL_WORD(VIRGL_DESTROY_OBJECT, VIRGL_OBJECT_SURFACE, 1); submit.words[1] = 1; submit.size = 8;
    if (!render_command(s, &submit, offsetof(struct render_submit, words) + submit.size, TRUE)) return FALSE;
    ref.header.type = GPU_CTX_DETACH_RESOURCE;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ref.header.context = 0; ref.header.type = GPU_DETACH_BACKING;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ref.header.type = GPU_RESOURCE_UNREF;
    if (!render_command(s, &ref, sizeof(ref), FALSE)) return FALSE;
    ctx.header.type = GPU_CTX_DESTROY;
    if (!render_command(s, &ctx, sizeof(ctx.header), FALSE)) return FALSE;
    puts("Offscreen VirGL clear/copy/scaled-blit/readback completed; no scanout or Gallium integration tested");
    return TRUE;
}

static BOOL run_probe(struct state *s, BOOL require_virgl, BOOL render)
{
    UQUAD have, wanted;
    ULONG caps, scanouts, generation, bytes, i;
    struct gpu_display_info *display;
    BOOL virgl_capset = FALSE;
    if (s->common->device_status != 0) {
        puts("FAIL: device is already initialized; refusing takeover"); return FALSE;
    }
    s->changed = TRUE;
    if (!reset(s)) return FALSE;
    s->common->device_status = VIRTIO_CONFIG_S_ACKNOWLEDGE | VIRTIO_CONFIG_S_DRIVER;
    s->common->device_feature_select = 0;
    __sync_synchronize();
    have = s->common->device_feature;
    s->common->device_feature_select = 1;
    __sync_synchronize();
    have |= (UQUAD)s->common->device_feature << 32;
    printf("device features: %08lx:%08lx\n", (unsigned long)(have >> 32),
        (unsigned long)(ULONG)have);
    if (!(have & (1ULL << VIRTIO_F_VERSION_1))) return FALSE;
    wanted = (1ULL << VIRTIO_F_VERSION_1) | (have & GPU_F_VIRGL);
    s->common->guest_feature_select = 0; s->common->guest_feature = (ULONG)wanted;
    s->common->guest_feature_select = 1; s->common->guest_feature = (ULONG)(wanted >> 32);
    __sync_synchronize();
    s->common->device_status |= VIRTIO_CONFIG_S_FEATURES_OK;
    __sync_synchronize();
    if (!(s->common->device_status & VIRTIO_CONFIG_S_FEATURES_OK)) return FALSE;
    printf("negotiated VIRGL: %s\n", (wanted & GPU_F_VIRGL) ? "yes" : "no");
    for (i = 0; i < 10; i++) {
        generation = s->common->config_generation;
        scanouts = *(volatile ULONG *)(s->config + 8);
        caps = *(volatile ULONG *)(s->config + 12);
        if (generation == s->common->config_generation) break;
    }
    if (i == 10 || !scanouts || scanouts > 16 || caps > 16 || s->common->num_queues < 2)
        return FALSE;
    printf("scanouts=%lu capsets=%lu queues=%u\n", (unsigned long)scanouts,
        (unsigned long)caps, s->common->num_queues);
    s->dma = HIDD_PCIDriver_AllocPCIMem(s->driver, DMA_BYTES);
    if (!s->dma || ((IPTR)s->dma & 15)) return FALSE;
    memset(s->dma, 0, DMA_BYTES);
    s->desc = s->dma;
    s->avail = (APTR)((UBYTE *)s->dma + 256);
    s->used = (APTR)((UBYTE *)s->dma + 512);
    if (!setup_queue(s, 0) || !setup_queue(s, 1)) return FALSE;
    s->common->device_status |= VIRTIO_CONFIG_S_DRIVER_OK;
    if (!query(s, GPU_GET_DISPLAY, 0, 0, GPU_OK_DISPLAY, sizeof(*display), &bytes)) return FALSE;
    display = (APTR)((UBYTE *)s->dma + RESPONSE_OFFSET);
    printf("display query: %lu bytes, first=%lux%lu enabled=%lu\n", (unsigned long)bytes,
        (unsigned long)display->display[0].rect.width,
        (unsigned long)display->display[0].rect.height,
        (unsigned long)display->display[0].enabled);
    for (i = 0; i < caps; i++) {
        struct capset_info *info;
        ULONG id, version, size, j, hash = 2166136261UL;
        UBYTE *payload = (UBYTE *)s->dma + RESPONSE_OFFSET + sizeof(struct gpu_header);
        if (!query(s, GPU_GET_CAPSET_INFO, i, 0, GPU_OK_CAPSET_INFO, sizeof(*info), &bytes)) return FALSE;
        info = (APTR)((UBYTE *)s->dma + RESPONSE_OFFSET);
        id = info->id; version = info->version; size = info->size;
        if (!id || !size || size > RESPONSE_BYTES - sizeof(struct gpu_header)) return FALSE;
        if (!query(s, GPU_GET_CAPSET, id, version, GPU_OK_CAPSET,
                   sizeof(struct gpu_header) + size, &bytes)) return FALSE;
        if (id == 1 || id == 2) virgl_capset = TRUE;
        for (j = 0; j < size; j++) hash = (hash ^ payload[j]) * 16777619UL;
        printf("capset index=%lu id=%lu version=%lu size=%lu fnv1a=%08lx\n",
            (unsigned long)i, (unsigned long)id, (unsigned long)version,
            (unsigned long)size, (unsigned long)hash);
    }
    if (require_virgl && (!(wanted & GPU_F_VIRGL) || !virgl_capset)) {
        puts("FAIL: required VirGL is unavailable"); return FALSE;
    }
    if (render) return render_probe(s);
    puts("control queue queries completed (no rendering tested)");
    return TRUE;
}

static BOOL cleanup(struct state *s)
{
    BOOL safe = TRUE;
    unsigned i;
    if (s->changed && !reset(s)) {
        puts("FAIL: reset did not complete; DMA allocation retained until reboot");
        safe = FALSE;
    }
    if (s->owned) {
        UWORD command = safe ? s->command :
            ((s->command & ~PCI_MASTER) | PCI_INTX_DISABLE);
        HIDD_PCIDevice_WriteConfigWord(s->device, PCI_COMMAND, command);
    }
    if (s->copy_pixels && safe) HIDD_PCIDriver_FreePCIMem(s->driver, s->copy_pixels);
    if (s->pixels && safe) HIDD_PCIDriver_FreePCIMem(s->driver, s->pixels);
    if (s->dma && safe) HIDD_PCIDriver_FreePCIMem(s->driver, s->dma);
    for (i = 0; i < 6; i++)
        if (s->bars[i]) HIDD_PCIDriver_UnmapPCI(s->driver, s->bars[i], s->bar_sizes[i]);
    if (s->owned && safe) HIDD_PCIDevice_Release(s->device);
    if (!safe) puts("FAIL: PCI ownership and DMA quarantined; reboot before reuse");
    if (s->owner_name && safe) FreeVec(s->owner_name);
    return safe;
}

int main(void)
{
    struct state s = {0};
    IPTR args[3] = {0};
    struct RDArgs *rda;
    OOP_Object *pci = NULL;
    struct Hook hook = { .h_Entry = (APTR)enumerate_gpu, .h_Data = &s };
    struct TagItem requirements[] = {
        {tHidd_PCI_VendorID, VIRTIO_PCI_VENDOR_ID}, {TAG_DONE, 0}
    };
    int result = RETURN_FAIL;
    BOOL opened_oop = FALSE;
    puts("virtiogpuprobe 0.4: x86_64 PCI/control-queue diagnostic");
#if !defined(__x86_64__)
    puts("FAIL: this diagnostic supports x86_64 only"); return RETURN_FAIL;
#endif
    rda = ReadArgs("RUN/S,REQUIREVIRGL/S,RENDER/S", args, NULL);
    if (!rda) { PrintFault(IoErr(), "virtiogpuprobe"); return RETURN_FAIL; }
    if (args[2] && !args[0]) { puts("FAIL: RENDER needs RUN"); goto done; }
    if (args[1] && !args[0]) { puts("FAIL: REQUIREVIRGL needs RUN"); goto done; }
    OOPBase = OpenLibrary("oop.library", 0);
    if (!OOPBase) goto done;
    opened_oop = TRUE;
    HiddPCIDeviceAttrBase = OOP_ObtainAttrBase(IID_Hidd_PCIDevice);
    if (!HiddPCIDeviceAttrBase) goto done;
    pci = OOP_NewObject(NULL, CLID_Hidd_PCI, NULL);
    if (!pci) goto done;
    HIDD_PCI_EnumDevices(pci, &hook, requirements);
    if (s.matches != 1) {
        printf("FAIL: expected one dedicated non-VGA virtio-gpu, found %u\n", s.matches); goto done;
    }
    printf("GPU PCI %lu:%lu.%lu vendor=1af4 device=1050\n",
        (unsigned long)attr(s.device, aHidd_PCIDevice_Bus),
        (unsigned long)attr(s.device, aHidd_PCIDevice_Dev),
        (unsigned long)attr(s.device, aHidd_PCIDevice_Sub));
    if (!capabilities(&s)) { puts("FAIL: invalid or missing modern capabilities"); goto done; }
    if (!args[0]) { result = RETURN_OK; goto done; }
    /* Obtain stores the string pointer. Keep it valid if reset failure
     * quarantines ownership beyond this process's lifetime. */
    s.owner_name = AllocVec(80, MEMF_ANY);
    if (!s.owner_name) goto done;
    strcpy(s.owner_name, "virtiogpuprobe (reset failure requires reboot)");
    {
        CONST_STRPTR owner = HIDD_PCIDevice_Obtain(s.device, s.owner_name);
        if (owner) { printf("FAIL: device already owned by %s\n", owner); goto done; }
    }
    s.owned = TRUE;
    s.command = HIDD_PCIDevice_ReadConfigWord(s.device, PCI_COMMAND);
    s.driver = (APTR)attr(s.device, aHidd_PCIDevice_Driver);
    if (!s.driver) goto done;
    /* Decode MMIO for inspection, but do not enable DMA or change interrupts
     * before refusing a device that is already initialized. */
    HIDD_PCIDevice_WriteConfigWord(s.device, PCI_COMMAND, s.command | PCI_MEMORY);
    if (!map_windows(&s)) { puts("FAIL: BAR mapping/validation"); goto done; }
    if (s.common->device_status != 0) {
        puts("FAIL: device is already initialized; refusing takeover"); goto done;
    }
    HIDD_PCIDevice_WriteConfigWord(s.device, PCI_COMMAND,
        s.command | PCI_MEMORY | PCI_MASTER | PCI_INTX_DISABLE);
    if (run_probe(&s, args[1] != 0 || args[2] != 0, args[2] != 0)) result = RETURN_OK;
 done:
    if (!cleanup(&s)) result = RETURN_FAIL;
    if (pci) OOP_DisposeObject(pci);
    if (HiddPCIDeviceAttrBase) OOP_ReleaseAttrBase(IID_Hidd_PCIDevice);
    if (opened_oop) CloseLibrary(OOPBase);
    if (result == RETURN_OK)
        puts(args[0] ? "PASS: control queries completed; device reset and PCI resources released"
                     : "PASS: PCI discovery only; RUN enables control-queue probe");
    else puts("FAIL: qualification did not complete");
    FreeArgs(rda);
    return result;
}
