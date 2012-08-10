struct ata_Bus;

struct ata_BusDriver
{
    void  (*ata_out)(UBYTE val, UWORD offset, IPTR port, APTR data);
    UBYTE (*ata_in)(UWORD offset, IPTR port, APTR data);
    void  (*ata_outl)(ULONG val, UWORD offset, IPTR port, APTR data);
    VOID  (*ata_insw)(APTR address, UWORD port, ULONG count, APTR data);
    VOID  (*ata_outsw)(APTR address, UWORD port, ULONG count, APTR data);
    /*
     * The following are optional.
     * If they are NULL, our bus doesn't support 32-bit transfers.
     */
    VOID  (*ata_insl)(APTR address, UWORD port, ULONG count, APTR data);
    VOID  (*ata_outsl)(APTR address, UWORD port, ULONG count, APTR data);

    /*
     * The following doesn't require high speed, and it is going
     * to become HIDD methods.
     */
    BOOL (*CreateInterrupt)(struct ata_Bus *bus);
    VOID (*AckInterrupt)(struct ata_Bus *bus);
};
