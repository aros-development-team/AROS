/*
    Copyright 2025, The AROS Development Team. All rights reserved.

    Portions of this file (the V3D packet definitions, command-list
    processing, shader-record relocation and render-control-list
    generation) are derived from Mesa's MIT-licensed VC4 Gallium driver
    kernel-validation sources, src/gallium/drivers/vc4/kernel/
    (vc4_packet.h, vc4_validate.c, vc4_render_cl.c):

      Copyright (C) 2014-2015 Broadcom

      Permission is hereby granted, free of charge, to any person obtaining a
      copy of this software and associated documentation files (the "Software"),
      to deal in the Software without restriction, including without limitation
      the rights to use, copy, modify, merge, publish, distribute, sublicense,
      and/or sell copies of the Software, and to permit persons to whom the
      Software is furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice (including the next
      paragraph) shall be included in all copies or substantial portions of the
      Software.

      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
      THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
      FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
      IN THE SOFTWARE.

    VC4 Gallium 3D HIDD - DRM compatibility layer implementation

    Replaces the DRM ioctl interface with direct AROS hardware access.
    Mesa's VC4 Gallium driver calls vc4_ioctl() which wraps drmIoctl().
    We intercept at the drmIoctl level and dispatch to our handlers.

    This file implements:
    - BO (Buffer Object) management via mailbox GPU memory allocation
    - Bin CL processing: strip GEM_HANDLES, patch addresses
    - Shader record relocation: resolve BO handle indices to GPU bus addresses
    - RCL (Render Control List) generation from surface configuration
    - V3D job submission (binning + rendering)
*/

#define DEBUG 0
#include <aros/debug.h>

#include <proto/exec.h>
#include <proto/mbox.h>
#include <proto/kernel.h>

#include <hardware/videocore.h>
#include <errno.h>

/* stdc's errno.h has no ETIME; the value must match posixc's (what the
 * Mesa side is built against) — vc4_bufmgr.c tests ret == -ETIME to
 * classify a timeout_ns=0 wait as "busy" instead of an error. */
#ifndef ETIME
#define ETIME 92
#endif

#include "vc4gallium_intern.h"
#include "vc4_drm_aros.h"
#include "vc4_v3d.h"

/* vc4_v3d.h provides ARM_PERIIOBASE; bcm2708.h gives us the 1 MHz
 * system timer used by the hybrid GPU wait below. */
#include <hardware/bcm2708.h>

static inline ULONG v3d_now_us(void)
{
    return AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);
}

/* Base-free memset. This hidd is kickstart-resident; the stdc memset
 * stub caches the FIRST caller task's per-task StdCBase in our globals
 * and calls through that dead base from every later process (2nd-GL-app
 * crash). A local strong definition also catches clang's implicit
 * struct-init memset calls.
 *
 * gcc's -ftree-loop-distribute-patterns (on at -O2) recognizes the byte
 * loop as the memset idiom and replaces it with a call to memset — this
 * very function, which then spins forever. clang deliberately skips
 * idiom recognition inside functions named after the idiom. Turn the
 * pass off here, the way libcs building their own string routines do. */
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((optimize("no-tree-loop-distribute-patterns")))
#endif
void *memset(void *dst, int c, __SIZE_TYPE__ n)
{
    UBYTE *d = dst;
    while (n--)
        *d++ = (UBYTE)c;
    return dst;
}

/* Base-free "%02x " formatter for the diagnostic hexdumps (replaces the
 * stdc snprintf stub for the same reason as memset above). Returns the
 * 3 bytes written; caller guarantees space. */
static ULONG vc4_hexbyte(char *dst, UBYTE b)
{
    static const char hex[] = "0123456789abcdef";
    dst[0] = hex[b >> 4];
    dst[1] = hex[b & 0xf];
    dst[2] = ' ';
    dst[3] = '\0';
    return 3;
}

/* Mailbox property channel */
#define VCMB_PROPCHAN   8

#ifdef MBoxBase
#undef MBoxBase
#endif
#define MBoxBase    sd->MBoxBase

/* ---- V3D Packet Constants (from Mesa's MIT-licensed
 *      src/gallium/drivers/vc4/kernel/vc4_packet.h) ---- */

/* Packet opcodes */
#define VC4_PACKET_HALT                         0
#define VC4_PACKET_NOP                          1
#define VC4_PACKET_FLUSH                        4
#define VC4_PACKET_FLUSH_ALL                    5
#define VC4_PACKET_START_TILE_BINNING           6
#define VC4_PACKET_INCREMENT_SEMAPHORE          7
#define VC4_PACKET_WAIT_ON_SEMAPHORE            8
#define VC4_PACKET_BRANCH                       16
#define VC4_PACKET_BRANCH_TO_SUB_LIST           17
#define VC4_PACKET_STORE_MS_TILE_BUFFER         24
#define VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF 25
#define VC4_PACKET_STORE_FULL_RES_TILE_BUFFER   26
#define VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER    27
#define VC4_PACKET_STORE_TILE_BUFFER_GENERAL    28
#define VC4_PACKET_LOAD_TILE_BUFFER_GENERAL     29
#define VC4_PACKET_GL_INDEXED_PRIMITIVE         32
#define VC4_PACKET_GL_ARRAY_PRIMITIVE           33
#define VC4_PACKET_PRIMITIVE_LIST_FORMAT        56
#define VC4_PACKET_GL_SHADER_STATE              64
#define VC4_PACKET_NV_SHADER_STATE              65
#define VC4_PACKET_CONFIGURATION_BITS           96
#define VC4_PACKET_FLAT_SHADE_FLAGS             97
#define VC4_PACKET_POINT_SIZE                   98
#define VC4_PACKET_LINE_WIDTH                   99
#define VC4_PACKET_RHT_X_BOUNDARY              100
#define VC4_PACKET_DEPTH_OFFSET                101
#define VC4_PACKET_CLIP_WINDOW                 102
#define VC4_PACKET_VIEWPORT_OFFSET             103
#define VC4_PACKET_Z_CLIPPING                  104
#define VC4_PACKET_CLIPPER_XY_SCALING          105
#define VC4_PACKET_CLIPPER_Z_SCALING           106
#define VC4_PACKET_TILE_BINNING_MODE_CONFIG    112
#define VC4_PACKET_TILE_RENDERING_MODE_CONFIG  113
#define VC4_PACKET_CLEAR_COLORS                114
#define VC4_PACKET_TILE_COORDINATES            115
#define VC4_PACKET_GEM_HANDLES                 254

/* Packet sizes (including opcode byte) */
static const UBYTE vc4_packet_sizes[256] = {
    [VC4_PACKET_HALT]                       = 1,
    [VC4_PACKET_NOP]                        = 1,
    [VC4_PACKET_FLUSH]                      = 1,
    [VC4_PACKET_FLUSH_ALL]                  = 1,
    [VC4_PACKET_START_TILE_BINNING]         = 1,
    [VC4_PACKET_INCREMENT_SEMAPHORE]        = 1,
    [VC4_PACKET_WAIT_ON_SEMAPHORE]          = 1,
    [VC4_PACKET_BRANCH]                     = 5,
    [VC4_PACKET_BRANCH_TO_SUB_LIST]         = 5,
    [VC4_PACKET_STORE_MS_TILE_BUFFER]       = 1,
    [VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF] = 1,
    [VC4_PACKET_STORE_FULL_RES_TILE_BUFFER] = 5,
    [VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER]  = 5,
    [VC4_PACKET_STORE_TILE_BUFFER_GENERAL]  = 7,
    [VC4_PACKET_LOAD_TILE_BUFFER_GENERAL]   = 7,
    [VC4_PACKET_GL_INDEXED_PRIMITIVE]       = 14,
    [VC4_PACKET_GL_ARRAY_PRIMITIVE]         = 10,
    [48]                                    = 1,  /* COMPRESSED_PRIMITIVE */
    [49]                                    = 1,  /* CLIPPED_COMPRESSED_PRIMITIVE */
    [VC4_PACKET_PRIMITIVE_LIST_FORMAT]      = 2,
    [VC4_PACKET_GL_SHADER_STATE]            = 5,
    [VC4_PACKET_NV_SHADER_STATE]            = 5,
    [66]                                    = 5,  /* VG_SHADER_STATE */
    [VC4_PACKET_CONFIGURATION_BITS]         = 4,
    [VC4_PACKET_FLAT_SHADE_FLAGS]           = 5,
    [VC4_PACKET_POINT_SIZE]                 = 5,
    [VC4_PACKET_LINE_WIDTH]                 = 5,
    [VC4_PACKET_RHT_X_BOUNDARY]            = 3,
    [VC4_PACKET_DEPTH_OFFSET]              = 5,
    [VC4_PACKET_CLIP_WINDOW]               = 9,
    [VC4_PACKET_VIEWPORT_OFFSET]           = 5,
    [VC4_PACKET_Z_CLIPPING]                = 9,
    [VC4_PACKET_CLIPPER_XY_SCALING]        = 9,
    [VC4_PACKET_CLIPPER_Z_SCALING]         = 9,
    [VC4_PACKET_TILE_BINNING_MODE_CONFIG]  = 16,
    [VC4_PACKET_TILE_RENDERING_MODE_CONFIG]= 11,
    [VC4_PACKET_CLEAR_COLORS]              = 14,
    [VC4_PACKET_TILE_COORDINATES]          = 3,
    [VC4_PACKET_GEM_HANDLES]               = 9,
};

/* Tile buffer constants */
#define VC4_TILE_BUFFER_SIZE    (64 * 64 * 4)

/* Load/Store tile buffer bits */
#define VC4_LOADSTORE_TILE_BUFFER_NONE                  0
#define VC4_STORE_TILE_BUFFER_DISABLE_VG_MASK_CLEAR     (1 << 15)
#define VC4_STORE_TILE_BUFFER_DISABLE_ZS_CLEAR          (1 << 14)
#define VC4_STORE_TILE_BUFFER_DISABLE_COLOR_CLEAR       (1 << 13)
#define VC4_LOADSTORE_TILE_BUFFER_EOF                   (1 << 3)
#define VC4_LOADSTORE_FULL_RES_EOF                      (1 << 3)
#define VC4_LOADSTORE_FULL_RES_DISABLE_CLEAR_ALL        (1 << 2)
#define VC4_LOADSTORE_FULL_RES_DISABLE_ZS               (1 << 1)
#define VC4_LOADSTORE_FULL_RES_DISABLE_COLOR            (1 << 0)

/* RCL surface flag */
#define VC4_SUBMIT_RCL_SURFACE_READ_IS_FULL_RES         (1 << 0)

/* Binning config flags */
#define VC4_BIN_CONFIG_AUTO_INIT_TSDA                   (1 << 2)
#define VC4_BIN_CONFIG_ALLOC_BLOCK_SIZE_MASK            (3 << 5)
#define VC4_BIN_CONFIG_ALLOC_BLOCK_SIZE_128             (2 << 5)
#define VC4_BIN_CONFIG_ALLOC_INIT_BLOCK_SIZE_MASK       (3 << 3)
#define VC4_BIN_CONFIG_ALLOC_INIT_BLOCK_SIZE_32         (0 << 3)

/* roundup helper */
#define ROUNDUP(x, y) (((x) + (y) - 1) & ~((y) - 1))
#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

static struct vc4galliumstaticdata *fd_to_sd(int fd)
{
    struct vc4_aros_fd *afd = (struct vc4_aros_fd *)(IPTR)fd;
    if (afd && afd->magic == VC4_AROS_FD_MAGIC)
        return afd->sd;
    bug("[VC4Gallium] BUG: invalid fd 0x%08x\n", fd);
    return NULL;
}

/* ---- Buffer Object Management ---- */

static ULONG bo_alloc_handle(struct vc4galliumstaticdata *sd)
{
    ULONG i;
    for (i = 1; i < VC4_MAX_BOS; i++)
    {
        ULONG h = (sd->bo_next_handle + i) % VC4_MAX_BOS;
        if (h == 0) continue;
        if (sd->bo_table[h].refcount == 0)
        {
            sd->bo_next_handle = h;
            return h;
        }
    }
    return 0;
}

/*
 * Report what the driver holds in the VideoCore heap when an allocation
 * fails. The firmware only ever answers "handle 0" — it never says how
 * much is left — so the useful numbers are the partition size (gpu_mem=
 * in config.txt) against our own running total. A large gap between the
 * two means someone else (firmware framebuffer, another client, or a BO
 * we forgot to free) owns the difference.
 *
 * Must be called WITHOUT mbox_lock held: it takes bo_lock, and the rest
 * of the driver locks bo_lock before mbox_lock.
 */
static void gpu_mem_report(struct vc4galliumstaticdata *sd, ULONG failed_size)
{
    static ULONG report_n = 0;
    ULONG pool_bytes = 0, bo_bytes = 0, bo_live = 0;
    int i;

    /* Rate-limit: the first few failures carry the information, the rest
     * would drown the log (Mesa retries every draw). */
    if (++report_n > 3 && (report_n & 63) != 0)
        return;

    if (sd->vcram_size == 0)
    {
        ObtainSemaphore(&sd->mbox_lock);
        sd->mbox_msg[0] = AROS_LE2LONG(9 * 4);
        sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
        sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_GETVCRAM);
        sd->mbox_msg[3] = AROS_LE2LONG(4 * 2);      /* value buffer size = 8 */
        sd->mbox_msg[4] = 0;                        /* request length = 0 */
        sd->mbox_msg[5] = 0;
        sd->mbox_msg[6] = 0;
        sd->mbox_msg[7] = 0;                        /* end tag */
        sd->mbox_msg[8] = 0;

        if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
            != (volatile unsigned int *)-1)
        {
            sd->vcram_base = AROS_LE2LONG(sd->mbox_msg[5]);
            sd->vcram_size = AROS_LE2LONG(sd->mbox_msg[6]);
        }
        ReleaseSemaphore(&sd->mbox_lock);
    }

    for (i = 0; i < VC4_NUM_POOL_SETS; i++)
        pool_bytes += sd->pool[i].tile.size + sd->pool[i].exec.size +
                      sd->pool[i].rcl.size + sd->pool[i].binoverflow.size;

    ObtainSemaphore(&sd->bo_lock);
    {
        ULONG h;
        for (h = 1; h < VC4_MAX_BOS; h++)
        {
            if (sd->bo_table[h].refcount > 0 && sd->bo_table[h].gpu_handle)
            {
                bo_live++;
                bo_bytes += sd->bo_table[h].size;
            }
        }
    }
    ReleaseSemaphore(&sd->bo_lock);

    bug("[VC4Gallium] OOM: %u KB request failed. VC partition %u KB at "
        "0x%08x; driver holds %u KB in %u allocs (pools %u KB, %u BOs "
        "%u KB)\n",
        failed_size / 1024, sd->vcram_size / 1024, sd->vcram_base,
        sd->gpu_mem_bytes / 1024, sd->gpu_mem_allocs,
        pool_bytes / 1024, bo_live, bo_bytes / 1024);
}

/* Allocate GPU memory via mailbox; returns physical address (0x3fffffff masked). */
static void gpu_mem_free(struct vc4galliumstaticdata *sd, ULONG gpu_handle,
                         ULONG size);

