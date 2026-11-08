/*
    Copyright 2026, The AROS Development Team. All rights reserved.

    Desc: The driver's operations on its scan-out bitmaps, done by the SDMA
          engine through the display's own DRM client. Copies and fills are
          submitted and left running; reads, and CPU access to a bitmap
          (Amdgpu_2D_Sync), wait for them.
*/

#include <aros/debug.h>
#include <proto/exec.h>
#include <proto/oop.h>
#include <string.h>

#include <amdgpu_drm.h>

#include "amdgpu_intern.h"
#include "amdgpu_2d.h"

extern long long ktime_get(void);

#define SDMA_PACKET(op, sub, n)     ((((n) & 0xFFFF) << 16) | (((sub) & 0xFF) << 8) | ((op) & 0xFF))
#define SDMA_OP_NOP                 0x0
#define SDMA_OP_COPY                0x1
#define SDMA_SUBOP_COPY_SUB_WINDOW  0x4
#define SDMA_OP_CONSTANT_FILL       0xb
#define SDMA_FILL_DWORDS            (2u << 30)

#define IB_SLOTS        8
#define IB_SLOT_DW      16384
#define VA_SPAN         (256ULL << 20)      /* GPU address room per buffer handle */
#define WAIT_NS         2000000000LL

struct gpu_buf
{
    ULONG       handle;
    UQUAD       va;
    UQUAD       size;
    APTR        map;
};

static struct
{
    struct SignalSemaphore      lock;
    struct Amdgpu_staticdata    *sd;
    BOOL                        tried;
    BOOL                        ok;
    int                         fd;
    ULONG                       ctx;
    ULONG                       sdma;           /* SDMA IP major version */
    UQUAD                       va_base, va_end;

    struct gpu_buf              ib;
    UQUAD                       slot_seq[IB_SLOTS];
    ULONG                       slot;
    ULONG                       *cs;
    ULONG                       cdw;
    struct drm_amdgpu_bo_list_entry bos[4];
    ULONG                       nbos;

    UQUAD                       last_seq;
    UQUAD                       done_seq;

    struct gpu_buf              staging;        /* GTT, for reads */
    struct gpu_buf              temp;           /* VRAM, for overlapping copies */
} sdma2d;

/* ---- buffers ---- */

static UQUAD sdma_va(ULONG handle, UQUAD size)
{
    UQUAD va = sdma2d.va_base + (UQUAD)handle * VA_SPAN;

    if (!handle || size > VA_SPAN || va + VA_SPAN > sdma2d.va_end)
        return 0;
    return va;
}

static BOOL sdma_map_va(ULONG handle, UQUAD size, UQUAD *out)
{
    struct drm_amdgpu_gem_va req;
    UQUAD va = sdma_va(handle, size);

    if (!va)
        return FALSE;
    memset(&req, 0, sizeof(req));
    req.handle = handle;
    req.operation = AMDGPU_VA_OP_MAP;
    req.flags = AMDGPU_VM_PAGE_READABLE | AMDGPU_VM_PAGE_WRITEABLE | AMDGPU_VM_PAGE_EXECUTABLE;
    req.va_address = va;
    req.map_size = (size + 4095) & ~4095ULL;
    if (drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_GEM_VA, &req))
        return FALSE;
    *out = va;
    return TRUE;
}

static void sdma_unmap_va(ULONG handle, UQUAD va, UQUAD size)
{
    struct drm_amdgpu_gem_va req;

    memset(&req, 0, sizeof(req));
    req.handle = handle;
    req.operation = AMDGPU_VA_OP_UNMAP;
    req.va_address = va;
    req.map_size = (size + 4095) & ~4095ULL;
    drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_GEM_VA, &req);
}

static void sdma_buf_free(struct gpu_buf *b)
{
    struct drm_gem_close close;

    if (!b->handle)
        return;
    if (b->map)
        drmMUnmap(sdma2d.fd, b->handle);
    if (b->va)
        sdma_unmap_va(b->handle, b->va, b->size);
    memset(&close, 0, sizeof(close));
    close.handle = b->handle;
    drmIoctl(sdma2d.fd, DRM_IOCTL_GEM_CLOSE, &close);
    memset(b, 0, sizeof(*b));
}

