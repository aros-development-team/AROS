/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2712 (VC6) HDMI audio register map for Raspberry Pi 5.
*/

#include "DriverData.h"
#include "rpihdmi-soc.h"
#include "rpihdmi-hwaccess.h"

/* The DREQs are board-dependent and always come from the device tree;
 * DriverInit drops the output if the lookup fails. */
struct RPiHDMISoc rpihdmi_bcm2712_hdmi0_soc = {
    .dma_dreq = 12,
    .hsm_clock = 108000000,

    .regs = {
        /* HD block, HDMI0 set */
        .mai_ctl            = 0x010,
        .mai_thr            = 0x014,
        .mai_fmt            = 0x018,
        .mai_data           = 0x01C,
        .mai_smp            = 0x020,

        /* HDMI block: the audio group sits 8 bytes above the BCM2711 offsets */
        .mai_channel_map    = 0x0A4,
        .mai_config         = 0x0A8,
        .audio_packet_cfg   = 0x0C0,
        .ram_packet_cfg     = 0x0C4,
        .ram_packet_status  = 0x0CC,
        .crp_cfg            = 0x0D0,
        .cts_0              = 0x0D4,
        .cts_1              = 0x0D8,
        .scheduler_control  = 0x0E8,

        /* Packet block */
        .packet_start       = 0x000,
    },

    .mai_dreq_threshold   = 0x1C,
    .mai_panic_threshold  = 0x10,
    .hdmi_mai_channel_map = 0x10,
    .name = "HDMI 0",

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};

struct RPiHDMISoc rpihdmi_bcm2712_hdmi1_soc = {
    .dma_dreq = 13,
    .hsm_clock = 108000000,

    .regs = {
        /* HD block, HDMI1 set */
        .mai_ctl            = 0x030,
        .mai_thr            = 0x034,
        .mai_fmt            = 0x038,
        .mai_data           = 0x03C,
        .mai_smp            = 0x040,

        /* HDMI block, see HDMI 0 */
        .mai_channel_map    = 0x0A4,
        .mai_config         = 0x0A8,
        .audio_packet_cfg   = 0x0C0,
        .ram_packet_cfg     = 0x0C4,
        .ram_packet_status  = 0x0CC,
        .crp_cfg            = 0x0D0,
        .cts_0              = 0x0D4,
        .cts_1              = 0x0D8,
        .scheduler_control  = 0x0E8,

        /* Packet block */
        .packet_start       = 0x000,
    },

    .mai_dreq_threshold  = 0x1C,
    .mai_panic_threshold = 0x10,
    .hdmi_mai_channel_map = 0x10,
    .name = "HDMI 1",

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};
