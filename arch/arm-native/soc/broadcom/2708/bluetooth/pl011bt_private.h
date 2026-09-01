/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Private state for pl011bt.resource: the PL011 UART that the BCM283x wires
    to the on-board Bluetooth controller on the Raspberry Pi 3, 3B+, Zero 2 W
    and 4.

    The resource owns the transport and nothing above it. HCI framing is
    h4bthci.device beside this, the protocols are rom/bluetooth, and the
    Broadcom patchram upload is brcmbt.fwl.
*/

#ifndef PL011BT_PRIVATE_H_
#define PL011BT_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <exec/types.h>
#include <inttypes.h>

/*
 * Receive ring, filled by the PL011 interrupt and drained by PL011BTRead().
 * A power of two, so the index arithmetic masks rather than divides.
 *
 * Sized against the traffic, not against a tick period. The BCM43430A1 does
 * not send one advertising report per event: during an LE scan it batches
 * every advert it heard into a single HCI event of up to 1220 bytes. Two of
 * those back to back overflow a 2 KB ring, and an overflow costs the H:4
 * framer its synchronisation just as a FIFO overrun does. 8 KB holds six.
 */
#define PL011BT_RX_RING         8192

/*
 * Bound on the BUSY spins. Both places that wait -- the clock generator
 * stopping and the UART draining -- are microseconds on working hardware, so
 * this is a bound on a fault rather than a timeout anyone should reach.
 */
#define PL011BT_WAIT_LIMIT      1000000

struct PL011BTBase
{
    struct Node                 pl011bt_Node;
    struct SignalSemaphore      pl011bt_Sem;
    unsigned int                pl011bt_periiobase;

    ULONG                       pl011bt_Caps;
    APTR                        pl011bt_Owner;
    ULONG                       pl011bt_ClockHz;    /* UART reference clock */
    ULONG                       pl011bt_Baud;
    ULONG                       pl011bt_ConfigFlags;
    ULONG                       pl011bt_RXErrors;   /* status reports emitted */

    /*
     * One mailbox message buffer for the life of the resource, allocated and
     * aligned once. The firmware writes its reply straight to memory, so the
     * buffer has to own its cache lines exclusively -- see the MBOX_MSG_ALIGN
     * comment in mbox.conf. Both users hold pl011bt_Sem.
     */
    APTR                        pl011bt_MBoxRaw;
    ULONG                      *pl011bt_MBoxMsg;

    /*
     * The FIFO holds sixteen bytes, which at 115200 baud is 1.4 ms. Nothing
     * scheduled can be relied on to visit it that often: polling at 10 ms lost
     * data on every burst, and polling at 1 ms still reported OVERRUN on real
     * hardware during an LE scan. The interrupt owns the FIFO and the ring,
     * PL011BTRead() owns the tail, so a late reader costs latency rather than
     * bytes. A single producer and a single consumer need no interlock.
     */
    volatile ULONG              pl011bt_RXHead;     /* written by the handler */
    volatile ULONG              pl011bt_RXTail;     /* written by the reader  */
    volatile ULONG              pl011bt_RXDropped;
    APTR                        pl011bt_RXIRQ;      /* KrnAddIRQHandler handle */
    UBYTE                       pl011bt_RXRing[PL011BT_RX_RING];
};

#define ARM_PERIIOBASE PL011BTBase->pl011bt_periiobase
#include <hardware/bcm2708.h>
#include <hardware/pl011uart.h>
#include <hardware/videocore.h>

#endif /* PL011BT_PRIVATE_H_ */
