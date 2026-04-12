/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA.
*/

/*
 * Emulex LPe32002 Lancer-G7 SLI-4 Hardware Operations
 *
 * Implements the low-level hardware interface for the Lancer-G7 chipset,
 * including firmware communication via the bootstrap mailbox, queue
 * creation/destruction, and DMA ring management.
 */

#include <exec/types.h>
#include <exec/io.h>
#include <exec/errors.h>

#include <proto/exec.h>
#include <proto/oop.h>

#include <hidd/pci.h>

#include <hardware/intbits.h>

#include <string.h>

#include "lpe32002.h"
#include "lpe32002_hw.h"
#include "unit.h"
#include LC_LIBDEFS_FILE

/* Redirect LIBBASE through the unit's back-pointer to device base */
#undef LIBBASE
#define LIBBASE (unit->lpeu_device)

/* ============================================================
 * Firmware Readiness
 * ============================================================ */

/*
 * Wait for the Lancer-G7 firmware to signal readiness.
 * Polls the SLIPORT_STATUS register until the RDY bit is set or
 * the ERR bit indicates a failure.
 */
int lpe_fw_wait_ready(struct LPe32002Unit *unit)
{
    ULONG status;
    int timeout = 30000; /* ~30 seconds at 1ms poll intervals */

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    while (timeout > 0) {
        status = LPE_REG_READ32(unit->lpeu_bar0, SLI4_SLIPORT_STATUS);

        if (status & SLI4_STATUS_ERR) {
            ULONG err1 = LPE_REG_READ32(unit->lpeu_bar0, SLI4_SLIPORT_ERROR1);
            ULONG err2 = LPE_REG_READ32(unit->lpeu_bar0, SLI4_SLIPORT_ERROR2);
            D(bug("[%s] %s: Firmware error! status=%08lx err1=%08lx err2=%08lx\n",
                unit->lpeu_name, __func__, status, err1, err2));
            return -1;
        }

        if (status & SLI4_STATUS_RDY) {
            D(bug("[%s] %s: Firmware ready (status=%08lx)\n",
                unit->lpeu_name, __func__, status));
            return 0;
        }

        /* Delay ~1ms using timer */
        {
            struct timerequest *tr = &unit->lpeu_DelayReq;
            tr->tr_time.tv_secs = 0;
            tr->tr_time.tv_micro = 1000;
            tr->tr_node.io_Command = TR_ADDREQUEST;
            DoIO((struct IORequest *)tr);
        }
        timeout--;
    }

    D(bug("[%s] %s: Firmware not ready (timeout)\n", unit->lpeu_name, __func__));
    return -1;
}

/* ============================================================
 * Bootstrap Mailbox
 * ============================================================ */

/*
 * Issue a command through the bootstrap mailbox.
 * The bootstrap mailbox is used for initial communication with firmware
 * before the full mailbox queue is established.
 *
 * Protocol:
 * 1. Write low 32 bits of DMA address with BMBX_HI=0
 * 2. Write high 32 bits of DMA address with BMBX_HI=1
 * 3. Poll for BMBX_RDY
 * 4. Read response from DMA buffer
 */
