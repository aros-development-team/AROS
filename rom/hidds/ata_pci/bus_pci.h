#include <asm/io.h>

struct bus_data
{
    struct ata_BusDriver pub;
    port_t               ioBase;
    port_t               ioAlt;
    port_t               dmaBase;
    UBYTE                irqNum;
    BOOL                 use80wire;
    OOP_Object           *pciDev;
};