static BOOL sdma_buf_alloc(struct gpu_buf *b, UQUAD size, ULONG domain, UQUAD flags, BOOL map)
{
    union drm_amdgpu_gem_create req;

    memset(b, 0, sizeof(*b));
    memset(&req, 0, sizeof(req));
    size = (size + 4095) & ~4095ULL;
    req.in.bo_size = size;
    req.in.alignment = 4096;
    req.in.domains = domain;
    req.in.domain_flags = flags;
    if (drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_GEM_CREATE, &req))
        return FALSE;
    b->handle = req.out.handle;
    b->size = size;
    if (!sdma_map_va(b->handle, size, &b->va) || (map && !(b->map = drmMMap(sdma2d.fd, b->handle, NULL, NULL))))
    {
        sdma_buf_free(b);
        return FALSE;
    }
    return TRUE;
}

/* ---- submission ---- */

static BOOL sdma_wait(UQUAD seq)
{
    union drm_amdgpu_wait_cs req;

    if (!seq || seq <= sdma2d.done_seq)
        return TRUE;
    memset(&req, 0, sizeof(req));
    req.in.handle = seq;
    req.in.ip_type = AMDGPU_HW_IP_DMA;
    req.in.ctx_id = sdma2d.ctx;
    req.in.timeout = ktime_get() + WAIT_NS;
    if (drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_WAIT_CS, &req) || req.out.status)
    {
        bug("[Amdgpu:2D] SDMA did not finish submission %llu, 2D stays on the CPU\n", (unsigned long long)seq);
        sdma2d.ok = FALSE;
        return FALSE;
    }
    sdma2d.done_seq = seq;
    return TRUE;
}

static void sdma_use(ULONG handle)
{
    ULONG i;

    for (i = 0; i < sdma2d.nbos; i++)
        if (sdma2d.bos[i].bo_handle == handle)
            return;
    sdma2d.bos[sdma2d.nbos].bo_handle = handle;
    sdma2d.bos[sdma2d.nbos].bo_priority = 0;
    sdma2d.nbos++;
}

static BOOL sdma_begin(void)
{
    sdma2d.slot = (sdma2d.slot + 1) % IB_SLOTS;
    if (!sdma_wait(sdma2d.slot_seq[sdma2d.slot]))
        return FALSE;
    sdma2d.cs = (ULONG *)sdma2d.ib.map + sdma2d.slot * IB_SLOT_DW;
    sdma2d.cdw = 0;
    sdma2d.nbos = 0;
    sdma_use(sdma2d.ib.handle);
    return TRUE;
}

static BOOL sdma_submit(void)
{
    struct drm_amdgpu_cs_chunk_ib ib;
    struct drm_amdgpu_bo_list_in bolist;
    struct drm_amdgpu_cs_chunk chunks[2];
    UQUAD chunk_ptrs[2];
    union drm_amdgpu_cs cs;

    if (!sdma2d.cdw)
        return TRUE;
    while (sdma2d.cdw & 7)
        sdma2d.cs[sdma2d.cdw++] = SDMA_PACKET(SDMA_OP_NOP, 0, 0);

    memset(&ib, 0, sizeof(ib));
    ib.va_start = sdma2d.ib.va + (UQUAD)sdma2d.slot * IB_SLOT_DW * 4;
    ib.ib_bytes = sdma2d.cdw * 4;
    ib.ip_type = AMDGPU_HW_IP_DMA;

    memset(&bolist, 0, sizeof(bolist));
    bolist.bo_number = sdma2d.nbos;
    bolist.bo_info_size = sizeof(struct drm_amdgpu_bo_list_entry);
    bolist.bo_info_ptr = (UQUAD)(IPTR)sdma2d.bos;

    chunks[0].chunk_id = AMDGPU_CHUNK_ID_IB;
    chunks[0].length_dw = sizeof(ib) / 4;
    chunks[0].chunk_data = (UQUAD)(IPTR)&ib;
    chunks[1].chunk_id = AMDGPU_CHUNK_ID_BO_HANDLES;
    chunks[1].length_dw = sizeof(bolist) / 4;
    chunks[1].chunk_data = (UQUAD)(IPTR)&bolist;
    chunk_ptrs[0] = (UQUAD)(IPTR)&chunks[0];
    chunk_ptrs[1] = (UQUAD)(IPTR)&chunks[1];

    memset(&cs, 0, sizeof(cs));
    cs.in.ctx_id = sdma2d.ctx;
    cs.in.num_chunks = 2;
    cs.in.chunks = (UQUAD)(IPTR)chunk_ptrs;
    sdma2d.cdw = 0;
    if (drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_CS, &cs))
    {
        bug("[Amdgpu:2D] SDMA submission refused, 2D stays on the CPU\n");
        sdma2d.ok = FALSE;
        return FALSE;
    }
    sdma2d.slot_seq[sdma2d.slot] = sdma2d.last_seq = cs.out.handle;
    return TRUE;
}

