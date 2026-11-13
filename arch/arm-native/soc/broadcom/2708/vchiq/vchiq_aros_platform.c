/**
 * Copyright (c) 2010-2012 Broadcom. All rights reserved.
 * Copyright (c) 2012-2015 Oleksandr Tymoshenko <gonzo@freebsd.org>
 * Copyright (C) 2026, The AROS Development Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the above-listed copyright holders may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * ALTERNATIVELY, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2, as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The BCM2835/BCM2711 platform layer: shared memory, doorbells and bulk
 * page lists. Based on FreeBSD's vchiq_2835_arm.c and vchiq_kmod.c.
 *
 * The slots, the fragments and the page lists all live in one arena that
 * is remapped Normal Non-Cacheable, so only the client buffers of bulk
 * transfers need cache maintenance.
 */

#include <aros/debug.h>
#include <aros/kernel.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>

#include "interface/vchiq_arm/vchiq_arm.h"
#include "interface/vchiq_arm/vchiq_2835.h"
#include "interface/vchiq_arm/vchiq_connected.h"
#include "vchiq_aros.h"

#include <proto/kernel.h>
#include <proto/mbox.h>

APTR KernelBase __attribute__((used)) = NULL;
APTR MBoxBase __attribute__((used)) = NULL;

static IPTR vchiq_periiobase;
#define ARM_PERIIOBASE vchiq_periiobase
#include <hardware/bcm2708.h>
#include <hardware/videocore.h>

#define VCHIQ_BELL0             (ARM_PERIIOBASE + 0x00b840)
#define VCHIQ_BELL2             (ARM_PERIIOBASE + 0x00b848)

/* ARM doorbell 0; on the BCM2711 GIC it is SPI 34, which is INTID 66 too */
#define VCHIQ_DOORBELL_IRQ      IRQ_ARM_DOORBELL_0

#define VCTAG_VCHIQ_INIT        0x00048010

/* What the firmware assumes on a 64-bit ARM; fragments are two lines */
#define VCHIQ_CACHE_LINE        64
#define VCHIQ_FRAGMENT_SIZE     (2 * VCHIQ_CACHE_LINE)

#define TOTAL_SLOTS             (VCHIQ_SLOT_ZERO_SLOTS + 2 * 32)
#define MAX_FRAGMENTS           (VCHIQ_NUM_CURRENT_BULKS * 2)
#define PAGELIST_POOL_SIZE      (256 * 1024)

/* The VideoCore reaches only the first 1GB through the 0xC0000000 alias */
#define VC_BUS_ADDR(pa)         (0xC0000000 | (ULONG)(IPTR)(pa))
#define VC_BUS_LIMIT            0x40000000ULL

#define PAGE_MASK_AROS          (PAGE_SIZE - 1)
#define ALIGN_UP(x, a)          (((IPTR)(x) + (a) - 1) & ~(IPTR)((a) - 1))

typedef struct vchiq_2835_state_struct {
    int inited;
    VCHIQ_ARM_STATE_T arm_state;
} VCHIQ_2835_ARM_STATE_T;

typedef struct bulkinfo_struct {
    PAGELIST_T *pagelist;
    ULONG pagelist_size;
    void *buf;
    size_t size;
} BULKINFO_T;

extern VCHIQ_STATE_T g_state;

static char *g_slot_mem;
static IPTR g_slot_phys;
static int g_slot_mem_size;

/* BCM2711 bulks name pages by physical address >> 4, so they are not
 * limited to the 1GB alias */
static int g_long_bulk_space;

static char *g_fragments_base;
static char *g_free_fragments;
static struct semaphore g_free_fragments_sema;
static struct SignalSemaphore g_free_fragments_mutex;

static struct MemHeader g_pagelist_mh;
static struct SignalSemaphore g_pagelist_lock;

static struct Task *g_irq_task;

void udelay(unsigned int usec)
{
    ULONG start = AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO);

    while ((ULONG)(AROS_LE2LONG(*(volatile ULONG *)SYSTIMER_CLO) - start) < usec)
        ;
}

/*
 * The doorbell only wakes the IRQ task: the remote events end in up(),
 * which sleeps on a semaphore and must not run in interrupt context.
 */