int lpe_bmbx_command(struct LPe32002Unit *unit, struct sli4_mbox_cmd *cmd)
{
    IPTR dma_addr;
    ULONG reg;
    int timeout;

    D(bug("[%s] %s: opcode=0x%04lx\n", unit->lpeu_name, __func__,
        (ULONG)cmd->hdr.opcode));

    /* Copy command into DMA-able bootstrap mailbox buffer */
    CopyMem(cmd, unit->lpeu_bmbx, sizeof(struct sli4_mbox_cmd));

    dma_addr = (IPTR)unit->lpeu_bmbx_dma;

    /* Write low address (BMBX_HI=0) */
    reg = ((ULONG)(dma_addr & 0xFFFFFFFC)) | 0;  /* bit 1 = 0 for low */
    LPE_REG_WRITE32(unit->lpeu_bar1, SLI4_BOOTSTRAP_MBOX_DB, reg);

    /* Write high address (BMBX_HI=1) */
    reg = ((ULONG)((dma_addr >> 32) & 0xFFFFFFFC)) | SLI4_BMBX_HI;
    LPE_REG_WRITE32(unit->lpeu_bar1, SLI4_BOOTSTRAP_MBOX_DB, reg);

    /* Poll for completion */
    timeout = 5000;
    while (timeout > 0) {
        reg = LPE_REG_READ32(unit->lpeu_bar1, SLI4_BOOTSTRAP_MBOX_DB);
        if (reg & SLI4_BMBX_RDY) {
            /* Copy response back */
            CopyMem(unit->lpeu_bmbx, cmd, sizeof(struct sli4_mbox_cmd));

            if (cmd->hdr.status != SLI4_MBOX_STATUS_SUCCESS) {
                D(bug("[%s] %s: Mailbox command failed, status=0x%02lx additional=0x%02lx\n",
                    unit->lpeu_name, __func__,
                    (ULONG)cmd->hdr.status, (ULONG)cmd->hdr.additional_status));
                return -1;
            }

            D(bug("[%s] %s: Command completed successfully\n",
                unit->lpeu_name, __func__));
            return 0;
        }

        {
            struct timerequest *tr = &unit->lpeu_DelayReq;
            tr->tr_time.tv_secs = 0;
            tr->tr_time.tv_micro = 100;
            tr->tr_node.io_Command = TR_ADDREQUEST;
            DoIO((struct IORequest *)tr);
        }
        timeout--;
    }

    D(bug("[%s] %s: Bootstrap mailbox timeout!\n", unit->lpeu_name, __func__));
    return -1;
}

/* ============================================================
 * Firmware Information Commands
 * ============================================================ */

/*
 * Read firmware revision and SLI parameters.
 */
int lpe_hw_read_rev(struct LPe32002Unit *unit)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_READ_REV;
    cmd.hdr.subsystem = 0x01;  /* Common subsystem */
    cmd.hdr.request_length = 24;

    if (lpe_bmbx_command(unit, &cmd) != 0)
        return -1;

    unit->lpeu_sli_params.sli_rev = cmd.payload[0] & 0xFF;
    unit->lpeu_sli_params.sli_family = (cmd.payload[0] >> 8) & 0xFF;
    unit->lpeu_sli_params.fw_rev[0] = cmd.payload[2];
    unit->lpeu_sli_params.fw_rev[1] = cmd.payload[3];

    D(bug("[%s] %s: SLI rev=%ld family=%ld FW rev=%08lx.%08lx\n",
        unit->lpeu_name, __func__,
        unit->lpeu_sli_params.sli_rev,
        unit->lpeu_sli_params.sli_family,
        unit->lpeu_sli_params.fw_rev[0],
        unit->lpeu_sli_params.fw_rev[1]));

    return 0;
}

/*
 * Read port configuration including queue limits and WWN.
 */
int lpe_hw_read_config(struct LPe32002Unit *unit)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_READ_CONFIG;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 0;

    if (lpe_bmbx_command(unit, &cmd) != 0)
        return -1;

    unit->lpeu_sli_params.max_xri = cmd.payload[0] & 0xFFFF;
    unit->lpeu_sli_params.max_rpi = (cmd.payload[0] >> 16) & 0xFFFF;
    unit->lpeu_sli_params.max_vpi = cmd.payload[1] & 0xFFFF;
    unit->lpeu_sli_params.max_vfi = (cmd.payload[1] >> 16) & 0xFFFF;
    unit->lpeu_sli_params.max_fcfi = cmd.payload[2] & 0xFFFF;
    unit->lpeu_sli_params.max_eq = cmd.payload[3] & 0xFFFF;
    unit->lpeu_sli_params.max_cq = (cmd.payload[3] >> 16) & 0xFFFF;
    unit->lpeu_sli_params.max_wq = cmd.payload[4] & 0xFFFF;
    unit->lpeu_sli_params.max_rq = (cmd.payload[4] >> 16) & 0xFFFF;

    D(bug("[%s] %s: max_xri=%ld max_rpi=%ld max_eq=%ld max_cq=%ld max_wq=%ld max_rq=%ld\n",
        unit->lpeu_name, __func__,
        unit->lpeu_sli_params.max_xri,
        unit->lpeu_sli_params.max_rpi,
        unit->lpeu_sli_params.max_eq,
        unit->lpeu_sli_params.max_cq,
        unit->lpeu_sli_params.max_wq,
        unit->lpeu_sli_params.max_rq));

    return 0;
}

