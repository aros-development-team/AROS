/*
    Copyright © 2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: A600/A1200/A4000 ATA HIDD PIO interface functions
    Lang: English
*/
#include <aros/debug.h>

#define DIO(x)
#define DDATA(x)

#include "interface_pio.h"

static void ata_out(struct pio_data *data, UBYTE val, UWORD offset)
{
    volatile UBYTE *addr;
    addr = data->port;
    DIO(bug("%p REG %d <- %02X\n", addr, offset, val));
    addr[offset * 4] = val;
}

static UBYTE ata_in(struct pio_data *data, UWORD offset)
{
    volatile UBYTE *addr;
    UBYTE v;

    addr = data->port;
    v = addr[offset * 4];
    DIO(bug("%p REG %d -> %02X\n", addr, offset, v));
    return v;
}

static void ata_outsw(struct pio_data *data, APTR address, ULONG count)
{
    volatile UWORD *addr = (UWORD*)data->dataport;

    DDATA(bug("WOUT %p %p %d\n", addr, address, count));

    asm volatile(
"1:     move.w (%[address])+,(%[port])  \n"
"       move.w (%[address])+,(%[port])  \n"
"       subq.l #1,%[count]              \n"
"       bnes 1b                         \n"
        ::[count]"d"(count >> 2),[address]"a"(address),[port]"a"(addr));
}

static void ata_outsl(struct pio_data *data, APTR address, ULONG count)
{
    volatile ULONG *addr = (ULONG*)data->dataport;

    DDATA(bug("LOUT %p %p %d\n", addr, address, count));

    asm volatile(
"1:     move.l (%[address])+,(%[port])  \n"
"       move.l (%[address])+,(%[port])  \n"
"       subq.l #1,%[count]              \n"
"       bnes 1b                         \n"
        ::[count]"d"(count >> 3),[address]"a"(address),[port]"a"(addr));
}

static void ata_insw(struct pio_data *data, APTR address, ULONG count)
{
    volatile UWORD *addr = (UWORD*)data->dataport;

    DDATA(bug("WIN %p %p %d\n", addr, address, count));

    asm volatile(
"1:     move.w (%[port]),(%[address])+  \n"
"       move.w (%[port]),(%[address])+  \n"
"       subq.l #1,%[count]              \n"
"       bnes 1b                         \n"
        ::[count]"d"(count >> 2),[address]"a"(address),[port]"a"(addr));
}

static void ata_insl(struct pio_data *data, APTR address, ULONG count)
{
    volatile ULONG *addr = (ULONG*)data->dataport;

    DDATA(bug("LIN %p %p %d\n", addr, address, count));

    asm volatile(
"1:     move.l (%[port]),(%[address])+  \n"
"       move.l (%[port]),(%[address])+  \n"
"       subq.l #1,%[count]              \n"
"       bnes 1b                         \n"
        ::[count]"d"(count >> 3),[address]"a"(address),[port]"a"(addr));
}

const APTR bus_FuncTable[] =
{
    ata_out,
    ata_in,
    (APTR *)-1
};

const APTR pio_FuncTable[] =
{
    ata_outsw,
    ata_insw,
    ata_outsl,
    ata_insl,
    (APTR *)-1
};
