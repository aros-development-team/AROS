/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#include <proto/exec.h>
#include <hidd/pci.h>

#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/newstyle.h>
#include <string.h>

#include "virtio_debug.h"
#include "virtio_intern.h"
#include "virtio_queue.h"

#define VIRTIO_PAGE_SIZE        4096
#define VIRTIO_PAGE_ALIGN(x)    (((x) + (VIRTIO_PAGE_SIZE - 1)) & ~(VIRTIO_PAGE_SIZE - 1))

static ULONG vq_alloc_size(int size, ULONG *avail_off, ULONG *used_off)
{
    ULONG desc_bytes  = sizeof(struct virtq_desc) * size;
    ULONG avail_bytes = sizeof(UWORD) * (3 + size);
    ULONG used_bytes  = sizeof(UWORD) * 3 + sizeof(struct virtq_used_elem) * size;
    ULONG total;

    *avail_off = VIRTIO_PAGE_ALIGN(desc_bytes);
    *used_off  = VIRTIO_PAGE_ALIGN(*avail_off + avail_bytes);
    total      = VIRTIO_PAGE_ALIGN(*used_off + used_bytes);
    return total;
}

struct virtio_queue *virtio_alloc_queue(device_t dev, int qid, int size)
{
    struct virtio_queue *vq;
    struct VirtIOBase *VirtIOBase = dev ? dev->dev_VirtIOBase : NULL;
    ULONG bytes, avail_off, used_off;
    UWORD i;

    if (!dev || !dev->dev_PCIDriverObject || !VirtIOBase)
        return NULL;
    if (size <= 0 || (size & (size - 1)) != 0)
        return NULL;

    vq = AllocMem(sizeof(*vq), MEMF_CLEAR);
    if (!vq)
        return NULL;

    vq->dev    = dev;
    vq->q_Index = qid;
    vq->q_Size = size;
    InitSemaphore(&vq->q_Lock);
    NEWLIST(&vq->q_Pending);

    bytes = vq_alloc_size(size, &avail_off, &used_off);
    vq->q_RingBytes = bytes;

    vq->q_RingBase = HIDD_PCIDriver_AllocPCIMem(dev->dev_PCIDriverObject, bytes);
    if (!vq->q_RingBase) {
        FreeMem(vq, sizeof(*vq));
        return NULL;
    }
    memset(vq->q_RingBase, 0, bytes);

    vq->q_Desc  = (volatile struct virtq_desc  *) vq->q_RingBase;
    vq->q_Avail = (volatile struct virtq_avail *)((UBYTE *)vq->q_RingBase + avail_off);
    vq->q_Used  = (volatile struct virtq_used  *)((UBYTE *)vq->q_RingBase + used_off);

    {
        UQUAD base = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, vq->q_RingBase);
        vq->q_DescPhys  = base;
        vq->q_AvailPhys = base + avail_off;
        vq->q_UsedPhys  = base + used_off;
    }

    vq->q_Reqs = AllocMem(sizeof(struct virtio_request) * size, MEMF_CLEAR);
    if (!vq->q_Reqs) {
        HIDD_PCIDriver_FreePCIMem(dev->dev_PCIDriverObject, vq->q_RingBase);
        FreeMem(vq, sizeof(*vq));
        return NULL;
    }

    for (i = 0; i < size; i++) {
        vq->q_Desc[i].next  = (i + 1 == size) ? 0 : (i + 1);
        vq->q_Desc[i].flags = (i + 1 == size) ? 0 : VIRTQ_DESC_F_NEXT;
    }
    vq->q_FreeHead    = 0;
    vq->q_NumFree     = size;
    vq->q_LastUsedIdx = 0;

    vq->q_Avail->flags = 0;
    vq->q_Avail->idx   = 0;
    vq->q_Used->flags  = 0;
    vq->q_Used->idx    = 0;

    return vq;
}

void virtio_free_queue(struct virtio_queue *vq)
{
    struct VirtIOBase *VirtIOBase;

    if (!vq) return;
    VirtIOBase = (vq->dev) ? vq->dev->dev_VirtIOBase : NULL;
    if (vq->q_RingBase && vq->dev && vq->dev->dev_PCIDriverObject && VirtIOBase) {
        HIDD_PCIDriver_FreePCIMem(vq->dev->dev_PCIDriverObject, vq->q_RingBase);
        vq->q_RingBase = NULL;
    }
    if (vq->q_Reqs) {
        FreeMem(vq->q_Reqs, sizeof(struct virtio_request) * vq->q_Size);
        vq->q_Reqs = NULL;
    }
    FreeMem(vq, sizeof(*vq));
}

/* Caller holds vq->q_Lock. Allocates head/middle/tail descriptor indices. */
static int vq_alloc_chain(struct virtio_queue *vq, UWORD count, UWORD *idxs)
{
    UWORD i, cur, prev = 0;
    if (vq->q_NumFree < count) return -1;

    cur = vq->q_FreeHead;
    for (i = 0; i < count; i++) {
        idxs[i] = cur;
        prev = cur;
        cur = vq->q_Desc[cur].next;
    }
    vq->q_FreeHead = cur;
    vq->q_NumFree -= count;

    /* Detach our chain (clear NEXT on tail) */
    vq->q_Desc[prev].flags &= ~VIRTQ_DESC_F_NEXT;
    vq->q_Desc[prev].next   = 0;
    return 0;
}