static APTR gpu_mem_alloc(struct vc4galliumstaticdata *sd, ULONG size, ULONG align, ULONG flags, ULONG *out_handle)
{
    APTR phys = NULL;
    ULONG gpu_handle;

    ObtainSemaphore(&sd->mbox_lock);

    D(bug("[VC4Gallium] gpu_mem_alloc: requesting %d bytes align=%d flags=0x%x mbox_msg=0x%08x\n",
        size, align, flags, (ULONG)(IPTR)sd->mbox_msg));

    /* Allocate memory.
     * Mailbox property tag layout:
     *   [0] total size  [1] VCTAG_REQ  [2] tag_id
     *   [3] value_buf_size  [4] request_indicator  [5..] values
     *   [...] end_tag(0)
     * msg[4] = request value length in bytes (bit 31 clear).
     * Response overwrites msg[4] with 0x80000000|resp_len.
     */
    sd->mbox_msg[0] = AROS_LE2LONG(10 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_ALLOCMEM);
    sd->mbox_msg[3] = AROS_LE2LONG(4 * 3);        /* value buffer size = 12 */
    sd->mbox_msg[4] = AROS_LE2LONG(4 * 3);         /* request indicator = 12 */
    sd->mbox_msg[5] = AROS_LE2LONG(size);           /* value[0] = size */
    sd->mbox_msg[6] = AROS_LE2LONG(align);          /* value[1] = alignment */
    sd->mbox_msg[7] = AROS_LE2LONG(flags);          /* value[2] = flags */
    sd->mbox_msg[8] = 0;                            /* end tag */
    sd->mbox_msg[9] = 0;

    /* MBoxCall (not MBoxWrite + MBoxRead) so the request/response pair is
     * atomic under the resource's own mbox_Sem. A separate write+read drops
     * that lock between the two, letting another mailbox user (e.g. vc4gfx's
     * cursor/mode MBoxCall) inject a request and consume our reply. */
    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
        == (volatile unsigned int *)-1)
    {
        bug("[VC4Gallium] gpu_mem_alloc: ALLOCMEM MBoxCall failed\n");
        ReleaseSemaphore(&sd->mbox_lock);
        gpu_mem_report(sd, size);
        return NULL;
    }

    D(bug("[VC4Gallium] gpu_mem_alloc: ALLOCMEM response: msg[1]=0x%08x msg[4]=0x%08x msg[5]=0x%08x\n",
        AROS_LE2LONG(sd->mbox_msg[1]), AROS_LE2LONG(sd->mbox_msg[4]),
        AROS_LE2LONG(sd->mbox_msg[5])));

    gpu_handle = AROS_LE2LONG(sd->mbox_msg[5]);
    if (gpu_handle == 0)
    {
        bug("[VC4Gallium] gpu_mem_alloc: firmware returned handle=0 for %d bytes (OOM?)\n", size);
        ReleaseSemaphore(&sd->mbox_lock);
        gpu_mem_report(sd, size);
        return NULL;
    }

    /* Lock memory to get physical address */
    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_LOCKMEM);
    sd->mbox_msg[3] = AROS_LE2LONG(4);             /* value buffer size = 4 */
    sd->mbox_msg[4] = AROS_LE2LONG(4);              /* request indicator = 4 */
    sd->mbox_msg[5] = AROS_LE2LONG(gpu_handle);     /* value[0] = handle */
    sd->mbox_msg[6] = 0;                            /* end tag */
    sd->mbox_msg[7] = 0;

    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
        == (volatile unsigned int *)-1)
    {
        bug("[VC4Gallium] gpu_mem_alloc: LOCKMEM MBoxCall failed\n");
        ReleaseSemaphore(&sd->mbox_lock);
        /* The ALLOCMEM above succeeded — hand the handle back or those
         * bytes stay committed in the firmware heap until reboot. */
        gpu_mem_free(sd, gpu_handle, 0);
        return NULL;
    }

    phys = (APTR)(AROS_LE2LONG(sd->mbox_msg[5]) & 0x3fffffff);

    D(bug("[VC4Gallium] gpu_mem_alloc: LOCKMEM response: msg[4]=0x%08x msg[5]=0x%08x -> phys=0x%08x\n",
        AROS_LE2LONG(sd->mbox_msg[4]), AROS_LE2LONG(sd->mbox_msg[5]), (ULONG)phys));

    sd->gpu_mem_bytes += size;
    sd->gpu_mem_allocs++;

    ReleaseSemaphore(&sd->mbox_lock);

    if (out_handle)
        *out_handle = gpu_handle;

    D(bug("[VC4Gallium] gpu_mem_alloc: %d bytes -> phys=0x%08x gpu_handle=0x%08x\n",
        size, (ULONG)phys, gpu_handle));

    return phys;
}

/* size is what was passed to gpu_mem_alloc (0 = don't touch the accounting,
 * for a handle freed before it was ever counted). */
static void gpu_mem_free(struct vc4galliumstaticdata *sd, ULONG gpu_handle,
                         ULONG size)
{
    D(bug("[VC4Gallium] gpu_mem_free: gpu_handle=0x%08x\n", gpu_handle));

    ObtainSemaphore(&sd->mbox_lock);

    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_UNLOCKMEM);
    sd->mbox_msg[3] = AROS_LE2LONG(4);             /* value buffer size */
    sd->mbox_msg[4] = AROS_LE2LONG(4);              /* request indicator */
    sd->mbox_msg[5] = AROS_LE2LONG(gpu_handle);     /* value[0] = handle */
    sd->mbox_msg[6] = 0;                            /* end tag */
    sd->mbox_msg[7] = 0;

    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg);

    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_FREEMEM);
    sd->mbox_msg[3] = AROS_LE2LONG(4);             /* value buffer size */
    sd->mbox_msg[4] = AROS_LE2LONG(4);              /* request indicator */
    sd->mbox_msg[5] = AROS_LE2LONG(gpu_handle);     /* value[0] = handle */
    sd->mbox_msg[6] = 0;                            /* end tag */
    sd->mbox_msg[7] = 0;

    MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg);

    if (size)
    {
        sd->gpu_mem_bytes -= size;
        sd->gpu_mem_allocs--;
    }

    ReleaseSemaphore(&sd->mbox_lock);
}

static int v3d_wait_seqno(struct vc4galliumstaticdata *sd, ULONG seqno, ULONG timeout_loops);

/*
 * Reclaim GPU memory by waiting for all in-flight jobs and freeing
 * the pooled frame BOs. Called when gpu_mem_alloc fails as a last
 * resort before reporting OOM to Mesa.
 */
static void gpu_mem_reclaim(struct vc4galliumstaticdata *sd)
{
    struct vc4_v3d_state *v3d = &sd->v3d;
    int i;

    D(bug("[VC4Gallium] gpu_mem_reclaim: attempting to free pool BOs\n"));

    if (v3d->finished_seqno < v3d->seqno)
        v3d_wait_seqno(sd, v3d->seqno, 50000000);

    for (i = 0; i < VC4_NUM_POOL_SETS; i++)
    {
        struct vc4_frame_bo *pools[] = {
            &sd->pool[i].tile,
            &sd->pool[i].exec,
            &sd->pool[i].rcl,
            &sd->pool[i].binoverflow
        };
        int p;
        for (p = 0; p < 4; p++)
        {
            if (pools[p]->vaddr)
            {
                D(bug("[VC4Gallium] gpu_mem_reclaim: freeing pool[%d].%s (%d bytes)\n",
                    i, (p == 0) ? "tile" : (p == 1) ? "exec" :
                       (p == 2) ? "rcl" : "binoverflow",
                    pools[p]->size));
                gpu_mem_free(sd, pools[p]->gpu_handle, pools[p]->size);
                pools[p]->vaddr = NULL;
                pools[p]->bus_addr = 0;
                pools[p]->gpu_handle = 0;
                pools[p]->size = 0;
            }
        }
        sd->pool[i].seqno = 0;
    }
}

/* ---- Helper: resolve BO handle to bus address ---- */

static ULONG bo_bus_addr(struct vc4galliumstaticdata *sd,
                         ULONG *bo_handles, ULONG bo_handle_count,
                         ULONG hindex)
{
    ULONG handle;
    if (hindex >= bo_handle_count)
    {
        bug("[VC4Gallium] bo_bus_addr: hindex %d >= count %d\n",
            hindex, bo_handle_count);
        return 0;
    }
    handle = bo_handles[hindex];
    if (handle == 0 || handle >= VC4_MAX_BOS || sd->bo_table[handle].refcount == 0)
    {
        bug("[VC4Gallium] bo_bus_addr: invalid handle %d at hindex %d\n",
            handle, hindex);
        return 0;
    }
    return sd->bo_table[handle].bus_addr;
}

/* ---- QPU Shader Instruction Scanner ----
 *
 * Scan QPU instructions to extract uniform-relocation metadata, mirroring
 * Mesa's MIT-licensed kernel/vc4_validate_shaders.c. Extracts:
 * - uniforms_size: total bytes of uniform data the shader reads
 * - num_texture_samples: number of TMU submissions
 * - texture_samples[]: per-sample p_offset[0..3] into the uniform stream
 * - uniform_addr_offsets[]: indices of UNIFORMS_ADDRESS uniform words
 *
 * Mesa's submitted uniform stream is, per shader:
 *   [tex_handle_0..N-1] [uniform_data_0..M-1]
 * N = num_texture_samples, M = uniforms_size/4 reads; tex handles first.
 */

/* QPU instruction field extraction. Bit positions and signal/waddr enum
 * values mirror Mesa's vc4_qpu_defines.h (see the kernel/Mesa-shipped
 * mesa-21.0.2/src/gallium/drivers/vc4/vc4_qpu_defines.h). */
#define QPU_GET_FIELD(inst, shift, nbits) \
    (((inst) >> (shift)) & ((1ULL << (nbits)) - 1))
#define QPU_SIG(inst)       QPU_GET_FIELD(inst, 60, 4)
#define QPU_RADDR_A(inst)   QPU_GET_FIELD(inst, 18, 6)
#define QPU_RADDR_B(inst)   QPU_GET_FIELD(inst, 12, 6)
#define QPU_WADDR_ADD(inst) QPU_GET_FIELD(inst, 38, 6)
#define QPU_WADDR_MUL(inst) QPU_GET_FIELD(inst, 32, 6)
#define QPU_OP_ADD(inst)    QPU_GET_FIELD(inst, 24, 5)
#define QPU_ADD_B(inst)     QPU_GET_FIELD(inst, 9, 3)
#define QPU_BRANCH_REL_BIT  ((UQUAD)1 << 51)
#define QPU_BRANCH_REG_BIT  ((UQUAD)1 << 50)
#define QPU_BRANCH_TARGET(inst) ((LONG)((ULONG)((inst) & 0xffffffffULL)))

#define QPU_R_UNIF          32
#define QPU_MUX_A           6
#define QPU_MUX_B           7

/* Signal field values. Note: these are enum-position values matching
 * Mesa's `enum qpu_sig_bits` — QPU_SIG_NONE is 1, not 0. */
#define QPU_SIG_SW_BREAKPOINT        0
#define QPU_SIG_NONE                 1
#define QPU_SIG_THREAD_SWITCH        2
#define QPU_SIG_PROG_END             3
#define QPU_SIG_WAIT_FOR_SCOREBOARD  4
#define QPU_SIG_SCOREBOARD_UNLOCK    5
#define QPU_SIG_LAST_THREAD_SWITCH   6
#define QPU_SIG_COVERAGE_LOAD        7
#define QPU_SIG_COLOR_LOAD           8
#define QPU_SIG_COLOR_LOAD_END       9
#define QPU_SIG_LOAD_TMU0           10
#define QPU_SIG_LOAD_TMU1           11
#define QPU_SIG_ALPHA_MASK_LOAD     12
#define QPU_SIG_SMALL_IMM           13
#define QPU_SIG_LOAD_IMM            14
#define QPU_SIG_BRANCH              15

#define QPU_A_ADD            0

/* TMU write addresses: S triggers submission, T/R/B are parameter writes */
#define QPU_W_TMU0_S        56
#define QPU_W_TMU0_T        57
#define QPU_W_TMU0_R        58
#define QPU_W_TMU0_B        59
#define QPU_W_TMU1_S        60
#define QPU_W_TMU1_T        61
#define QPU_W_TMU1_R        62
#define QPU_W_TMU1_B        63

/* Other write addresses used by the validator */
#define QPU_W_TMU_NOSWAP        36
#define QPU_W_HOST_INT          38
#define QPU_W_NOP               39
#define QPU_W_UNIFORMS_ADDRESS  40
#define QPU_W_TLB_STENCIL_SETUP 43
#define QPU_W_TLB_Z             44
#define QPU_W_TLB_COLOR_MS      45
#define QPU_W_TLB_COLOR_ALL     46
#define QPU_W_TLB_ALPHA_MASK    47
#define QPU_W_VPM               48
#define QPU_W_VPMVCD_SETUP      49
#define QPU_W_VPM_ADDR          50
#define QPU_W_MUTEX_RELEASE     51

static inline BOOL is_tmu_write(ULONG waddr)
{
    return (waddr >= QPU_W_TMU0_S && waddr <= QPU_W_TMU1_B);
}

static inline BOOL is_tmu_submit(ULONG waddr)
{
    return (waddr == QPU_W_TMU0_S || waddr == QPU_W_TMU1_S);
}

static void scan_shader_qpu(APTR shader_data, ULONG shader_size,
                             struct vc4_bo_entry *bo)
{
    UQUAD *inst = (UQUAD *)shader_data;
    ULONG num_inst = shader_size / 8;
    ULONG uniforms_size = 0;
    ULONG tex_samples = 0;
    ULONG num_uniform_addr = 0;
    ULONG i, j;

    /* Per-TMU state: track parameter writes until submit */
    struct {
        ULONG p_offset[4];
        ULONG write_count;
        BOOL  is_direct;
    } tmu_setup[2];

    for (j = 0; j < 2; j++)
    {
        tmu_setup[j].write_count = 0;
        tmu_setup[j].is_direct = FALSE;
        for (i = 0; i < 4; i++)
            tmu_setup[j].p_offset[i] = ~0UL;
    }

    for (i = 0; i < num_inst; i++)
    {
        UQUAD instr = inst[i];
        ULONG sig = QPU_SIG(instr);

        /* Branches have no uniform/TMU side effects (the branch-address
         * uniform scheme is not used by Mesa's vc4 compiler). */
        if (sig == QPU_SIG_BRANCH)
            continue;

        ULONG raddr_a = QPU_RADDR_A(instr);
        ULONG raddr_b = QPU_RADDR_B(instr);
        ULONG waddr_add = QPU_WADDR_ADD(instr);
        ULONG waddr_mul = QPU_WADDR_MUL(instr);

        /* Writes: add ALU and mul ALU independently (LOAD_IMM writes too) */
        ULONG tmu_waddrs[2] = { waddr_add, waddr_mul };
        ULONG w;
        for (w = 0; w < 2; w++)
        {
            ULONG waddr = tmu_waddrs[w];

            if (waddr == QPU_W_UNIFORMS_ADDRESS)
            {
                /* Uniform stream reset (add unif_addr, imm, unif). The
                 * uniform operand — counted by the read accounting below —
                 * is the slot the kernel patches with the stream's GPU
                 * address. Mirrors require_uniform_address_uniform(). */
                if (num_uniform_addr < VC4_MAX_UNIFORM_ADDR)
                    bo->uniform_addr_offsets[num_uniform_addr++] =
                        uniforms_size / 4;
                continue;
            }

            if (!is_tmu_write(waddr))
                continue;

            int tmu = (waddr > QPU_W_TMU0_B) ? 1 : 0;
            BOOL submit = is_tmu_submit(waddr);
            /* A submit with no prior T/R/B parameter writes is a direct
             * (UBO) memory read: its address uniform arrives via raddr and
             * is counted by the read accounting below. */
            BOOL is_direct = submit && tmu_setup[tmu].write_count == 0;

            if (tmu_setup[tmu].write_count < 4)
            {
                tmu_setup[tmu].p_offset[tmu_setup[tmu].write_count] =
                    uniforms_size;
            }
            tmu_setup[tmu].write_count++;

            /* Every non-direct TMU parameter write implicitly pops one
             * word (P0/P1 config) from the uniform FIFO — there is no
             * raddr read for it. */
            if (is_direct)
                tmu_setup[tmu].is_direct = TRUE;
            else
                uniforms_size += 4;

            if (submit)
            {
                /* Record this texture sample. Cap the count at the array
                 * size — silently dropping samples would inflate
                 * num_texture_samples past VC4_MAX_TEX_SAMPLES and let
                 * later consumers read past texture_samples[]. */
                if (tex_samples < VC4_MAX_TEX_SAMPLES)
                {
                    struct vc4_texture_sample_info *s =
                        &bo->texture_samples[tex_samples];
                    s->is_direct = tmu_setup[tmu].is_direct;
                    for (j = 0; j < 4; j++)
                        s->p_offset[j] = tmu_setup[tmu].p_offset[j];
                    tex_samples++;
                }
                else
                {
                    bug("[VC4Gallium] scan_shader: texture sample limit "
                        "(%d) reached, dropping subsequent samples\n",
                        (ULONG)VC4_MAX_TEX_SAMPLES);
                }

                /* Reset TMU state for next sample */
                tmu_setup[tmu].write_count = 0;
                tmu_setup[tmu].is_direct = FALSE;
                for (j = 0; j < 4; j++)
                    tmu_setup[tmu].p_offset[j] = ~0UL;
            }
        }

        /* Uniform FIFO reads: at most ONE word per instruction no matter
         * how many ports reference UNIF — both ports deliver the same
         * word. LOAD_IMM instructions have no register reads. */
        if (sig != QPU_SIG_LOAD_IMM &&
            (raddr_a == QPU_R_UNIF ||
             (raddr_b == QPU_R_UNIF && sig != QPU_SIG_SMALL_IMM)))
            uniforms_size += 4;
    }

    bo->uniforms_size = uniforms_size;
    bo->num_texture_samples = tex_samples;
    bo->num_uniform_addr_offsets = num_uniform_addr;

}

/* ---- IOCTL Handlers ---- */