/* Room for n more dwords in the IB being built (and its padding),
   submitting it and starting the next when it is full */
static BOOL sdma_room(ULONG n)
{
    if (sdma2d.cdw + n + 8 <= IB_SLOT_DW)
        return TRUE;
    return sdma_submit() && sdma_begin();
}

static void sdma_emit_fill(UQUAD va, UQUAD bytes, ULONG value)
{
    ULONG *p = sdma2d.cs + sdma2d.cdw;

    p[0] = SDMA_PACKET(SDMA_OP_CONSTANT_FILL, 0, 0) | SDMA_FILL_DWORDS;
    p[1] = (ULONG)va;
    p[2] = (ULONG)(va >> 32);
    p[3] = value;
    p[4] = (ULONG)(bytes - 1);
    sdma2d.cdw += 5;
}

struct sdma_surf
{
    UQUAD   va;
    ULONG   pitch;          /* in pixels */
    ULONG   height;
    LONG    x, y;
};

static void sdma_emit_copy(const struct sdma_surf *src, const struct sdma_surf *dst, LONG w, LONG h)
{
    ULONG shift = sdma2d.sdma >= 7 ? 16 : 13;
    ULONG *p = sdma2d.cs + sdma2d.cdw;

    p[0] = SDMA_PACKET(SDMA_OP_COPY, SDMA_SUBOP_COPY_SUB_WINDOW, 0) | (2u << 29);
    p[1] = (ULONG)src->va;
    p[2] = (ULONG)(src->va >> 32);
    p[3] = src->x | (src->y << 16);
    p[4] = (src->pitch - 1) << shift;
    p[5] = src->pitch * src->height - 1;
    p[6] = (ULONG)dst->va;
    p[7] = (ULONG)(dst->va >> 32);
    p[8] = dst->x | (dst->y << 16);
    p[9] = (dst->pitch - 1) << shift;
    p[10] = dst->pitch * dst->height - 1;
    p[11] = (w - 1) | ((h - 1) << 16);
    p[12] = 0;
    sdma2d.cdw += 13;
}

/* the next packet reads what the ones before it wrote */
static void sdma_emit_barrier(void)
{
    sdma2d.cs[sdma2d.cdw++] = SDMA_PACKET(SDMA_OP_NOP, 0, 0);
}

/* ---- setup ---- */

static BOOL sdma_info(ULONG query, ULONG ip, APTR out, ULONG size)
{
    struct drm_amdgpu_info req;

    memset(&req, 0, sizeof(req));
    req.return_pointer = (UQUAD)(IPTR)out;
    req.return_size = size;
    req.query = query;
    req.query_hw_ip.type = ip;
    return drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_INFO, &req) == 0;
}

static BOOL sdma_ready(void)
{
    struct drm_amdgpu_info_hw_ip hwip;
    struct drm_amdgpu_info_device dev;
    union drm_amdgpu_ctx ctx;

    if (sdma2d.tried)
        return sdma2d.ok;
    sdma2d.tried = TRUE;
    if (!sdma2d.sd || !sdma2d.sd->kms.has3d)
        return FALSE;
    sdma2d.fd = sdma2d.sd->kms.fd;

    memset(&hwip, 0, sizeof(hwip));
    memset(&dev, 0, sizeof(dev));
    if (!sdma_info(AMDGPU_INFO_HW_IP_INFO, AMDGPU_HW_IP_DMA, &hwip, sizeof(hwip)) || !hwip.available_rings ||
        hwip.hw_ip_version_major < 4 || !sdma_info(AMDGPU_INFO_DEV_INFO, 0, &dev, sizeof(dev)))
    {
        bug("[Amdgpu:2D] no SDMA engine, 2D stays on the CPU\n");
        return FALSE;
    }
    sdma2d.sdma = hwip.hw_ip_version_major;
    sdma2d.va_base = (dev.virtual_address_offset + VA_SPAN - 1) & ~(VA_SPAN - 1);
    sdma2d.va_end = dev.virtual_address_max;

    memset(&ctx, 0, sizeof(ctx));
    ctx.in.op = AMDGPU_CTX_OP_ALLOC_CTX;
    if (drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_CTX, &ctx))
        return FALSE;
    sdma2d.ctx = ctx.out.alloc.ctx_id;

    if (!sdma_buf_alloc(&sdma2d.ib, IB_SLOTS * IB_SLOT_DW * 4, AMDGPU_GEM_DOMAIN_GTT,
                        AMDGPU_GEM_CREATE_CPU_GTT_USWC, TRUE))
    {
        bug("[Amdgpu:2D] no command buffer, 2D stays on the CPU\n");
        return FALSE;
    }
    sdma2d.slot = IB_SLOTS - 1;
    sdma2d.ok = TRUE;
    bug("[Amdgpu:2D] SDMA %lu.%lu\n", (unsigned long)hwip.hw_ip_version_major,
        (unsigned long)hwip.hw_ip_version_minor);
    return TRUE;
}

