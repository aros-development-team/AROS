/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: PCI bus driver for ata.device
    Lang: English
*/

#include <aros/asmcall.h>
#include <aros/debug.h>
#include <asm/io.h>
#include <exec/lists.h>
#include <hardware/ahci.h>
#include <hidd/pci.h>
#include <oop/oop.h>
#include <proto/exec.h>
#include <proto/oop.h>

#include <string.h>

#include "ata.h"
#include "pci.h"

/*
 * Low-level bus I/O functions.
 * They are called directly via array of pointers supplied by our driver.
 * This should be kept this way even if bus driver part is made into a HIDD
 * some day, for speedup.
 */

#if defined(__i386__) || defined(__x86_64__)

/* On x86 ATA devices use I/O space, data is ignored. */

static void ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    outb(val, offset + port);
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    return inb(offset + port);
}

static void ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
    outl(val, offset + port);
}

static VOID ata_insw(APTR address, UWORD port, ULONG count, APTR data)
{
    insw(port, address, count >> 1);
}

static VOID ata_insl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        insw(port, address, count >> 1);
    else
        insl(port, address, count >> 2);
}

static VOID ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    outsw(port, address, count >> 1);
}

static VOID ata_outsl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        outsw(port, address, count >> 1);
    else
        outsl(port, address, count >> 2);
}

#else

/*
 * This came from SAM440 port. Data is a base address of mapped ISA I/O space.
 * I believe this should work fine on all non-x86 architectures.
 */

static VOID ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
    outb(val, (uint8_t *)(port + offset + data));
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
    return inb((uint8_t *)(port + offset + data));
}

static VOID ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
    outl_le(val, (uint32_t *)(port + offset + data));
}

static VOID ata_insw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        *addr++ = inw(p);
        count -= 2;
    }
}

static VOID ata_insl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        ata_insw(address, port, count, data);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + data);
        
        while(count)
        {
            *addr++ = inl(p);
            count -= 4;
        }
    }
}

static VOID ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + data);
    
    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }
}

static VOID ata_outsl(APTR address, UWORD port, ULONG count, APTR data)
{
    if (count & 2)
        ata_outsw(address, port, count, data);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + data);
        
        while(count)
        {
            outl(*addr++, p);
            count -= 4;
        }
    }
}

#endif

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

static const struct ata_BusDriver pci_driver = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insl,
    ata_outsl,
    CreateInterrupt
};