static void vchiq_doorbell_irq(void *data1, void *data2)
{
    ULONG status = AROS_LE2LONG(*(volatile ULONG *)VCHIQ_BELL0);

    if ((status & 0x4) && g_irq_task)
        Signal(g_irq_task, SIGBREAKF_CTRL_F);
}

static void vchiq_irq_task(void)
{
    for (;;)
    {
        Wait(SIGBREAKF_CTRL_F);
        remote_event_pollall(&g_state);
    }
}

void remote_event_signal(REMOTE_EVENT_T *event)
{
    wmb();
    event->fired = 1;
    dsb(sy);

    if (event->armed)
    {
        /* trigger vc interrupt */
        dsb(sy);
        *(volatile ULONG *)VCHIQ_BELL2 = 0;
    }
}

static int vchiq_mbox_init(ULONG channelbase)
{
    UBYTE raw[64 + 63];
    volatile ULONG *msg = (volatile ULONG *)ALIGN_UP(raw, 64);
    volatile unsigned int *reply;

    msg[0] = AROS_LONG2LE(7 * sizeof(ULONG));
    msg[1] = AROS_LONG2LE(VCTAG_REQ);
    msg[2] = AROS_LONG2LE(VCTAG_VCHIQ_INIT);
    msg[3] = AROS_LONG2LE(4);
    msg[4] = AROS_LONG2LE(0);
    msg[5] = AROS_LONG2LE(channelbase);
    msg[6] = 0;

    reply = MBoxCall((void *)VCMB_BASE, VCMB_PROPCHAN, (void *)msg);
    if (reply != (volatile unsigned int *)msg)
        return -EIO;

    /* The firmware zeroes the value when it has taken the slots */
    if ((AROS_LE2LONG(msg[1]) != VCTAG_RESP) || (AROS_LE2LONG(msg[5]) != 0))
    {
        bug("[VCHIQ] VCHIQ_INIT refused (code 0x%08x, value 0x%08x)\n",
            AROS_LE2LONG(msg[1]), AROS_LE2LONG(msg[5]));
        return -EIO;
    }

    return 0;
}

static APTR vchiq_alloc_arena(ULONG size)
{
    APTR raw;
    IPTR base;

    raw = AllocMem(size + PAGE_SIZE, MEMF_ANY);
    if (!raw)
        return NULL;

    base = ALIGN_UP(raw, PAGE_SIZE);
    g_slot_phys = (IPTR)KrnVirtualToPhysical((APTR)base);

    /* Clean first, or a dirty line lands on top of shared data later */
    CacheClearE((APTR)base, size, CACRF_ClearD);

    if (((UQUAD)g_slot_phys + size > VC_BUS_LIMIT)
        || !KrnMapGlobal((APTR)base, (APTR)base, size,
                         MAP_Readable | MAP_Writable | MAP_WriteThrough))
    {
        bug("[VCHIQ] no arena at 0x%p: above 1GB or not mappable Normal-NC\n",
            (void *)base);
        FreeMem(raw, size + PAGE_SIZE);
        return NULL;
    }

    memset((APTR)base, 0, size);

    return (APTR)base;
}

