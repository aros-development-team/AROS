/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    BCM2711 GENET Ethernet — Unit creation and task
*/

#include <aros/debug.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>

#include <devices/sana2.h>
#include <proto/exec.h>
#include <proto/kernel.h>

#include <string.h>

#include "genet.h"

APTR KernelBase = NULL;

#define UNIT_STACK_SIZE 16384

/* ============================================================
 * IRQ handler — signals unit task on RX/TX completion
 * ============================================================ */

static void genet_IRQHandler(struct GENETUnit *unit, void *data2)
{
    (void)data2;
    if (unit->gn_Task && unit->gn_IntSig != (ULONG)-1)
        Signal(unit->gn_Task, 1 << unit->gn_IntSig);
}

/* ============================================================
 * RX packet delivery
 * ============================================================ */

static void genet_RxPackets(struct GENETUnit *unit)
{
    ULONG prod_idx = AROS_LE2LONG(*(volatile ULONG *)(unit->gn_RegBase + RDMA_PROD_INDEX));

    while ((unit->gn_RxConsIdx & 0xFFFF) != (prod_idx & 0xFFFF)) {
        ULONG rx_idx = unit->gn_RxIdx;
        IPTR desc = unit->gn_RxDescBase + rx_idx * DMA_DESC_SIZE;
        ULONG len_stat = AROS_LE2LONG(*(volatile ULONG *)(desc + DMA_DESC_LENGTH_STATUS));
        ULONG length = (len_stat >> DMA_BUFLENGTH_SHIFT) & DMA_BUFLENGTH_MASK;
        UBYTE *pkt = unit->gn_RxBuffer + rx_idx * RX_BUF_LENGTH + 2; /* +2 for RBUF_ALIGN_2B */
        struct IOSana2Req *req;

        length -= 2; /* Subtract alignment padding */

        if (length >= ETH_HEADERSIZE && length <= ETH_MAXPACKETSIZE) {
            /* Try to deliver to a waiting read request */
            ObtainSemaphore(&unit->gn_Lock);
            req = (struct IOSana2Req *)RemHead((struct List *)&unit->gn_ReadList);
            ReleaseSemaphore(&unit->gn_Lock);

            if (req) {
                UBYTE *dest = req->ios2_Data;
                struct Opener *opener = req->ios2_BufferManagement;

                /* Extract Ethernet header info */
                CopyMem(pkt, req->ios2_DstAddr, ETH_ADDRSIZE);
                CopyMem(pkt + ETH_ADDRSIZE, req->ios2_SrcAddr, ETH_ADDRSIZE);
                req->ios2_PacketType = (pkt[12] << 8) | pkt[13];
                req->ios2_DataLength = length - ETH_HEADERSIZE;

                /* Copy payload via buffer management function */
                if (opener && opener->rx_function) {
                    opener->rx_function(dest, pkt + ETH_HEADERSIZE, req->ios2_DataLength);
                } else {
                    CopyMem(pkt + ETH_HEADERSIZE, dest, req->ios2_DataLength);
                }

                req->ios2_Req.io_Error = 0;
                ReplyMsg((struct Message *)req);

                unit->gn_Stats.PacketsReceived++;
            } else {
                /* No reader — packet dropped */
                unit->gn_Stats.UnknownTypesReceived++;
            }
        }

        /* Advance consumer index */
        unit->gn_RxConsIdx = (unit->gn_RxConsIdx + 1) & 0xFFFF;
        *(volatile ULONG *)(unit->gn_RegBase + RDMA_CONS_INDEX) = AROS_LONG2LE(unit->gn_RxConsIdx);

        if (++unit->gn_RxIdx >= RX_DESCS)
            unit->gn_RxIdx = 0;
    }
}

/* ============================================================
 * TX packet sending
 * ============================================================ */

static void genet_TxPackets(struct GENETUnit *unit)
{
    struct IOSana2Req *req;

    ObtainSemaphore(&unit->gn_Lock);
    while ((req = (struct IOSana2Req *)RemHead((struct List *)&unit->gn_WriteList))) {
        ReleaseSemaphore(&unit->gn_Lock);

        /* Build Ethernet frame in a temporary buffer */
        UBYTE frame[ETH_MAXPACKETSIZE];
        ULONG framelen;
        struct Opener *opener = req->ios2_BufferManagement;

        /* Destination address */
        if (req->ios2_Req.io_Command == S2_BROADCAST) {
            memset(frame, 0xFF, ETH_ADDRSIZE);
        } else {
            CopyMem(req->ios2_DstAddr, frame, ETH_ADDRSIZE);
        }

        /* Source address */
        CopyMem(unit->gn_DevAddr, frame + ETH_ADDRSIZE, ETH_ADDRSIZE);

        /* EtherType */
        frame[12] = (req->ios2_PacketType >> 8) & 0xFF;
        frame[13] = req->ios2_PacketType & 0xFF;

        /* Payload */
        framelen = ETH_HEADERSIZE + req->ios2_DataLength;
        if (opener && opener->tx_function) {
            opener->tx_function(frame + ETH_HEADERSIZE, req->ios2_Data, req->ios2_DataLength);
        } else {
            CopyMem(req->ios2_Data, frame + ETH_HEADERSIZE, req->ios2_DataLength);
        }

        /* Flush cache and send */
        CacheClearE(frame, framelen, CACRF_ClearD);

        if (genet_HW_Send(unit, frame, framelen) == 0) {
            req->ios2_Req.io_Error = 0;
            unit->gn_Stats.PacketsSent++;
        } else {
            req->ios2_Req.io_Error = S2ERR_TX_FAILURE;
        }

        ReplyMsg((struct Message *)req);
        ObtainSemaphore(&unit->gn_Lock);
    }
    ReleaseSemaphore(&unit->gn_Lock);
}