/*
 * Read Service Parameters including WWNN and WWPN.
 */
int lpe_hw_read_sparm(struct LPe32002Unit *unit)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_READ_SPARM;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 16;

    if (lpe_bmbx_command(unit, &cmd) != 0)
        return -1;

    /* Extract WWPN and WWNN from response payload */
    CopyMem(&cmd.payload[4], unit->lpeu_sli_params.wwnn, 8);
    CopyMem(&cmd.payload[6], unit->lpeu_sli_params.wwpn, 8);

    D(bug("[%s] %s: WWNN=%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
        unit->lpeu_name, __func__,
        unit->lpeu_sli_params.wwnn[0], unit->lpeu_sli_params.wwnn[1],
        unit->lpeu_sli_params.wwnn[2], unit->lpeu_sli_params.wwnn[3],
        unit->lpeu_sli_params.wwnn[4], unit->lpeu_sli_params.wwnn[5],
        unit->lpeu_sli_params.wwnn[6], unit->lpeu_sli_params.wwnn[7]));

    D(bug("[%s] %s: WWPN=%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
        unit->lpeu_name, __func__,
        unit->lpeu_sli_params.wwpn[0], unit->lpeu_sli_params.wwpn[1],
        unit->lpeu_sli_params.wwpn[2], unit->lpeu_sli_params.wwpn[3],
        unit->lpeu_sli_params.wwpn[4], unit->lpeu_sli_params.wwpn[5],
        unit->lpeu_sli_params.wwpn[6], unit->lpeu_sli_params.wwpn[7]));

    return 0;
}

/*
 * Request SLI-4 features from firmware.
 */
int lpe_hw_request_features(struct LPe32002Unit *unit)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_REQUEST_FEATURES;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 8;

    /* Request features: performance hints, SLI-4 params */
    cmd.payload[0] = 0x00000003;

    if (lpe_bmbx_command(unit, &cmd) != 0)
        return -1;

    unit->lpeu_sli_params.feature_flags = cmd.payload[0];

    D(bug("[%s] %s: features=0x%08lx\n",
        unit->lpeu_name, __func__, unit->lpeu_sli_params.feature_flags));

    return 0;
}

/*
 * Read link status to determine current FC link speed.
 */
int lpe_hw_read_link_stat(struct LPe32002Unit *unit)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_READ_LINK_STAT;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 0;

    if (lpe_bmbx_command(unit, &cmd) != 0)
        return -1;

    /* Extract link speed from response */
    ULONG link_info = cmd.payload[0];
    switch (link_info & 0xFF) {
        case 0x00: unit->lpeu_sli_params.link_speed = 0; break;
        case 0x01: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_1G; break;
        case 0x02: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_2G; break;
        case 0x04: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_4G; break;
        case 0x08: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_8G; break;
        case 0x10: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_16G; break;
        case 0x20: unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_32G; break;
        default:   unit->lpeu_sli_params.link_speed = FC_LINK_SPEED_32G; break;
    }

    unit->lpeu_sli_params.topology = (link_info >> 8) & 0xFF;

    D(bug("[%s] %s: link speed=%ld Mbps topology=0x%02lx\n",
        unit->lpeu_name, __func__,
        unit->lpeu_sli_params.link_speed,
        (ULONG)unit->lpeu_sli_params.topology));

    return 0;
}

/* ============================================================
 * Queue Allocation Helpers
 * ============================================================ */

static int lpe_alloc_queue_mem(struct LPe32002Unit *unit, struct sli4_queue *q,
                               UWORD type, UWORD depth, UWORD entry_size)
{
    q->type = type;
    q->size = depth;
    q->entry_size = entry_size;
    q->total_size = depth * entry_size;
    q->index = 0;
    q->num_proc = 0;
    q->id = 0;

    q->base = AllocMem(q->total_size, MEMF_PUBLIC | MEMF_CLEAR);
    if (!q->base) {
        D(bug("[%s] %s: Failed to alloc queue memory (%ld bytes)\n",
            unit->lpeu_name, __func__, q->total_size));
        return -1;
    }

    q->base_dma = HIDD_PCIDriver_CPUtoPCI(unit->lpeu_PCIDriver, q->base);

    D(bug("[%s] %s: queue type=%ld depth=%ld entry_size=%ld @ %p (DMA %p)\n",
        unit->lpeu_name, __func__, (ULONG)type, (ULONG)depth,
        (ULONG)entry_size, q->base, q->base_dma));

    return 0;
}