int vchiq_platform_init(VCHIQ_STATE_T *state)
{
    VCHIQ_SLOT_ZERO_T *vchiq_slot_zero;
    int frag_mem_size;
    char *arena;
    int i;

    g_slot_mem_size = PAGE_ALIGN(TOTAL_SLOTS * VCHIQ_SLOT_SIZE);
    frag_mem_size = PAGE_ALIGN(VCHIQ_FRAGMENT_SIZE * MAX_FRAGMENTS);

    arena = vchiq_alloc_arena(g_slot_mem_size + frag_mem_size
                              + PAGELIST_POOL_SIZE);
    if (!arena)
        return -ENOMEM;
    g_slot_mem = arena;

    vchiq_slot_zero = vchiq_init_slots(g_slot_mem, g_slot_mem_size);
    if (!vchiq_slot_zero)
        return -EINVAL;

    vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_OFFSET_IDX] =
        VC_BUS_ADDR(g_slot_phys + g_slot_mem_size);
    vchiq_slot_zero->platform_data[VCHIQ_PLATFORM_FRAGMENTS_COUNT_IDX] =
        MAX_FRAGMENTS;

    g_fragments_base = g_slot_mem + g_slot_mem_size;

    g_free_fragments = g_fragments_base;
    for (i = 0; i < (MAX_FRAGMENTS - 1); i++)
    {
        *(char **)&g_fragments_base[i * VCHIQ_FRAGMENT_SIZE] =
            &g_fragments_base[(i + 1) * VCHIQ_FRAGMENT_SIZE];
    }
    *(char **)&g_fragments_base[i * VCHIQ_FRAGMENT_SIZE] = NULL;
    _sema_init(&g_free_fragments_sema, MAX_FRAGMENTS);
    InitSemaphore(&g_free_fragments_mutex);

    /* Page lists come from a private heap behind the fragments */
    g_pagelist_mh.mh_Node.ln_Type = NT_MEMORY;
    g_pagelist_mh.mh_Node.ln_Name = "vchiq pagelists";
    g_pagelist_mh.mh_Attributes = MEMF_ANY;
    g_pagelist_mh.mh_Lower = g_fragments_base + frag_mem_size;
    g_pagelist_mh.mh_Upper = (char *)g_pagelist_mh.mh_Lower + PAGELIST_POOL_SIZE;
    g_pagelist_mh.mh_First = (struct MemChunk *)g_pagelist_mh.mh_Lower;
    g_pagelist_mh.mh_First->mc_Next = NULL;
    g_pagelist_mh.mh_First->mc_Bytes = PAGELIST_POOL_SIZE;
    g_pagelist_mh.mh_Free = PAGELIST_POOL_SIZE;
    InitSemaphore(&g_pagelist_lock);

    if (vchiq_init_state(state, vchiq_slot_zero, 0/*slave*/) != VCHIQ_SUCCESS)
        return -EINVAL;

    g_irq_task = NewCreateTask(TASKTAG_PC,        vchiq_irq_task,
                               TASKTAG_NAME,      "VCHIQ doorbell",
                               TASKTAG_STACKSIZE, VCHIQ_THREAD_STACK,
                               TASKTAG_PRI,       VCHIQ_THREAD_PRI + 10,
                               TAG_DONE);
    if (!g_irq_task)
        return -ENOMEM;

    if (!KrnAddIRQHandler(VCHIQ_DOORBELL_IRQ, vchiq_doorbell_irq, NULL, NULL))
        return -EIO;

    if (vchiq_mbox_init(VC_BUS_ADDR(g_slot_phys)) != 0)
        return -EIO;

    vchiq_log_info(vchiq_arm_log_level,
        "vchiq_init - done (slots %p, phys %p)",
        vchiq_slot_zero, (void *)g_slot_phys);

    vchiq_call_connected_callbacks();

    return 0;
}

/* Find the SoC and the services the platform layer needs */
int vchiq_aros_platform_init(void)
{
    KernelBase = OpenResource("kernel.resource");
    MBoxBase = OpenResource("mbox.resource");
    if (!KernelBase || !MBoxBase)
        return -ENODEV;

    vchiq_periiobase = KrnGetSystemAttr(KATTR_PeripheralBase);
    if (vchiq_periiobase == 0)
        return -ENODEV;

    /* The Pi 5 VideoCore has no media blocks behind VCHIQ */
    if (vchiq_periiobase == BCM2712_PERIIOBASE)
        return -ENODEV;

    g_long_bulk_space = (vchiq_periiobase == BCM2711_PERIIOBASE);

    return 0;
}

VCHIQ_STATUS_T vchiq_platform_init_state(VCHIQ_STATE_T *state)
{
    VCHIQ_STATUS_T status = VCHIQ_SUCCESS;
    VCHIQ_2835_ARM_STATE_T *platform;

    platform = kzalloc(sizeof(VCHIQ_2835_ARM_STATE_T), GFP_KERNEL);
    if (!platform)
        return VCHIQ_ERROR;
    state->platform_state = (VCHIQ_PLATFORM_STATE_T)platform;

    platform->inited = 1;
    status = vchiq_arm_init_state(state, &platform->arm_state);
    if (status != VCHIQ_SUCCESS)
        platform->inited = 0;

    return status;
}

VCHIQ_ARM_STATE_T *vchiq_platform_get_arm_state(VCHIQ_STATE_T *state)
{
    VCHIQ_2835_ARM_STATE_T *platform =
        (VCHIQ_2835_ARM_STATE_T *)state->platform_state;

    if (!platform->inited)
        BUG();

    return &platform->arm_state;
}

