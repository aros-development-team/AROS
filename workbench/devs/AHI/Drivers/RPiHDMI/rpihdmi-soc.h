#ifndef AHI_Drivers_RPiHDMI_soc_h
#define AHI_Drivers_RPiHDMI_soc_h

#include <exec/types.h>

struct RPiHDMIData;

struct RPiHDMISoc {
    IPTR mai_base;
    IPTR hdmi_base;
    ULONG mai_data_bus;
    ULONG dma_dreq;
    ULONG hsm_clock;

    void (*init)(struct RPiHDMIData *dd);
    void (*stop)(struct RPiHDMIData *dd);
};

extern const struct RPiHDMISoc rpihdmi_bcm283x_soc;
extern const struct RPiHDMISoc rpihdmi_bcm2711_soc;

#endif
