/*
    Copyright © 2004, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <exec/tasks.h>
#include <exec/memory.h>
#include <utility/utility.h>
#include <oop/oop.h>
#include <libraries/expansion.h>
#include <libraries/configvars.h>

#include <dos/bptr.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/bootloader.h>
#include <proto/expansion.h>

#include "ata.h"
#include LC_LIBDEFS_FILE

static
AROS_UFP3(LIBBASETYPEPTR, ata_init,
    AROS_UFPA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFPA(BPTR, slist, A0),
    AROS_UFPA(struct ExecBase *, SysBase, A6));

static LONG __used __ata_entry(void) 
{
    return -1;
}

static const char ata_VersionID[] = VERSION_STRING;
static const char ata_Name[] = NAME_STRING;

static const APTR inittabl[4];
extern void *const LIBFUNCTABLE[];

/* What are these? Naah, just kidding ;) */
static const struct Resident ata_resident __used = {
    RTC_MATCHWORD,
    (struct Resident *)&ata_resident,
    (APTR)&LIBEND,
    RTF_AUTOINIT|RTF_COLDSTART,
    VERSION_NUMBER,
    NT_DEVICE,
    4,				    // Is the priority not too low here?
    (UBYTE*)ata_Name,
    (UBYTE*)&ata_VersionID[6],
    (ULONG*)inittabl
};

static const APTR inittabl[4] = {
    (APTR)sizeof(LIBBASETYPE),
    (APTR)LIBFUNCTABLE,
    NULL,
    &ata_init
};

/*
    Scan only the buses listed here. I'll probably throw it away and let
    specific driver modules do the thing
*/
static struct __bus {
    ULONG port;
    UBYTE irq;
} Buses[MAX_BUS] = {
    {0x1f0, 14},
    {0x170, 15},
    {0x168, 10},
    {0x1e8, 11},
};

#define SysBase (unit->au_Base->ata_SysBase)

/* Add a bootnode using expansion.library */
static BOOL AddVolume(ULONG StartCyl, ULONG EndCyl, struct ata_Unit *unit)
{
    struct ExpansionBase *ExpansionBase;
    struct DeviceNode *devnode;
    ULONG *pp;
    static int volnum;

    ExpansionBase = (struct ExpansionBase *)OpenLibrary("expansion.library",
                                                        40L);

    if (ExpansionBase)
    {
        pp = AllocMem(24*4, MEMF_PUBLIC | MEMF_CLEAR);

        if (pp)
        {
            /* This should be dealt with using some sort of volume manager or such. */
            switch (unit->au_DevType)
            {
                case DG_DIRECT_ACCESS:
                    pp[0] = (ULONG)"afs.handler";
                    break;
                case DG_CDROM:
                    pp[0] = (ULONG)"cdrom.handler";
                    break;
                default:
                    D(bug("IDE: AddVolume called on unknown devicetype\n"));
            }
            pp[1] = (ULONG)ata_Name;
            pp[2] = unit->au_UnitNum;
            pp[DE_TABLESIZE + 4] = DE_BOOTBLOCKS;
            pp[DE_SIZEBLOCK + 4] = 1 << (unit->au_SectorShift - 2);
            pp[DE_NUMHEADS + 4] = unit->au_Heads;
            pp[DE_SECSPERBLOCK + 4] = 1;
            pp[DE_BLKSPERTRACK + 4] = unit->au_Sectors;
            pp[DE_RESERVEDBLKS + 4] = 2;
            pp[DE_LOWCYL + 4] = StartCyl;
            pp[DE_HIGHCYL + 4] = EndCyl;
            pp[DE_NUMBUFFERS + 4] = 10;
            pp[DE_BUFMEMTYPE + 4] = MEMF_PUBLIC | MEMF_CHIP;
	    pp[DE_MAXTRANSFER + 4] = 0x00200000;
            pp[DE_MASK + 4] = 0x7FFFFFFE;
            pp[DE_BOOTPRI + 4] = ((!unit->au_DevType) ? 0 : 10);
            pp[DE_DOSTYPE + 4] = 0x444F5301;
            pp[DE_BOOTBLOCKS + 4] = 2;
            devnode = MakeDosNode(pp);

            if (devnode)
            {
                if ((devnode->dn_OldName =
                     MKBADDR(AllocMem(5, MEMF_PUBLIC | MEMF_CLEAR))))
                {
                    if( !unit->au_DevType )
                    {
                        AROS_BSTR_putchar(devnode->dn_OldName, 0, 'D');
                        AROS_BSTR_putchar(devnode->dn_OldName, 1, 'H');
                        AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + volnum);
                    }
                    else
                    {
                        AROS_BSTR_putchar(devnode->dn_OldName, 0, 'C');
                        AROS_BSTR_putchar(devnode->dn_OldName, 1, 'D');
                        AROS_BSTR_putchar(devnode->dn_OldName, 2, '0' + volnum);
                    }
                    AROS_BSTR_setstrlen(devnode->dn_OldName, 3);
                    devnode->dn_NewName = AROS_BSTR_ADDR(devnode->dn_OldName);

                    D(bug("-Adding volume %s with SC=%d, EC=%d\n",
                          &(devnode->dn_NewName[0]), StartCyl, EndCyl));
                    AddBootNode(pp[DE_BOOTPRI + 4], 0, devnode, 0);
                    volnum++;

                    return TRUE;
                }
            }
        }

        CloseLibrary((struct Library *)ExpansionBase);
    }

    return FALSE;
}
#undef SysBase