/* ============================================================
 * Unit task — main loop
 * ============================================================ */

void genet_UnitTask(void)
{
    struct GENETUnit *unit;
    ULONG sigmask;

    unit = (struct GENETUnit *)FindTask(NULL)->tc_UserData;
    D(bug("[genet] UnitTask started for unit %ld\n", unit->gn_UnitNum));

    unit->gn_IntSig = AllocSignal(-1);
    if (unit->gn_IntSig == (ULONG)-1) {
        D(bug("[genet] Cannot allocate signal\n"));
        return;
    }

    sigmask = (1 << unit->gn_IntSig) | SIGBREAKF_CTRL_C;

    /* Register IRQ handler for GENET RX/TX completion (GIC SPI 157) */
    unit->gn_IRQHandle = KrnAddIRQHandler(GENET_IRQ_0, genet_IRQHandler, unit, SysBase);

    for (;;) {
        ULONG signals = Wait(sigmask);

        if (signals & SIGBREAKF_CTRL_C)
            break;

        /* Process RX */
        genet_RxPackets(unit);

        /* Process TX */
        genet_TxPackets(unit);
    }

    if (unit->gn_IRQHandle) {
        KrnRemIRQHandler(unit->gn_IRQHandle);
        unit->gn_IRQHandle = NULL;
    }

    FreeSignal(unit->gn_IntSig);
}

/* ============================================================
 * Unit creation/deletion
 * ============================================================ */

struct GENETUnit *genet_CreateUnit(struct GENETBase *base, ULONG unitnum)
{
    struct GENETUnit *unit;

    unit = AllocVec(sizeof(struct GENETUnit), MEMF_CLEAR | MEMF_PUBLIC);
    if (!unit)
        return NULL;

    unit->gn_UnitNum = unitnum;
    unit->gn_Device = base;
    unit->gn_RegBase = (IPTR)KrnGetSystemAttr(KATTR_PeripheralBase) == 0xFE000000 ? GENET_BASE_DEFAULT : 0;
    unit->gn_PhyAddr = GENET_PHY_ADDR;

    InitSemaphore(&unit->gn_Lock);
    NEWLIST(&unit->gn_ReadList);
    NEWLIST(&unit->gn_WriteList);
    NEWLIST(&unit->gn_EventList);
    NEWLIST(&unit->gn_ReadOrphanList);
    NEWLIST(&unit->gn_TypeTrackers);

    /* Allocate RX buffer ring */
    unit->gn_RxBuffer = AllocVec(RX_DESCS * RX_BUF_LENGTH, MEMF_CLEAR | MEMF_PUBLIC);
    if (!unit->gn_RxBuffer) {
        FreeVec(unit);
        return NULL;
    }

    /* Read MAC address from hardware (UMAC registers, set by firmware) */
    if (unit->gn_RegBase) {
        ULONG mac0 = AROS_LE2LONG(*(volatile ULONG *)(unit->gn_RegBase + UMAC_MAC0));
        ULONG mac1 = AROS_LE2LONG(*(volatile ULONG *)(unit->gn_RegBase + UMAC_MAC1));

        unit->gn_DevAddr[0] = (mac0 >> 24) & 0xFF;
        unit->gn_DevAddr[1] = (mac0 >> 16) & 0xFF;
        unit->gn_DevAddr[2] = (mac0 >> 8) & 0xFF;
        unit->gn_DevAddr[3] = mac0 & 0xFF;
        unit->gn_DevAddr[4] = (mac1 >> 8) & 0xFF;
        unit->gn_DevAddr[5] = mac1 & 0xFF;

        CopyMem(unit->gn_DevAddr, unit->gn_OrgAddr, ETH_ADDRSIZE);
        D(bug("[genet] MAC: %02lx:%02lx:%02lx:%02lx:%02lx:%02lx\n",
              unit->gn_DevAddr[0], unit->gn_DevAddr[1], unit->gn_DevAddr[2],
              unit->gn_DevAddr[3], unit->gn_DevAddr[4], unit->gn_DevAddr[5]));
    }

    return unit;
}

void genet_DeleteUnit(struct GENETBase *base, struct GENETUnit *unit)
{
    if (unit->gn_Flags & GNF_ONLINE)
        genet_HW_Stop(unit);

    if (unit->gn_RxBuffer)
        FreeVec(unit->gn_RxBuffer);

    FreeVec(unit);
}
