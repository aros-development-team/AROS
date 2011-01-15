
#include <hidd/irq.h>
#include <asm/io.h>

#define IDE_DMA
#define IDE_32BIT
#define IDE_HIDDIRQ

#define MAX_XFER_HOST AB_XFER_UDMA6

#define ata_out(val, offset, port)  outb((val), (offset)+(port))
#define ata_in(offset, port)        inb((offset)+(port))
#define ata_outl(val, offset, port) outl((val), (offset)+(port))