static void lpe_free_queue_mem(struct sli4_queue *q)
{
    if (q->base) {
        FreeMem(q->base, q->total_size);
        q->base = NULL;
    }
}

/* ============================================================
 * Queue Creation via Mailbox Commands
 * ============================================================ */

int lpe_queue_create_eq(struct LPe32002Unit *unit, struct sli4_queue *eq)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    if (lpe_alloc_queue_mem(unit, eq, SLI4_QTYPE_EQ, LPE_EQ_DEPTH, SLI4_EQ_ENTRY_SIZE) != 0)
        return -1;

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_CREATE_EQ;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 56;

    /* EQ create payload: page count, valid, delay multiplier, count */
    cmd.payload[0] = 1;                        /* number of pages */
    cmd.payload[1] = 1 | (5 << 13) | (7 << 29); /* valid, delay=5us, count=256 */
    cmd.payload[4] = (ULONG)(IPTR)eq->base_dma; /* page address low */
    cmd.payload[5] = (ULONG)((IPTR)eq->base_dma >> 32); /* page address high */

    if (lpe_bmbx_command(unit, &cmd) != 0) {
        lpe_free_queue_mem(eq);
        return -1;
    }

    eq->id = cmd.payload[0] & 0xFFFF;
    D(bug("[%s] %s: Created EQ id=%ld\n", unit->lpeu_name, __func__, (ULONG)eq->id));
    return 0;
}

int lpe_queue_create_cq(struct LPe32002Unit *unit, struct sli4_queue *cq, struct sli4_queue *eq)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s(eq_id=%ld)\n", unit->lpeu_name, __func__, (ULONG)eq->id));

    if (lpe_alloc_queue_mem(unit, cq, SLI4_QTYPE_CQ, LPE_CQ_DEPTH, SLI4_CQ_ENTRY_SIZE) != 0)
        return -1;

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_CREATE_CQ;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 56;

    cmd.payload[0] = 1;                        /* number of pages */
    cmd.payload[1] = 1 | (0 << 12) | ((ULONG)eq->id << 22); /* valid, CQ count=256, assoc EQ */
    cmd.payload[4] = (ULONG)(IPTR)cq->base_dma;
    cmd.payload[5] = (ULONG)((IPTR)cq->base_dma >> 32);

    if (lpe_bmbx_command(unit, &cmd) != 0) {
        lpe_free_queue_mem(cq);
        return -1;
    }

    cq->id = cmd.payload[0] & 0xFFFF;
    D(bug("[%s] %s: Created CQ id=%ld\n", unit->lpeu_name, __func__, (ULONG)cq->id));
    return 0;
}

int lpe_queue_create_wq(struct LPe32002Unit *unit, struct sli4_queue *wq, struct sli4_queue *cq)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s(cq_id=%ld)\n", unit->lpeu_name, __func__, (ULONG)cq->id));

    if (lpe_alloc_queue_mem(unit, wq, SLI4_QTYPE_WQ, LPE_WQ_DEPTH, SLI4_WQ_ENTRY_SIZE) != 0)
        return -1;

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_CREATE_WQ;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 56;

    cmd.payload[0] = 1;                        /* page count */
    cmd.payload[1] = (ULONG)cq->id;           /* associated CQ ID */
    cmd.payload[4] = (ULONG)(IPTR)wq->base_dma;
    cmd.payload[5] = (ULONG)((IPTR)wq->base_dma >> 32);

    if (lpe_bmbx_command(unit, &cmd) != 0) {
        lpe_free_queue_mem(wq);
        return -1;
    }

    wq->id = cmd.payload[0] & 0xFFFF;
    D(bug("[%s] %s: Created WQ id=%ld\n", unit->lpeu_name, __func__, (ULONG)wq->id));
    return 0;
}

