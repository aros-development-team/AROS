/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    BCM2712 (VC6) HDMI audio register map for Raspberry Pi 5.
*/

#include "DriverData.h"
#include "rpihdmi-soc.h"
#include "rpihdmi-hwaccess.h"

struct RPiHDMISoc rpihdmi_bcm2712_hdmi0_soc = {
    .dma_dreq = 10,
    .hsm_clock = 108000000,

    .regs = {
        /* HD block, HDMI0 set */
        .mai_ctl            = 0x010,
        .mai_thr            = 0x014,
        .mai_fmt            = 0x018,
        .mai_data           = 0x01C,
        .mai_smp            = 0x020,

        /* HDMI block */
        .mai_channel_map    = 0x09C,
        .mai_config         = 0x0A0,
        .audio_packet_cfg   = 0x0B8,
        .ram_packet_cfg     = 0x0BC,
        .ram_packet_status  = 0x0C4,
        .crp_cfg            = 0x0C8,
        .cts_0              = 0x0CC,
        .cts_1              = 0x0D0,
        .scheduler_control  = 0x0E0,

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
    .dma_dreq = 17,
    .hsm_clock = 108000000,

    .regs = {
        /* HD block, HDMI1 set */
        .mai_ctl            = 0x030,
        .mai_thr            = 0x034,
        .mai_fmt            = 0x038,
        .mai_data           = 0x03C,
        .mai_smp            = 0x040,

        /* HDMI block */
        .mai_channel_map    = 0x09C,
        .mai_config         = 0x0A0,
        .audio_packet_cfg   = 0x0B8,
        .ram_packet_cfg     = 0x0BC,
        .ram_packet_status  = 0x0C4,
        .crp_cfg            = 0x0C8,
        .cts_0              = 0x0CC,
        .cts_1              = 0x0D0,
        .scheduler_control  = 0x0E0,

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