/* Caller holds vq->q_Lock. */
static void vq_free_chain(struct virtio_queue *vq, UWORD head)
{
    UWORD idx = head;
    UWORD count = 0;
    UWORD last;

    for (;;) {
        count++;
        if (!(vq->q_Desc[idx].flags & VIRTQ_DESC_F_NEXT)) break;
        idx = vq->q_Desc[idx].next;
    }
    last = idx;

    vq->q_Desc[last].flags = VIRTQ_DESC_F_NEXT;
    vq->q_Desc[last].next  = vq->q_FreeHead;
    vq->q_FreeHead = head;
    vq->q_NumFree += count;
}

static int virtio_submit_now(struct virtio_queue *vq, struct virtio_request *templ)
{
    device_t dev = vq->dev;
    struct VirtIOBase *VirtIOBase = dev ? dev->dev_VirtIOBase : NULL;
    struct virtio_request *slot;
    UWORD descs_needed = (templ->vr_DataLength > 0) ? 3 : 2;
    UWORD idxs[3];
    UQUAD hdr_phys, status_phys, data_phys = 0;
    UWORD avail_idx;
    ULONG hdr_len = sizeof(slot->vr_Header), data_len = templ->vr_DataLength;
    ULONG status_len = 1;

    if (!dev || !dev->dev_PCIDriverObject || !VirtIOBase) return -1;

    ObtainSemaphore(&vq->q_Lock);
    if (vq_alloc_chain(vq, descs_needed, idxs) != 0) {
        ReleaseSemaphore(&vq->q_Lock);
        return -1;
    }

    slot = &vq->q_Reqs[idxs[0]];
    *slot = *templ;
    slot->vr_HeadDesc  = idxs[0];
    slot->vr_DescCount = descs_needed;
    slot->vr_InUse     = 1;
    slot->vr_Status    = 0xff;

    /* Translate buffers to PCI-DMA addresses. */
    {
        APTR p = CachePreDMA(&slot->vr_Header, &hdr_len, DMA_ReadFromRAM);
        hdr_phys = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, p);
    }
    if (hdr_len != sizeof(slot->vr_Header))
        goto undo_header;
    if (templ->vr_DataLength > 0) {
        ULONG f = templ->vr_IsWrite ? DMA_ReadFromRAM : 0;
        APTR p = CachePreDMA(templ->vr_DataAddr, &data_len, f);
        data_phys = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, p);
        if (data_len != templ->vr_DataLength)
            goto undo_data;
    }
    {
        APTR p = CachePreDMA(&slot->vr_Status, &status_len, 0);
        status_phys = (UQUAD)(IPTR)HIDD_PCIDriver_CPUtoPCI(dev->dev_PCIDriverObject, p);
    }

    if (status_len != 1) {
        CachePostDMA(&slot->vr_Status, &status_len, DMA_NoModify);
        goto undo_data;
    }

    /* Build descriptors. */
    vq->q_Desc[idxs[0]].addr  = hdr_phys;
    vq->q_Desc[idxs[0]].len   = sizeof(slot->vr_Header);
    vq->q_Desc[idxs[0]].flags = VIRTQ_DESC_F_NEXT;
    vq->q_Desc[idxs[0]].next  = idxs[1];

    if (templ->vr_DataLength > 0) {
        UWORD data_flags = VIRTQ_DESC_F_NEXT;
        if (!templ->vr_IsWrite) data_flags |= VIRTQ_DESC_F_WRITE; /* device writes => guest reads */
        vq->q_Desc[idxs[1]].addr  = data_phys;
        vq->q_Desc[idxs[1]].len   = templ->vr_DataLength;
        vq->q_Desc[idxs[1]].flags = data_flags;
        vq->q_Desc[idxs[1]].next  = idxs[2];

        vq->q_Desc[idxs[2]].addr  = status_phys;
        vq->q_Desc[idxs[2]].len   = 1;
        vq->q_Desc[idxs[2]].flags = VIRTQ_DESC_F_WRITE;
        vq->q_Desc[idxs[2]].next  = 0;
    } else {
        vq->q_Desc[idxs[1]].addr  = status_phys;
        vq->q_Desc[idxs[1]].len   = 1;
        vq->q_Desc[idxs[1]].flags = VIRTQ_DESC_F_WRITE;
        vq->q_Desc[idxs[1]].next  = 0;
    }

    /* Place head index in available ring. */
    avail_idx = vq->q_Avail->idx;
    vq->q_Avail->ring[avail_idx % vq->q_Size] = idxs[0];
    /* Memory barrier ensuring descriptors are visible before idx update */
    __sync_synchronize();
    vq->q_Avail->idx = avail_idx + 1;
    __sync_synchronize();

    ReleaseSemaphore(&vq->q_Lock);

    /* Notify the device. */
    if (vq->q_Notify) {
        *vq->q_Notify = vq->q_Index;
    }

    return idxs[0];