/*
    Here shall we start. Make function static as it shouldn't be visible from
    outside.
*/
static
AROS_UFH3(LIBBASETYPEPTR, ata_init,
    AROS_UFHA(LIBBASETYPEPTR, LIBBASE, D0),
    AROS_UFHA(BPTR, slist, A0),
    AROS_UFHA(struct ExecBase *, SysBase, A6))
{
    AROS_USERFUNC_INIT

    struct BootLoaderBase *BootLoaderBase;

    /*
	I've decided to use memory pools again. Alloc everything needed from 
	a pool, so that we avoid memory fragmentation.
    */
    LIBBASE->ata_SysBase = SysBase;
    LIBBASE->ata_MemPool = CreatePool(MEMF_CLEAR | MEMF_PUBLIC | MEMF_SEM_PROTECTED , 8192, 4096);

    D(bug("[ATA] ata.device initialization\n"));

    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase != NULL)
    {
	struct List *list;
	struct Node *node;

	list = (struct List *)GetBootInfo(BL_Args);
	if (list)
	{
	    ForeachNode(list, node)
	    {
		if (strncmp(node->ln_Name, "ATA=", 4) == 0)
		{
		    if (strstr(node->ln_Name, "32bit"))
		    {
			D(bug("[ATA] Using 32-bit IO transfers\n"));
			LIBBASE->ata_32bit = TRUE;
		    }
		    if (strstr(node->ln_Name, "forcedma"))
		    {
			D(bug("[ATA] DANGEROUS: Forcing DMA mode\n"));
			LIBBASE->ata_ForceDMA = TRUE;
		    }
		}
	    }
	}
    }

    if (LIBBASE->ata_MemPool)
    {
	LIBBASE->ata_UtilityBase = (struct UtilityBase *)OpenLibrary("utility.library",0);

	if (LIBBASE->ata_UtilityBase)
	{
	    if ((LIBBASE->ata_OOPBase = OpenLibrary("oop.library",0)))
	    {
		int i;
		struct ata_Bus ab;
		ab.ab_Base = LIBBASE;
		InitSemaphore(&ab.ab_Lock);
		
		/*
		    Go through all buses available and look for devices.
		    If at least one device is present, create ata_Bus structure
		    there and proper ata_Unit things.
		*/
    
		for (i=0; i<MAX_BUS; i++)
		{
		    /* Scan a bus in order to find anything on it. */
		    ab.ab_Port = Buses[i].port;
		    ab.ab_Irq  = Buses[i].irq;
		    ata_ScanBus(&ab);
		    
		    /* Is at least one of the devices there? */
		    if ((ab.ab_Dev[0] > DEV_UNKNOWN) | (ab.ab_Dev[1] > DEV_UNKNOWN))
		    {
			/* 
			    Yes, then prepare proper structures for it.  The 
			    bus structure first.
			*/
			struct ata_Bus *bus = 
				(struct ata_Bus *)AllocPooled(LIBBASE->ata_MemPool, sizeof(struct ata_Bus));

			/* Bus setup */
			bus->ab_Base = LIBBASE;
			InitSemaphore(&bus->ab_Lock);
			bus->ab_Port = ab.ab_Port;
			bus->ab_Irq  = ab.ab_Irq;
			bus->ab_Dev[0] = ab.ab_Dev[0];
			bus->ab_Dev[1] = ab.ab_Dev[1];
			bus->ab_IntHandler = (HIDDT_IRQ_Handler *)AllocPooled(LIBBASE->ata_MemPool,
			    sizeof(HIDDT_IRQ_Handler));

			/* PRD will be used later on by DMA. It's the table of all memory transfer requests */
			bus->ab_PRD = AllocPooled(LIBBASE->ata_MemPool, 8192 + 160);

			/* 
			    If PRD begins on one 64K page, but 514 prd entries (maximal transfer in
			    LBA48 mode) would not fit on this page, move the pointer to the next page.

			    We have allocated enough memory to be able to do that, allthough it's not
			    nice at all.
			*/
			if ((((IPTR)bus->ab_PRD + 0x10000) & 0xffff0000) - (IPTR)bus->ab_PRD < 514*8)
			{
			    bus->ab_PRD = (APTR)(((IPTR)bus->ab_PRD + 0xffff) & ~0xffff);
			}

			/* If the master is there, alloc Unit structure for it */
			if (bus->ab_Dev[0] > DEV_UNKNOWN) {
			    bus->ab_Units[0] = AllocPooled(LIBBASE->ata_MemPool, sizeof(struct ata_Unit));
			}
			/* If the slave is there, do the same */
			if (bus->ab_Dev[1] > DEV_UNKNOWN) {
			    bus->ab_Units[1] = AllocPooled(LIBBASE->ata_MemPool, sizeof(struct ata_Unit));
			}
			/* And store pointer to the bus in device's base */
			LIBBASE->ata_Buses[i] = bus;

			/* Anything on the bus? Prepare a task handling it */
			ata_InitBusTask(bus, i);
		    }
		}
		
		/*
		    All buses scanned here. Now it's time to init units, gather 
		    all information needed for device operation.
		*/
		ata_InitUnits(LIBBASE);
		/* Try to setup daemon task looking for diskchanges */
		ata_InitDaemonTask(LIBBASE);


		if (1)
		{
		    UBYTE bus;
		    for (bus=0; bus < MAX_BUS; bus++)
		    {
			UBYTE dev;

			if (LIBBASE->ata_Buses[bus])
			{
			    for (dev=0; dev < MAX_UNIT; dev++)
			    {
				struct ata_Unit *unit = LIBBASE->ata_Buses[bus]->ab_Units[dev];
				if (unit)
				{
				    if (unit->au_Flags & AF_ATAPI)
					AddVolume(0,0,unit);
				    else
					AddVolume(0,unit->au_Capacity,unit);
				}
			    }
			}
		    }
		}
		
		/* Dummy test removed later ;) */
		if(0)
		{
		    struct MsgPort *mp = CreateMsgPort();
		    struct MsgPort2 *mp2 = CreateMsgPort();
		    struct IOStdReq *ios = CreateIORequest(mp, sizeof(struct IOStdReq));
		    struct timerequest *timerio = CreateIORequest(mp2, sizeof(struct timerequest));
		    struct Device *TimerBase;

		    OpenDevice("timer.device", UNIT_VBLANK, timerio, 0);
		    TimerBase = timerio->tr_node.io_Device;

		    UBYTE *buffer = AllocMem(1024 * 1024, MEMF_PUBLIC | MEMF_CLEAR);
		    ULONG packet = 512;

		    /* And force OpenDevice call */
		    AROS_LVO_CALL3(void, 
			AROS_LCA(struct IORequest *, (struct IORequest *)ios, A1),
			AROS_LCA(ULONG, 0x0101, D0),
			AROS_LCA(ULONG, 0, D1),
			LIBBASETYPEPTR, LIBBASE, 1, ata);
		    
		    int i;

		    ULONG *buf = (ULONG*)buffer;

#if 0		    
		    D(bug("[ATA.test] Store first 1MB of data in RAM\n"));
		    ios->io_Command = CMD_READ;
		    ios->io_Data = buf;
		    ios->io_Offset = 0;
		    ios->io_Length = 1024*1024;
		    DoIO(ios);
#endif
#if 0
		    for (i=0; i < 1024*1024/4; i++)
			buf[i] = i*2 + 0xf0000000;
    
		    ios->io_Command = CMD_WRITE;
		    ios->io_Data = buf;
		    ios->io_Offset = 0;
		    ios->io_Length = 1024*1024;
		    DoIO(ios);
#endif
		    while (packet <= 1024*1024)
		    {
			int cnt = 0,count;
			ULONG time;
			struct timeval start, end;

			D(bug("[ATA.test] Read linear 1MB in packets of %d bytes: ", packet));
			ios->io_Command = CMD_READ;
			ios->io_Data = buffer;
			
			GetSysTime(&start);
			for (count=0; count < 10; count++)
			{
			    for (cnt=0; cnt < 1024*1024; cnt+=packet)
			    {
				ios->io_Offset = cnt;
				ios->io_Length = packet;

				DoIO(ios);
			    }
			}
			GetSysTime(&end);
			SubTime(&end,&start);

			time = (end.tv_secs*1000000 + end.tv_micro) / 10;

			D(bug("%dus, %dKB/s\n", time, (ULONG)(1024000000/time)));

			packet *= 2;
		    }
#if 0
		    D(bug("[ATA.test] Store first 1MB of data on drive again\n"));
		    ios->io_Command = CMD_WRITE;
		    ios->io_Data = buf;
		    ios->io_Offset = 0;
		    ios->io_Length = 1024*1024;
		    DoIO(ios);
#endif
		    D(bug("[ATA.test] Read 1MB of data at once\n"));
		    
		    ios->io_Command = CMD_READ;
		    ios->io_Data = buffer;
		    ios->io_Offset = 0;
		    ios->io_Length = 1024*1024;
		    DoIO(ios);

		    D(bug("[ATA.test] Read 1MB of data sector by sector\n"));

		    UBYTE buff[512];
		    ios->io_Data = buff;
		    ios->io_Length = 512;
		    for (packet = 0; packet < 1024*1024; packet += 512)
		    {
			int i;
			BOOL changed = FALSE;

			ios->io_Offset = packet;
			DoIO(ios);

			for (i=0; i < 512; i++)
			{
			    if (buff[i] != buffer[packet + i])
			    {
				changed = TRUE;
			    }
			}
			if (changed) {
			    D(bug("[ATA.test] failed read sector %d\n", packet >> 9));

//			    for (i=0; i<4; i++)
//			    {
//				D(bug("%02x : %02x ", buff[i], buffer[packet + i]));
//			    }
//			    while(1);
			}
		    }

		    D(bug("[ATA] sector 0x1: %s\n",buffer + 512));

		    D(bug("[ATA.test] all tests done\n"));
		    
		}

		return LIBBASE;
	    }
	    CloseLibrary((struct Library *)LIBBASE->ata_UtilityBase);
	}
	CloseLibrary(LIBBASE->ata_MemPool);
    }

    DeletePool(LIBBASE->ata_MemPool);

    return LIBBASE;

    AROS_USERFUNC_EXIT
}