int lpe_queue_create_rq(struct LPe32002Unit *unit,
                         struct sli4_queue *rq_hdr, struct sli4_queue *rq_data,
                         struct sli4_queue *cq)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s(cq_id=%ld)\n", unit->lpeu_name, __func__, (ULONG)cq->id));

    /* Allocate header RQ */
    if (lpe_alloc_queue_mem(unit, rq_hdr, SLI4_QTYPE_RQ, LPE_RQ_DEPTH, SLI4_RQ_ENTRY_SIZE) != 0)
        return -1;

    /* Allocate data RQ */
    if (lpe_alloc_queue_mem(unit, rq_data, SLI4_QTYPE_RQ, LPE_RQ_DEPTH, SLI4_RQ_ENTRY_SIZE) != 0) {
        lpe_free_queue_mem(rq_hdr);
        return -1;
    }

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_CREATE_RQ;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 72;

    cmd.payload[0] = 2;                               /* page count (hdr + data) */
    cmd.payload[1] = (ULONG)cq->id | (1 << 16);       /* CQ ID, dual buffer */
    cmd.payload[2] = LPE_RQ_DEPTH;                    /* buffer size / entry count */
    cmd.payload[4] = (ULONG)(IPTR)rq_hdr->base_dma;   /* header RQ page */
    cmd.payload[5] = (ULONG)((IPTR)rq_hdr->base_dma >> 32);
    cmd.payload[6] = (ULONG)(IPTR)rq_data->base_dma;  /* data RQ page */
    cmd.payload[7] = (ULONG)((IPTR)rq_data->base_dma >> 32);

    if (lpe_bmbx_command(unit, &cmd) != 0) {
        lpe_free_queue_mem(rq_hdr);
        lpe_free_queue_mem(rq_data);
        return -1;
    }

    rq_hdr->id = cmd.payload[0] & 0xFFFF;
    rq_data->id = (cmd.payload[0] >> 16) & 0xFFFF;

    D(bug("[%s] %s: Created RQ hdr_id=%ld data_id=%ld\n",
        unit->lpeu_name, __func__, (ULONG)rq_hdr->id, (ULONG)rq_data->id));
    return 0;
}

int lpe_queue_create_mq(struct LPe32002Unit *unit, struct sli4_queue *mq, struct sli4_queue *cq)
{
    struct sli4_mbox_cmd cmd;

    D(bug("[%s] %s(cq_id=%ld)\n", unit->lpeu_name, __func__, (ULONG)cq->id));

    if (lpe_alloc_queue_mem(unit, mq, SLI4_QTYPE_MQ, LPE_MQ_DEPTH, SLI4_MQ_ENTRY_SIZE) != 0)
        return -1;

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = SLI4_MBOX_CMD_CREATE_MQ;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 56;

    cmd.payload[0] = 1;
    cmd.payload[1] = 1 | ((ULONG)cq->id << 16);  /* valid, associated CQ */
    cmd.payload[4] = (ULONG)(IPTR)mq->base_dma;
    cmd.payload[5] = (ULONG)((IPTR)mq->base_dma >> 32);

    if (lpe_bmbx_command(unit, &cmd) != 0) {
        lpe_free_queue_mem(mq);
        return -1;
    }

    mq->id = cmd.payload[0] & 0xFFFF;
    D(bug("[%s] %s: Created MQ id=%ld\n", unit->lpeu_name, __func__, (ULONG)mq->id));
    return 0;
}

void lpe_queue_destroy(struct LPe32002Unit *unit, struct sli4_queue *q)
{
    struct sli4_mbox_cmd cmd;
    UWORD opcode;

    if (!q->base)
        return;

    switch (q->type) {
        case SLI4_QTYPE_EQ: opcode = SLI4_MBOX_CMD_DESTROY_EQ; break;
        case SLI4_QTYPE_CQ: opcode = SLI4_MBOX_CMD_DESTROY_CQ; break;
        case SLI4_QTYPE_WQ: opcode = SLI4_MBOX_CMD_DESTROY_WQ; break;
        case SLI4_QTYPE_RQ: opcode = SLI4_MBOX_CMD_DESTROY_RQ; break;
        case SLI4_QTYPE_MQ: opcode = SLI4_MBOX_CMD_DESTROY_MQ; break;
        default: return;
    }

    D(bug("[%s] %s: Destroying queue type=%ld id=%ld\n",
        unit->lpeu_name, __func__, (ULONG)q->type, (ULONG)q->id));

    memset(&cmd, 0, sizeof(cmd));
    cmd.hdr.opcode = opcode;
    cmd.hdr.subsystem = 0x01;
    cmd.hdr.request_length = 4;
    cmd.payload[0] = q->id;

    lpe_bmbx_command(unit, &cmd);
    lpe_free_queue_mem(q);
}

