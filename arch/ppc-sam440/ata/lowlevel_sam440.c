#include <exec/types.h>
#include <asm/io.h>
#include <asm/amcc440.h>

VOID ata_out(UBYTE val, UWORD offset, IPTR port)
{
    outb(val, (uint8_t *)(port + offset + PCIC0_IO));
}

UBYTE ata_in(UWORD offset, IPTR port)
{
    return inb((uint8_t *)(port + offset + PCIC0_IO));
}

VOID ata_outl(ULONG val, UWORD offset, IPTR port)
{
    outl_le(val, (uint32_t *)(port + offset + PCIC0_IO));
}

VOID ata_insw(APTR address, UWORD port, ULONG count)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + PCIC0_IO);
    
    while(count)
    {
        *addr++ = inw(p);
        count -= 2;
    }
}

VOID ata_insl(APTR address, UWORD port, ULONG count)
{
    if (count & 2)
        ata_insw(address, port, count);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + PCIC0_IO);
        
        while(count)
        {
            *addr++ = inl(p);
            count -= 4;
        }
    }
}

VOID ata_outsw(APTR address, UWORD port, ULONG count)
{
    UWORD *addr = address;
    UWORD *p = (UWORD*)(port + PCIC0_IO);
    
    while(count)
    {
        outw(*addr++, p);
        count -= 2;
    }
}

VOID ata_outsl(APTR address, UWORD port, ULONG count)
{
    if (count & 2)
        ata_outsw(address, port, count);
    else
    {
        ULONG *addr = address;
        ULONG *p = (ULONG*)(port + PCIC0_IO);
        
        while(count)
        {
            outl(*addr++, p);
            count -= 4;
        }
    }
}

void ata_400ns(ULONG port)
{
    register ULONG tick_old, tick;
    
    asm volatile("mftbl %0":"=r"(tick_old));
    
    do {
	asm volatile("mftbl %0":"=r"(tick));	
    } while(tick < (tick_old + 60));
}
