/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Private state for bwfm.resource - Broadcom FullMAC WiFi chip bring-up
    over SDIO (consumes sdio.resource). Phase 2: chip identification, core
    enumeration and RAM sizing. The SANA-II network front-end (bwfm.device)
    will be built on top of this chip layer later.
*/

#ifndef BWFM_PRIVATE_H_
#define BWFM_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <inttypes.h>

/* One silicon-backplane core, as enumerated from the EROM. */
struct bwfm_core
{
    uint16_t                co_id;
    uint16_t                co_rev;
    uint32_t                co_base;
    uint32_t                co_wrapbase;
};

#define BWFM_MAX_CORES          24

/*
 * Firmware-event hand-off ring. The device's RX pump is the single SDPCM FIFO
 * reader; it posts received event frames here, and an in-progress control op
 * (join / scan) pops them. One bus reader avoids a pump-vs-control race on the
 * FIFO and keeps the chip mailbox serviced while a join/scan waits.
 */
#define BWFM_EVRING_SLOTS       8
#define BWFM_EVSLOT_SIZE        2048

struct bwfm_evslot
{
    uint16_t                ev_len;
    uint8_t                 ev_data[BWFM_EVSLOT_SIZE];
};

struct BWFMBase
{
    struct Node             bwfm_Node;
    struct SignalSemaphore  bwfm_Sem;

    unsigned int            bwfm_periiobase;    /* for the busy-wait timer */
    uint32_t                bwfm_bar0;          /* Current backplane window base */

    /* Identified chip */
    uint16_t                bwfm_chip;          /* ChipCommon chip ID */
    uint8_t                 bwfm_chiprev;
    int                     bwfm_attached;

    /* Enumerated cores */
    struct bwfm_core        bwfm_cores[BWFM_MAX_CORES];
    int                     bwfm_ncores;

    /* Capabilities / RAM layout */
    uint32_t                bwfm_cc_caps;
    uint32_t                bwfm_cc_caps_ext;
    uint8_t                 bwfm_pmurev;
    uint32_t                bwfm_rambase;
    uint32_t                bwfm_ramsize;
    uint32_t                bwfm_srsize;

    /* SDPCM / BCDC control channel */
    uint8_t                 bwfm_txseq;     /* next host->device frame seqnr */
    uint8_t                 bwfm_txmax;     /* firmware tx-credit window (maxseqnr) */
    uint16_t                bwfm_reqid;

    /* Event hand-off from the RX pump to an in-progress join/scan */
    int                     bwfm_evlisten;  /* a control op is collecting events */
    uint16_t                bwfm_evhead;    /* ring producer index */
    uint16_t                bwfm_evtail;    /* ring consumer index */
    struct bwfm_evslot      bwfm_evring[BWFM_EVRING_SLOTS];
};

#endif /* BWFM_PRIVATE_H_ */