static int ioctl_get_param(struct vc4galliumstaticdata *sd, struct drm_vc4_get_param *args)
{
    struct vc4_v3d_state *v3d = &sd->v3d;

    if (!v3d->v3d_available)
        return -1;

    switch (args->param)
    {
    case DRM_VC4_PARAM_V3D_IDENT0:
        args->value = v3d->ident0;
        break;
    case DRM_VC4_PARAM_V3D_IDENT1:
        args->value = v3d->ident1;
        break;
    case DRM_VC4_PARAM_V3D_IDENT2:
        args->value = v3d->ident2;
        break;
    case DRM_VC4_PARAM_SUPPORTS_BRANCHES:
        args->value = 1;
        break;
    case DRM_VC4_PARAM_SUPPORTS_ETC1:
        args->value = 1;
        break;
    case DRM_VC4_PARAM_SUPPORTS_THREADED_FS:
        args->value = 1;
        break;
    case DRM_VC4_PARAM_SUPPORTS_FIXED_RCL_ORDER:
        args->value = 1;
        break;
    case DRM_VC4_PARAM_SUPPORTS_MADVISE:
        args->value = 0;
        break;
    case DRM_VC4_PARAM_SUPPORTS_PERFMON:
        args->value = 0;
        break;
    default:
        return -1;
    }

    return 0;
}

static int ioctl_create_bo(struct vc4galliumstaticdata *sd, struct drm_vc4_create_bo *args)
{
    ULONG handle;
    APTR vaddr;
    ULONG gpu_handle;

    ObtainSemaphore(&sd->bo_lock);
    handle = bo_alloc_handle(sd);
    if (handle == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        bug("[VC4Gallium] create_bo: out of handles\n");
        return -1;
    }

    vaddr = gpu_mem_alloc(sd, args->size, 4096,
                          VCMEM_L1NONALLOCATING | VCMEM_ZERO,
                          &gpu_handle);
    if (!vaddr)
    {
        /* Retry after reclaiming pool memory */
        ReleaseSemaphore(&sd->bo_lock);
        gpu_mem_reclaim(sd);
        ObtainSemaphore(&sd->bo_lock);

        vaddr = gpu_mem_alloc(sd, args->size, 4096,
                              VCMEM_L1NONALLOCATING | VCMEM_ZERO,
                              &gpu_handle);
        if (!vaddr)
        {
            ReleaseSemaphore(&sd->bo_lock);
            bug("[VC4Gallium] create_bo: allocation failed for %d bytes (even after reclaim)\n", args->size);
            return -1;
        }
    }

    sd->bo_table[handle].vaddr = vaddr;
    sd->bo_table[handle].bus_addr = GPU_BUS_ADDR(vaddr);
    sd->bo_table[handle].gpu_handle = gpu_handle;
    sd->bo_table[handle].size = args->size;
    sd->bo_table[handle].refcount = 1;
    sd->bo_table[handle].seqno = 0;
    sd->bo_table[handle].is_shader = FALSE;
    sd->bo_table[handle].cpu_mapped = FALSE;
    sd->bo_table[handle].tiling_modifier = 0; /* DRM_FORMAT_MOD_LINEAR */

    ReleaseSemaphore(&sd->bo_lock);

    args->handle = handle;

    D(bug("[VC4Gallium] create_bo: handle=%d size=%d vaddr=0x%08x bus=0x%08x gpu_h=0x%08x\n",
        handle, args->size, (ULONG)vaddr, GPU_BUS_ADDR(vaddr), gpu_handle));

    return 0;
}

static int ioctl_create_shader_bo(struct vc4galliumstaticdata *sd, struct drm_vc4_create_shader_bo *args)
{
    ULONG handle;
    APTR vaddr;
    ULONG gpu_handle;

    ObtainSemaphore(&sd->bo_lock);
    handle = bo_alloc_handle(sd);
    if (handle == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }

    vaddr = gpu_mem_alloc(sd, args->size, 4096,
                          VCMEM_L1NONALLOCATING,
                          &gpu_handle);
    if (!vaddr)
    {
        ReleaseSemaphore(&sd->bo_lock);
        gpu_mem_reclaim(sd);
        ObtainSemaphore(&sd->bo_lock);

        vaddr = gpu_mem_alloc(sd, args->size, 4096,
                              VCMEM_L1NONALLOCATING,
                              &gpu_handle);
        if (!vaddr)
        {
            ReleaseSemaphore(&sd->bo_lock);
            bug("[VC4Gallium] create_shader_bo: allocation failed for %d bytes (even after reclaim)\n", args->size);
            return -1;
        }
    }

    CopyMem((APTR)(IPTR)args->data, vaddr, args->size);

    /* Flush CPU-written QPU instructions to RAM. V3D fetches shader code
     * through the uncached 0xC0000000 alias and would otherwise miss dirty
     * ARM L1 lines, stalling the render thread on FS code fetch. */
    CacheClearE(vaddr, args->size, CACRF_ClearD);
    asm volatile("dsb sy" ::: "memory");

    sd->bo_table[handle].vaddr = vaddr;
    sd->bo_table[handle].bus_addr = GPU_BUS_ADDR(vaddr);
    sd->bo_table[handle].gpu_handle = gpu_handle;
    sd->bo_table[handle].size = args->size;
    sd->bo_table[handle].refcount = 1;
    sd->bo_table[handle].seqno = 0;
    sd->bo_table[handle].is_shader = TRUE;
    sd->bo_table[handle].cpu_mapped = FALSE;
    sd->bo_table[handle].tiling_modifier = 0; /* DRM_FORMAT_MOD_LINEAR */

    scan_shader_qpu(vaddr, args->size, &sd->bo_table[handle]);

    ReleaseSemaphore(&sd->bo_lock);

    args->handle = handle;

    D(bug("[VC4Gallium] create_shader_bo: handle=%d size=%d uniforms=%d tex=%d\n",
        handle, args->size,
        sd->bo_table[handle].uniforms_size,
        sd->bo_table[handle].num_texture_samples));

    return 0;
}

static int ioctl_mmap_bo(struct vc4galliumstaticdata *sd, struct drm_vc4_mmap_bo *args)
{
    ObtainSemaphore(&sd->bo_lock);

    if (args->handle == 0 || args->handle >= VC4_MAX_BOS ||
        sd->bo_table[args->handle].refcount == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }

    args->offset = (UQUAD)(IPTR)sd->bo_table[args->handle].vaddr;

    /* The CPU now has a (cached) pointer to this BO and may dirty its
     * lines — submit_cl's flush loop must include it from here on. */
    sd->bo_table[args->handle].cpu_mapped = TRUE;

    ReleaseSemaphore(&sd->bo_lock);

    return 0;
}

void vc4_aros_bo_unref_locked(struct vc4galliumstaticdata *sd, ULONG handle)
{
    if (handle == 0 || handle >= VC4_MAX_BOS)
        return;
    if (sd->bo_table[handle].refcount == 0)
        return;

    sd->bo_table[handle].refcount--;

    if (sd->bo_table[handle].refcount == 0)
    {
        if (sd->bo_table[handle].gpu_handle)
            gpu_mem_free(sd, sd->bo_table[handle].gpu_handle,
                         sd->bo_table[handle].size);
        sd->bo_table[handle].vaddr = NULL;
        sd->bo_table[handle].bus_addr = 0;
        sd->bo_table[handle].gpu_handle = 0;
        sd->bo_table[handle].size = 0;
        sd->bo_table[handle].seqno = 0;
        sd->bo_table[handle].tiling_modifier = 0;
        sd->bo_table[handle].is_shader = FALSE;
        sd->bo_table[handle].external = FALSE;
        sd->bo_table[handle].cpu_mapped = FALSE;
        sd->bo_table[handle].uniforms_size = 0;
        sd->bo_table[handle].num_texture_samples = 0;
        sd->bo_table[handle].num_uniform_addr_offsets = 0;
        D(bug("[VC4Gallium] bo_unref: freed handle=%d\n", handle));
    }
}

/*
 * GEM_OPEN: wrap a scanout page as an external BO. `name` is the page's
 * physical address, as announced by the bridge's get_scanout entry —
 * arbitrary names are refused. The entry carries no gpu_handle, so the
 * normal refcount/GEM_CLOSE machinery drops the table slot without
 * touching firmware memory (the page belongs to the framebuffer).
 */
static int ioctl_gem_open(struct vc4galliumstaticdata *sd, struct drm_gem_open *args)
{
    ULONG handle;

    if (args->name == 0 || sd->scanout_size == 0 ||
        (args->name != sd->scanout_phys[0] && args->name != sd->scanout_phys[1]))
        return -1;

    ObtainSemaphore(&sd->bo_lock);

    /* Already wrapped? Just take another reference. */
    for (handle = 1; handle < VC4_MAX_BOS; handle++)
    {
        if (sd->bo_table[handle].refcount > 0 &&
            sd->bo_table[handle].external &&
            (ULONG)(IPTR)sd->bo_table[handle].vaddr == args->name)
        {
            sd->bo_table[handle].refcount++;
            ReleaseSemaphore(&sd->bo_lock);
            args->handle = handle;
            args->size = sd->bo_table[handle].size;
            return 0;
        }
    }

    handle = bo_alloc_handle(sd);
    if (handle == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }

    sd->bo_table[handle].vaddr = (APTR)(IPTR)args->name;
    sd->bo_table[handle].bus_addr = GPU_BUS_ADDR(args->name);
    sd->bo_table[handle].gpu_handle = 0;
    sd->bo_table[handle].size = sd->scanout_size;
    sd->bo_table[handle].refcount = 1;
    sd->bo_table[handle].seqno = 0;
    sd->bo_table[handle].is_shader = FALSE;
    sd->bo_table[handle].external = TRUE;
    sd->bo_table[handle].cpu_mapped = FALSE;
    sd->bo_table[handle].tiling_modifier = 0; /* DRM_FORMAT_MOD_LINEAR */

    ReleaseSemaphore(&sd->bo_lock);

    args->handle = handle;
    args->size = sd->scanout_size;

    D(bug("[VC4Gallium] gem_open: scanout page 0x%08x -> handle=%d size=%d\n",
        args->name, handle, (ULONG)args->size));

    return 0;
}

static int ioctl_gem_close(struct vc4galliumstaticdata *sd, struct drm_gem_close *args)
{
    ObtainSemaphore(&sd->bo_lock);

    if (args->handle == 0 || args->handle >= VC4_MAX_BOS ||
        sd->bo_table[args->handle].refcount == 0)
    {
        ReleaseSemaphore(&sd->bo_lock);
        return -1;
    }

    vc4_aros_bo_unref_locked(sd, args->handle);

    ReleaseSemaphore(&sd->bo_lock);
    return 0;
}

/* Forensics: snapshot of the most recent submission, dumped by the
 * v3d_wait_seqno timeout path so a hang can be diagnosed post-mortem. */
static struct
{
    ULONG seqno;
    ULONG rcl_bus, rcl_size, bin_size;
    UWORD width, height;
    UBYTE min_x, min_y, max_x, max_y;
    ULONG cw_hindex, zsw_hindex;
    const volatile UBYTE *rcl_vaddr;
    const volatile UBYTE *tile_alloc_vaddr; /* first per-tile sublist */
} vc4_last_submit;

void v3d_dump_last_submit(void)
{
    bug("[VC4Gallium]   last submit: seqno=%d rcl=0x%08x+%d bin=%d "
        "%dx%d tiles x=%d..%d y=%d..%d cw=%d zsw=%d\n",
        vc4_last_submit.seqno, vc4_last_submit.rcl_bus,
        vc4_last_submit.rcl_size, vc4_last_submit.bin_size,
        (ULONG)vc4_last_submit.width, (ULONG)vc4_last_submit.height,
        (ULONG)vc4_last_submit.min_x, (ULONG)vc4_last_submit.max_x,
        (ULONG)vc4_last_submit.min_y, (ULONG)vc4_last_submit.max_y,
        vc4_last_submit.cw_hindex, vc4_last_submit.zsw_hindex);

    if (vc4_last_submit.rcl_vaddr && vc4_last_submit.rcl_size)
    {
        const volatile UBYTE *rb = vc4_last_submit.rcl_vaddr;
        ULONG size = vc4_last_submit.rcl_size;
        ULONG head = size < 96 ? size : 96;
        ULONG tail_start = (size > head + 32) ? size - 32 : head;
        ULONG i;

        for (i = 0; i < head; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < head; j++)
                p += vc4_hexbyte(line + p, rb[i + j]);
            bug("[VC4Gallium]   rcl[%04d]: %s\n", i, line);
        }
        for (i = tail_start; i < size; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < size; j++)
                p += vc4_hexbyte(line + p, rb[i + j]);
            bug("[VC4Gallium]   rcl[%04d]: %s\n", i, line);
        }
    }

    /* First per-tile sublist as written by the binner: all zeros here means
     * the binner never produced tile lists for this job. */
    if (vc4_last_submit.tile_alloc_vaddr)
    {
        const volatile UBYTE *tb = vc4_last_submit.tile_alloc_vaddr;
        ULONG i;
        for (i = 0; i < 32; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16; j++)
                p += vc4_hexbyte(line + p, tb[i + j]);
            bug("[VC4Gallium]   tile0[%02d]: %s\n", i, line);
        }
    }
}

/* Counter advance lives in vc4_v3d.c (shared with the galliumclass wait
 * loop, and Disable()-protected — see vc4_v3d_advance_counters). */
#define v3d_advance_counters(v3d) vc4_v3d_advance_counters(v3d)

/*
 * Poll V3D until the given seqno has completed.
 * Drives v3d->finished_seqno from the V3D_RFC hardware counter.
 * timeout_loops: max poll iterations (0 = no limit).
 * Returns 0 on success, -1 on timeout.
 */
static int v3d_wait_seqno(struct vc4galliumstaticdata *sd, ULONG seqno, ULONG timeout_loops)
{
    struct vc4_v3d_state *v3d = &sd->v3d;
    /* Serialize against submit_cl and other waiters touching V3D state.
     * Nestable — submit_cl already holds render_lock when it calls us to
     * wait for a pool set to free up. */
    ObtainSemaphore(&sd->render_lock);
    /* Tiered wait (see VC4_GPUWAIT_* in vc4gallium_intern.h): tight spin
     * for µs precision, then ~1 ms timer naps that keep the CPU free for
     * input handling, then vblank blocking for long waits with the
     * existing timeout accounting. Blocking from the start quantized
     * every wait to ~20 ms; pure spinning starves the mouse. */
    BYTE wsig = vc4_wait_enter(sd);
    ULONG ticks = 0;
    /* Both paths bound the wait to the same wall clock: a vblank tick is
     * ~20 ms, the signal-less fallback paces itself at VC4_GPUWAIT_NAP_US
     * instead of polling flat out for timeout_loops iterations. */
    ULONG budget = (wsig >= 0) ? (timeout_loops / 70000 + 4)
                               : (timeout_loops / 70000 * 20000
                                    / VC4_GPUWAIT_NAP_US + 4);
    ULONG spin_start = v3d_now_us();

    while (v3d->finished_seqno < seqno)
    {
        vc4_v3d_service_interrupts(v3d);
        v3d_advance_counters(v3d);
        if (v3d->finished_seqno >= seqno)
            break;

        /* Binner out-of-memory (both the INTCTL latch and the PCS.BMOOM
         * level condition) is handled inside vc4_v3d_service_interrupts,
         * which feeds the overspill BO. Don't ack OUTOMEM here — a blind
         * W1C would eat the edge-triggered latch before the feed runs. */

        {
            ULONG waited = v3d_now_us() - spin_start;
            if (waited < VC4_GPUWAIT_SPIN_US)
                continue;
            if (waited < VC4_GPUWAIT_NAP_WINDOW)
            {
                vc4_gpu_nap(sd, VC4_GPUWAIT_NAP_US);
                continue;
            }
        }

        if (budget > 0 && ++ticks >= budget)
        {
            /* Timeout: GPU is stuck. Reset both control threads so the
             * next submission can actually take effect (writing CT1CA
             * while CTRUN is asserted does not reload the renderer
             * PC). Bump finished_seqno so subsequent waits don't keep
             * timing out trying to catch up to a render that never
             * completed, and re-snapshot BFC/RFC so the delta tracking
             * in v3d_advance_counters doesn't double-count the post-
             * reset jumps. */
            bug("[VC4Gallium] v3d_wait_seqno TIMEOUT: want seqno=%d fin=%d "
                "— resetting CT0/CT1 and aborting\n",
                seqno, v3d->finished_seqno);
            bug("[VC4Gallium]   PCS=0x%08x CT0: CS=0x%08x CA=0x%08x EA=0x%08x\n",
                V3D_READ(v3d, V3D_PCS), V3D_READ(v3d, V3D_CT0CS),
                V3D_READ(v3d, V3D_CT0CA), V3D_READ(v3d, V3D_CT0EA));
            bug("[VC4Gallium]   CT1: CS=0x%08x CA=0x%08x EA=0x%08x pending_render=%d\n",
                V3D_READ(v3d, V3D_CT1CS), V3D_READ(v3d, V3D_CT1CA),
                V3D_READ(v3d, V3D_CT1EA), (LONG)v3d->pending_render);
            bug("[VC4Gallium]   BFC=%d RFC=%d INTCTL=0x%08x ERRSTAT=0x%08x SRQCS=0x%08x\n",
                V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff,
                V3D_READ(v3d, V3D_INTCTL), V3D_READ(v3d, V3D_ERRSTAT),
                V3D_READ(v3d, V3D_SRQCS));
            bug("[VC4Gallium]   BPCA=0x%08x BPCS=0x%08x BPOA=0x%08x BPOS=0x%08x "
                "handed=%d cnt: oom=%d fl=%d fr=%d\n",
                V3D_READ(v3d, V3D_BPCA), V3D_READ(v3d, V3D_BPCS),
                V3D_READ(v3d, V3D_BPOA), V3D_READ(v3d, V3D_BPOS),
                v3d->overflow_handed, v3d->int_outomem, v3d->int_fldone,
                v3d->int_frdone);
            v3d_dump_last_submit();
            V3D_WRITE(v3d, V3D_CT0CS, V3D_CTCS_CTRSTA);
            V3D_WRITE(v3d, V3D_CT1CS, V3D_CTCS_CTRSTA);
            /* The reset killed EVERY in-flight job, not just the one we
             * waited for (a pool-reuse wait targets an older seqno), so
             * catch the bookkeeping up to the newest submission — else the
             * next wait times out a second time chasing a job that can no
             * longer complete. Drop any pending CT1 handoff too: kicking
             * it now would re-run a dead job's RCL. */
            v3d->pending_render = FALSE;
            v3d->finished_seqno = v3d->seqno;
            v3d->last_bfc = V3D_READ(v3d, V3D_BFC) & 0xff;
            v3d->last_rfc = V3D_READ(v3d, V3D_RFC) & 0xff;
            v3d->bfc_completed = v3d->seqno;
            v3d->rfc_completed = v3d->seqno;
            vc4_wait_leave(sd, wsig);
            ReleaseSemaphore(&sd->render_lock);
            return -1;
        }

        /* Block until the next vblank (~20 ms, CPU-free) on the registered
         * path; spin otherwise (no free signal — rare). */
        if (wsig >= 0)
            Wait(1UL << wsig);
    }

    vc4_wait_leave(sd, wsig);
    ReleaseSemaphore(&sd->render_lock);
    return 0;
}