AROS_LH1(BPTR, expunge,
    AROS_LHA(LIBBASETYPEPTR, LIBBASE, D0),
    struct ExecBase *, sysBase, 3, ata)
{
    AROS_LIBFUNC_INIT

    /*
	Cannot expunge now. If would be anyway quite complex expunge,
	as all the filesystems would have to been told that they should
	release (close) the device
    */
    ((struct Library *)LIBBASE)->lib_Flags |= LIBF_DELEXP;

    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH0I(ULONG, null,
    LIBBASETYPEPTR, LIBBASE, 4, ata)
{
    AROS_LIBFUNC_INIT

    return 0;

    AROS_LIBFUNC_EXIT
}

/*
    Called from exec's OpenDevice call - Open a device

    A note to device enumeration. I've decided not to could the devices one 
    after another, but instead to use the unit number in more amiga-scsi-like
    manner. The lowest 8 bits of the unit number defines the unit within one
    bus, whereas the bits 15:8 define the bus number. That is, the master device
    on bus 0 (0x1f0 IO address) would be the unit 0x0000, the slave on the same
    bus would be 0x0001. Similary, the master and slave on the second bus (0x170
    address) would be 0x0100 and 0x0101, respectively.
*/
AROS_LH3(void, open,
    AROS_LHA(struct IORequest *, iorq, A1),
    AROS_LHA(ULONG, unitnum, D0),
    AROS_LHA(ULONG, flags, D1),
    LIBBASETYPEPTR, LIBBASE, 1, ata)
{
    AROS_LIBFUNC_INIT

    ULONG bus, dev;
    
    /* Assume it failed */
    iorq->io_Error = IOERR_OPENFAIL;

    /* Extract bus and device numbers */
    bus = unitnum >> 1;			// 0xff00 >> 8
    dev = (unitnum & 0x1);		// 0x00ff

    /* Is the bus number within allowed range and is it allocated at all? */
    if ((bus < MAX_BUS) && (LIBBASE->ata_Buses[bus] != NULL))
    {
	/* Yes - good. The bus exists and there is something on it */
	struct ata_Bus *b = LIBBASE->ata_Buses[bus];

	/* Is the device within allowed range? Is memory allocated for it? */
	if ((dev < MAX_UNIT) && (b->ab_Units[dev] != NULL))
	{
	    /* Cool. Got the device here. */
	    struct ata_Unit *unit = b->ab_Units[dev];
	    
	    /* Prepare IORequest structure so that DoIO's will work */
	    iorq->io_Device = (struct Device *)LIBBASE;
	    iorq->io_Unit = (struct Unit *)unit;

	    /* Increase use counters and clear delayed expunge flag */
	    unit->au_Unit.unit_OpenCnt++;
	    LIBBASE->ata_Device.dd_Library.lib_OpenCnt++;
	    LIBBASE->ata_Device.dd_Library.lib_Flags &= ~LIBF_DELEXP;

	    /* All fine, no errors */
	    iorq->io_Error = 0;
	}
    }

    AROS_LIBFUNC_EXIT
}

/* Close given device */
AROS_LH1(ULONG, close,
    AROS_LHA(struct IORequest *, iorq, A1),
    LIBBASETYPEPTR, LIBBASE, 2, ata)
{
    AROS_LIBFUNC_INIT

    struct ata_Unit *unit = (struct ata_Unit *)iorq->io_Unit;

    /* First of all make the important fields of struct IORequest invalid! */
    iorq->io_Unit = (struct Unit *)~0;
    iorq->io_Device = (struct Device *)~0;
    
    /* Decrease use counters of unit and ata.device */
    unit->au_Unit.unit_OpenCnt--;
    LIBBASE->ata_Device.dd_Library.lib_OpenCnt--;

    /* Don't ever think about expunge yet ;) */
    
    return 0;

    AROS_LIBFUNC_EXIT
}