static ULONG vc_bulk_page(IPTR pa)
{
    return g_long_bulk_space ? (ULONG)(pa >> 4) : VC_BUS_ADDR(pa);
}

static int create_pagelist(char *buf, size_t count, unsigned short type,
                           BULKINFO_T *bi)
{
    PAGELIST_T *pagelist;
    uint32_t *addrs;
    unsigned int num_pages, i;
    IPTR offset;
    ULONG pagelist_size;
    ULONG run_ceil = g_long_bulk_space ? 0x100 : PAGE_SIZE;
    ULONG page_step = g_long_bulk_space ? (PAGE_SIZE >> 4) : PAGE_SIZE;
    ULONG base_addr, next_addr, addr;
    int run, addridx;

    offset = (IPTR)buf & PAGE_MASK_AROS;
    num_pages = (count + offset + PAGE_SIZE - 1) / PAGE_SIZE;

    bi->pagelist = NULL;
    bi->buf = buf;
    bi->size = count;

    pagelist_size = sizeof(PAGELIST_T) + num_pages * sizeof(uint32_t);

    ObtainSemaphore(&g_pagelist_lock);
    pagelist = Allocate(&g_pagelist_mh, pagelist_size);
    ReleaseSemaphore(&g_pagelist_lock);
    if (!pagelist)
    {
        vchiq_log_error(vchiq_core_log_level,
            "Unable to allocate pagelist memory");
        return -ENOMEM;
    }
    bi->pagelist_size = pagelist_size;

    pagelist->length = count;
    pagelist->type = type;
    pagelist->offset = offset;

    /* Group the pages into runs of contiguous pages */
    addrs = pagelist->addrs;
    addridx = 0;
    run = 0;
    base_addr = vc_bulk_page((IPTR)KrnVirtualToPhysical(buf - offset));
    next_addr = base_addr + page_step;
    for (i = 1; i < num_pages; i++)
    {
        addr = vc_bulk_page((IPTR)KrnVirtualToPhysical(buf - offset
                                                       + i * PAGE_SIZE));
        if ((addr == next_addr) && (run < run_ceil - 1))
        {
            next_addr += page_step;
            run++;
        }
        else
        {
            addrs[addridx++] = (base_addr & ~(run_ceil - 1)) + run;
            base_addr = addr;
            next_addr = addr + page_step;
            run = 0;
        }
    }
    addrs[addridx++] = (base_addr & ~(run_ceil - 1)) + run;

    /* Partial cache lines (fragments) require special measures */
    if ((type == PAGELIST_READ) &&
        ((pagelist->offset & (VCHIQ_CACHE_LINE - 1)) ||
         ((pagelist->offset + pagelist->length) & (VCHIQ_CACHE_LINE - 1))))
    {
        char *fragments;

        down(&g_free_fragments_sema);

        ObtainSemaphore(&g_free_fragments_mutex);
        fragments = g_free_fragments;
        WARN_ON(fragments == NULL);
        g_free_fragments = *(char **)g_free_fragments;
        ReleaseSemaphore(&g_free_fragments_mutex);

        pagelist->type = PAGELIST_READ_WITH_FRAGMENTS
            + (fragments - g_fragments_base) / VCHIQ_FRAGMENT_SIZE;
    }

    /* Clean (and for a read, drop) the CPU's copy of the client buffer */
    CacheClearE(buf, count, CACRF_ClearD);
    dsb(sy);

    bi->pagelist = pagelist;

    return 0;
}