static int ioctl_wait_seqno(struct vc4galliumstaticdata *sd, struct drm_vc4_wait_seqno *args)
{
    if (sd->v3d.finished_seqno >= args->seqno)
        return 0;

    /* timeout_ns == 0 is Mesa's non-blocking busy probe (PIPE_TRANSFER_
     * DONTBLOCK and friends). Poll once — the V3D IRQ is masked, so
     * nothing advances the counters unless polled — then report busy via
     * ETIME instead of blocking out the full budget. */
    if (args->timeout_ns == 0)
    {
        ObtainSemaphore(&sd->render_lock);
        vc4_v3d_service_interrupts(&sd->v3d);
        v3d_advance_counters(&sd->v3d);
        ReleaseSemaphore(&sd->render_lock);

        if (sd->v3d.finished_seqno >= args->seqno)
            return 0;
        /* Negative errno crosses the bridge (the mesa3dgl-side shim
         * decodes it into Mesa's errno); a bare -1 reads as EINVAL
         * there and vc4_bo_wait() abort()s on anything but ETIME. */
        return -ETIME;
    }

    /* ~1 second budget at ~3.5M iters/sec. Longer than a real frame, short
     * enough to fail fast when the GPU is wedged. Report a timeout as
     * ETIME, which Mesa's vc4_bo_wait() treats as
     * "still busy" and carries on, but abort()s the process on ANY
     * other errno — a slow frame must not be fatal (returning any other
     * error killed the client via __stdc_jmp2exit). */
    if (v3d_wait_seqno(sd, args->seqno, 3500000) != 0)
        return -ETIME;
    return 0;
}

static int ioctl_wait_bo(struct vc4galliumstaticdata *sd, struct drm_vc4_wait_bo *args)
{
    struct drm_vc4_wait_seqno wait;

    /* An unknown handle counts as idle. Reporting an error here would be
     * fatal: vc4_bo_wait() abort()s the client on any errno but ETIME. */
    if (args->handle == 0 || args->handle >= VC4_MAX_BOS ||
        sd->bo_table[args->handle].refcount == 0)
        return 0;

    /* The BO's OWN last user, not the newest job in flight: a BO no
     * submission has touched since, or never touched at all, is idle no
     * matter what the GPU is doing now. */
    wait.seqno = sd->bo_table[args->handle].seqno;
    if (wait.seqno == 0)
        return 0;
    wait.timeout_ns = args->timeout_ns;
    return ioctl_wait_seqno(sd, &wait);
}

/* ---- Pooled frame BO allocation ----
 *
 * Reuse GPU BOs across frames. Only reallocate if the requested size
 * exceeds the current allocation. This avoids 12 mailbox round-trips
 * per frame (2 per alloc + 2 per free, times 3 BOs).
 */
static APTR pool_bo_get(struct vc4galliumstaticdata *sd,
                         struct vc4_frame_bo *pool,
                         ULONG size, ULONG align, ULONG flags)
{
    if (pool->size >= size && pool->vaddr)
        return pool->vaddr;

    if (pool->vaddr)
    {
        gpu_mem_free(sd, pool->gpu_handle, pool->size);
        pool->vaddr = NULL;
        pool->size = 0;
    }

    /* Round up to avoid frequent reallocs */
    ULONG alloc_size = ROUNDUP(size + size / 4, 4096);

    pool->vaddr = gpu_mem_alloc(sd, alloc_size, align, flags, &pool->gpu_handle);
    if (!pool->vaddr)
    {
        /* Try without the 25% headroom */
        alloc_size = ROUNDUP(size, 4096);
        pool->vaddr = gpu_mem_alloc(sd, alloc_size, align, flags, &pool->gpu_handle);
        if (!pool->vaddr)
            return NULL;
    }

    pool->bus_addr = GPU_BUS_ADDR(pool->vaddr);
    pool->size = alloc_size;

    D(bug("[VC4Gallium] pool_bo_get: grew to %d bytes (requested %d)\n",
        alloc_size, size));

    return pool->vaddr;
}

/* ---- Unaligned memory access helpers ----
 *
 * GPU memory on RPi is mapped Device/Strongly-Ordered.  On ARMv7
 * that means *any* word access to an unaligned address aborts,
 * regardless of SCTLR.A — unaligned-access support only applies to
 * Normal memory.  CL packets are byte-packed, so addresses within
 * packets sit at arbitrary byte offsets and must be issued as
 * separate byte transactions.
 *
 * Neither a plain byte sequence (gcc -O2 store-merges back to STR)
 * nor a __packed struct (still emits one STR — fine on Normal,
 * fatal on Device) is safe.  Volatile byte stores are the only
 * portable way to guarantee STRB/LDRB: the compiler may not coalesce
 * or reorder volatile accesses.
 */
static inline ULONG get_u32_unaligned(const UBYTE *p)
{
    const volatile UBYTE *vp = (const volatile UBYTE *)p;
    return (ULONG)vp[0] | ((ULONG)vp[1] << 8) |
           ((ULONG)vp[2] << 16) | ((ULONG)vp[3] << 24);
}

static inline void put_u32_unaligned(UBYTE *p, ULONG val)
{
    volatile UBYTE *vp = (volatile UBYTE *)p;
    vp[0] = (UBYTE)(val & 0xFF);
    vp[1] = (UBYTE)((val >> 8) & 0xFF);
    vp[2] = (UBYTE)((val >> 16) & 0xFF);
    vp[3] = (UBYTE)(val >> 24);
}

/* ---- Bin CL Processing ----
 *
 * Walk the user-submitted bin CL, stripping GEM_HANDLES packets and
 * patching addresses:
 * - TILE_BINNING_MODE_CONFIG: replace tile alloc/state addresses
 * - GL_SHADER_STATE: replace shader record offset with GPU address
 * - GL_INDEXED_PRIMITIVE: replace index buffer offset with GPU address
 *
 * GEM_HANDLES sets the "current BO" for subsequent packets.
 * Returns the output CL size, or 0 on error.
 */
struct exec_state {
    struct vc4galliumstaticdata *sd;
    ULONG *bo_handles;
    ULONG bo_handle_count;
    ULONG current_bo[2];        /* Current BO bus addresses set by GEM_HANDLES */
    /* Tile alloc BO */
    ULONG tile_alloc_bus;
    ULONG tile_state_bus;
    ULONG tile_alloc_offset;    /* Offset of tile alloc within tile BO */
    ULONG tile_alloc_size;      /* Size of that region (what the BO holds) */
    ULONG bin_tiles_x;
    ULONG bin_tiles_y;
    /* Shader rec info: flags collected from each GL_SHADER_STATE packet,
     * in CL order (relocate_shader_recs walks the shader_rec buffer using
     * them). Points at sd->shader_state_scratch, sized shader_state_max. */
    ULONG *shader_state_addrs;      /* Low bits from GL_SHADER_STATE */
    ULONG shader_state_max;         /* Capacity of shader_state_addrs */
    ULONG shader_state_count;
    /* Shader rec validated bus address base */
    ULONG shader_rec_bus;
    /* Validated uniforms bus address base */
    ULONG uniforms_bus;
};

static ULONG process_bin_cl(struct exec_state *exec,
                            UBYTE *src, ULONG src_size,
                            UBYTE *dst, ULONG dst_max)
{
    ULONG src_off = 0;
    ULONG dst_off = 0;
    ULONG shader_rec_p = exec->shader_rec_bus;

    /* RCL-only submissions: Mesa's vc4_tile_blit (glReadPixels staging
     * copies, texture updates of busy resources, u_blitter surface copies)
     * legitimately carries NO bin CL. Rather than special-casing the whole
     * pipeline, synthesize a minimal draw-less bin CL: the binner auto-inits
     * tile state, the FLUSH emits empty per-tile lists (as for any geometry-
     * free tile), INCREMENT pairs with the RCL's WAIT_ON_SEMAPHORE, FLDONE
     * fires, and BFC keeps tracking seqno 1:1. Rejecting these (the old
     * behaviour) silently broke every internal Mesa blit. */
    if (src_size == 0)
    {
        if (dst_max < 19)
            return 0;

        dst[0] = VC4_PACKET_TILE_BINNING_MODE_CONFIG;
        put_u32_unaligned(dst + 1, exec->tile_alloc_bus);
        put_u32_unaligned(dst + 5, exec->tile_alloc_size);
        put_u32_unaligned(dst + 9, exec->tile_state_bus);
        dst[13] = exec->bin_tiles_x;
        dst[14] = exec->bin_tiles_y;
        dst[15] = VC4_BIN_CONFIG_AUTO_INIT_TSDA |
                  VC4_BIN_CONFIG_ALLOC_INIT_BLOCK_SIZE_32 |
                  VC4_BIN_CONFIG_ALLOC_BLOCK_SIZE_128;
        dst[16] = VC4_PACKET_START_TILE_BINNING;
        dst[17] = VC4_PACKET_INCREMENT_SEMAPHORE;
        dst[18] = VC4_PACKET_FLUSH;
        return 19;
    }

    while (src_off < src_size)
    {
        UBYTE cmd = src[src_off];
        ULONG pkt_size = vc4_packet_sizes[cmd];

        if (pkt_size == 0)
        {
            bug("[VC4Gallium] process_bin_cl: unknown packet 0x%02x at offset %d\n",
                cmd, src_off);
            return 0;
        }

        if (src_off + pkt_size > src_size)
        {
            bug("[VC4Gallium] process_bin_cl: packet 0x%02x overflows CL (%d + %d > %d)\n",
                cmd, src_off, pkt_size, src_size);
            return 0;
        }

        switch (cmd)
        {
        case VC4_PACKET_GEM_HANDLES:
        {
            /* 1 byte cmd + handle0 (4) + handle1 (4) = 9 bytes */
            ULONG hindex0 = get_u32_unaligned(src + src_off + 1);
            ULONG hindex1 = get_u32_unaligned(src + src_off + 5);

            exec->current_bo[0] = bo_bus_addr(exec->sd, exec->bo_handles,
                                               exec->bo_handle_count, hindex0);
            if (hindex1 != 0)
                exec->current_bo[1] = bo_bus_addr(exec->sd, exec->bo_handles,
                                                   exec->bo_handle_count, hindex1);

            /* Don't copy GEM_HANDLES to output — it's not a real HW packet */
            src_off += pkt_size;
            continue;
        }

        case VC4_PACKET_GL_SHADER_STATE:
        {
            /* GL_SHADER_STATE: 1 byte cmd + 4 byte address.
             * Low 4 bits are flags (nr_attributes + extended bit).
             * The address field is the offset into the shader_rec buffer.
             * We replace it with the GPU bus address of the validated
             * shader record.
             */
            ULONG addr = get_u32_unaligned(src + src_off + 1);
            ULONG low_bits = addr & 0xf;

            /* Record this shader state's flags for shader rec relocation.
             * Fail the submission when the table is full — truncating
             * would leave later GL_SHADER_STATE packets pointing at
             * records relocate_shader_recs never wrote. */
            if (exec->shader_state_count >= exec->shader_state_max)
            {
                bug("[VC4Gallium] process_bin_cl: more than %lu "
                    "GL_SHADER_STATE packets in one CL\n",
                    (unsigned long)exec->shader_state_max);
                return 0;
            }
            exec->shader_state_addrs[exec->shader_state_count++] = low_bits;

            /* Compute shader record packet size for advancing shader_rec_p */
            ULONG nr_attr = low_bits & 0x7;
            if (nr_attr == 0) nr_attr = 8;
            ULONG rec_size = (low_bits & 0x8) ? (100 + nr_attr * 4) : (36 + nr_attr * 8);

            /* Copy packet to output, patch address */
            if (dst_off + pkt_size > dst_max) return 0;
            dst[dst_off] = cmd;
            put_u32_unaligned(dst + dst_off + 1, (shader_rec_p | low_bits));

            shader_rec_p += ROUNDUP(rec_size, 16);

            dst_off += pkt_size;
            src_off += pkt_size;
            continue;
        }

        case VC4_PACKET_GL_INDEXED_PRIMITIVE:
        {
            /* GL_INDEXED_PRIMITIVE: 1 byte cmd + 1 byte mode + 4 byte count +
             * 4 byte index_offset + 4 byte max_index = 14 bytes.
             * The index_offset (at byte 6) needs BO address relocation.
             */
            if (dst_off + pkt_size > dst_max) return 0;
            CopyMem(src + src_off, dst + dst_off, pkt_size);

            /* Patch index buffer address: offset is at byte 6 (after cmd+mode+count) */
            ULONG ib_offset = get_u32_unaligned(src + src_off + 6);
            put_u32_unaligned(dst + dst_off + 6, exec->current_bo[0] + ib_offset);

            dst_off += pkt_size;
            src_off += pkt_size;
            continue;
        }

        case VC4_PACKET_TILE_BINNING_MODE_CONFIG:
        {
            /* TILE_BINNING_MODE_CONFIG: 16 bytes.
             * bytes 1-4:  tile alloc address -> our tile_alloc_bus
             * bytes 5-8:  tile alloc size
             * bytes 9-12: tile state address -> our tile_state_bus
             * byte 13:    tiles_x
             * byte 14:    tiles_y
             * byte 15:    flags
             */
            if (dst_off + pkt_size > dst_max) return 0;
            CopyMem(src + src_off, dst + dst_off, pkt_size);

            exec->bin_tiles_x = src[src_off + 13];
            exec->bin_tiles_y = src[src_off + 14];

            /* Patch tile alloc address, size, and tile state address. The
             * size must be what the tile BO actually holds — recomputing it
             * from the packet's tile counts could claim more than the BO
             * was sized for. */
            put_u32_unaligned(dst + dst_off + 1, exec->tile_alloc_bus);
            put_u32_unaligned(dst + dst_off + 5, exec->tile_alloc_size);
            put_u32_unaligned(dst + dst_off + 9, exec->tile_state_bus);

            /* Patch flags: set auto-init TSDA, block sizes */
            UBYTE flags = dst[dst_off + 15];
            flags &= ~(VC4_BIN_CONFIG_ALLOC_INIT_BLOCK_SIZE_MASK |
                        VC4_BIN_CONFIG_ALLOC_BLOCK_SIZE_MASK);
            flags |= VC4_BIN_CONFIG_AUTO_INIT_TSDA |
                     VC4_BIN_CONFIG_ALLOC_INIT_BLOCK_SIZE_32 |
                     VC4_BIN_CONFIG_ALLOC_BLOCK_SIZE_128;
            dst[dst_off + 15] = flags;

            dst_off += pkt_size;
            src_off += pkt_size;
            continue;
        }

        default:
            if (dst_off + pkt_size > dst_max) return 0;
            CopyMem(src + src_off, dst + dst_off, pkt_size);
            dst_off += pkt_size;
            src_off += pkt_size;
            continue;
        }
    }

    return dst_off;
}

