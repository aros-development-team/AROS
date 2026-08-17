#ifndef AHI_Drivers_RPiHDMI_dma_h
#define AHI_Drivers_RPiHDMI_dma_h

#include <exec/types.h>

#include "DriverData.h"

void dma_build_control_blocks(struct RPiHDMIData *dd);
void dma_setup(struct RPiHDMIData *dd);
void dma_stop(struct RPiHDMIData *dd);
void dma_irq_handler(struct RPiHDMIData *data, void *data2);

/* The audio DMA channel is allocated at runtime from dma.resource. */

/* DMA control block TI bits */
#define DMA_TI_INTEN           (1 << 0)
#define DMA_TI_WAIT_RESP       (1 << 3)
#define DMA_TI_DEST_DREQ       (1 << 6)
#define DMA_TI_SRC_INC         (1 << 8)
#define DMA_TI_BURST_LENGTH(x) (((x) & 0xF) << 12)
#define DMA_TI_PERMAP(x)       (((x) & 0x1F) << 16)
#define DMA_TI_NO_WIDE_BURSTS  (1 << 26)

/* DMA CS bits */
/* Run state and acknowledge live in bcm2708_dma.h, shared with the other
 * DMA-driven AHI driver - they have to agree. */
#define DMA_CS_ACTIVE          (1 << 0)
#define DMA_CS_END             (1 << 1)
#define DMA_CS_INT             (1 << 2)
#define DMA_CS_ABORT           (1 << 30)
#define DMA_CS_RESET           (1 << 31)

#endif
