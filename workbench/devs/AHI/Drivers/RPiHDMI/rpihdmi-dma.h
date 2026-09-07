#ifndef AHI_Drivers_RPiHDMI_dma_h
#define AHI_Drivers_RPiHDMI_dma_h

#include <exec/types.h>

#include "DriverData.h"

void dma_build_control_blocks(struct RPiHDMIData *dd);
int dma_active_cb(struct RPiHDMIData *dd);
void dma_set_txfr_len(struct RPiHDMIData *dd, int i, ULONG bytes);
void dma_setup(struct RPiHDMIData *dd);
void dma_stop(struct RPiHDMIData *dd);
void dma_irq_handler(struct RPiHDMIData *data, void *data2);

ULONG dma_probe_dreq(struct RPiHDMIData *dd, ULONG expect);
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

/* DMA4 (BCM2712 channels 6-11): 40-bit addresses, own control block layout,
 * CS bit 31 is HALT and there is no reset bit. */
#define DMA4_TI_INTEN          (1 << 0)
#define DMA4_TI_WAIT_RESP      (1 << 2)
#define DMA4_TI_PERMAP(x)      (((x) & 0x1F) << 9)
#define DMA4_TI_D_DREQ         (1 << 15)

/* Guarded: bcm2708.h defines these too. */
#ifndef DMA4_XI_ADDR_HI
#define DMA4_XI_ADDR_HI(a)     ((ULONG)(((UQUAD)(a) >> 32) & 0xFF))
#endif
#define DMA4_XI_BURST_LEN(x)   (((x) & 0xF) << 8)
#define DMA4_XI_INC            (1 << 12)

#define DMA4_CS_ACTIVE         (1 << 0)
#define DMA4_CS_END            (1 << 1)
#define DMA4_CS_INT            (1 << 2)
#define DMA4_CS_DMA_BUSY       (1 << 24)
#define DMA4_CS_WAIT_FOR_WRITES (1 << 28)
#define DMA4_CS_HALT           (1UL << 31)

#define DMA4_DEBUG_RESET       (1 << 23)

/* CS[9:8], documented as reserved, but a kick without them is rejected. */
#define DMA4_CS_PROT           (3UL << 8)

#define DMA4_REG_CS            0x00
#define DMA4_REG_CB            0x04
#define DMA4_REG_DEBUG         0x0C
#define DMA4_REG_LEN           0x24

/* Control block addresses are stored shifted right by 5. */
#ifndef DMA4_CB_ADDR
#define DMA4_CB_ADDR(a)        ((ULONG)((UQUAD)(a) >> 5))
#endif

#endif