/* ---- Shader Record Relocation ----
 *
 * Mesa's shader_rec buffer contains, for each shader record:
 *   [handle0][handle1]...[handleN]   (N = 3 + nr_attributes) uint32 each
 *   [shader_record_packet_data]      (variable size)
 *
 * The handle indices reference entries in the bo_handles array.
 * Shader record offsets that need patching:
 *   offset 4:  FS code address        (handle[0])
 *   offset 8:  FS uniform address     (set to validated uniforms position)
 *   offset 16: VS code address        (handle[1])
 *   offset 20: VS uniform address     (set to validated uniforms position)
 *   offset 28: CS code address        (handle[2])
 *   offset 32: CS uniform address     (set to validated uniforms position)
 *   offset 36 + i*8: VBO i base addr  (handle[3+i])
 *
 * Uniform stream layout per shader (in Mesa's uniform buffer):
 *   [tex_handle_0..N-1]  N = num_texture_samples, each is a BO handle index
 *   [uniform_data_0..M]  remaining uniform values (plain data or need reloc)
 * Total uniform reads = uniforms_size/4 (from QPU scan).
 * The first N reads are texture p0 addresses.
 */

/* Uniform offsets in shader record: FS=8, VS=20, CS=32 */
static const ULONG shader_code_offsets[]    = { 4, 16, 28 };
static const ULONG shader_uniform_offsets[] = { 8, 20, 32 };

static int relocate_shader_recs(struct exec_state *exec,
                                UBYTE *shader_rec_data,
                                ULONG shader_rec_size,
                                UBYTE *out_rec_data,
                                ULONG *uniforms_src,
                                ULONG uniforms_src_count,
                                ULONG *uniforms_dst,
                                ULONG uniforms_dst_max,
                                ULONG *out_uniforms_count)
{
    ULONG src_off = 0;
    ULONG dst_off = 0;
    ULONG uni_src_idx = 0;    /* Current position in source uniform stream */
    ULONG uni_dst_idx = 0;    /* Current position in output uniform stream */
    ULONG i, rec;

    for (rec = 0; rec < exec->shader_state_count; rec++)
    {
        ULONG low_bits = exec->shader_state_addrs[rec];
        ULONG nr_attributes = low_bits & 0x7;
        ULONG extended = low_bits & 0x8;
        ULONG nr_relocs, packet_size;

        if (nr_attributes == 0)
            nr_attributes = 8;

        nr_relocs = 3 + nr_attributes;
        packet_size = extended ? (100 + nr_attributes * 4) : (36 + nr_attributes * 8);

        /* Validate source data bounds */
        if (src_off + nr_relocs * 4 + packet_size > shader_rec_size)
        {
            bug("[VC4Gallium] relocate_shader_recs: overflow at rec %d "
                "(off %d + %d handles + %d data > %d)\n",
                rec, src_off, nr_relocs * 4, packet_size, shader_rec_size);
            return -1;
        }

        ULONG *handles = (ULONG *)(shader_rec_data + src_off);
        UBYTE *pkt_src = shader_rec_data + src_off + nr_relocs * 4;
        UBYTE *pkt_dst = out_rec_data + dst_off;

        CopyMem(pkt_src, pkt_dst, packet_size);

        /* Process each shader stage: FS(0), VS(1), CS(2) */
        for (i = 0; i < 3; i++)
        {
            ULONG code_off = shader_code_offsets[i];
            ULONG uni_off  = shader_uniform_offsets[i];

            /* Patch shader code address */
            ULONG src_offset = get_u32_unaligned(pkt_src + code_off);
            ULONG code_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                           exec->bo_handle_count, handles[i]);
            put_u32_unaligned(pkt_dst + code_off, code_addr + src_offset);

            /* Look up shader BO to get uniform metadata. Bounds-check
             * the same way bo_bus_addr() does — handles[i] is the hindex
             * into bo_handles[], whose value is the bo_table[] handle. */
            if (handles[i] >= exec->bo_handle_count)
            {
                bug("[VC4Gallium] relocate_shader_recs: rec %d stage %d "
                    "hindex %d >= bo_handle_count %d\n",
                    rec, i, handles[i], exec->bo_handle_count);
                return -1;
            }
            ULONG shader_handle = exec->bo_handles[handles[i]];
            if (shader_handle == 0 || shader_handle >= VC4_MAX_BOS ||
                exec->sd->bo_table[shader_handle].refcount == 0)
            {
                bug("[VC4Gallium] relocate_shader_recs: rec %d stage %d "
                    "invalid shader handle %d\n", rec, i, shader_handle);
                return -1;
            }
            struct vc4_bo_entry *shader_bo = &exec->sd->bo_table[shader_handle];
            ULONG num_tex = shader_bo->num_texture_samples;
            ULONG num_uniforms = shader_bo->uniforms_size / 4;  /* Data words the QPU reads */
            ULONG src_total = num_tex + num_uniforms;  /* Total words in source per shader */

            /* Set uniform address to current position in validated uniform output */
            put_u32_unaligned(pkt_dst + uni_off, exec->uniforms_bus + uni_dst_idx * 4);

            D(bug("[VC4Gallium]   shader[%d] rec %d: code=0x%08x, %d uniforms (%d tex)\n",
                i, rec, get_u32_unaligned(pkt_dst + code_off), num_uniforms, num_tex));

            /* Source per shader (Mesa's cl_start_shader_reloc): N tex-handle
             * words then M data words (N+M total). Output is the M data words;
             * texture p0 words within them are patched below using p_offset
             * from shader validation. */
            if (uni_src_idx + src_total > uniforms_src_count)
            {
                bug("[VC4Gallium] relocate_shader_recs: uniform overflow at rec %d "
                    "shader %d (idx %d + %d > %d)\n",
                    rec, i, uni_src_idx, src_total, uniforms_src_count);
                return -1;
            }
            if (uni_dst_idx + num_uniforms > uniforms_dst_max)
            {
                bug("[VC4Gallium] relocate_shader_recs: uniform dst overflow\n");
                return -1;
            }

            {
                /* Tex-handle indices first, then the actual uniform data */
                ULONG *tex_handles_u = &uniforms_src[uni_src_idx];
                ULONG *uniform_data = &uniforms_src[uni_src_idx + num_tex];
                ULONG *uniforms_out = &uniforms_dst[uni_dst_idx];
                ULONG uni_start_p = exec->uniforms_bus + uni_dst_idx * 4;
                ULONG d, t, u;

                for (d = 0; d < num_uniforms; d++)
                    uniforms_out[d] = uniform_data[d];

                /* Texture P0 relocation: for each texture sample, patch
                 * the P0 word in the output uniforms with the texture
                 * BO's bus address + the P0 offset field. */
                for (t = 0; t < num_tex; t++)
                {
                    if (t >= VC4_MAX_TEX_SAMPLES)
                        break;

                    struct vc4_texture_sample_info *sample =
                        &shader_bo->texture_samples[t];
                    ULONG p0_word_idx = sample->p_offset[0] / 4;

                    if (sample->p_offset[0] == ~0UL || p0_word_idx >= num_uniforms)
                        continue;

                    /* Get texture BO bus address from handle index */
                    ULONG tex_bus = bo_bus_addr(exec->sd, exec->bo_handles,
                                                exec->bo_handle_count,
                                                tex_handles_u[t]);
                    if (tex_bus == 0)
                        continue;

                    /* P0 format: bits [31:12] = level-0 offset, bits [11:0]
                     * = config (TYPE[7:4], MIPLVLS[3:0], FLIPY, CMMODE,
                     * CSWIZ). ADD the whole word:
                     * the BO base is 4K-aligned, so the add applies the
                     * offset and keeps the config bits. Masking the low
                     * bits off forced type=RGBA8888 on every texture,
                     * which scrambled all 8-bit and 565 sampling. The
                     * same formula covers is_direct (P0 = plain address). */
                    ULONG p0_val = uniforms_out[p0_word_idx];
                    uniforms_out[p0_word_idx] = tex_bus + p0_val;

                    D(bug("[VC4Gallium]   tex[%d]: p0_idx=%d p0=0x%08x -> 0x%08x\n",
                        t, p0_word_idx, p0_val, uniforms_out[p0_word_idx]));
                }

                /* Fill in UNIFORMS_ADDRESS slots: the kernel uses
                 * uniform_addr_offsets[] to find which uniform words
                 * need the shader's uniform stream GPU address. */
                for (u = 0; u < shader_bo->num_uniform_addr_offsets; u++)
                {
                    ULONG o = shader_bo->uniform_addr_offsets[u];
                    if (o < num_uniforms)
                        uniforms_out[o] = uni_start_p;
                }
            }

            uni_src_idx += src_total;
            uni_dst_idx += num_uniforms;
        }

        /* Patch VBO attribute base addresses (at offset 36 + i*8) */
        for (i = 0; i < nr_attributes; i++)
        {
            ULONG o = 36 + i * 8;
            ULONG vbo_offset = get_u32_unaligned(pkt_src + o);
            ULONG addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                      exec->bo_handle_count, handles[3 + i]);
            put_u32_unaligned(pkt_dst + o, addr + vbo_offset);
        }

        src_off += nr_relocs * 4 + packet_size;
        dst_off += ROUNDUP(packet_size, 16);
    }

    *out_uniforms_count = uni_dst_idx;
    return 0;
}

/* ---- RCL Generation ----
 *
 * Build the Render Control List. The RCL tells the GPU how to load,
 * process, and store each tile's pixel data.
 *
 * Based on Mesa's MIT-licensed src/gallium/drivers/vc4/kernel/vc4_render_cl.c.
 */

/* RCL builder helpers */
struct rcl_builder {
    UBYTE *data;
    ULONG offset;
    ULONG max_size;
};

/* Volatile byte writes — see put_u32_unaligned above for the why
 * (gcc -O2 store-merging vs Device-mapped RCL BO memory). */
static inline void rcl_u8(struct rcl_builder *b, UBYTE val)
{
    if (b->offset < b->max_size)
        ((volatile UBYTE *)b->data)[b->offset] = val;
    b->offset++;
}

static inline void rcl_u16(struct rcl_builder *b, UWORD val)
{
    if (b->offset + 1 < b->max_size)
    {
        volatile UBYTE *vp = (volatile UBYTE *)b->data + b->offset;
        vp[0] = (UBYTE)(val & 0xFF);
        vp[1] = (UBYTE)(val >> 8);
    }
    b->offset += 2;
}

static inline void rcl_u32(struct rcl_builder *b, ULONG val)
{
    if (b->offset + 3 < b->max_size)
    {
        volatile UBYTE *vp = (volatile UBYTE *)b->data + b->offset;
        vp[0] = (UBYTE)(val & 0xFF);
        vp[1] = (UBYTE)((val >> 8) & 0xFF);
        vp[2] = (UBYTE)((val >> 16) & 0xFF);
        vp[3] = (UBYTE)(val >> 24);
    }
    b->offset += 4;
}

