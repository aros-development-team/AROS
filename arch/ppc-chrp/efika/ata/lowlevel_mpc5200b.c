#define DEBUG 0

#include <aros/debug.h>
#include <exec/types.h>
#include <asm/io.h>
#include <asm/mpc5200b.h>
#include <exec/exec.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <exec/nodes.h>
#include <utility/utility.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/oop.h>
#include <proto/openfirmware.h>

#include "ata.h"

extern uint8_t *sram;
extern bestcomm_t *bestcomm;
extern uint32_t bestcomm_taskid;

extern void bestcomm_init();

UBYTE *mbar;
uint32_t bus_frequency;

volatile ata_5k2_t *ata_5k2;

static void ata_out(UBYTE val, UWORD offset, IPTR port, APTR data)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	if (port +offset * 4 == 0x3a7c)
	{
		ULONG val = inl(&mbar[port + offset * 4]);
		outb(0, &mbar[port + 1 + offset * 4]);
	}

	outl((ULONG)val << 24, &mbar[port + offset * 4]);
}

static UBYTE ata_in(UWORD offset, IPTR port, APTR data)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	if (port +offset * 4 == 0x3a7c)
	{
		ULONG val = inl(&mbar[port + offset * 4]);
		outb(0, &mbar[port + 1 + offset * 4]);
	}

	return inl(&mbar[port + offset * 4]) >> 24;
}

static void ata_outl(ULONG val, UWORD offset, IPTR port, APTR data)
{
	while(inl(&ata_5k2->ata_status) & 0x80000000) asm volatile("nop");

	outl_le(val, &mbar[port + offset * 4]);
}

static VOID ata_insw(APTR address, UWORD port, ULONG count, APTR data)
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

static VOID ata_outsw(APTR address, UWORD port, ULONG count, APTR data)
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

static void ata_400ns()
{
	register ULONG tick_old, tick;

	asm volatile("mftbl %0":"=r"(tick_old));

	do {
		asm volatile("mftbl %0":"=r"(tick));
	} while(tick < (tick_old + 15));
}

static AROS_UFIH1(ata_Interrupt, void *, data)
{
    AROS_USERFUNC_INIT
    /*
     * Our interrupt handler should call this function.
     * It's our problem how to store bus pointer. Here we use h_Data for it.
     */
    ata_HandleIRQ(data);

    return FALSE;

    AROS_USERFUNC_EXIT
}

/* Actually a quick hack. Proper implementation really needs HIDDizing this code. */
static BOOL CreateInterrupt(struct ata_Bus *bus)
{
    struct Interrupt *IntHandler = &bus->ab_IntHandler;

    /*
        Prepare nice interrupt for our bus. Even if interrupt sharing is enabled,
        it should work quite well
    */
    IntHandler->is_Node.ln_Pri = 10;
    IntHandler->is_Node.ln_Name = bus->ab_Task->tc_Node.ln_Name;
    IntHandler->is_Code = (VOID_FUNC)ata_Interrupt;
    IntHandler->is_Data = bus;

    AddIntServer(INTB_KERNEL + bus->ab_IRQ, IntHandler);
        
    return TRUE;
}

static const struct ata_BusDriver mpc_driver = 
{
    ata_out,
    ata_in,
    ata_outl,
    ata_insw,
    ata_outsw,
    ata_insw,	/* These are intentionally the same as 16-bit routines */
    ata_outsw,
    CreateInterrupt
};

static int ata_mpc_init(struct ataBase *LIBBASE)
{
    int i;
    /*
     * I've decided to use memory pools again. Alloc everything needed from
     * a pool, so that we avoid memory fragmentation.
     */
    LIBBASE->ata_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);
    if (LIBBASE->ata_MemPool == NULL)
        return FALSE;

    void *OpenFirmwareBase = OpenResource("openfirmware.resource");
    void *key = OF_OpenKey("/builtin");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "reg");
    	if (prop)
    	{
    		intptr_t *m_ = OF_GetPropValue(prop);
			mbar = (UBYTE *)*m_;
			ata_5k2 = (ata_5k2_t *)(*m_ + 0x3a00);

    		D(bug("[ATA] MBAR located at %08x\n", mbar));
    	}

    	/* Get the bus frequency for Efika */
    	prop = OF_FindProperty(key, "bus-frequency");
    	if (prop)
    	{
    		bus_frequency = *(uint32_t *)OF_GetPropValue(prop);
    		D(bug("[ATA] bus frequency: %d\n", bus_frequency));
    	}
    }

    key = OF_OpenKey("/builtin/ata");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "reg");
		if (prop)
		{
			ata_5k2 = *(ata_5k2_t **)OF_GetPropValue(prop);

			D(bug("[ATA] ATA registers at %08x\n", ata_5k2));
		}
    }

    key = OF_OpenKey("/builtin/ata/bestcomm-task");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "taskid");
		if (prop)
		{
			bestcomm_taskid = *(uint32_t *)OF_GetPropValue(prop);

			D(bug("[ATA] ATA uses bestcomm task %d\n", bestcomm_taskid));
		}
    }

    key = OF_OpenKey("/builtin/sram");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "reg");
    	if (prop)
    	{
    		sram = *(void **)OF_GetPropValue(prop);
    	}
    	D(bug("[ATA] SRAM at %08x\n", sram));
    }

    key = OF_OpenKey("/builtin/bestcomm");
    if (key)
    {
    	void *prop = OF_FindProperty(key, "reg");
    	if (prop)
    	{
    		bestcomm = *(void **)OF_GetPropValue(prop);
    	}
    	D(bug("[ATA] bestcomm at %08x\n", bestcomm));
    }

    D(bug("[ATA] ata_config=%08x\n", inl(&ata_5k2->ata_config)));
    D(bug("[ATA] ata_status=%08x\n", inl(&ata_5k2->ata_status)));
    D(bug("[ATA] ata_pio1=%08x\n", inl(&ata_5k2->ata_pio1)));
    D(bug("[ATA] ata_pio2=%08x\n", inl(&ata_5k2->ata_pio2)));

    /* Disable XLB pipelining... */
    D(bug("[ATA] xlb_config=%08x\n", inl(mbar+0x1f40)));
    outl(inl(mbar + 0x1f40) | 0x80000000, mbar + 0x1f40);

    outl(0, &ata_5k2->ata_invalid);
    outl(0xc3000000, &ata_5k2->ata_config);

    for (i=0; i < 100 / 4; i++)
    	ata_400ns();

	/* Hacky timing pokes */
    outl(0x03000000, &ata_5k2->ata_config);
    outl(132 << 16, &ata_5k2->ata_invalid);

    /* PIO2 timing table. Replace it by correct calculations soon !!! */

#warning TODO: Set the timings in right way!

    outl(0x21270e00, &ata_5k2->ata_pio1);
    outl(0x03050600, &ata_5k2->ata_pio2);

    bestcomm_init();

    /*
     * FIXME: This code uses static data variables.
     * Move them into DriverData instead.
     */
    ata_RegisterBus(0x3a60, 0x3a5c - 8, MPC5200B_ATA, 0, FALSE, &mpc_driver, NULL, LIBBASE);

    return TRUE;
}

ADD2INITLIB(ata_mpc_init, 20)
