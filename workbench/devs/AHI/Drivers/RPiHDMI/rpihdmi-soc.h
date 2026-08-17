
struct RPiHDMISoc {
    ULONG mai_base;
    ULONG hdmi_base;
    ULONG mai_data_bus;
    ULONG dma_dreq;
    ULONG hsm_clock;

    ULONG (*dma_irq)(IPTR peribase, ULONG channel);

    void (*init)(struct RPiHDMIData *dd);
    void (*stop)(struct RPiHDMIData *dd);
};