/* The scan-out buffer of `bm`, mapped for the GPU, or NULL */
static struct Amdgpu_FB *sdma_fb(OOP_Object *bm)
{
    struct Amdgpu_staticdata *sd = sdma2d.sd;
    struct BitmapData *data;
    struct Amdgpu_FB *fb;

    if (!bm || OOP_OCLASS(bm) != sd->amdgpuonbmclass)
        return NULL;
    data = OOP_INST_DATA(OOP_OCLASS(bm), bm);
    fb = &data->fb;
    if (!fb->handle || fb->bpp != 32)
        return NULL;
    if (!fb->va && !sdma_map_va(fb->handle, fb->size, &fb->va))
        return NULL;
    return fb;
}

static BOOL sdma_buf_fit(struct gpu_buf *b, UQUAD size, ULONG domain, UQUAD flags, BOOL map)
{
    if (b->handle && b->size >= size)
        return TRUE;
    if (b->handle && !sdma_wait(sdma2d.last_seq))
        return FALSE;
    sdma_buf_free(b);
    return sdma_buf_alloc(b, size, domain, flags, map);
}

/* ---- the operations (amdgpu_2d.h) ---- */

BOOL Amdgpu_2D_CopyBox(OOP_Object *bm, LONG sx, LONG sy, LONG dx, LONG dy, LONG w, LONG h)
{
    struct Amdgpu_FB *fb;
    struct sdma_surf src, dst, tmp;
    BOOL overlap, ok = FALSE;

    if (w <= 0 || h <= 0)
        return TRUE;
    overlap = sx < dx + w && dx < sx + w && sy < dy + h && dy < sy + h;
    ObtainSemaphore(&sdma2d.lock);
    if (sdma_ready() && (fb = sdma_fb(bm)) &&
        (!overlap || sdma_buf_fit(&sdma2d.temp, (UQUAD)w * h * 4, AMDGPU_GEM_DOMAIN_VRAM,
                                  AMDGPU_GEM_CREATE_NO_CPU_ACCESS, FALSE)) &&
        sdma_begin())
    {
        src.va = dst.va = fb->va;
        src.pitch = dst.pitch = fb->pitch / 4;
        src.height = dst.height = fb->height;
        src.x = sx; src.y = sy;
        dst.x = dx; dst.y = dy;
        sdma_use(fb->handle);
        if (!overlap)
        {
            sdma_emit_copy(&src, &dst, w, h);
            ok = sdma_submit();
        }
        else
        {
            tmp.va = sdma2d.temp.va;
            tmp.pitch = w;
            tmp.height = h;
            tmp.x = tmp.y = 0;
            sdma_use(sdma2d.temp.handle);
            sdma_emit_copy(&src, &tmp, w, h);
            sdma_emit_barrier();
            sdma_emit_copy(&tmp, &dst, w, h);
            ok = sdma_submit();
        }
    }
    ReleaseSemaphore(&sdma2d.lock);
    return ok;
}

