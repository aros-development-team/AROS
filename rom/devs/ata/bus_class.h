struct ATA_BusData
{
    APTR  *pioVectors;
    APTR  *pio32Vectors;
    APTR  *dmaVectors;
    ULONG  pioDataSize;
    ULONG  dmaDataSize;
    void  *pioInterface;
    void  *dmaInterface;
};