static void rcl_store_before_load(struct rcl_builder *b)
{
    rcl_u8(b, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
    rcl_u16(b,
        VC4_LOADSTORE_TILE_BUFFER_NONE |
        VC4_STORE_TILE_BUFFER_DISABLE_COLOR_CLEAR |
        VC4_STORE_TILE_BUFFER_DISABLE_ZS_CLEAR |
        VC4_STORE_TILE_BUFFER_DISABLE_VG_MASK_CLEAR);
    rcl_u32(b, 0);
}

static void rcl_tile_coords(struct rcl_builder *b, UBYTE x, UBYTE y)
{
    rcl_u8(b, VC4_PACKET_TILE_COORDINATES);
    rcl_u8(b, x);
    rcl_u8(b, y);
}

static ULONG rcl_full_res_offset(ULONG base_addr, ULONG surf_offset,
                                  ULONG width, ULONG tile_width,
                                  UBYTE x, UBYTE y)
{
    return base_addr + surf_offset +
           VC4_TILE_BUFFER_SIZE * (DIV_ROUND_UP(width, tile_width) * y + x);
}

static int build_rcl(struct exec_state *exec,
                     struct drm_vc4_submit_cl *args,
                     UBYTE *rcl_data, ULONG rcl_max_size,
                     ULONG *out_size)
{
    struct rcl_builder b = { rcl_data, 0, rcl_max_size };
    UBYTE min_x = args->min_x_tile;
    UBYTE min_y = args->min_y_tile;
    UBYTE max_x = args->max_x_tile;
    UBYTE max_y = args->max_y_tile;
    UBYTE xtiles = max_x - min_x + 1;
    UBYTE ytiles = max_y - min_y + 1;
    UBYTE xi, yi;
    /* Even RCL-only submissions (bin_cl_size == 0) get a synthesized bin
     * CL from process_bin_cl, so the semaphore interlock and per-tile
     * branch lists are always present. */
    BOOL has_bin = TRUE;
    BOOL positive_x = TRUE, positive_y = TRUE;

    /* Resolve surface BO addresses */
    ULONG color_write_addr = 0;
    ULONG color_read_addr = 0;
    ULONG zs_read_addr = 0;
    ULONG zs_write_addr = 0;
    ULONG msaa_color_write_addr = 0;
    ULONG msaa_zs_write_addr = 0;
    BOOL has_color_write = (args->color_write.hindex != (ULONG)~0);
    BOOL has_color_read  = (args->color_read.hindex != (ULONG)~0);
    BOOL has_zs_read     = (args->zs_read.hindex != (ULONG)~0);
    BOOL has_zs_write    = (args->zs_write.hindex != (ULONG)~0);
    BOOL has_msaa_color  = (args->msaa_color_write.hindex != (ULONG)~0);
    BOOL has_msaa_zs     = (args->msaa_zs_write.hindex != (ULONG)~0);

    if (has_color_write)
        color_write_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                        exec->bo_handle_count,
                                        args->color_write.hindex);
    if (has_color_read)
        color_read_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                       exec->bo_handle_count,
                                       args->color_read.hindex);
    if (has_zs_read)
        zs_read_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                    exec->bo_handle_count,
                                    args->zs_read.hindex);
    if (has_zs_write)
        zs_write_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                     exec->bo_handle_count,
                                     args->zs_write.hindex);
    if (has_msaa_color)
        msaa_color_write_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                             exec->bo_handle_count,
                                             args->msaa_color_write.hindex);
    if (has_msaa_zs)
        msaa_zs_write_addr = bo_bus_addr(exec->sd, exec->bo_handles,
                                          exec->bo_handle_count,
                                          args->msaa_zs_write.hindex);

    /* Fixed RCL order */
    if (args->flags & VC4_SUBMIT_CL_FIXED_RCL_ORDER)
    {
        if (!(args->flags & VC4_SUBMIT_CL_RCL_ORDER_INCREASING_X))
            positive_x = FALSE;
        if (!(args->flags & VC4_SUBMIT_CL_RCL_ORDER_INCREASING_Y))
            positive_y = FALSE;
    }

    /* RCL header order matches Mesa's MIT-licensed vc4_render_cl.c:344-365 —
     * emit CLEAR_COLORS + TILE_COORDINATES(0,0) + None-mode STORE first to
     * latch the clear values into the tile buffer, THEN emit
     * TILE_RENDERING_MODE_CONFIG. */

    /* Clear colors (if requested) */
    if (args->flags & VC4_SUBMIT_CL_USE_CLEAR_COLOR)
    {
        rcl_u8(&b, VC4_PACKET_CLEAR_COLORS);
        rcl_u32(&b, args->clear_color[0]);
        rcl_u32(&b, args->clear_color[1]);
        rcl_u32(&b, args->clear_z);
        rcl_u8(&b, args->clear_s);

        rcl_tile_coords(&b, 0, 0);

        /* Store in None mode to trigger tile buffer clear */
        rcl_u8(&b, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
        rcl_u16(&b, VC4_LOADSTORE_TILE_BUFFER_NONE);
        rcl_u32(&b, 0);
    }

    /* Tile rendering mode config */
    rcl_u8(&b, VC4_PACKET_TILE_RENDERING_MODE_CONFIG);
    rcl_u32(&b, has_color_write ?
            (color_write_addr + args->color_write.offset) : 0);
    rcl_u16(&b, args->width);
    rcl_u16(&b, args->height);
    rcl_u16(&b, args->color_write.bits);

    D(bug("[VC4Gallium] RCL render config: addr=0x%08x %dx%d color.bits=0x%04x zs.bits=0x%04x flags=0x%x clear=0x%08x,0x%08x z=0x%08x s=0x%02x\n",
        has_color_write ? (color_write_addr + args->color_write.offset) : 0,
        (ULONG)args->width, (ULONG)args->height,
        (UWORD)args->color_write.bits, (UWORD)args->zs_write.bits,
        (ULONG)args->flags,
        (ULONG)args->clear_color[0], (ULONG)args->clear_color[1],
        (ULONG)args->clear_z, (UBYTE)args->clear_s));

    /* Diagnostics: which BO in the handle list is the color target?
     * If color_write.hindex points to a different BO than what
     * DisplayResource ends up reading from, we have a back/front
     * mismatch in the state tracker layer. */
    if (has_color_write && args->color_write.hindex < exec->bo_handle_count)
    {
        ULONG color_handle = exec->bo_handles[args->color_write.hindex];
        D(bug("[VC4Gallium]   color_write hindex=%d -> handle=%d vaddr=0x%08x bus=0x%08x\n",
            (ULONG)args->color_write.hindex, color_handle,
            (ULONG)(IPTR)exec->sd->bo_table[color_handle].vaddr,
            exec->sd->bo_table[color_handle].bus_addr));
    }

    for (yi = 0; yi < ytiles; yi++)
    {
        UBYTE y = positive_y ? (min_y + yi) : (max_y - yi);
        for (xi = 0; xi < xtiles; xi++)
        {
            UBYTE x = positive_x ? (min_x + xi) : (max_x - xi);
            BOOL first = (xi == 0 && yi == 0);
            BOOL last  = (xi == xtiles - 1 && yi == ytiles - 1);

            /* Load color buffer */
            if (has_color_read)
            {
                if (args->color_read.flags & VC4_SUBMIT_RCL_SURFACE_READ_IS_FULL_RES)
                {
                    rcl_u8(&b, VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER);
                    rcl_u32(&b,
                        rcl_full_res_offset(color_read_addr,
                                            args->color_read.offset,
                                            args->width, 64, x, y) |
                        VC4_LOADSTORE_FULL_RES_DISABLE_ZS);
                }
                else
                {
                    rcl_u8(&b, VC4_PACKET_LOAD_TILE_BUFFER_GENERAL);
                    rcl_u16(&b, args->color_read.bits);
                    rcl_u32(&b, color_read_addr + args->color_read.offset);
                }
            }

            /* Load Z/S buffer */
            if (has_zs_read)
            {
                if (has_color_read)
                {
                    /* Execute previous load */
                    rcl_tile_coords(&b, x, y);
                    rcl_store_before_load(&b);
                }

                if (args->zs_read.flags & VC4_SUBMIT_RCL_SURFACE_READ_IS_FULL_RES)
                {
                    rcl_u8(&b, VC4_PACKET_LOAD_FULL_RES_TILE_BUFFER);
                    rcl_u32(&b,
                        rcl_full_res_offset(zs_read_addr,
                                            args->zs_read.offset,
                                            args->width, 64, x, y) |
                        VC4_LOADSTORE_FULL_RES_DISABLE_COLOR);
                }
                else
                {
                    rcl_u8(&b, VC4_PACKET_LOAD_TILE_BUFFER_GENERAL);
                    rcl_u16(&b, args->zs_read.bits);
                    rcl_u32(&b, zs_read_addr + args->zs_read.offset);
                }
            }

            /* Tile coordinates (needed for clipping and triggering loads) */
            rcl_tile_coords(&b, x, y);

            /* Wait for binner on first tile */
            if (first && has_bin)
                rcl_u8(&b, VC4_PACKET_WAIT_ON_SEMAPHORE);

            /* Branch to per-tile bin list */
            if (has_bin)
            {
                rcl_u8(&b, VC4_PACKET_BRANCH_TO_SUB_LIST);
                rcl_u32(&b, exec->tile_alloc_bus +
                        (y * exec->bin_tiles_x + x) * 32);
            }

            /* Store MSAA color */
            if (has_msaa_color)
            {
                BOOL last_write = (!has_msaa_zs && !has_zs_write && !has_color_write);
                ULONG bits = VC4_LOADSTORE_FULL_RES_DISABLE_ZS;
                if (!last_write)
                    bits |= VC4_LOADSTORE_FULL_RES_DISABLE_CLEAR_ALL;
                else if (last)
                    bits |= VC4_LOADSTORE_FULL_RES_EOF;
                rcl_u8(&b, VC4_PACKET_STORE_FULL_RES_TILE_BUFFER);
                rcl_u32(&b,
                    rcl_full_res_offset(msaa_color_write_addr,
                                        args->msaa_color_write.offset,
                                        args->width, 64, x, y) | bits);
            }

            /* Store MSAA Z/S */
            if (has_msaa_zs)
            {
                BOOL last_write = (!has_zs_write && !has_color_write);
                ULONG bits = VC4_LOADSTORE_FULL_RES_DISABLE_COLOR;
                if (has_msaa_color)
                    rcl_tile_coords(&b, x, y);
                if (!last_write)
                    bits |= VC4_LOADSTORE_FULL_RES_DISABLE_CLEAR_ALL;
                else if (last)
                    bits |= VC4_LOADSTORE_FULL_RES_EOF;
                rcl_u8(&b, VC4_PACKET_STORE_FULL_RES_TILE_BUFFER);
                rcl_u32(&b,
                    rcl_full_res_offset(msaa_zs_write_addr,
                                        args->msaa_zs_write.offset,
                                        args->width, 64, x, y) | bits);
            }

            /* Store Z/S buffer */
            if (has_zs_write)
            {
                BOOL last_write = !has_color_write;
                if (has_msaa_color || has_msaa_zs)
                    rcl_tile_coords(&b, x, y);

                rcl_u8(&b, VC4_PACKET_STORE_TILE_BUFFER_GENERAL);
                rcl_u16(&b, args->zs_write.bits |
                    (last_write ? 0 : VC4_STORE_TILE_BUFFER_DISABLE_COLOR_CLEAR));
                rcl_u32(&b,
                    (zs_write_addr + args->zs_write.offset) |
                    ((last && last_write) ? VC4_LOADSTORE_TILE_BUFFER_EOF : 0));
            }

            /* Store color buffer (final store for this tile) */
            if (has_color_write)
            {
                if (has_msaa_color || has_msaa_zs || has_zs_write)
                    rcl_tile_coords(&b, x, y);

                if (last)
                    rcl_u8(&b, VC4_PACKET_STORE_MS_TILE_BUFFER_AND_EOF);
                else
                    rcl_u8(&b, VC4_PACKET_STORE_MS_TILE_BUFFER);
            }
        }
    }

    *out_size = b.offset;
    if (b.offset > rcl_max_size)
    {
        bug("[VC4Gallium] build_rcl: RCL overflow (%d > %d)\n", b.offset, rcl_max_size);
        return -1;
    }

    return 0;
}

/*
 * Submit a render job to V3D. From Mesa's bin_cl (GEM_HANDLES interleaved),
 * shader records and uniforms, we:
 * 1. Get pooled tile state + tile alloc memory (reused across frames)
 * 2. Process bin CL: strip GEM_HANDLES, patch addresses
 * 3. Relocate shader records: resolve handle indices to GPU bus addrs
 * 4. Copy uniforms (as-is for now — no texture relocation yet)
 * 5. Build the RCL from surface config
 * 6. Submit binning CL (CT0) + render CL (CT1) concurrently, return seqno
 */
static int do_submit_cl(struct vc4galliumstaticdata *sd, struct drm_vc4_submit_cl *args)
{
    struct vc4_v3d_state *v3d = &sd->v3d;
    ULONG *bo_handles;
    ULONG tiles_x, tiles_y, tile_count;
    ULONG tile_state_size, tile_alloc_size, tile_bo_size;
    APTR tile_vaddr;
    ULONG exec_size, bin_cl_max;
    APTR exec_vaddr;
    ULONG exec_bus;
    ULONG bin_cl_offset, shader_rec_offset, uniforms_offset;
    ULONG bin_cl_out_size;
    ULONG rcl_max_size, rcl_size;
    APTR rcl_vaddr;
    ULONG rcl_bus;
    ULONG seqno;
    int ret;
    struct exec_state exec;

    if (!v3d->v3d_available)
        return -1;

    ULONG _t_entry = VC4G_NOW_US();

    bo_handles = (ULONG *)(IPTR)args->bo_handles;

    tiles_x = args->max_x_tile - args->min_x_tile + 1;
    tiles_y = args->max_y_tile - args->min_y_tile + 1;

    /* Tile counts for sizing the tile state/alloc BO. These must match the
     * BINNER's view, i.e. the tile counts in the TILE_BINNING_MODE_CONFIG
     * packet (always the full framebuffer), NOT the draw-bounds max_*_tile
     * from the submit args: tile state is indexed by global tile coordinate
     * with the packet's row width, so when a frame's draws don't reach the
     * right/bottom edge the packet counts exceed the draw-bounds counts and
     * a BO sized from the latter lets the binner write tile state past its
     * area into the sublists. Prescan the packet for the real tile
     * counts; keep the draw-bounds counts as the floor (they are what
     * the synthesized RCL-only bin CL uses). */
    {
        ULONG bin_tiles_x = args->max_x_tile + 1;
        ULONG bin_tiles_y = args->max_y_tile + 1;
        UBYTE *src = (UBYTE *)(IPTR)args->bin_cl;
        ULONG off = 0;

        while (off < args->bin_cl_size)
        {
            UBYTE cmd = src[off];
            ULONG pkt_size = vc4_packet_sizes[cmd];

            if (pkt_size == 0 || off + pkt_size > args->bin_cl_size)
                break;      /* malformed CL — process_bin_cl rejects it */
            if (cmd == VC4_PACKET_TILE_BINNING_MODE_CONFIG)
            {
                if (src[off + 13] > bin_tiles_x)
                    bin_tiles_x = src[off + 13];
                if (src[off + 14] > bin_tiles_y)
                    bin_tiles_y = src[off + 14];
            }
            off += pkt_size;
        }

        tile_count = bin_tiles_x * bin_tiles_y;
    }

    D(bug("[VC4Gallium] submit_cl: %dx%d tiles (%dx%d), bin_cl=%d shader_rec=%d uniforms=%d bo_count=%d\n",
        tiles_x, tiles_y, args->width, args->height,
        args->bin_cl_size, args->shader_rec_size,
        args->uniforms_size, args->bo_handle_count));

    /* ---- Step 0: Select pool set and wait if it's still in flight ---- */
    /*
     * Double-buffered pools: alternate between two sets so the CPU can
     * prepare frame N+1 while the GPU renders frame N using the other set.
     * We only block if the pool set we want to use hasn't finished yet.
     */
    {
        ULONG pi = sd->pool_idx;
        if (sd->pool[pi].seqno > v3d->finished_seqno)
        {
            /* This pool set is still in use by the GPU — wait for it */
            v3d_wait_seqno(sd, sd->pool[pi].seqno, 10000000);
        }
    }
    ULONG pi = sd->pool_idx;

    /* ---- Step 1: Get pooled tile state + alloc memory ---- */
    tile_state_size = 48 * tile_count;
    ULONG tile_alloc_offset = ROUNDUP(tile_state_size, 4096);
    tile_alloc_size = ROUNDUP(32 * tile_count, 256) + 1024 * 1024;  /* + 1MB overflow */
    tile_bo_size = tile_alloc_offset + tile_alloc_size;

    tile_vaddr = pool_bo_get(sd, &sd->pool[pi].tile, tile_bo_size, 4096,
                              VCMEM_L1NONALLOCATING | VCMEM_ZERO);
    if (!tile_vaddr)
    {
        bug("[VC4Gallium] submit_cl: failed to alloc tile BO (%d bytes)\n", tile_bo_size);
        return -1;
    }

    /* Zero the tile state area — binning needs clean state each frame.
     * When pool_bo_get reuses an existing BO, old tile state must be cleared. */
    {
        ULONG *p = (ULONG *)tile_vaddr;
        ULONG n = tile_state_size / 4;
        ULONG i;
        for (i = 0; i < n; i++)
            p[i] = 0;
    }

    /* ---- Step 2: Get pooled exec BO for validated bin CL + shader recs + uniforms ---- */
    bin_cl_max = args->bin_cl_size + 256;
    /* Match the per-section ROUNDUP(16) below so the BO is large enough
     * to hold all three sections after alignment padding. */
    exec_size = ROUNDUP(bin_cl_max, 16) +
                ROUNDUP(args->shader_rec_size, 16) +
                args->uniforms_size +
                32; /* extra slack for trailing alignment */
    exec_size = ROUNDUP(exec_size, 4096);

    exec_vaddr = pool_bo_get(sd, &sd->pool[pi].exec, exec_size, 4096,
                              VCMEM_L1NONALLOCATING | VCMEM_ZERO);
    if (!exec_vaddr)
    {
        bug("[VC4Gallium] submit_cl: failed to alloc exec BO (%d bytes)\n", exec_size);
        return -1;
    }
    exec_bus = sd->pool[pi].exec.bus_addr;

    ULONG _t_after_pools = VC4G_NOW_US();

    /* Section offsets are referenced as ULONG arrays later (handles,
     * uniforms) — keep them 16-byte aligned so the loads/stores hit
     * naturally aligned addresses on Device memory. */
    bin_cl_offset = 0;
    shader_rec_offset = ROUNDUP(bin_cl_max, 16);
    uniforms_offset = ROUNDUP(shader_rec_offset + args->shader_rec_size, 16);

    /* ---- Step 3: Set up exec state and process bin CL ---- */
    exec.sd = sd;
    exec.bo_handles = bo_handles;
    exec.bo_handle_count = args->bo_handle_count;
    exec.current_bo[0] = 0;
    exec.current_bo[1] = 0;
    exec.tile_alloc_bus = sd->pool[pi].tile.bus_addr + tile_alloc_offset;
    exec.tile_state_bus = sd->pool[pi].tile.bus_addr;
    exec.tile_alloc_offset = tile_alloc_offset;
    exec.tile_alloc_size = tile_alloc_size;
    exec.bin_tiles_x = args->max_x_tile + 1;  /* Will be overwritten by bin config */
    exec.bin_tiles_y = args->max_y_tile + 1;
    exec.shader_state_count = 0;
    exec.shader_rec_bus = exec_bus + shader_rec_offset;
    exec.uniforms_bus = exec_bus + uniforms_offset;

    /* Grow the persistent GL_SHADER_STATE scratch to cover this CL. A CL
     * can hold at most bin_cl_size/5 shader-state packets (5 bytes each);
     * that bound scales with the scene, and a complex one overflows any
     * fixed guess. Serialized under render_lock, so no locking here. */
    {
        ULONG need = args->bin_cl_size / 5 + 1;

        if (sd->shader_state_scratch_max < need)
        {
            if (sd->shader_state_scratch)
                FreeVec(sd->shader_state_scratch);
            sd->shader_state_scratch = AllocVec(need * sizeof(ULONG), MEMF_ANY);
            sd->shader_state_scratch_max =
                sd->shader_state_scratch ? need : 0;
        }
        if (!sd->shader_state_scratch)
        {
            bug("[VC4Gallium] submit_cl: no memory for %lu shader-state slots\n",
                (unsigned long)need);
            return -1;
        }
        exec.shader_state_addrs = sd->shader_state_scratch;
        exec.shader_state_max = sd->shader_state_scratch_max;
    }

    bin_cl_out_size = process_bin_cl(&exec,
                                      (UBYTE *)(IPTR)args->bin_cl, args->bin_cl_size,
                                      (UBYTE *)exec_vaddr + bin_cl_offset, bin_cl_max);
    if (bin_cl_out_size == 0)
    {
        bug("[VC4Gallium] submit_cl: bin CL processing failed\n");
        return -1;
    }

    D(bug("[VC4Gallium] submit_cl: bin CL %d -> %d bytes, %d shader recs\n",
        args->bin_cl_size, bin_cl_out_size, exec.shader_state_count));

    ULONG _t_after_bin = VC4G_NOW_US();

    /* ---- Step 4: Relocate shader records + process uniforms ---- */
    {
        ULONG uniforms_out_count = 0;
        ret = relocate_shader_recs(&exec,
                                    (UBYTE *)(IPTR)args->shader_rec, args->shader_rec_size,
                                    (UBYTE *)exec_vaddr + shader_rec_offset,
                                    (ULONG *)(IPTR)args->uniforms,
                                    args->uniforms_size / 4,
                                    (ULONG *)((UBYTE *)exec_vaddr + uniforms_offset),
                                    args->uniforms_size / 4,
                                    &uniforms_out_count);
        if (ret != 0)
        {
            bug("[VC4Gallium] submit_cl: shader rec + uniform relocation failed\n");
            return -1;
        }
        D(bug("[VC4Gallium] submit_cl: relocated %d uniform words\n", uniforms_out_count));
    }

    ULONG _t_after_shader = VC4G_NOW_US();

    /* ---- Step 6: Build RCL ---- */
    /* Estimate RCL size: generous upper bound */
    rcl_max_size = 11 +     /* TILE_RENDERING_MODE_CONFIG */
                   14 + 3 + 7 +  /* CLEAR_COLORS + TILE_COORDS + STORE_GENERAL */
                   tiles_x * tiles_y * (
                       7 +  /* LOAD_TILE_BUFFER_GENERAL (color) */
                       3 + 7 +  /* TILE_COORDS + STORE (for Z/S load) */
                       7 +  /* LOAD_TILE_BUFFER_GENERAL (Z/S) */
                       3 +  /* TILE_COORDINATES */
                       1 +  /* WAIT_ON_SEMAPHORE */
                       5 +  /* BRANCH_TO_SUB_LIST */
                       5 +  /* STORE_FULL_RES (MSAA color) */
                       3 +  /* TILE_COORDINATES */
                       5 +  /* STORE_FULL_RES (MSAA zs) */
                       3 +  /* TILE_COORDINATES */
                       7 +  /* STORE_TILE_BUFFER_GENERAL (zs) */
                       3 +  /* TILE_COORDINATES */
                       1    /* STORE_MS_TILE_BUFFER */
                   ) + 64;  /* safety margin */

    rcl_vaddr = pool_bo_get(sd, &sd->pool[pi].rcl, ROUNDUP(rcl_max_size, 4096), 4096,
                             VCMEM_L1NONALLOCATING | VCMEM_ZERO);
    if (!rcl_vaddr)
    {
        bug("[VC4Gallium] submit_cl: failed to alloc RCL BO\n");
        return -1;
    }
    rcl_bus = sd->pool[pi].rcl.bus_addr;

    ret = build_rcl(&exec, args, (UBYTE *)rcl_vaddr, rcl_max_size, &rcl_size);
    if (ret != 0)
        return -1;

    D(bug("[VC4Gallium] submit_cl: RCL %d bytes, tile_alloc=0x%08x tile_state=0x%08x\n",
        rcl_size, exec.tile_alloc_bus, exec.tile_state_bus));

    /* Forensics snapshot for the timeout dump in v3d_wait_seqno. */
    vc4_last_submit.seqno     = v3d->seqno + 1;
    vc4_last_submit.rcl_bus   = rcl_bus;
    vc4_last_submit.rcl_size  = rcl_size;
    vc4_last_submit.bin_size  = args->bin_cl_size;
    vc4_last_submit.width     = args->width;
    vc4_last_submit.height    = args->height;
    vc4_last_submit.min_x     = args->min_x_tile;
    vc4_last_submit.min_y     = args->min_y_tile;
    vc4_last_submit.max_x     = args->max_x_tile;
    vc4_last_submit.max_y     = args->max_y_tile;
    vc4_last_submit.cw_hindex = args->color_write.hindex;
    vc4_last_submit.zsw_hindex = args->zs_write.hindex;
    vc4_last_submit.rcl_vaddr = (const volatile UBYTE *)rcl_vaddr;
    vc4_last_submit.tile_alloc_vaddr =
        (const volatile UBYTE *)tile_vaddr + tile_alloc_offset;

    ULONG _t_after_rcl = VC4G_NOW_US();

    /* ---- Step 7: Submit to V3D ---- */
    /*
     * V3D supports pipelined binning + rendering: Thread 0 (binner) and
     * Thread 1 (renderer) run concurrently. The RCL uses WAIT_ON_SEMAPHORE
     * on the first tile, and the bin CL ends with INCREMENT_SEMAPHORE +
     * FLUSH, so the hardware interlock is already in place. Submit both
     * threads before waiting — renderer will stall on semaphore until
     * each tile's bin data is ready.
     */

    /*
     * Make the CPU-written CL/shader/uniform/RCL bytes visible to V3D, which
     * fetches through the uncached 0xC0000000 alias. Every buffer here — the
     * exec/rcl/tile pools AND every referenced BO — comes from gpu_mem_alloc,
     * i.e. VideoCore GPU-pool memory that is mapped UNCACHED on the ARM. So
     * there are NO dirty ARM cache lines to clean: a dsb that drains the CPU
     * write buffer (the volatile byte stores from process_bin_cl /
     * relocate_shader_recs / build_rcl and the tile_state zeroing) to RAM is
     * all the GPU needs.
     *
     * The per-frame CacheClearE walks that used to live here (exec/rcl/tile +
     * every cpu_mapped BO) cleaned nothing yet DOMINATED submit time: the
     * arm-native c7,c14 loop (syscall.c:118) issues one op per cache line over
     * the whole BO size whether or not a line is present, measured at ~1.6 ms
     * on a 300x300 window. cpu_mapped BOs are gpu_mem_alloc'd too (MMAP_BO
     * hands back the identity vaddr), so they were as pointless to walk as the
     * GPU-only buffers the walk already skipped.
     */
    asm volatile("dsb sy" ::: "memory");

    {
        volatile UBYTE *rb = (volatile UBYTE *)rcl_vaddr;
        D(bug("[VC4Gallium] DIAG RCL[0..31]: %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x\n",
            rb[0],rb[1],rb[2],rb[3],rb[4],rb[5],rb[6],rb[7],
            rb[8],rb[9],rb[10],rb[11],rb[12],rb[13],rb[14],rb[15],
            rb[16],rb[17],rb[18],rb[19],rb[20],rb[21],rb[22],rb[23],
            rb[24],rb[25],rb[26],rb[27],rb[28],rb[29],rb[30],rb[31]));
    }

#if DEBUG
    /* DIAG: dump entire validated bin CL and RCL in 16-byte chunks
     * so we can verify opcode placement (0x07 INCREMENT_SEMAPHORE,
     * 0x08 WAIT_ON_SEMAPHORE, 0x11 BRANCH_TO_SUB_LIST etc). */
    {
        volatile UBYTE *bb = (volatile UBYTE *)((UBYTE *)exec_vaddr + bin_cl_offset);
        ULONG i;
        D(bug("[VC4Gallium] DIAG bin_cl full (size=%d):\n", bin_cl_out_size));
        for (i = 0; i < bin_cl_out_size; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < bin_cl_out_size; j++)
                p += vc4_hexbyte(line + p, bb[i + j]);
            D(bug("[VC4Gallium]   bin[%03d]: %s\n", i, line));
        }
    }
    {
        volatile UBYTE *rb = (volatile UBYTE *)rcl_vaddr;
        ULONG i;
        D(bug("[VC4Gallium] DIAG rcl full (size=%d):\n", rcl_size));
        for (i = 0; i < rcl_size; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < rcl_size; j++)
                p += vc4_hexbyte(line + p, rb[i + j]);
            D(bug("[VC4Gallium]   rcl[%03d]: %s\n", i, line));
        }
    }
