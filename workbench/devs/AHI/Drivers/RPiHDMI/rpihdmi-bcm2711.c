#include "DriverData.h"
#include "rpihdmi-soc.h"

struct RPiHDMISoc rpihdmi_bcm2711_hdmi0_soc = {
    .dma_dreq = 10,
};

struct RPiHDMISoc rpihdmi_bcm2711_hdmi1_soc = {
    .dma_dreq = 17,
};