/* ============================================================
 * Full Queue Setup/Teardown
 * ============================================================ */

int lpe_setup_queues(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    /* Create Event Queue */
    if (lpe_queue_create_eq(unit, &unit->lpeu_eq) != 0)
        goto fail;

    /* Create Completion Queues */
    if (lpe_queue_create_cq(unit, &unit->lpeu_cq_wq, &unit->lpeu_eq) != 0)
        goto fail_eq;

    if (lpe_queue_create_cq(unit, &unit->lpeu_cq_rq, &unit->lpeu_eq) != 0)
        goto fail_cq_wq;

    if (lpe_queue_create_cq(unit, &unit->lpeu_cq_mq, &unit->lpeu_eq) != 0)
        goto fail_cq_rq;

    /* Create Work Queue (TX) */
    if (lpe_queue_create_wq(unit, &unit->lpeu_wq, &unit->lpeu_cq_wq) != 0)
        goto fail_cq_mq;

    /* Create Receive Queues (header + data pair) */
    if (lpe_queue_create_rq(unit, &unit->lpeu_rq_hdr, &unit->lpeu_rq_data,
                             &unit->lpeu_cq_rq) != 0)
        goto fail_wq;

    /* Create Mailbox Queue */
    if (lpe_queue_create_mq(unit, &unit->lpeu_mq, &unit->lpeu_cq_mq) != 0)
        goto fail_rq;

    D(bug("[%s] %s: All queues created successfully\n", unit->lpeu_name, __func__));
    return 0;

fail_rq:
    lpe_queue_destroy(unit, &unit->lpeu_rq_data);
    lpe_queue_destroy(unit, &unit->lpeu_rq_hdr);
fail_wq:
    lpe_queue_destroy(unit, &unit->lpeu_wq);
fail_cq_mq:
    lpe_queue_destroy(unit, &unit->lpeu_cq_mq);
fail_cq_rq:
    lpe_queue_destroy(unit, &unit->lpeu_cq_rq);
fail_cq_wq:
    lpe_queue_destroy(unit, &unit->lpeu_cq_wq);
fail_eq:
    lpe_queue_destroy(unit, &unit->lpeu_eq);
fail:
    return -1;
}

void lpe_teardown_queues(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    lpe_queue_destroy(unit, &unit->lpeu_mq);
    lpe_queue_destroy(unit, &unit->lpeu_rq_data);
    lpe_queue_destroy(unit, &unit->lpeu_rq_hdr);
    lpe_queue_destroy(unit, &unit->lpeu_wq);
    lpe_queue_destroy(unit, &unit->lpeu_cq_mq);
    lpe_queue_destroy(unit, &unit->lpeu_cq_rq);
    lpe_queue_destroy(unit, &unit->lpeu_cq_wq);
    lpe_queue_destroy(unit, &unit->lpeu_eq);
}

/* ============================================================
 * Interrupt Control
 * ============================================================ */

void lpe_eq_arm(struct LPe32002Unit *unit, struct sli4_queue *eq, BOOL arm)
{
    ULONG val = SLI4_EQCQ_DB_CLEAR |
                ((ULONG)eq->id << SLI4_EQCQ_DB_QID_SHIFT) |
                (eq->num_proc & SLI4_EQCQ_DB_NUM_MASK);

    if (arm)
        val |= SLI4_EQCQ_DB_EVENT;

    LPE_REG_WRITE32(unit->lpeu_bar1, SLI4_EQ_DOORBELL, val);
    eq->num_proc = 0;
}

