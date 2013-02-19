/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI IDE PIO interface functions
    Lang: English
*/

#include <aros/libcall.h>

#include "interface_pio.h"

AROS_LH2(void, ata_out,
          AROS_LHA(UBYTE, val, D0),
          AROS_LHA(UWORD, offset, D1),
          struct pio_data *, data, 1, PCIATA)
{
    AROS_LIBFUNC_INIT

    outb(val, offset + data->ioBase);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(UBYTE, ata_in,
          AROS_LHA(UWORD, offset, D1),
          struct pio_data *, data, 2, PCIATA)
{
    AROS_LIBFUNC_INIT

    return inb(offset + data->ioBase);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_out_alt,
          AROS_LHA(UBYTE, val, D0),
          AROS_LHA(UWORD, offset, D1),
          struct pio_data *, data, 3, PCIATA)
{
    AROS_LIBFUNC_INIT

    outb(val, offset + data->ioAlt);

    AROS_LIBFUNC_EXIT
}

AROS_LH1(UBYTE, ata_in_alt,
          AROS_LHA(UWORD, offset, D1),
          struct pio_data *, data, 4, PCIATA)
{
    AROS_LIBFUNC_INIT

    return inb(offset + data->ioAlt);

    AROS_LIBFUNC_EXIT
}

#if defined(__i386__) || defined(__x86_64__)

AROS_LH2(void, ata_outsw,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 5, PCIATA)
{
    AROS_LIBFUNC_INIT

    outsw(port, address, count >> 1);

    AROS_LIBFUNC_EXIT    
}

AROS_LH2(void, ata_insw,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 6, PCIATA)
{
    AROS_LIBFUNC_INIT

    insw(data->ioBase, address, count >> 1);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_outsl,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 5, PCIATA)
{
    AROS_LIBFUNC_INIT

    if (count & 2)
        outsw(data->ioBase, address, count >> 1);
    else
        outsl(data->ioBase, address, count >> 2);

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_insl,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 6, PCIATA)
{
    AROS_LIBFUNC_INIT

    if (count & 2)
        insw(data->ioBase, address, count >> 1);
    else
        insl(data->ioBase, address, count >> 2);

    AROS_LIBFUNC_EXIT
}

#else

AROS_LH2(void, ata_outsw,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 5, PCIATA)
{
    AROS_LIBFUNC_INIT

    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);

    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_insw,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 6, PCIATA)
{
    AROS_LIBFUNC_INIT

    UWORD *addr = address;
    
    while(count)
    {
        *addr++ = inw(data->ioBase);
        count -= 2;
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_outsl,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 5, PCIATA)
{
    AROS_LIBFUNC_INIT

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
    
    AROS_LIBFUNC_EXIT
}

AROS_LH2(void, ata_insl,
          AROS_LHA(APTR, address, A0),
          AROS_LHA(ULONG, count, D0),
          struct pio_data *, data, 6, PCIATA)
{
    AROS_LIBFUNC_INIT

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

    AROS_LIBFUNC_EXIT
}

#endif

AROS_LH0(void, ata_ackInt,
          struct pio_data *, data, 7, PCIATA)
{
    AROS_LIBFUNC_INIT

    /* Nothing to do here for PCI */

    AROS_LIBFUNC_EXIT
}

AROS_INTH1(ata_Interrupt, APTR, data)
{
    AROS_INTFUNC_INIT
    /*
     * Our interrupt handler should call this function.
     * It's our problem how to store bus pointer. Here we use h_Data for it.
     */
    ata_HandleIRQ(data);

    return FALSE;

    AROS_INTFUNC_EXIT
}

/* Actually a quick hack. Proper implementation really needs HIDDizing this code. */
static BOOL CreateInterrupt(struct ata_Bus *bus)
{
    struct Interrupt *IntHandler = &bus->ab_IntHandler;

    /*
        Prepare nice interrupt for our bus. Even if interrupt sharing is enabled,
        it should work quite well
    */
    IntHandler->is_Node.ln_Type = NT_INTERRUPT;
    IntHandler->is_Node.ln_Pri = 10;
    IntHandler->is_Node.ln_Name = bus->ab_Task->tc_Node.ln_Name;
    IntHandler->is_Code = (VOID_FUNC)ata_Interrupt;
    IntHandler->is_Data = bus;

    AddIntServer(INTB_KERNEL + bus->ab_IRQ, IntHandler);
    return TRUE;
}

const APTR pio_FuncTable[] =
{
    AROS_SLIB_ENTRY(ata_out    , PCIATA, 1),
    AROS_SLIB_ENTRY(ata_in     , PCIATA, 2),
    AROS_SLIB_ENTRY(ata_out_alt, PCIATA, 3),
    AROS_SLIB_ENTRY(ata_in_alt , PCIATA, 4),
    AROS_SLIB_ENTRY(ata_outsw  , PCIATA, 5),
    AROS_SLIB_ENTRY(ata_insw   , PCIATA, 6),
    AROS_SLIB_ENTRY(ata_ackInt , PCIATA, 7),
    (APTR)-1
};

const APTR pio32_FuncTable[] =
{
    AROS_SLIB_ENTRY(ata_outsl  , PCIATA, 5),
    AROS_SLIB_ENTRY(ata_insl   , PCIATA, 6)
};
