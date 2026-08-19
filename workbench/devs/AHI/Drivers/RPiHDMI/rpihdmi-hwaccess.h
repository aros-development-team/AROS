/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.
    Author: Fabian Schmieder (@metaneutrons)

    Raspberry Pi HDMI MAI Audio Register Definitions & Access Macros
*/

#ifndef AHI_Drivers_RPiHDMI_rpihdmi_hwaccess_h
#define AHI_Drivers_RPiHDMI_rpihdmi_hwaccess_h

#include <exec/types.h>
#include "DriverData.h"

/*
 * Broadcom HDMI MAI Audio Registers
 */
#define HDMI_MAI_CTL            0x010UL
#define HDMI_MAI_DATA           0x014UL
#define HDMI_MAI_STATUS         0x018UL
#define HDMI_MAI_FMT            0x01CUL
#define HDMI_AUDIO_PACKET       0x020UL
#define HDMI_RAM_PACKET_CONFIG  0x0A0UL
#define HDMI_RAM_PACKET_STATUS  0x0A4UL

/* MAI_CTL Bits */
#define MAI_CTL_ENABLE          (1 << 0)
#define MAI_CTL_CHANNELS_2      (1 << 1)
#define MAI_CTL_FORMAT_16BIT    (0 << 4)
#define MAI_CTL_FORMAT_24BIT    (1 << 4)
#define MAI_CTL_FLUSH           (1 << 7)
#define MAI_CTL_DREQ_EN         (1 << 8)

/* MAI_STATUS Bits */
#define MAI_STATUS_BUSY         (1 << 0)
#define MAI_STATUS_EMPTY        (1 << 1)
#define MAI_STATUS_FULL         (1 << 2)

/* DMA DREQ line for HDMI Audio */
#define DMA_DREQ_HDMI_MAI       10

/* DMA Control Block Transfer Information (TI) */
#define DMA_TI_INTEN            (1 << 0)
#define DMA_TI_TDMODE           (0 << 1)
#define DMA_TI_WAIT_RESP        (1 << 3)
#define DMA_TI_DEST_DREQ        (1 << 6)
#define DMA_TI_SRC_INC          (1 << 8)
#define DMA_TI_DEST_INC         (0 << 4)
#define DMA_TI_PERMAP(x)        (((x) & 0x1F) << 16)

/* DMA Channel Control/Status (CS) */
#define DMA_CS_ACTIVE           (1 << 0)
#define DMA_CS_END              (1 << 1)
#define DMA_CS_INT              (1 << 2)
#define DMA_CS_DREQ             (1 << 3)
#define DMA_CS_RESET            (1 << 31)

/* Hardware Function Prototypes */
BOOL rpihdmi_hw_init(struct RPiHDMIData *dd, ULONG rate);
void rpihdmi_hw_cleanup(struct RPiHDMIData *dd);
void rpihdmi_hw_start_dma(struct RPiHDMIData *dd);
void rpihdmi_hw_stop_dma(struct RPiHDMIData *dd);
ULONG rpihdmi_hw_irq_handler(struct RPiHDMIData *dd);

#endif /* AHI_Drivers_RPiHDMI_rpihdmi_hwaccess_h */