void lpe_cq_arm(struct LPe32002Unit *unit, struct sli4_queue *cq, BOOL arm)
{
    ULONG val = SLI4_EQCQ_DB_CLEAR |
                ((ULONG)cq->id << SLI4_EQCQ_DB_QID_SHIFT) |
                (cq->num_proc & SLI4_EQCQ_DB_NUM_MASK);

    if (arm)
        val |= SLI4_EQCQ_DB_EVENT;

    LPE_REG_WRITE32(unit->lpeu_bar1, SLI4_CQ_DOORBELL, val);
    cq->num_proc = 0;
}

void lpe_irq_enable(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    /* Arm all EQs and CQs */
    lpe_eq_arm(unit, &unit->lpeu_eq, TRUE);
    lpe_cq_arm(unit, &unit->lpeu_cq_wq, TRUE);
    lpe_cq_arm(unit, &unit->lpeu_cq_rq, TRUE);
    lpe_cq_arm(unit, &unit->lpeu_cq_mq, TRUE);
}

void lpe_irq_disable(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    /* Disarm (don't set EVENT bit) */
    lpe_eq_arm(unit, &unit->lpeu_eq, FALSE);
}

int lpe_request_irq(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s(IRQ %ld)\n", unit->lpeu_name, __func__, (ULONG)unit->lpeu_IRQ));

    if (!unit->lpeu_IntsAdded) {
        AddIntServer(INTB_KERNEL + unit->lpeu_IRQ,
                     &unit->lpeu_irqhandler);
        AddIntServer(INTB_VERTB, &unit->lpeu_touthandler);
        unit->lpeu_IntsAdded = TRUE;
    }
    return 0;
}

void lpe_free_irq(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    if (unit->lpeu_IntsAdded) {
        RemIntServer(INTB_KERNEL + unit->lpeu_IRQ,
                     &unit->lpeu_irqhandler);
        RemIntServer(INTB_VERTB, &unit->lpeu_touthandler);
        unit->lpeu_IntsAdded = FALSE;
    }
}

/* ============================================================
 * Hardware Initialization
 * ============================================================ */

void lpe_hw_reset(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    /* Issue a firmware reset via PHYSDEV_CONTROL */
    LPE_REG_WRITE32(unit->lpeu_bar0, SLI4_PHYSDEV_CONTROL, SLI4_PHYSDEV_FW_RESET);
    LPE_WRITE_FLUSH(unit->lpeu_bar0);

    /* Wait for firmware to become ready again */
    lpe_fw_wait_ready(unit);
}

int lpe_hw_init(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s()\n", unit->lpeu_name, __func__));

    /* Wait for firmware ready */
    if (lpe_fw_wait_ready(unit) != 0) {
        D(bug("[%s] %s: Firmware not ready, attempting reset\n",
            unit->lpeu_name, __func__));
        lpe_hw_reset(unit);
        if (lpe_fw_wait_ready(unit) != 0)
            return -1;
    }

    /* Read firmware revision */
    if (lpe_hw_read_rev(unit) != 0) {
        D(bug("[%s] %s: Failed to read firmware revision\n",
            unit->lpeu_name, __func__));
        return -1;
    }

    /* Request features */
    if (lpe_hw_request_features(unit) != 0) {
        D(bug("[%s] %s: Failed to request features\n",
            unit->lpeu_name, __func__));
        return -1;
    }

    /* Read port configuration */
    if (lpe_hw_read_config(unit) != 0) {
        D(bug("[%s] %s: Failed to read configuration\n",
            unit->lpeu_name, __func__));
        return -1;
    }

    /* Read service parameters (WWPN/WWNN) */
    if (lpe_hw_read_sparm(unit) != 0) {
        D(bug("[%s] %s: Failed to read SPARM\n",
            unit->lpeu_name, __func__));
        return -1;
    }

    /* Set up the SLI-4 queues */
    if (lpe_setup_queues(unit) != 0) {
        D(bug("[%s] %s: Failed to create queues\n",
            unit->lpeu_name, __func__));
        return -1;
    }

    D(bug("[%s] %s: Hardware initialized successfully\n",
        unit->lpeu_name, __func__));
    return 0;
}

/* ============================================================
 * Transmit Frame
 * ============================================================ */