#endif

    /* Binner overspill pool. The VC4 binner draws its tile-list blocks from
     * the overspill pool (BPOA/BPOS); leaving it 0 makes the binner OOM on
     * the very first block (PCS=BMOOM, CT0CA=0). We start each frame with
     * BPOA/BPOS = 0 and feed a dedicated BO on-demand from the OUTOMEM IRQ
     * (vc4_v3d_service_interrupts). It's a separate BO from tile_alloc —
     * pointing BPOA at tile_alloc would let the binner overwrite the sublists
     * it just emitted — and one per pool set, so frame N+1's binner can't
     * clobber frame N's lists while CT1 still reads them. overflow_handed = 0
     * is reset per frame for the (currently disabled) OUTOMEM handler;
     * OUTOMEM stays masked because asserting the V3D IRQ wedges the firmware
     * mailbox (see vc4_v3d_init). Note: ERRSTAT bit 12 (0x00001000) is
     * latched on every submission even when nothing is wrong — don't
     * treat it as a fault. */
    if (!pool_bo_get(sd, &sd->pool[pi].binoverflow, VC4_BIN_OVERFLOW_SIZE, 4096,
                     VCMEM_L1NONALLOCATING | VCMEM_ZERO))
    {
        bug("[VC4Gallium] submit_cl: failed to alloc binner overflow BO\n");
        return -1;
    }
    v3d->overflow_bus = sd->pool[pi].binoverflow.bus_addr;
    v3d->overflow_size = sd->pool[pi].binoverflow.size;
    v3d->overflow_handed = 0;
    V3D_WRITE(v3d, V3D_BPOA, 0);
    V3D_WRITE(v3d, V3D_BPOS, 0);

    /* Clear caches: flush both L2 and the per-slice caches before every bin
     * job — without the slice flush, V3D reads stale QPU instructions,
     * uniforms or texture data from the per-slice caches even after we've
     * patched RAM. L2CCLR auto-clears; we don't need to write L2CENA because
     * L2 is enabled by default after reset. */
    V3D_WRITE(v3d, V3D_L2CACTL, V3D_L2CACTL_L2CCLR);
    V3D_WRITE(v3d, V3D_SLCACTL, V3D_SLCACTL_ALL);

    /* Drain the pipeline state left by the PREVIOUS submission before
     * touching CT0 or pending_ct1*. Two submissions can arrive
     * back-to-back (Mesa flushes mid-frame on shader/state changes, no
     * wait in between): the previous bin may still be running, and its
     * FLDONE→CT1 handoff may not have been serviced yet. The old code
     * blindly W1C-cleared INTCTL here, eating that FLDONE — the previous
     * frame's render was then silently dropped and the seqno bookkeeping
     * ran one behind forever, hanging the client. Consume events properly
     * and let the handoff complete instead. */
    {
        ULONG spins = 0;
        while ((V3D_READ(v3d, V3D_CT0CS) & V3D_CTCS_CTRUN) ||
               v3d->pending_render)
        {
            vc4_v3d_service_interrupts(v3d);
            v3d_advance_counters(v3d);
            if (++spins > 100000000)
            {
                bug("[VC4Gallium] submit_cl: drain timeout (CT0CS=0x%08x "
                    "pending=%d) — proceeding\n",
                    V3D_READ(v3d, V3D_CT0CS), (LONG)v3d->pending_render);
                break;
            }
        }
    }

    /* Clear latched ERRSTAT bits (W1C) so the post-kick read reflects only
     * this submission. Without this, sticky bits (e.g. VPAERGL from a
     * previous CS allocation) make it impossible to tell when in the
     * binner/render timeline the error actually fired. */
    V3D_WRITE(v3d, V3D_ERRSTAT, 0xffffffff);

    /* DIAG: state immediately before kick */
    D(bug("[VC4Gallium] DIAG V3D pre-kick: CT0CS=0x%08x CT1CS=0x%08x PCS=0x%08x BFC=0x%02x RFC=0x%02x VPMBASE=0x%08x VPACNTL=0x%08x\n",
        V3D_READ(v3d, V3D_CT0CS), V3D_READ(v3d, V3D_CT1CS),
        V3D_READ(v3d, V3D_PCS),
        V3D_READ(v3d, V3D_BFC) & 0xff, V3D_READ(v3d, V3D_RFC) & 0xff,
        V3D_READ(v3d, V3D_VPMBASE), V3D_READ(v3d, V3D_VPACNTL)));

#if DEBUG
    /* DIAG: dump validated shader_rec section + uniform section. Layout
     * per v3d_packet_v21.xml "Shader Record": byte 0 = flags
     * (single-threaded / point-size / clipping), byte 3 = FS num_varyings,
     * bytes 4-11 = FS code+uniforms; byte 14 = VS attribute_array_select_bits,
     * byte 15 = VS total_attributes_size, bytes 16-23 = VS code+uniforms;
     * byte 26/27 = CS select_bits / total_size, bytes 28-35 = CS code+uniforms;
     * attribute records (8 bytes each) start at offset 36. */
    {
        volatile UBYTE *sb = (volatile UBYTE *)((UBYTE *)exec_vaddr + shader_rec_offset);
        ULONG i;
        D(bug("[VC4Gallium] DIAG shader_rec full (size=%d, at exec+0x%x -> bus 0x%08x):\n",
            args->shader_rec_size, shader_rec_offset, exec_bus + shader_rec_offset));
        for (i = 0; i < args->shader_rec_size; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < args->shader_rec_size; j++)
                p += vc4_hexbyte(line + p, sb[i + j]);
            D(bug("[VC4Gallium]   srec[%03d]: %s\n", i, line));
        }
    }
    {
        volatile UBYTE *ub = (volatile UBYTE *)((UBYTE *)exec_vaddr + uniforms_offset);
        ULONG i;
        D(bug("[VC4Gallium] DIAG uniforms full (size=%d):\n", args->uniforms_size));
        for (i = 0; i < args->uniforms_size; i += 16)
        {
            ULONG j;
            char line[16*3 + 8];
            ULONG p = 0;
            for (j = 0; j < 16 && (i + j) < args->uniforms_size; j++)
                p += vc4_hexbyte(line + p, ub[i + j]);
            D(bug("[VC4Gallium]   uni[%03d]: %s\n", i, line));
        }
    }

    /* DIAG: dump first 64 bytes of every non-shader, non-color-target BO
     * referenced by this submission. For typical draws that's the VBO(s)
     * — if Mesa never actually uploaded vertex data, this region will be
     * all zeros (or whatever the firmware initialized the BO to) and we
     * know the GPU is binning a degenerate primitive. Skip shader code
     * BOs (already validated) and the color target (just receives writes,
     * no useful pre-kick content). */
    {
        ULONG bi;
        for (bi = 0; bi < args->bo_handle_count; bi++)
        {
            ULONG bh = bo_handles[bi];
            struct vc4_bo_entry *be;

            if (!bh || bh >= VC4_MAX_BOS)
                continue;
            be = &sd->bo_table[bh];
            if (!be->vaddr || be->size == 0 || be->is_shader)
                continue;
            if (args->color_write.hindex != (ULONG)~0 &&
                bi == args->color_write.hindex)
                continue;
            if (args->zs_write.hindex != (ULONG)~0 &&
                bi == args->zs_write.hindex)
                continue;
            {
                volatile UBYTE *vb = (volatile UBYTE *)be->vaddr;
                ULONG dump_len = be->size < 64 ? be->size : 64;
                ULONG i;
                D(bug("[VC4Gallium] DIAG BO[hindex=%d handle=%d bus=0x%08x size=%d]:\n",
                    bi, bh, be->bus_addr, be->size));
                for (i = 0; i < dump_len; i += 16)
                {
                    ULONG j;
                    char line[16*3 + 8];
                    ULONG p = 0;
                    for (j = 0; j < 16 && (i + j) < dump_len; j++)
                        p += vc4_hexbyte(line + p, vb[i + j]);
                    D(bug("[VC4Gallium]   bo[%03d]: %s\n", i, line));
                }
            }
        }
    }