undo_data:
    if (templ->vr_DataLength)
        CachePostDMA(templ->vr_DataAddr, &data_len,
                     DMA_NoModify | (templ->vr_IsWrite ? DMA_ReadFromRAM : 0));
undo_header:
    CachePostDMA(&slot->vr_Header, &hdr_len, DMA_ReadFromRAM | DMA_NoModify);
    /* Allocation detached the tail, but has not yet rebuilt the chain. */
    vq_free_chain(vq, idxs[0]);
    slot->vr_InUse = 0;
    ReleaseSemaphore(&vq->q_Lock);
    return -1;
}

int virtio_submit_blk_request(struct virtio_queue *vq, struct virtio_request *templ)
{
    int result;
    UWORD needed = templ->vr_DataLength ? 3 : 2;

    ObtainSemaphore(&vq->q_Lock);
    if (!IsListEmpty(&vq->q_Pending) || vq->q_NumFree < needed) {
        struct IOStdReq *io = IOStdReq(templ->vr_IORequest);
        /* Retain the input offset high word until descriptors are available.
         * The caller cannot inspect/reuse a request before its reply. */
        io->io_Actual = templ->vr_Header.sector >> 23;
        AddTail(&vq->q_Pending, &io->io_Message.mn_Node);
        result = 0;
    } else {
        result = virtio_submit_now(vq, templ);
    }
    ReleaseSemaphore(&vq->q_Lock);
    return result;
}

ULONG virtio_process_used(struct virtio_queue *vq)
{
    ULONG handled = 0;
    UWORD used_idx;

    ObtainSemaphore(&vq->q_Lock);
    used_idx = vq->q_Used->idx;
    __sync_synchronize();

    while (vq->q_LastUsedIdx != used_idx) {
        UWORD slot_idx = vq->q_LastUsedIdx % vq->q_Size;
        UWORD head     = vq->q_Used->ring[slot_idx].id;
        ULONG ulen     = vq->q_Used->ring[slot_idx].len;
        struct virtio_request *req;

        if (head < vq->q_Size) {
            req = &vq->q_Reqs[head];
            if (req->vr_InUse) {
                struct IORequest *io = req->vr_IORequest;

                ULONG len = sizeof(req->vr_Header);
                CachePostDMA(&req->vr_Header, &len, DMA_ReadFromRAM);
                if (req->vr_DataLength) {
                    len = req->vr_DataLength;
                    CachePostDMA(req->vr_DataAddr, &len,
                                 req->vr_IsWrite ? DMA_ReadFromRAM : 0);
                }
                len = 1;
                CachePostDMA(&req->vr_Status, &len, 0);

                /* Translate virtio status into AROS error. */
                if (io) {
                    UBYTE st = req->vr_Status;
                    if (st == VIRTIO_BLK_S_OK) {
                        io->io_Error = 0;
                        IOStdReq(io)->io_Actual = req->vr_DataLength;
                    } else if (st == VIRTIO_BLK_S_UNSUPP) {
                        io->io_Error = IOERR_NOCMD;
                    } else {
                        io->io_Error = IOERR_ABORTED;
                    }
                }

                req->vr_InUse = 0;
                vq_free_chain(vq, head);

                /* Defer the ReplyMsg out of interrupt context.
                 * The IO task signals owner. We do it inline as we run on
                 * the IO task in our model. */
                if (io) {
                    ReplyMsg(&io->io_Message);
                }
                handled++;
            }
        }

        vq->q_LastUsedIdx++;
        (void)ulen;
    }
    while (!IsListEmpty(&vq->q_Pending)) {
        struct IOStdReq *io = (struct IOStdReq *)vq->q_Pending.lh_Head;
        struct virtio_request templ;
        BOOL flush = io->io_Command == CMD_UPDATE;
        if (vq->q_NumFree < (flush ? 2 : 3))
            break;
        Remove(&io->io_Message.mn_Node);
        memset(&templ, 0, sizeof(templ));
        templ.vr_IORequest = (struct IORequest *)io;
        if (flush) {
            templ.vr_Header.type = VIRTIO_BLK_T_FLUSH;
        } else {
            templ.vr_DataAddr = io->io_Data;
            templ.vr_DataLength = io->io_Length;
            templ.vr_IsWrite = io->io_Command != CMD_READ &&
                io->io_Command != TD_READ64 && io->io_Command != NSCMD_TD_READ64;
            templ.vr_Header.type = templ.vr_IsWrite ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
            templ.vr_Header.sector = ((UQUAD)io->io_Actual << 23) | (io->io_Offset >> 9);
        }
        io->io_Actual = 0;
        if (virtio_submit_now(vq, &templ) < 0) {
            io->io_Error = IOERR_ABORTED;
            ReplyMsg(&io->io_Message);
        }
    }
    ReleaseSemaphore(&vq->q_Lock);

    return handled;
}