/*
 * Submit a frame for transmission via the Work Queue.
 * Builds a SLI4_WQE_SEND_FRAME WQE and rings the WQ doorbell.
 */
int lpe_tx_frame(struct LPe32002Unit *unit, APTR data, ULONG len)
{
    struct sli4_queue *wq = &unit->lpeu_wq;
    struct sli4_wq_entry *wqe;
    struct lpe_tx_desc *txd;
    UWORD idx;

    idx = wq->index;
    wqe = (struct sli4_wq_entry *)((UBYTE *)wq->base + (idx * wq->entry_size));
    txd = &unit->lpeu_tx_descs[idx];

    /* Set up TX buffer */
    if (!txd->buf.virt) {
        txd->buf.virt = AllocMem(FC_RXTX_ALLOC_BUFSIZE, MEMF_PUBLIC | MEMF_CLEAR);
        if (!txd->buf.virt)
            return -1;
        txd->buf.dma = HIDD_PCIDriver_CPUtoPCI(unit->lpeu_PCIDriver, txd->buf.virt);
        txd->buf.size = FC_RXTX_ALLOC_BUFSIZE;
    }

    CopyMem(data, txd->buf.virt, len);
    txd->length = len;

    /* Build the Work Queue Entry (SEND_FRAME) */
    memset(wqe, 0, sizeof(*wqe));
    wqe->words[0] = (ULONG)(IPTR)txd->buf.dma;              /* BDE address low */
    wqe->words[1] = (ULONG)((IPTR)txd->buf.dma >> 32);      /* BDE address high */
    wqe->words[2] = len;                                      /* BDE length */
    wqe->words[3] = 0;
    wqe->words[4] = 0;
    wqe->words[5] = 0;
    wqe->words[6] = 0;
    wqe->words[7] = (SLI4_WQE_SEND_FRAME << 0) |            /* opcode */
                     (0x01 << 8);                              /* command type */
    wqe->words[8] = 0;
    wqe->words[9] = (idx & 0xFFFF);                           /* request tag (WQ index) */

    /* Advance producer index */
    wq->index = (idx + 1) % wq->size;

    /* Ring the WQ doorbell */
    {
        ULONG db = ((ULONG)wq->id << SLI4_WQ_DB_QID_SHIFT) |
                   (1 & SLI4_WQ_DB_IDX_MASK);
        LPE_REG_WRITE32(unit->lpeu_bar1, SLI4_WQ_DOORBELL, db);
    }

    D(bug("[%s] %s: Queued frame idx=%ld len=%ld\n",
        unit->lpeu_name, __func__, (ULONG)idx, len));

    return 0;
}

/* ============================================================
 * Completion Queue Processing
 * ============================================================ */

/*
 * Process entries in a Completion Queue.
 * Returns the number of completions processed.
 */
int lpe_process_cq(struct LPe32002Unit *unit, struct sli4_queue *cq)
{
    struct sli4_cq_entry *cqe;
    int processed = 0;

    while (1) {
        cqe = (struct sli4_cq_entry *)((UBYTE *)cq->base + (cq->index * cq->entry_size));

        if (!(cqe->word3 & SLI4_CQ_ENTRY_VALID))
            break;

        /* Clear valid bit for reuse */
        cqe->word3 &= ~SLI4_CQ_ENTRY_VALID;

        cq->index = (cq->index + 1) % cq->size;
        cq->num_proc++;
        processed++;
    }

    return processed;
}

/* ============================================================
 * MAC (WWPN) Address Configuration
 * ============================================================ */

void lpe_set_mac(struct LPe32002Unit *unit)
{
    D(bug("[%s] %s: WWPN=%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
        unit->lpeu_name, __func__,
        (ULONG)unit->lpeu_dev_addr[0], (ULONG)unit->lpeu_dev_addr[1],
        (ULONG)unit->lpeu_dev_addr[2], (ULONG)unit->lpeu_dev_addr[3],
        (ULONG)unit->lpeu_dev_addr[4], (ULONG)unit->lpeu_dev_addr[5],
        (ULONG)unit->lpeu_dev_addr[6], (ULONG)unit->lpeu_dev_addr[7]));
}
