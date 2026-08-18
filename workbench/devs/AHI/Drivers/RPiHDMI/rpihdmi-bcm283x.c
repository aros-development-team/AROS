#include "DriverData.h"
#include "rpihdmi-soc.h"
#include "rpihdmi-hwaccess.h"

const struct RPiHDMISoc rpihdmi_bcm283x_soc = {
    .mai_base     = 0x808000,
    .hdmi_base    = 0x902000,
    .mai_data_bus = 0x7E808020,
    .dma_dreq     = 17,
    .hsm_clock    = 163680000,

    .regs = {
        .mai_ctl            = 0x014,
        .mai_thr            = 0x018,
        .mai_fmt            = 0x01C,
        .mai_data           = 0x020,
        .mai_smp            = 0x02C,

        .mai_channel_map   = 0x090,
        .mai_config        = 0x094,
        .audio_packet_cfg  = 0x09C,
        .ram_packet_cfg    = 0x0A0,
        .ram_packet_status = 0x0A4,
        .crp_cfg           = 0x0A8,
        .cts_0             = 0x0AC,
        .cts_1             = 0x0B0,
        .scheduler_control = 0x0C0,
        .packet_start      = 0x400,
    },

    .mai_dreq_threshold = 0x10,
    .mai_panic_threshold = 0x10,
    .hdmi_mai_channel_map = 0x08,
    .name = "HDMI Audio",

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};
