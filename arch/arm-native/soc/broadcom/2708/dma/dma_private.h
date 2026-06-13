/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
*/

#ifndef DMA_PRIVATE_H_
#define DMA_PRIVATE_H_

#include <exec/nodes.h>
#include <exec/semaphores.h>
#include <exec/tasks.h>
#include <devices/timer.h>
#include <inttypes.h>

/* Per-channel completion-IRQ state. Each DMA channel has a dedicated
 * ARM IRQ line (IRQ_DMA0 + channel); the handler W1Cs the INT flag and
 * signals the registered waiter. */
struct DMAChWait {
    APTR                    irq_handle;
    struct Task * volatile  waiter;
    volatile BYTE           sig;
};

struct DMABase {
    struct Node                 dma_Node;
    struct SignalSemaphore      dma_Sem;
    unsigned int                dma_periiobase;
    unsigned int                dma_InUse;      /* Bitmask of allocated channels */

    struct DMAChWait            dma_Wait[15];

    /* Lazily opened timer.device (UNIT_MICROHZ) template for the wait
     * safety pulse — cloned into stack requests per use. */
    struct timerequest          dma_TimerTemplate;
    BOOL                        dma_TimerOk;
    BOOL                        dma_TimerTried;
};

#define ARM_PERIIOBASE DMABase->dma_periiobase
#include <hardware/bcm2708.h>

#endif /* DMA_PRIVATE_H_ */