#endif

    /* Submit binning CL (Thread 0). The FLDONE IRQ handler will kick CT1
     * with the RCL addresses stashed below. Deferring the render kick to
     * the bin-done interrupt guarantees the binner's FLUSH has committed
     * per-tile sublist writes to memory before the renderer fetches them.
     *
     * Stash CT1 addresses BEFORE writing CT0EA — writing CT0EA is what
     * starts the binner, and FLDONE can fire as soon as a small bin CL
     * completes, racing with our stash if we did it afterward. */
    v3d->pending_ct1ca = rcl_bus;
    v3d->pending_ct1ea = rcl_bus + rcl_size;
    v3d->pending_render = TRUE;
    asm volatile("dsb sy" ::: "memory");

    V3D_WRITE(v3d, V3D_CT0CA, exec_bus + bin_cl_offset);
    V3D_WRITE(v3d, V3D_CT0EA, exec_bus + bin_cl_offset + bin_cl_out_size);

    vc4_v3d_service_interrupts(v3d);

    /* DIAG: did the V3D accept the new CL addresses? */
    D(bug("[VC4Gallium] DIAG V3D post-kick: CT0CS=0x%08x CT0CA=0x%08x CT0EA=0x%08x CT1CS=0x%08x CT1CA=0x%08x CT1EA=0x%08x PCS=0x%08x ERRSTAT=0x%08x\n",
        V3D_READ(v3d, V3D_CT0CS), V3D_READ(v3d, V3D_CT0CA), V3D_READ(v3d, V3D_CT0EA),
        V3D_READ(v3d, V3D_CT1CS), V3D_READ(v3d, V3D_CT1CA), V3D_READ(v3d, V3D_CT1EA),
        V3D_READ(v3d, V3D_PCS), V3D_READ(v3d, V3D_ERRSTAT)));

    /* Assign seqno and return immediately — don't wait for GPU.
     * Mesa will call WAIT_SEQNO or WAIT_BO when it actually needs
     * the result. The next submit_cl will only block if it needs
     * to reuse this pool set (which won't happen until 2 frames later
     * due to double-buffering). */
    seqno = ++v3d->seqno;
    args->seqno = seqno;

    /* Tag this pool set with the seqno so we know when it's safe to reuse */
    sd->pool[pi].seqno = seqno;

    /* Tag every referenced BO with it too: WAIT_BO answers from this, and
     * Mesa's BO cache probes each candidate with a zero-timeout WAIT_BO
     * before reusing it. Answering from the newest job instead made every
     * cached BO look busy, so the cache never hit and every frame
     * allocated fresh buffers until the firmware heap ran dry. */
    {
        ULONG bi;
        for (bi = 0; bi < args->bo_handle_count; bi++)
        {
            ULONG bh = bo_handles[bi];

            if (bh && bh < VC4_MAX_BOS && sd->bo_table[bh].refcount)
                sd->bo_table[bh].seqno = seqno;
        }
    }

    sd->pool_idx = (pi + 1) % VC4_NUM_POOL_SETS;

    D(bug("[VC4Gallium] submit_cl: submitted seqno=%d on pool %d\n", seqno, pi));

    {

        ULONG _t_done = VC4G_NOW_US();
        VC4G_PROFF("[VC4Prof] submit_cl: pools=%ld bin=%ld shader=%ld rcl=%ld kick=%ld total=%ld "
                  "bin_in=%ld bin_out=%ld rec=%ld unif=%ld tiles=%ldx%ld\n",
            _t_after_pools - _t_entry,
            _t_after_bin - _t_after_pools,
            _t_after_shader - _t_after_bin,
            _t_after_rcl - _t_after_shader,
            _t_done - _t_after_rcl,
            _t_done - _t_entry,
            args->bin_cl_size, bin_cl_out_size,
            args->shader_rec_size, args->uniforms_size,
            tiles_x, tiles_y);
    }

    return 0;
}

/* Public submit entry. Serialize the whole submission against the wait /
 * display-blit paths (render_lock) so the V3D registers, the job seqno, the
 * pool ping-pong and the pending FLDONE->CT1 handoff are never touched
 * concurrently. do_submit_cl's internal pool-reuse wait re-enters
 * v3d_wait_seqno, which obtains render_lock again — safe, it nests. */
static int ioctl_submit_cl(struct vc4galliumstaticdata *sd, struct drm_vc4_submit_cl *args)
{
    int ret;

    ObtainSemaphore(&sd->render_lock);
    ret = do_submit_cl(sd, args);
    ReleaseSemaphore(&sd->render_lock);

    return ret;
}

static int ioctl_set_tiling(struct vc4galliumstaticdata *sd, struct drm_vc4_set_tiling *args)
{
    if (args->handle == 0 || args->handle >= VC4_MAX_BOS ||
        sd->bo_table[args->handle].refcount == 0)
    {
        errno = ENOENT;
        return -1;
    }
    sd->bo_table[args->handle].tiling_modifier = args->modifier;
    return 0;
}

static int ioctl_get_tiling(struct vc4galliumstaticdata *sd, struct drm_vc4_get_tiling *args)
{
    if (args->handle == 0 || args->handle >= VC4_MAX_BOS ||
        sd->bo_table[args->handle].refcount == 0)
    {
        errno = ENOENT;
        return -1;
    }
    args->modifier = sd->bo_table[args->handle].tiling_modifier;
    return 0;
}

static int ioctl_label_bo(struct vc4galliumstaticdata *sd, struct drm_vc4_label_bo *args)
{
    return 0;
}

static int ioctl_gem_madvise(struct vc4galliumstaticdata *sd, struct drm_vc4_gem_madvise *args)
{
    args->retained = 1;
    return 0;
}

/* ---- Main ioctl dispatcher ---- */

static int vc4_aros_ioctl_dispatch(struct vc4galliumstaticdata *sd,
                                   unsigned long cmd, void *arg)
{
    switch (cmd)
    {
    case DRM_VC4_GET_PARAM:
        return ioctl_get_param(sd, (struct drm_vc4_get_param *)arg);

    case DRM_VC4_CREATE_BO:
        return ioctl_create_bo(sd, (struct drm_vc4_create_bo *)arg);

    case DRM_VC4_CREATE_SHADER_BO:
        return ioctl_create_shader_bo(sd, (struct drm_vc4_create_shader_bo *)arg);

    case DRM_VC4_MMAP_BO:
        return ioctl_mmap_bo(sd, (struct drm_vc4_mmap_bo *)arg);

    case DRM_VC4_WAIT_SEQNO:
        return ioctl_wait_seqno(sd, (struct drm_vc4_wait_seqno *)arg);

    case DRM_VC4_WAIT_BO:
        return ioctl_wait_bo(sd, (struct drm_vc4_wait_bo *)arg);

    case DRM_VC4_SUBMIT_CL:
        return ioctl_submit_cl(sd, (struct drm_vc4_submit_cl *)arg);

    case DRM_VC4_SET_TILING:
        return ioctl_set_tiling(sd, (struct drm_vc4_set_tiling *)arg);

    case DRM_VC4_GET_TILING:
        return ioctl_get_tiling(sd, (struct drm_vc4_get_tiling *)arg);

    case DRM_VC4_LABEL_BO:
        return ioctl_label_bo(sd, (struct drm_vc4_label_bo *)arg);

    case DRM_VC4_GEM_MADVISE:
        return ioctl_gem_madvise(sd, (struct drm_vc4_gem_madvise *)arg);

    case DRM_GEM_CLOSE:
        return ioctl_gem_close(sd, (struct drm_gem_close *)arg);

    case DRM_GEM_OPEN:
        return ioctl_gem_open(sd, (struct drm_gem_open *)arg);

    default:
        bug("[VC4Gallium] unhandled ioctl 0x%lx\n", cmd);
        return -1;
    }
}

#if VC4G_PROFILE
/*
 * Classify the observed whole-CPU slowdown once per profile period:
 * pure ALU (core clock), cached-memory writes (TLB/cache health),
 * uncached BO writes (VC-region/write-buffer health), plus the
 * firmware's ARM clock and throttle flags. ~1.5 ms per 120 frames.
 */
static void vc4_prof_bench(struct vc4galliumstaticdata *sd)
{
    static UBYTE cached_buf[65536];
    static APTR nc_buf;
    static ULONG nc_handle;
    volatile ULONG acc = 0;
    ULONG t0, t1, us_alu, us_cached, us_nc = 0;
    ULONG arm_hz = 0, arm_meas_hz = 0, throttled = 0;
    ULONG sctlr = 0, actlr = 0;
    ULONG i;

    t0 = VC4G_NOW_US();
    for (i = 0; i < 100000; i++)
        acc += i ^ (acc << 1);
    t1 = VC4G_NOW_US();
    us_alu = t1 - t0;

    t0 = t1;
    for (i = 0; i < sizeof(cached_buf); i += 4)
        *(volatile ULONG *)&cached_buf[i] = i;
    t1 = VC4G_NOW_US();
    us_cached = t1 - t0;

    if (!nc_buf)
        nc_buf = gpu_mem_alloc(sd, 65536, 4096,
                               VCMEM_L1NONALLOCATING, &nc_handle);
    if (nc_buf)
    {
        t0 = VC4G_NOW_US();
        for (i = 0; i < 65536; i += 4)
            *(volatile ULONG *)((UBYTE *)nc_buf + i) = i;
        asm volatile("dsb sy" ::: "memory");
        t1 = VC4G_NOW_US();
        us_nc = t1 - t0;
    }

    /* Cache/branch-predictor/MMU control bits — a mid-run change here
     * (I-bit 12, Z-bit 11, C-bit 2) would explain a global slowdown. */
    {
        APTR ss = SuperState();
        asm volatile("mrc p15, 0, %0, c1, c0, 0" : "=r"(sctlr));
        asm volatile("mrc p15, 0, %0, c1, c0, 1" : "=r"(actlr));
        UserState(ss);
    }

    ObtainSemaphore(&sd->mbox_lock);
    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(VCTAG_GETCLKRATE);
    sd->mbox_msg[3] = AROS_LE2LONG(8);
    sd->mbox_msg[4] = AROS_LE2LONG(4);
    sd->mbox_msg[5] = AROS_LE2LONG(3);      /* clock id 3 = ARM */
    sd->mbox_msg[6] = 0;
    sd->mbox_msg[7] = 0;
    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
        != (volatile unsigned int *)-1)
        arm_hz = AROS_LE2LONG(sd->mbox_msg[6]);

    /* Measured (actual PLL) rate — 0x00030047. The plain GETCLKRATE above
     * reports the *configured* rate and can miss a real divider change.
     * Old firmware may not know the tag; 0 then. */
    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(0x00030047);
    sd->mbox_msg[3] = AROS_LE2LONG(8);
    sd->mbox_msg[4] = AROS_LE2LONG(4);
    sd->mbox_msg[5] = AROS_LE2LONG(3);      /* clock id 3 = ARM */
    sd->mbox_msg[6] = 0;
    sd->mbox_msg[7] = 0;
    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
        != (volatile unsigned int *)-1)
        arm_meas_hz = AROS_LE2LONG(sd->mbox_msg[6]);

    sd->mbox_msg[0] = AROS_LE2LONG(8 * 4);
    sd->mbox_msg[1] = AROS_LE2LONG(VCTAG_REQ);
    sd->mbox_msg[2] = AROS_LE2LONG(0x00030046);  /* GET_THROTTLED */
    sd->mbox_msg[3] = AROS_LE2LONG(4);
    sd->mbox_msg[4] = AROS_LE2LONG(4);
    sd->mbox_msg[5] = 0;
    sd->mbox_msg[6] = 0;
    sd->mbox_msg[7] = 0;
    if (MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (APTR)sd->mbox_msg)
        != (volatile unsigned int *)-1)
        throttled = AROS_LE2LONG(sd->mbox_msg[5]);
    ReleaseSemaphore(&sd->mbox_lock);

    bug("[VC4Prof] bench: alu=%lu us cached=%lu us nc=%lu us "
        "arm=%lu/%lu Hz throttled=0x%lx sctlr=0x%08lx actlr=0x%08lx\n",
        us_alu, us_cached, us_nc, arm_hz, arm_meas_hz, throttled,
        sctlr, actlr);
}
#endif

#if VC4G_PROFILE
/* Frame-budget accumulators: mesa_gap = time between ioctls (Mesa + app
 * CPU), ioctl = time inside the hidd. Reset every 120 submits. */
static ULONG fp_t0, fp_last_exit, fp_mesa, fp_ioctl;
static ULONG fp_prev_submit, fp_period, fp_frames;
#endif

int vc4_aros_ioctl(int fd, unsigned long request, void *arg)
{
    struct vc4galliumstaticdata *sd = fd_to_sd(fd);
    int ret;
    if (!sd)
        return -1;

    D(bug("[VC4Gallium] ioctl cmd=0x%02lx\n", request));

#if VC4G_PROFILE
    fp_t0 = VC4G_NOW_US();
    if (fp_last_exit)
        fp_mesa += fp_t0 - fp_last_exit;
#endif

    ret = vc4_aros_ioctl_dispatch(sd, request, arg);

#if VC4G_PROFILE
    {
        ULONG fp_t1 = VC4G_NOW_US();
        fp_ioctl += fp_t1 - fp_t0;
        fp_last_exit = fp_t1;
        if (request == DRM_VC4_SUBMIT_CL)
        {
            if (fp_prev_submit)
                fp_period += fp_t1 - fp_prev_submit;
            fp_prev_submit = fp_t1;
            if (++fp_frames >= 120)
            {
                VC4G_PROF("[VC4Prof] %lu frames: frame=%lu us (%lu fps) "
                          "mesa_gap=%lu us/f ioctl=%lu us/f\n",
                    fp_frames,
                    fp_period / fp_frames,
                    fp_period ? 1000000UL * fp_frames / fp_period : 0,
                    fp_mesa / fp_frames,
                    fp_ioctl / fp_frames);
                fp_mesa = 0; fp_ioctl = 0; fp_period = 0; fp_frames = 0;

                vc4_prof_bench(sd);
            }
        }
    }
#endif

    return ret;
}

/*
 * mmap replacement, called after MMAP_BO. RPi memory is directly
 * accessible, so just return the vaddr.
 */
void *vc4_aros_mmap(int fd, ULONG handle)
{
    struct vc4galliumstaticdata *sd = fd_to_sd(fd);

    if (!sd || handle == 0 || handle >= VC4_MAX_BOS)
        return NULL;

    D(bug("[VC4Gallium] mmap: handle=%d -> vaddr=0x%08x size=%d\n",
        handle, (ULONG)sd->bo_table[handle].vaddr, sd->bo_table[handle].size));

    return sd->bo_table[handle].vaddr;
}

/*
 * Free every outstanding BO and pool-set BO back to the firmware.
 * Called from DestroyPipeScreen (end of each GL session, so leftovers
 * can't poison the next one) and from Expunge so we don't leak
 * GPU-locked SDRAM until reboot if the module unloads while Mesa
 * still holds resources.
 */
void vc4_aros_release_all_bos(struct vc4galliumstaticdata *sd)
{
    ULONG i;

    D({
        ULONG freed_n = 0, freed_bytes = 0, live_n = 0;
        for (i = 1; i < VC4_MAX_BOS; i++)
            if (sd->bo_table[i].refcount > 0)
            {
                live_n++;
                freed_bytes += sd->bo_table[i].size;
                if (sd->bo_table[i].gpu_handle)
                    freed_n++;
            }
        bug("[VC4Gallium] release_all_bos: %u live BOs (%u gpu, %u KB "
            "firmware)\n", live_n, freed_n, freed_bytes / 1024);
    })

    /* Remove a live overlay plane first — it scans one of the BOs
     * about to be freed. */
    {
        extern void vc4_aros_clear_overlay(struct vc4galliumstaticdata *,
                                           OOP_Object *);
        vc4_aros_clear_overlay(sd, NULL);
    }

    /* Drain any async display-blit DMA so we don't free its source BO
     * while the channel is still reading it. */
    vc4_aros_dma_wait_idle(sd);
    sd->dma_pinned_handle = 0;

    /* Clear external (scanout-wrap) entries too — they carry no
     * gpu_handle but would otherwise hold their slots forever. */
    for (i = 1; i < VC4_MAX_BOS; i++)
    {
        if (sd->bo_table[i].refcount > 0)
        {
            if (sd->bo_table[i].gpu_handle)
                gpu_mem_free(sd, sd->bo_table[i].gpu_handle,
                             sd->bo_table[i].size);
            sd->bo_table[i].vaddr = NULL;
            sd->bo_table[i].bus_addr = 0;
            sd->bo_table[i].gpu_handle = 0;
            sd->bo_table[i].size = 0;
            sd->bo_table[i].refcount = 0;
            sd->bo_table[i].seqno = 0;
            sd->bo_table[i].tiling_modifier = 0;
            sd->bo_table[i].is_shader = FALSE;
            sd->bo_table[i].external = FALSE;
            sd->bo_table[i].cpu_mapped = FALSE;
        }
    }

    for (i = 0; i < VC4_NUM_POOL_SETS; i++)
    {
        struct vc4_frame_bo *pools[] = {
            &sd->pool[i].tile,
            &sd->pool[i].exec,
            &sd->pool[i].rcl,
            &sd->pool[i].binoverflow
        };
        int p;
        for (p = 0; p < 4; p++)
        {
            if (pools[p]->vaddr && pools[p]->gpu_handle)
            {
                gpu_mem_free(sd, pools[p]->gpu_handle, pools[p]->size);
                pools[p]->vaddr = NULL;
                pools[p]->bus_addr = 0;
                pools[p]->gpu_handle = 0;
                pools[p]->size = 0;
            }
        }
        sd->pool[i].seqno = 0;
    }

    if (sd->shader_state_scratch)
    {
        FreeVec(sd->shader_state_scratch);
        sd->shader_state_scratch = NULL;
        sd->shader_state_scratch_max = 0;
    }
}
