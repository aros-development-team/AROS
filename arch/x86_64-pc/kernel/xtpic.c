#include <asm/io.h>

#include <inttypes.h>

#include "kernel_base.h"
#include "kernel_intern.h"
#include "xtpic.h"

void core_XTPIC_DisableIRQ(uint8_t irqnum, struct PlatformData *pd)
{
    irqnum &= 15;
    pd->kb_XTPIC_Mask |= 1 << irqnum;

    if (irqnum >= 8)
        outb((pd->kb_XTPIC_Mask >> 8) & 0xff, 0xA1);
    else
        outb(pd->kb_XTPIC_Mask & 0xff, 0x21);
}

void core_XTPIC_EnableIRQ(uint8_t irqnum, struct PlatformData *pd)
{
    irqnum &= 15;
    pd->kb_XTPIC_Mask &= ~(1 << irqnum);    

    if (irqnum >= 8)
        outb((pd->kb_XTPIC_Mask >> 8) & 0xff, 0xA1);
    else
        outb(pd->kb_XTPIC_Mask & 0xff, 0x21);
}

void core_XTPIC_AckIntr(uint8_t intnum, struct PlatformData *pd)
{
    intnum &= 15;
    pd->kb_XTPIC_Mask |= 1 << intnum;

    if (intnum >= 8)
    {
        outb((pd->kb_XTPIC_Mask >> 8) & 0xff, 0xA1);
        outb(0x62, 0x20);
        outb(0x20, 0xa0);
    }
    else
    {
        outb(pd->kb_XTPIC_Mask & 0xff, 0x21);
        outb(0x20, 0x20);
    }
}