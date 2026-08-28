#ifndef AHI_Drivers_RPiHDMI_soc_h
#define AHI_Drivers_RPiHDMI_soc_h

#include <exec/types.h>

struct RPiHDMIData;

struct RPiHDMIRegs {
    ULONG mai_ctl;
    ULONG mai_thr;
    ULONG mai_fmt;
    ULONG mai_data;
    ULONG mai_smp;

    ULONG mai_channel_map;
    ULONG mai_config;
    ULONG audio_packet_cfg;
    ULONG ram_packet_cfg;
    ULONG ram_packet_status;

    ULONG crp_cfg;
    ULONG cts_0;
    ULONG cts_1;
    ULONG scheduler_control;

    ULONG packet_start;
};

struct RPiHDMISoc {
    IPTR mai_base;
    IPTR hdmi_base;
    IPTR packet_base;

    ULONG mai_data_bus;
    ULONG dma_dreq;
    ULONG hsm_clock;

    ULONG mai_dreq_threshold;
    ULONG mai_panic_threshold;
    ULONG hdmi_mai_channel_map;

    struct RPiHDMIRegs regs;

    const char *name;

    void (*init)(struct RPiHDMIData *dd);
    void (*stop)(struct RPiHDMIData *dd);
};

extern const struct RPiHDMISoc rpihdmi_bcm283x_soc;
extern struct RPiHDMISoc rpihdmi_bcm2711_hdmi0_soc;
extern struct RPiHDMISoc rpihdmi_bcm2711_hdmi1_soc;
extern struct RPiHDMISoc rpihdmi_bcm2712_hdmi0_soc;
extern struct RPiHDMISoc rpihdmi_bcm2712_hdmi1_soc;

#define BCM2711_BUS_PERIIOBASE 0x7E000000UL
#define BCM2712_BUS_PERIIOBASE 0x7C000000UL

#endif