BOOL Amdgpu_2D_Fill(OOP_Object *bm, LONG x, LONG y, LONG w, LONG h, ULONG pixel)
{
    struct Amdgpu_FB *fb;
    UQUAD max = (sdma2d.sdma >= 6 ? (1ULL << 30) : (1ULL << 22)) - 4;
    BOOL ok = FALSE;
    LONG row;

    if (w <= 0 || h <= 0)
        return TRUE;
    ObtainSemaphore(&sdma2d.lock);
    if (sdma_ready() && (fb = sdma_fb(bm)) && sdma_begin())
    {
        UQUAD va = fb->va + (UQUAD)y * fb->pitch + (UQUAD)x * 4;

        sdma_use(fb->handle);
        ok = TRUE;
        if (x == 0 && (ULONG)w * 4 == fb->pitch)
        {
            UQUAD left = (UQUAD)h * fb->pitch;

            while (ok && left)
            {
                UQUAD n = left < max ? left : max;

                if ((ok = sdma_room(5)))
                {
                    sdma_use(fb->handle);
                    sdma_emit_fill(va, n, pixel);
                    va += n;
                    left -= n;
                }
            }
        }
        else
        {
            for (row = 0; ok && row < h; row++, va += fb->pitch)
                if ((ok = sdma_room(5)))
                {
                    sdma_use(fb->handle);
                    sdma_emit_fill(va, (UQUAD)w * 4, pixel);
                }
        }
        ok = ok && sdma_submit();
    }
    ReleaseSemaphore(&sdma2d.lock);
    return ok;
}

BOOL Amdgpu_2D_Read(OOP_Object *bm, LONG x, LONG y, LONG w, LONG h, UBYTE *dst, ULONG dstmod)
{
    struct Amdgpu_FB *fb;
    struct sdma_surf src, stg;
    BOOL ok = FALSE;
    LONG row;

    if (w <= 0 || h <= 0)
        return TRUE;
    ObtainSemaphore(&sdma2d.lock);
    if (sdma_ready() && (fb = sdma_fb(bm)) &&
        sdma_buf_fit(&sdma2d.staging, (UQUAD)w * h * 4, AMDGPU_GEM_DOMAIN_GTT, 0, TRUE) &&
        sdma_begin())
    {
        src.va = fb->va;
        src.pitch = fb->pitch / 4;
        src.height = fb->height;
        src.x = x; src.y = y;
        stg.va = sdma2d.staging.va;
        stg.pitch = w;
        stg.height = h;
        stg.x = stg.y = 0;
        sdma_use(fb->handle);
        sdma_use(sdma2d.staging.handle);
        sdma_emit_copy(&src, &stg, w, h);
        if (sdma_submit() && sdma_wait(sdma2d.last_seq))
        {
            const UBYTE *s = sdma2d.staging.map;

            for (row = 0; row < h; row++)
                CopyMem((APTR)(s + (IPTR)row * w * 4), dst + (IPTR)row * dstmod, w * 4);
            ok = TRUE;
        }
    }
    ReleaseSemaphore(&sdma2d.lock);
    return ok;
}

void Amdgpu_2D_Sync(void)
{
    if (sdma2d.last_seq == sdma2d.done_seq)
        return;
    ObtainSemaphore(&sdma2d.lock);
    if (sdma2d.ok)
        sdma_wait(sdma2d.last_seq);
    ReleaseSemaphore(&sdma2d.lock);
}

/* Before a scan-out buffer is destroyed: work on it finished, its GPU
   mapping gone */
void Amdgpu_2D_ReleaseFB(struct Amdgpu_FB *fb)
{
    if (!fb->va)
        return;
    ObtainSemaphore(&sdma2d.lock);
    if (sdma2d.ok)
        sdma_wait(sdma2d.last_seq);
    sdma_unmap_va(fb->handle, fb->va, fb->size);
    fb->va = 0;
    ReleaseSemaphore(&sdma2d.lock);
}

void Amdgpu_2D_Init(struct Amdgpu_staticdata *sd)
{
    memset(&sdma2d, 0, sizeof(sdma2d));
    InitSemaphore(&sdma2d.lock);
    sdma2d.sd = sd;
}

void Amdgpu_2D_Exit(void)
{
    ObtainSemaphore(&sdma2d.lock);
    if (sdma2d.ok)
        sdma_wait(sdma2d.last_seq);
    sdma_buf_free(&sdma2d.staging);
    sdma_buf_free(&sdma2d.temp);
    sdma_buf_free(&sdma2d.ib);
    if (sdma2d.ctx)
    {
        union drm_amdgpu_ctx ctx;

        memset(&ctx, 0, sizeof(ctx));
        ctx.in.op = AMDGPU_CTX_OP_FREE_CTX;
        ctx.in.ctx_id = sdma2d.ctx;
        drmIoctl(sdma2d.fd, DRM_IOCTL_AMDGPU_CTX, &ctx);
        sdma2d.ctx = 0;
    }
    sdma2d.ok = FALSE;
    ReleaseSemaphore(&sdma2d.lock);
}
