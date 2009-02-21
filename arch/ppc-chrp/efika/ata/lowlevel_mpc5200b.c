#define DEBUG 0
#include <aros/debug.h>
#include <exec/types.h>
#include <asm/io.h>
#include <asm/mpc5200b.h>

UBYTE *mbar;

volatile ata_5k2_t *ata_5k2;

void ata_out(UBYTE val, UWORD offset, IPTR port)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	if (port +offset * 4 == 0x3a7c)
	{
		ULONG val = inl(&mbar[port + offset * 4]);
		outb(0, &mbar[port + 1 + offset * 4]);
	}

	outl((ULONG)val << 24, &mbar[port + offset * 4]);
}

UBYTE ata_in(UWORD offset, IPTR port)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	if (port +offset * 4 == 0x3a7c)
	{
		ULONG val = inl(&mbar[port + offset * 4]);
		outb(0, &mbar[port + 1 + offset * 4]);
	}

	return inl(&mbar[port + offset * 4]) >> 24;
}

void ata_outl(ULONG val, UWORD offset, IPTR port)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	outl_le(val, &mbar[port + offset * 4]);
}

VOID ata_insw(APTR address, UWORD port, ULONG count)
{
    UWORD *addr = address;
    volatile UWORD *p = (UWORD*)(&mbar[port]);

    D(bug("[ATA 5k2] insw(%08x, %04x)\n", address, count));

    count &= ~1;

	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	asm volatile("sync");
	while(count)
    {
//    	*addr++ = inw(p);
		*addr++ = *p;
        count -= 2;
    }
	asm volatile("sync");
}

VOID ata_insl(APTR address, UWORD port, ULONG count)
{
    ata_insw(address, port, count);
}

VOID ata_outsw(APTR address, UWORD port, ULONG count)
{
    UWORD *addr = address;
    volatile UWORD *p = (UWORD*)(&mbar[port]);

    D(bug("[ATA 5k2] outsw(%08x, %04x)\n", address, count));

    count &= ~1;

	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	asm volatile("sync");
	while(count)
    {
//    	outw(*addr++, p);
		*p = *addr++;
        count -= 2;
    }
	asm volatile("sync");
}

VOID ata_outsl(APTR address, UWORD port, ULONG count)
{
    ata_outsw(address, port, count);
}

void ata_400ns()
{
	register ULONG tick_old, tick;

	asm volatile("mftbl %0":"=r"(tick_old));

	do {
		asm volatile("mftbl %0":"=r"(tick));
	} while(tick < (tick_old + 15));
}
