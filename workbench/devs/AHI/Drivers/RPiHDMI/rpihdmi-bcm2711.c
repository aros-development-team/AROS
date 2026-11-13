/*
 *  BCM2711 (VC5) HDMI audio register map.
 *
 *  Unlike BCM2835/2836 the HD block is shared by both HDMI controllers:
 *  one 0x100 byte region (bus 0x7EF20000) holding two MAI register sets,
 *  HDMI0 at +0x10 and HDMI1 at +0x30. The RAM packet memory has moved out
 *  of the HDMI block into its own region, so packet_start is 0.
 *
 *  mai_base/hdmi_base/packet_base and dma_dreq are filled in from the
 *  device tree at init time; the values here are the register offsets
 *  within those blocks.
 *
 *  The MAI audio clock is the 108 MHz DVP clock, not the HSM clock.
 */

#include "DriverData.h"
#include "rpihdmi-soc.h"
#include "rpihdmi-hwaccess.h"

struct RPiHDMISoc rpihdmi_bcm2711_hdmi0_soc = {
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
        .mai_channel_map   = 0x09C,
        .mai_config        = 0x0A0,
        .audio_packet_cfg  = 0x0B8,
        .ram_packet_cfg    = 0x0BC,
        .ram_packet_status  = 0x0C4,
        .crp_cfg           = 0x0C8,
        .cts_0             = 0x0CC,
        .cts_1             = 0x0D0,
        .scheduler_control = 0x0E0,

        /* Packet block */
        .packet_start      = 0x000,
    },

    .mai_dreq_threshold  = 0x1C,
    .mai_panic_threshold = 0x10,
    .hdmi_mai_channel_map = 0x10,
    .name = "HDMI 0",

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};

struct RPiHDMISoc rpihdmi_bcm2711_hdmi1_soc = {
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
        .mai_channel_map   = 0x09C,
        .mai_config        = 0x0A0,
        .audio_packet_cfg  = 0x0B8,
        .ram_packet_cfg    = 0x0BC,
        .ram_packet_status  = 0x0C4,
        .crp_cfg           = 0x0C8,
        .cts_0             = 0x0CC,
        .cts_1             = 0x0D0,
        .scheduler_control = 0x0E0,

        /* Packet block */
        .packet_start      = 0x000,
    },

    .mai_dreq_threshold = 0x1C,
    .mai_panic_threshold = 0x10,
    .hdmi_mai_channel_map = 0x10,
    .name = "HDMI 1",

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};
