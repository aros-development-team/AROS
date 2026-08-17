#include "DriverData.h"
#include "rpihdmi-soc.h"
#include "rpihdmi-hwaccess.h"

const struct RPiHDMISoc rpihdmi_bcm283x_soc = {
    .mai_base     = 0x808000,
    .hdmi_base    = 0x902000,
    .mai_data_bus = 0x7E808020,
    .dma_dreq     = 17,
    .hsm_clock    = 163680000,

    .init         = hdmi_mai_init,
    .stop         = hdmi_mai_stop
};