static void free_pagelist(BULKINFO_T *bi, int actual)
{
    PAGELIST_T *pagelist = bi->pagelist;
    char *buf = bi->buf;

    vchiq_log_trace(vchiq_arm_log_level,
        "free_pagelist - %p, %d (%lu bytes @%p)",
        pagelist, (int)actual, (unsigned long)pagelist->length, buf);

    /*
     * The CPU may have pulled lines of the buffer back in while the
     * VideoCore wrote it, so drop them once more. Only whole lines: the
     * partial ones arrive through the fragments.
     */
    if ((pagelist->type >= PAGELIST_READ) && (actual > 0))
    {
        IPTR start = ALIGN_UP(buf, VCHIQ_CACHE_LINE);
        IPTR end = ((IPTR)buf + actual) & ~(IPTR)(VCHIQ_CACHE_LINE - 1);

        if (end > start)
            CacheClearE((APTR)start, end - start, CACRF_InvalidateD);
    }

    /* Deal with any partial cache lines (fragments) */
    if (pagelist->type >= PAGELIST_READ_WITH_FRAGMENTS)
    {
        char *fragments = g_fragments_base +
            (pagelist->type - PAGELIST_READ_WITH_FRAGMENTS) * VCHIQ_FRAGMENT_SIZE;
        int head_bytes, tail_bytes;

        head_bytes = (VCHIQ_CACHE_LINE - pagelist->offset) &
            (VCHIQ_CACHE_LINE - 1);
        tail_bytes = (pagelist->offset + actual) &
            (VCHIQ_CACHE_LINE - 1);

        if ((actual >= 0) && (head_bytes != 0))
        {
            if (head_bytes > actual)
                head_bytes = actual;

            memcpy(buf, fragments, head_bytes);
        }

        if ((actual >= 0) && (head_bytes < actual) && (tail_bytes != 0))
            memcpy(buf + actual - tail_bytes, fragments + VCHIQ_CACHE_LINE,
                   tail_bytes);

        ObtainSemaphore(&g_free_fragments_mutex);
        *(char **)fragments = g_free_fragments;
        g_free_fragments = fragments;
        ReleaseSemaphore(&g_free_fragments_mutex);
        up(&g_free_fragments_sema);
    }

    dsb(sy);

    ObtainSemaphore(&g_pagelist_lock);
    Deallocate(&g_pagelist_mh, pagelist, bi->pagelist_size);
    ReleaseSemaphore(&g_pagelist_lock);

    kfree(bi);
}

VCHIQ_STATUS_T vchiq_prepare_bulk_data(VCHIQ_BULK_T *bulk,
    VCHI_MEM_HANDLE_T memhandle, void *offset, int size, int dir)
{
    BULKINFO_T *bi;
    int ret;

    WARN_ON(memhandle != VCHI_MEM_HANDLE_INVALID);
    bi = kzalloc(sizeof(*bi), GFP_KERNEL);
    if (!bi)
        return VCHIQ_ERROR;

    ret = create_pagelist((char *)offset, size,
                          (dir == VCHIQ_BULK_RECEIVE)
                          ? PAGELIST_READ
                          : PAGELIST_WRITE,
                          bi);
    if (ret != 0)
    {
        kfree(bi);
        return VCHIQ_ERROR;
    }

    bulk->handle = memhandle;
    bulk->data = (void *)(IPTR)VC_BUS_ADDR(KrnVirtualToPhysical(bi->pagelist));

    /* Store the pagelist address in remote_data, which isn't used by the
       slave. */
    bulk->remote_data = bi;

    return VCHIQ_SUCCESS;
}

void vchiq_complete_bulk(VCHIQ_BULK_T *bulk)
{
    if (bulk && bulk->remote_data && bulk->actual)
        free_pagelist((BULKINFO_T *)bulk->remote_data, bulk->actual);
}

void vchiq_transfer_bulk(VCHIQ_BULK_T *bulk)
{
    /*
     * This should only be called on the master (VideoCore) side, but
     * provide an implementation to avoid the need for ifdefery.
     */
    BUG();
}

void vchiq_dump_platform_state(void *dump_context)
{
    char buf[80];
    int len;

    len = snprintf(buf, sizeof(buf), "  Platform: 2835 (VC master)");
    vchiq_dump(dump_context, buf, len + 1);
}

VCHIQ_STATUS_T vchiq_platform_suspend(VCHIQ_STATE_T *state)
{
    return VCHIQ_ERROR;
}

VCHIQ_STATUS_T vchiq_platform_resume(VCHIQ_STATE_T *state)
{
    return VCHIQ_SUCCESS;
}

void vchiq_platform_paused(VCHIQ_STATE_T *state)
{
}

void vchiq_platform_resumed(VCHIQ_STATE_T *state)
{
}

int vchiq_platform_videocore_wanted(VCHIQ_STATE_T *state)
{
    return 1; // autosuspend not supported - videocore always wanted
}

int vchiq_platform_use_suspend_timer(void)
{
    return 0;
}

void vchiq_dump_platform_use_state(VCHIQ_STATE_T *state)
{
    vchiq_log_info(vchiq_arm_log_level, "Suspend timer not in use");
}

void vchiq_platform_handle_timeout(VCHIQ_STATE_T *state)
{
    (void)state;
}
