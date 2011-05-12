
#define MAX_XFER_HOST AB_XFER_PIO0

#define IDE_GAYLEAMIGA

#define GAYLE_BASE_4000 0xdd2022 /* 0xdd2020.W, 0xdd2026.B, 0xdd202a.B ... (argh!) */
#define GAYLE_IRQ_4000  0xdd3020

#define GAYLE_BASE_1200 0xda0000 /* 0xda0000.W, 0xda0004.B, 0xda0008.B ... */
#define GAYLE_IRQ_1200  0xda9000
#define GAYLE_INT_1200  0xdaa000

#define GAYLE_IRQ_IDE	0x80
#define GAYLE_INT_IDE	0x80

extern VOID ata_insw(APTR address, IPTR port, ULONG count);
extern VOID ata_insl(APTR address, IPTR port, ULONG count);
extern VOID ata_outsw(APTR address, IPTR port, ULONG count);
extern VOID ata_outsl(APTR address, IPTR port, ULONG count);
void ata_out(UBYTE val, UWORD offset, IPTR port);
UBYTE ata_in(UWORD offset, IPTR port);
void ata_outl(ULONG val, UWORD offset, IPTR port);
