/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI IDE PIO interface functions
    Lang: English
*/

#include "interface_pio.h"

void ata_out(struct pio_data *data, UBYTE val, UWORD offset)
{
    outb(val, offset + data->ioBase);
}

UBYTE ata_in(struct pio_data *data, UWORD offset)
{
    return inb(offset + data->ioBase);
}

void ata_out_alt(struct pio_data *data, UBYTE val, UWORD offset)
{
    outb(val, offset + data->ioAlt);
}

UBYTE ata_in_alt(struct pio_data *data, UWORD offset)
{
    return inb(offset + data->ioAlt);
}

#if defined(__i386__) || defined(__x86_64__)

void ata_outsw(struct pio_data *data, APTR address, ULONG count)
{
    outsw(data->ioBase, address, count >> 1);
}

void ata_insw(struct pio_data *data, APTR address, ULONG count)
{
    insw(data->ioBase, address, count >> 1);
}

void ata_outsl(struct pio_data *data, APTR address, ULONG count)
{
    if (count & 2)
        outsw(data->ioBase, address, count >> 1);
    else
        outsl(data->ioBase, address, count >> 2);
}

void ata_insl(struct pio_data *data, APTR address, ULONG count)
{
    if (count & 2)
        insw(data->ioBase, address, count >> 1);
    else
        insl(data->ioBase, address, count >> 2);
}

#else

void ata_outsw(struct pio_data *data, APTR address, ULONG count)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);

    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }
}

void ata_insw(struct pio_data *data, APTR address, ULONG count)
{
    UWORD *addr = address;
    
    while(count)
    {
        *addr++ = inw(data->ioBase);
        count -= 2;
    }
}

void ata_outsl(struct pio_data *data, APTR address, ULONG count)
{
    if (count & 2)
    {
        ata_outsw(address, port, count, data);
    }
    else
    {
        ULONG *addr = address;
        
        while(count)
        {
            outl(*addr++, data->ioBase);
            count -= 4;
        }
    }
}

void ata_insl(struct pio_data *data, APTR address, ULONG count)
{
    if (count & 2)
    {
        ata_insw(address, count, data);
    }
    else
    {
        ULONG *addr = address;
        
        while(count)
        {
            *addr++ = inl(data->ioBase);
            count -= 4;
        }
    }
}

#endif

const APTR pio_FuncTable[] =
{
    ata_out,
    ata_in,
    ata_out_alt,
    ata_in_alt,
    ata_outsw,
    ata_insw,
    ata_outsl,
    ata_insl,
    (APTR *)-1
};
