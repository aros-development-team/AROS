
#define MAX_XFER_HOST AB_XFER_PIO0

#define IDE_GAYLEAMIGA

#define GAYLE_BASE_4000 0xdd2020
#define GAYLE_IRQ_4000  0x1000

#define GAYLE_BASE_1200 0xda0000
#define GAYLE_IRQ_1200  0x9000
#define GAYLE_INT_1200  0xA000

#define GAYLE_IRQ_IDE	0x80
#define GAYLE_INT_IDE	0x80

extern VOID ata_insw(APTR address, IPTR port, ULONG count);
extern VOID ata_insl(APTR address, IPTR port, ULONG count);
extern VOID ata_outsw(APTR address, IPTR port, ULONG count);
extern VOID ata_outsl(APTR address, IPTR port, ULONG count);
void ata_out(UBYTE val, UWORD offset, IPTR port);
UBYTE ata_in(UWORD offset, IPTR port);
void ata_outl(ULONG val, UWORD offset, IPTR port);
