struct ATA_BusData
{
    struct ATA_PIOInterface   *pioVectors;
    struct ATA_PIO32Interface *pio32Vectors;
    APTR                      *dmaVectors;
    ULONG                      pioDataSize;
    ULONG                      dmaDataSize;
    void                      *pioInterface;
    void                      *dmaInterface;
};
