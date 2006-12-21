/*
    Copyright © 2004-2006, The AROS Development Team. All rights reserved
    $Id$

    Desc:
    Lang: English
*/

/*
 * CHANGELOG:
 * DATE        NAME                ENTRY
 * ----------  ------------------  -------------------------------------------------------------------
 * 2006-12-20  T. Wiszkowski       Updated ATA Packet Interface to handle ATAPI/SCSI Commands
 *
 */

#define DEBUG 1
#include <aros/debug.h>

#include <exec/types.h>
#include <exec/exec.h>
#include <exec/resident.h>
#include <utility/utility.h>
#include <oop/oop.h>

#include <dos/bptr.h>

#include <proto/exec.h>
#include <devices/timer.h>

#include "ata.h"

/*
    Prototypes of static functions from lowlevel.c. I do not want to make them
    non-static as I'd like to remove as much symbols from global table as possible.
    Besides some of this functions could conflict with old ide.device or any other
    device.
*/
static ULONG ata_ReadSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_ReadMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_ReadDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_ReadDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteSector32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteSector64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteMultiple32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteMultiple64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_WriteDMA32(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG ata_WriteDMA64(struct ata_Unit *, UQUAD, ULONG, APTR, ULONG *);
static ULONG ata_Eject(struct ata_Unit *);

static ULONG atapi_ErrCmd();
static ULONG atapi_EndCmd(struct ata_Unit *unit);

static ULONG atapi_Read(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG atapi_Write(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG atapi_Eject(struct ata_Unit *);
static ULONG atapi_ReadDMA(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);
static ULONG atapi_WriteDMA(struct ata_Unit *, ULONG, ULONG, APTR, ULONG *);

/*
    Again piece of code which shouldn't be here. Geee. After removing all this 
    asm constrictuins this ata.device will really deserve for location in 
    /arch/common
*/
static VOID insw(APTR address, UWORD port, ULONG count)
{
    asm volatile ("rep insw"::"Nd"(port),"c"(count >> 1),"D"(address):"memory");
}

static VOID insl(APTR address, UWORD port, ULONG count)
{
    asm volatile ("rep insl"::"Nd"(port),"c"(count >> 2),"D"(address));
}

static VOID outsw(APTR address, UWORD port, ULONG count)
{
    asm volatile ("rep outsw"::"Nd"(port),"c"(count >> 1),"S"(address));
}

static VOID outsl(APTR address, UWORD port, ULONG count)
{
    asm volatile ("rep outsl"::"Nd"(port),"c"(count >> 2),"S"(address));
}

void ata_usleep(struct timerequest *tr, ULONG usec)
{
    tr->tr_node.io_Command = TR_ADDREQUEST;
    tr->tr_time.tv_micro = usec % 1000000;
    tr->tr_time.tv_secs = usec / 1000000;
    
    DoIO((struct IORequest *)tr);
}

/*
    Very short delay (TM)
*/
VOID ata_400ns(VOID)
{
    ata_in(ata_Control, 0x1f0);
    ata_in(ata_Control, 0x1f0);
    ata_in(ata_Control, 0x1f0);
    ata_in(ata_Control, 0x1f0);
}

static void ata_strcpy(const UBYTE *str1, UBYTE *str2, ULONG size)
{
    const UWORD *s1 = (UWORD *)str1;
    UWORD *s2 = (UWORD *)str2;
    int i = size>>1;

    while(i--)
    {
	s2[i] = s1[i] << 8 | s1[i] >> 8;
    }

    str2[size] = '\0';

    while (size--)
    {
	if (str2[size] != ' ' && str2[size] != '\0')
	{
	    str2[size+1] = '\0';
	    break;
	}
    }
}

/*
    DO NOT EVER LOOK AT THIS ONE. IT'S BADLY WRITTEN, UGLY AND WILL BE REPLACED
    AS SOON AS POSSIBLE!!!

    It simply initialize ata_Unit structures of all available devices, and 
    perhaps does some tiny setup (like changing read/write functions to the 
    faster ones)
*/
void ata_InitUnits(LIBBASETYPEPTR LIBBASE)
{
    int b,u;
    struct ata_Bus *bus=NULL;
    struct ata_Unit *unit=NULL;

    struct MsgPort *port = CreateMsgPort();
    struct timerequest *tr = (struct timerequest *)CreateIORequest((struct MsgPort *)port,
        sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0);

    for (b=0; b < MAX_BUS; b++)
    {
	if (LIBBASE->ata_Buses[b])
	{
	    bus = LIBBASE->ata_Buses[b];
	    dma_Init(bus);

	    for (u=0; u < MAX_UNIT; u++)
	    {
		if (bus->ab_Units[u])
		{
		    D(bug("%s: bus %ld, unit %ld: checking unit\n", __PRETTY_FUNCTION__, b, u));
		    unit = bus->ab_Units[u];
		    unit->au_Base = LIBBASE;
		    unit->au_Bus  = bus;
		    unit->au_Drive = AllocPooled(LIBBASE->ata_MemPool, sizeof(struct DriveIdent));
		    unit->au_UnitNum = b << 1 | u;	// b << 8 | u
		    unit->au_DevMask = 0xa0 | (u << 4);

		    if (LIBBASE->ata_32bit)
		    {
		        D(bug("%s: bus %ld, unit %ld: using 32bit io\n", __PRETTY_FUNCTION__, b, u));
			unit->au_ins = insl;
			unit->au_outs = outsl;
		    }
		    else
		    {
		        D(bug("%s: bus %ld, unit %ld: using 16bit io\n", __PRETTY_FUNCTION__, b, u));
			unit->au_ins = insw;
			unit->au_outs = outsw;
		    }

		    NEWLIST(&unit->au_SoftList);
	
		    /* Disable IRQ, select device */
		    ata_out(0x0a, ata_Control, bus->ab_Port);
		    ata_out(unit->au_DevMask, ata_DevHead, bus->ab_Port);
		    ata_usleep(tr, 200);
		    while (ata_in(ata_Status, bus->ab_Port) & ATAF_BUSY);
	
		    if (bus->ab_Dev[u] > DEV_ATA)
		    {
		        D(bug("%s: bus %ld, unit %ld: device seems to be an atapi optical device (?)\n", __PRETTY_FUNCTION__, b, u));
		        unit->au_SectorShift = 11;

			unit->au_Read32 = atapi_Read;
			unit->au_Read64 = NULL;
			unit->au_Write32 = atapi_Write;
			unit->au_Write64 = NULL;
			unit->au_Eject = atapi_Eject;

			unit->au_Flags |= AF_ATAPI | AF_DiscPresenceUnknown;
			ata_out(ATA_IDENTIFY_ATAPI, ata_Command, bus->ab_Port);
		    }
		    else
		    {
		        D(bug("%s: bus %ld, unit %ld: device seems to be a direct access device (?)\n", __PRETTY_FUNCTION__, b, u));
			unit->au_DevType = DG_DIRECT_ACCESS;
		        unit->au_SectorShift = 9;

			unit->au_Flags |= AF_DiscPresent;

			unit->au_Read32 = ata_ReadSector32;
			unit->au_Read64 = ata_ReadSector64;
			unit->au_Write32 = ata_WriteSector32;
			unit->au_Write64 = ata_WriteSector64;
			unit->au_Eject = ata_Eject;
		
			ata_out(ATA_IDENTIFY_DEVICE, ata_Command, bus->ab_Port);
		    }
		    
		    while (ata_in(ata_Status, bus->ab_Port) & ATAF_BUSY);
	
		    if (ata_in(ata_Status, bus->ab_Port) & ATAF_DATAREQ)
		    {
			unit->au_ins(unit->au_Drive, bus->ab_Port, 512);
		    }
		    
		    ata_strcpy(unit->au_Drive->id_Model, unit->au_Model, 40);
		    ata_strcpy(unit->au_Drive->id_SerialNumber, unit->au_SerialNumber, 20);
		    ata_strcpy(unit->au_Drive->id_FirmwareRev, unit->au_FirmwareRev, 8);

		    if (unit->au_Drive->id_General & 0x80)
		    {
			unit->au_Flags |= AF_Removable;
		    }

		    if (unit->au_Flags & AF_ATAPI)
		    {
			UBYTE type = (unit->au_Drive->id_General >> 8) & 0x1f;
			unit->au_DevType = type;
		        
			D(bug("%s: bus %ld, unit %ld: device type: %ld\n", __PRETTY_FUNCTION__, b, u, type));
		 	
			switch (type)
			{
			    case DG_CDROM:
			    case DG_WORM:
			    case DG_OPTICAL_DISK:
				unit->au_SectorShift = 11;
				unit->au_Flags |= AF_SlowDevice;
				unit->au_Heads = 1;
				unit->au_Sectors = 75;
				unit->au_Cylinders = 4440;
				break;

			    case DG_DIRECT_ACCESS:
				unit->au_SectorShift = 9;
				if (!strcmp("LS-120", &unit->au_Model[0]))
				{
				    unit->au_Flags |= AF_SlowDevice;
				    unit->au_Heads = 2;
				    unit->au_Sectors = 18;
				    unit->au_Cylinders = 6848;
				}
				else if (!strcmp("ZIP 100 ", &unit->au_Model[8]))
				{
				    unit->au_Flags &= ~AF_SlowDevice;
				    unit->au_Heads = 1;
				    unit->au_Sectors = 64;
				    unit->au_Cylinders = 3072;
				}
				break;
			}
			
		    }
		    
		    unit->au_NumLoop = unit->au_Flags & AF_ATAPI ?
			10000000 : 4000000;

		    D(bug("[ATA] Unit %03x %s '%s'", unit->au_UnitNum, 
			unit->au_Flags & AF_ATAPI ? "ATAPI":"ATA",unit->au_Model));

		    if (!(unit->au_Flags & AF_ATAPI))
		    {
			/*
			    For drive capacities > 8.3GB assume maximal possible layout.
			    It really doesn't matter here, as BIOS will not handle them in
			    CHS way anyway :)
			*/
			if (unit->au_Drive->id_LBASectors > (63*256*1024))
			{
			    unit->au_Cylinders = unit->au_Drive->id_LBASectors / (255*63);
			    unit->au_Heads = 255;
			    unit->au_Sectors = 63;
			}
			else
			{
			    unit->au_Cylinders = unit->au_Drive->id_OldLCylinders;
			    unit->au_Heads = unit->au_Drive->id_OldLHeads;
			    unit->au_Sectors = unit->au_Drive->id_OldLSectors;
			}

			unit->au_Capacity = unit->au_Drive->id_LBASectors;
		    }

		    /* If the drive supports Read/Write multiple, use them in 32-bit operations */
		    if (unit->au_Drive->id_RWMultipleSize)
		    {
			D(bug(" multi=%d",
			    unit->au_Drive->id_RWMultipleSize & 0xff));
			ata_out(unit->au_Drive->id_RWMultipleSize & 0xff, ata_Count, bus->ab_Port);
			ata_out(ATA_SET_MULTIPLE, ata_Command, bus->ab_Port);
			while (ata_in(ata_Status, bus->ab_Port) & ATAF_BUSY);
			unit->au_Read32 = ata_ReadMultiple32;
			unit->au_Read64 = ata_ReadMultiple64;
			unit->au_Write32 = ata_WriteMultiple32;
			unit->au_Write64 = ata_WriteMultiple64;
		    }

		    {
			if (unit->au_Drive->id_Capabilities & 0x200)
			    D(bug(", LBA"));
			if (unit->au_Drive->id_Capabilities & 0x100)
			    D(bug(", DMA"));
			if (unit->au_Drive->id_ConfigAvailable & 2)
			{
			    D(bug(", PIO%d",
				unit->au_Drive->id_PIOSupport+2));
			    if (unit->au_Drive->id_Commands2 & 0x400)
			    {
				D(bug(", LBA48"));
				unit->au_Capacity48 = unit->au_Drive->id_LBA48Sectors;
			    }

			    if (unit->au_Drive->id_ConfigAvailable & 4)
			    {
				D(bug(", UDMA=%x",
				    unit->au_Drive->id_UDMASupport));
				if (unit->au_Drive->id_UDMASupport & 0xff00)
				{
				    if (!(unit->au_Flags & AF_ATAPI) && LIBBASE->ata_ForceDMA)
				    {
					if (!(unit->au_Flags & AF_ATAPI))
					{
					    unit->au_Read32 = ata_ReadDMA32;
					    unit->au_Write32 = ata_WriteDMA32;
					    unit->au_Read64 = ata_ReadDMA64;
					    unit->au_Write64 = ata_WriteDMA64;
					}
					else
					{
					    unit->au_Read32 = atapi_ReadDMA;
					    unit->au_Write32= atapi_WriteDMA;
					}
			    	    }
				}
			    }
			}
		    }

		    /* Enable interrupts now */
		    ata_out(0x00, ata_Control, bus->ab_Port);

		    D(bug("\n"));
		}
	    }
	}
    }
    
    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(port);
}


/*
    Device scan routines
*/
void ata_ResetBus(struct timerequest *tr, struct ata_Bus *bus)
{
    ULONG port = bus->ab_Port;
    int cnt;

    /* Exclusive use of ATA registers */
    ObtainSemaphore(&bus->ab_Lock);

    /* Disable IRQ */
    ata_out(0x0a, ata_Control, port);
    /* Issue software reset */
    ata_out(0x0e, ata_Control, port);
    /* wait a while */
    ata_usleep(tr, 400);
    /* Clear reset signal */
    ata_out(0x0a, ata_Control, port);
    /* And wait again */
    ata_usleep(tr, 400);
    /* wait for dev0 to come online. Limited delay up to 30µs */
    if (bus->ab_Dev[0] != DEV_NONE)
    {
	cnt=400;    // 400ms delay for slowest devices to reply.
	while (cnt--)
	{
	    if ((ata_in(ata_Status, port) & ATAF_BUSY) == 0)
		break;
	    ata_usleep(tr, 1000);
	}
    }

    /* wait for dev1 to come online. Limited delay up to 30µs */
    if (bus->ab_Dev[1] != DEV_NONE)
    {
	ata_out(0xb0, ata_DevHead, port);
	ata_usleep(tr, 100);

	cnt=400;
	while (cnt--)
	{
	    if ((ata_in(ata_Status, port) & ATAF_BUSY) == 0)
		break;

	    ata_usleep(tr, 1000);
	}
    }

    ReleaseSemaphore(&bus->ab_Lock);
}

void ata_ScanBus(struct ata_Bus *bus)
{
    ULONG port = bus->ab_Port;
    UBYTE tmp1, tmp2;

    struct MsgPort *p = CreateMsgPort();
    struct timerequest *tr = (struct timerequest *)CreateIORequest((struct MsgPort *)p,
        sizeof(struct timerequest));
    OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)tr, 0);

    /* Exclusive use of ATA registers */
    ObtainSemaphore(&bus->ab_Lock);

    bus->ab_Dev[0] = DEV_NONE;
    bus->ab_Dev[1] = DEV_NONE;

    /* Disable IDE IRQ */
    ata_out(0x0a, ata_Control, port);

    /* Select device 0 */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Write some pattern to registers */
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);
    ata_out(0xaa, ata_Count, port);
    ata_out(0x55, ata_LBALow, port);
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);

    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);

    if ((tmp1 == 0x55) && (tmp2 == 0xaa))
	bus->ab_Dev[0] = DEV_UNKNOWN;

    /* Select device 1 */
    ata_out(0xb0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Write some pattern to registers */
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);
    ata_out(0xaa, ata_Count, port);
    ata_out(0x55, ata_LBALow, port);
    ata_out(0x55, ata_Count, port);
    ata_out(0xaa, ata_LBALow, port);

    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);

    if ((tmp1 == 0x55) && (tmp2 == 0xaa))
	bus->ab_Dev[1] = DEV_UNKNOWN;

    /*
	According to ATA specs it is quite possible, that the dev0 will respond
	for both self and dev1. Similar, when only dev1 is available, it may as
	well respond as dev0. Do more precise test now.
    */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);
    ata_ResetBus(tr, bus);
    
    /* check device 0 */
    ata_out(0xa0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Check basic signature. All live devices should provide it */
    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);

    if ((tmp1 == 0x01) && (tmp2 == 0x01))
    {
	/* Ok, ATA/ATAPI device. Get detailed signature */
	bus->ab_Dev[0] = DEV_UNKNOWN;
	tmp1 = ata_in(ata_LBAMid, port);
	tmp2 = ata_in(ata_LBAHigh, port);
    
	if ((tmp1 == 0x14) && (tmp2 == 0xeb))
	{
	    bus->ab_Dev[0] = DEV_ATAPI;
	}
	else if ((tmp1 == 0) && (tmp2 == 0) && ((ata_in(ata_Status, port) & 0xfe) != 0))
	    bus->ab_Dev[0] = DEV_ATA;
    }

    /* check device 1 */
    ata_out(0xb0, ata_DevHead, port);
    ata_usleep(tr, 100);

    /* Check basic signature. All live devices should provide it */
    tmp1 = ata_in(ata_Count, port);
    tmp2 = ata_in(ata_LBALow, port);
 
    if ((tmp1 == 0x01) && (tmp2 == 0x01))
    {
	/* Ok, ATA/ATAPI device. Get detailed signature */
	bus->ab_Dev[1] = DEV_UNKNOWN;
	tmp1 = ata_in(ata_LBAMid, port);
	tmp2 = ata_in(ata_LBAHigh, port);

	if ((tmp1 == 0x14) && (tmp2 == 0xeb))
	{
	    bus->ab_Dev[1] = DEV_ATAPI;
	}
	else if ((tmp1 == 0) && (tmp2 == 0) && ((ata_in(ata_Status, port) & 0xfe) != 0))
	    bus->ab_Dev[1] = DEV_ATA;
    }

    ReleaseSemaphore(&bus->ab_Lock);

    CloseDevice((struct IORequest *)tr);
    DeleteIORequest((struct IORequest *)tr);
    DeleteMsgPort(p);
}

#define bus (unit->au_Bus)

/*
    This function does interrupt waiting. It waits for a signal fired up from
    ATA interrupt handler. In order to avoid deadlock, it waits for CTRL_C
    signal too. The CTRL_C is sent by timer interrupt.
*/
int ata_WaitSleepyStatus(struct ata_Unit *unit, UBYTE *stat)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;

    /* Check whether we have to wait at all. Return immediatelly, if drive is not busy */
    while ((status = ata_in(ata_Status, port)) & ATAF_BUSY)
    {
	/* Increase timeout in disabled state */
	Disable();
	unit->au_Bus->ab_Timeout=1000;	/* Uncalibrated value!!!!!! */
	Enable();
	/* And wait. If CTRL_C received, return 0 (error) */
	if (Wait((1L << bus->ab_SleepySignal) | SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
	    return 0;
    };
    
    /* Clear timeout counter to avoid sending CTRL_C signal */
    Disable();
    unit->au_Bus->ab_Timeout=0;
    Enable();

    *stat = status;
    return 1;
}

int ata_WaitBusy(struct ata_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop;
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;

    do
    {
        status = ata_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);

    return cnt;
}

int ata_WaitBusyStatus(struct ata_Unit *unit, UBYTE *stat)
{
    ULONG cnt = unit->au_NumLoop;
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;

    do
    {
        status = ata_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);

    *stat = status;
    return cnt;
}

int ata_WaitBusyLongStatus(struct ata_Unit *unit, UBYTE *stat)
{
    ULONG cnt = unit->au_NumLoop;
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;

    do
    {
        status = ata_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);

    *stat = status;
    return cnt;
}


int ata_WaitBusyLong(struct ata_Unit *unit)
{
    ULONG cnt = unit->au_NumLoop << 2;
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;

    do
    {
        status = ata_in(ata_Status, port);
    } while((status & ATAF_BUSY) && --cnt);

    return cnt;
}

struct wait { ULONG time; ULONG cnt; };

static struct wait WaitTable[] = {
        {   1000, 20 },     //   1 ms x 20
        {   5000, 16 },     //   5 ms x 16
        {  10000, 20 },     //  10 ms x 20
        {  20000, 10 },     //  20 ms x 10
        {  50000, 10 },     //  50 ms x 10
        { 100000, 90 },     // 100 ms x 90
        {      0,  0 }      //------------ = 10000 ms
};

int ata_WaitBusySlow(struct ata_Unit *unit)
{
    int i=1000;
    ULONG port = unit->au_Bus->ab_Port;

    if (unit->au_Flags & AF_SlowDevice)
    {
        int t=0;

        do
        {
            if (!(ata_in(ata_Status, port) & ATAF_BUSY))
                return i;
        } while (--i);

        while (WaitTable[t].time)
        {
            int                 loop;
            struct timerequest  *tr;

            loop = WaitTable[t].cnt;

            tr = unit->au_Bus->ab_TimerIO;

            while(loop--)
            {
                tr->tr_node.io_Command = TR_ADDREQUEST;
                tr->tr_time.tv_secs = 0;
                tr->tr_time.tv_micro = WaitTable[t].time;
                DoIO((struct IORequest *)tr);
                if (!(ata_in(ata_Status, port) & ATAF_BUSY))
                    return 1;
            }
            t++;
        }
        return 0;
    }
    return ata_WaitBusy(unit);
}

/*
    Little helper function selects device described by unit structure and sets 
    LBA28 address
*/
static int ata_SetLBA28(struct ata_Unit *unit, ULONG block)
{
    ULONG port = unit->au_Bus->ab_Port;
    BOOL ret = FALSE;

    ata_out(((block >> 24) & 0x0f) | 0x40 | unit->au_DevMask, ata_DevHead, port);
    if (ata_WaitBusy(unit))
    {
	ata_out(block >> 16, ata_LBAHigh, port);
	ata_out(block >> 8, ata_LBAMid, port);
	ata_out(block, ata_LBALow, port);
	ret = TRUE;
    }

    return ret;
}

/*
    Little helper function selects device described by unit structure and sets 
    LBA48 address
*/
static int ata_SetLBA48(struct ata_Unit *unit, UQUAD block)
{
    ULONG port = unit->au_Bus->ab_Port;
    BOOL ret = FALSE;

    ata_out(0x40 | unit->au_DevMask, ata_DevHead, port);
    if (ata_WaitBusy(unit))
    {
	ata_out(block >> 40, ata_LBAHigh, port);
	ata_out(block >> 32, ata_LBAMid, port);
	ata_out(block >> 24, ata_LBALow, port);

	ata_out(block >> 16, ata_LBAHigh, port);
	ata_out(block >> 8, ata_LBAMid, port);
	ata_out(block, ata_LBALow, port);
	ret = TRUE;
    }

    return ret;
}

/* 
    Generic ReadSector32 command. Reads specified amount of sectors into specified block,
    whereas it's trying to minimize amount of used ATA READ SECTOR(S) commands
*/
static ULONG ata_ReadSector32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;

    /* Work only if the highest requested sector is still within addressable space */
    if ((block + count) <= unit->au_Capacity)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA28(unit, block))
	    {
		/* the maximal cout is 256 sectors. */
		cnt = (count > 256) ? 256 : count;

		ata_out(cnt & 0xff, ata_Count, port);
		
		/* 
		    Issue READ SECTOR(S) command and wait 400ns to let the drive change
		    BSY signal
		*/
		ata_out(ATA_READ, ata_Command, port);
		ata_400ns();

		/* Sleep and wait :) */
		if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;

		/* 
		    The interrupt is issued "cnt" times, everytime you get here, receive data
		    from ATA buffer and store it in ram
		*/
		for (i=0; i < cnt; i++)
		{
		    /* No data? Can't be! */
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    unit->au_ins(buffer, port, 1 << unit->au_SectorShift);
		    ata_400ns();

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		    
		    ata_WaitSleepyStatus(unit, &status);

		    buffer += 1 << unit->au_SectorShift;
		    *act += 1 << unit->au_SectorShift;
		}

		count -= cnt;
		block += cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }
    return TDERR_NotSpecified;
}

static ULONG ata_ReadMultiple32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;
    ULONG multicount = unit->au_Drive->id_RWMultipleSize & 0xff;

    if ((block + count) <= unit->au_Capacity)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA28(unit, block))
	    {
		cnt = (count > 256) ? 256 : count;

		ata_out(cnt & 0xff, ata_Count, port);
		
		ata_out(ATA_READ_MULTIPLE, ata_Command, port);
		ata_400ns();
		
		if (!ata_WaitSleepyStatus(unit, &status))
		    return TDERR_NotSpecified;

		for (i=0; i < cnt; i+=multicount)
		{		    
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    if (multicount > count) multicount = count;

		    unit->au_ins(buffer, port, multicount << unit->au_SectorShift);
		    ata_400ns();

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		
		    if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;

		    buffer += multicount << unit->au_SectorShift;
		    *act += multicount << unit->au_SectorShift;
		}

		count -= cnt;
		block += cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }
    return TDERR_NotSpecified;
}

static ULONG ata_ReadDMA32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt;

//    D(bug("[ATA] ReadDMA32(block=%d, count=%d, buff=%x) ",
//	block, count, buffer));

    /* In case of DMA transfers, the memory buffer has to be at even address */
    if ((IPTR)buffer & 0x1)
    {
	/* Redirect command to generic read sector (PIO mode). */
	return ata_ReadSector32(unit, block, count, buffer, act);
    }
    else
    {
	if ((block + count) <= unit->au_Capacity)
	{
	    do
	    {
		/* Select ATA device */
		if (ata_SetLBA28(unit, block))
		{
		    ULONG length;
		    
		    cnt = (count > 256) ? 256 : count;
		    
		    length = cnt << unit->au_SectorShift;

		    ata_out(cnt & 0xff, ata_Count, port);
		
		    dma_SetupPRD(unit, buffer, cnt, TRUE);
		    
		    CachePreDMA(buffer, &length, DMA_ReadFromRAM);

		    ata_out(ATA_READ_DMA, ata_Command, port);
		    dma_StartDMA(unit);
		
		    Disable();
		    unit->au_Bus->ab_Timeout = 1000;
		    Enable();
		
		    /*
			Be smart here!

			However, it is granted, that at the end of DMA transfer the interrupt
			is raised, don't belive here, that it will be the only interrupt within
			the DMA command. The drive may generate even more interrupts, so be
			prepared and wait untill the DMAF_Interrupt flag is set.
		    */
		    do
		    {
			if (Wait(1L << unit->au_Bus->ab_SleepySignal | SIGBREAKF_CTRL_C)
			    & SIGBREAKF_CTRL_C)
			{
			    D(bug("DMA timeout error\n"));
			    return TDERR_NotSpecified;
			}
		    } while(!(inb(unit->au_DMAPort + dma_Status) & DMAF_Interrupt));

		    Disable();
		    unit->au_Bus->ab_Timeout = 0;
		    Enable();
		
		    if (!ata_WaitBusyStatus(unit, &status))
			return TDERR_NotSpecified;

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		    
		    buffer += cnt << unit->au_SectorShift;
		    *act += cnt << unit->au_SectorShift;

		    dma_StopDMA(unit);

		    CachePostDMA(buffer, &length, DMA_ReadFromRAM);

		    count -= cnt;
		    block += cnt;
		}
		else
		{
		    return TDERR_NotSpecified;
		}
	    } while(count);
	    return 0;
	}
    }
    return TDERR_NotSpecified;
}


static ULONG ata_ReadSector64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;

    if ((block + (UQUAD)count) <= unit->au_Capacity48)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA48(unit, block))
	    {
		cnt = (count > 65536) ? 65536 : count;

		ata_out((cnt >> 8) & 0xff, ata_Count, port);
		ata_out(cnt & 0xff, ata_Count, port);
		
		ata_out(ATA_READ64, ata_Command, port);
		ata_400ns();
		
		for (i=0; i < cnt; i++)
		{
		    if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;
		    
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    unit->au_ins(buffer, port, 1 << unit->au_SectorShift);
		    ata_400ns();
		    
		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;

		    buffer += 1 << unit->au_SectorShift;
		    *act += 1 << unit->au_SectorShift;
		}

		count -= cnt;
		block += (UQUAD)cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }
    return TDERR_NotSpecified;
}

static ULONG ata_ReadMultiple64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;
    ULONG multicount = unit->au_Drive->id_RWMultipleSize & 0xff;

    if ((block + (UQUAD)count) <= unit->au_Capacity48)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA48(unit, block))
	    {
		cnt = (count > 65536) ? 65536 : count;

		ata_out(cnt & 0xff, ata_Count, port);
		ata_out((cnt >> 8) & 0xff, ata_Count, port);

		ata_out(ATA_READ_MULTIPLE64, ata_Command, port);
		ata_400ns();
		
		for (i=0; i < cnt; i+=multicount)
		{
		    if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;
		    
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    if (multicount > count) multicount = count;
		    unit->au_ins(buffer, port, multicount << unit->au_SectorShift);
		    ata_400ns();

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;

		    buffer += multicount << unit->au_SectorShift;
		    *act += multicount << unit->au_SectorShift;
		}

		count -= cnt;
		block += (UQUAD)cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }

    return TDERR_NotSpecified;
}

static ULONG ata_ReadDMA64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt;

    /* In case of DMA transfers, the memory buffer has to be at even address */
    if ((IPTR)buffer & 0x1)
    {
	/* Redirect command to generic read sector (PIO mode). */
	return ata_ReadSector64(unit, block, count, buffer, act);
    }
    else
    {
        if ((block + (UQUAD)count) <= unit->au_Capacity48)
	{
	    do
	    {
		/* Select ATA device */
		if (ata_SetLBA48(unit, block))
		{
		    ULONG length;

		    cnt = (count > 65536) ? 65536 : count;

		    length = cnt << unit->au_SectorShift;

		    ata_out((cnt >> 8) & 0xff, ata_Count, port);
		    ata_out(cnt & 0xff, ata_Count, port);
		
		    dma_SetupPRD(unit, buffer, cnt, TRUE);

		    CachePreDMA(buffer, &length, DMA_ReadFromRAM);

		    ata_out(ATA_READ_DMA64, ata_Command, port);
		    dma_StartDMA(unit);
		
		    Disable();
		    unit->au_Bus->ab_Timeout = 1000;
		    Enable();
		
		    do
		    {
			if (Wait(1L << unit->au_Bus->ab_SleepySignal | SIGBREAKF_CTRL_C)
			    & SIGBREAKF_CTRL_C)
			{
			    D(bug("DMA timeout error\n"));
			    return TDERR_NotSpecified;
			}
		    } while(!(inb(unit->au_DMAPort + dma_Status) & DMAF_Interrupt));

		    Disable();
		    unit->au_Bus->ab_Timeout = 0;
		    Enable();
		
		    if (!ata_WaitBusyStatus(unit, &status))
			return TDERR_NotSpecified;

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		    
		    buffer += cnt << unit->au_SectorShift;
		    *act += cnt << unit->au_SectorShift;

		    dma_StopDMA(unit);

		    CachePostDMA(buffer, &length, DMA_ReadFromRAM);

		    count -= cnt;
		    block += cnt;
		}
		else
		{
		    return TDERR_NotSpecified;
		}
	    } while(count);
	    return 0;
	}
    }

    return TDERR_NotSpecified;
}

static ULONG ata_WriteSector32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;

//    D(bug("SectorShift %d %d\n", unit->au_SectorShift, 1 << unit->au_SectorShift));

    if ((block + count) <= unit->au_Capacity)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA28(unit, block))
	    {
		cnt = (count > 256) ? 256 : count;

		ata_out(cnt & 0xff, ata_Count, port);
		
		ata_out(ATA_WRITE, ata_Command, port);
		ata_400ns();

		if (!ata_WaitBusyStatus(unit, &status))
		   return TDERR_NotSpecified;

		for (i=0; i < cnt; i++)
		{ 
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    unit->au_outs(buffer, port, 1 << unit->au_SectorShift);
		    ata_400ns();

		    if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;
	    
		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;

		    buffer += 1 << unit->au_SectorShift;
		    *act += 1 << unit->au_SectorShift;
		}

		count -= cnt;
		block += cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }
    return TDERR_TooFewSecs;
}

static ULONG ata_WriteSector64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt,i;

    if ((block + (UQUAD)count) <= unit->au_Capacity48)
    {
	do
	{
	    /* Select ATA device */
	    if (ata_SetLBA48(unit, block))
	    {
		cnt = (count > 65536) ? 65536 : count;

		ata_out(cnt & 0xff, ata_Count, port);
		ata_out((cnt >> 8) & 0xff, ata_Count, port);

		ata_out(ATA_WRITE64, ata_Command, port);
		ata_400ns();

		if (!ata_WaitBusyStatus(unit, &status))
		   return TDERR_NotSpecified;

		for (i=0; i < cnt; i++)
		{ 
		    if (!(status & ATAF_DATAREQ))
			return TDERR_NotSpecified;

		    unit->au_outs(buffer, port, 1 << unit->au_SectorShift);
		    ata_400ns();

		    if (!ata_WaitSleepyStatus(unit, &status))
			return TDERR_NotSpecified;
	    
		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;

		    buffer += 1 << unit->au_SectorShift;
		    *act += 1 << unit->au_SectorShift;
		}

		count -= cnt;
		block += (UQUAD)cnt;
	    }
	    else
	    {
		return TDERR_NotSpecified;
	    }
	} while(count);
	return 0;
    }
    return TDERR_NotSpecified;
}

static ULONG ata_WriteMultiple32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    return ata_WriteSector32(unit, block, count, buffer, act);
}

static ULONG ata_WriteMultiple64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    return ata_WriteSector64(unit, block, count, buffer, act);
}

static ULONG ata_WriteDMA32(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt;

    if ((IPTR)buffer & 0x1)
    {
	/* Redirect command to generic read sector (PIO mode). */
	return ata_WriteSector32(unit, block, count, buffer, act);
    }
    else
    {
        if ((block + count) <= unit->au_Capacity)
	{
	    do
	    {
		/* Select ATA device */
		if (ata_SetLBA28(unit, block))
		{
		    ULONG length;
		    
		    cnt = (count > 256) ? 256 : count;
		    
		    length = cnt << unit->au_SectorShift;

		    ata_out(cnt & 0xff, ata_Count, port);
		
		    dma_SetupPRD(unit, buffer, cnt, FALSE);
		    
		    CachePreDMA(buffer, &length, 0);

		    ata_out(ATA_WRITE_DMA, ata_Command, port);
		    dma_StartDMA(unit);

		    Disable();
		    unit->au_Bus->ab_Timeout = 1000;
		    Enable();

		    do
		    {
			if (Wait(1L << unit->au_Bus->ab_SleepySignal | SIGBREAKF_CTRL_C)
			    & SIGBREAKF_CTRL_C)
			{
			    D(bug("DMA timeout error\n"));
			    return TDERR_NotSpecified;
			}
		    } while(!(inb(unit->au_DMAPort + dma_Status) & DMAF_Interrupt));
		
//		D(bug("[ATA] Write got Command=%02x Status=%02x\n",
//		    inb(unit->au_DMAPort), inb(unit->au_DMAPort + 2)));
		
		    Disable();
		    unit->au_Bus->ab_Timeout = 0;
		    Enable();
	
		    if (!ata_WaitBusyStatus(unit, &status))
			return TDERR_NotSpecified;

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		    
		    buffer += cnt << unit->au_SectorShift;
		    *act += cnt << unit->au_SectorShift;

		    dma_StopDMA(unit);

		    CachePostDMA(buffer, &length, 0);

		    count -= cnt;
		    block += cnt;
		}
		else
		{
		    return TDERR_NotSpecified;
		}
	    } while(count);
	    return 0;
	}
    }
    return TDERR_NotSpecified;
}

static ULONG ata_WriteDMA64(struct ata_Unit *unit, UQUAD block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE status;
    ULONG cnt;

    if ((IPTR)buffer & 0x1)
    {
	/* Redirect command to generic read sector (PIO mode). */
	return ata_WriteSector64(unit, block, count, buffer, act);
    }
    else
    {
	if ((block + (UQUAD)count) <= unit->au_Capacity48)
	{
	    do
	    {
		/* Select ATA device */
		if (ata_SetLBA48(unit, block))
		{
		    ULONG length;
		    
		    cnt = (count > 65536) ? 65536 : count;
		    
		    length = cnt << unit->au_SectorShift;

		    ata_out((cnt >> 8) & 0xff, ata_Count, port);
		    ata_out(cnt & 0xff, ata_Count, port);
		
		    dma_SetupPRD(unit, buffer, cnt, FALSE);
		    
		    CachePreDMA(buffer, &length, 0);

		    ata_out(ATA_WRITE_DMA64, ata_Command, port);
		    dma_StartDMA(unit);

		    Disable();
		    unit->au_Bus->ab_Timeout = 1000;
		    Enable();

		    do
		    {
			if (Wait(1L << unit->au_Bus->ab_SleepySignal | SIGBREAKF_CTRL_C)
			    & SIGBREAKF_CTRL_C)
			{
			    D(bug("DMA timeout error\n"));
			    return TDERR_NotSpecified;
			}
		    } while(!(inb(unit->au_DMAPort + dma_Status) & DMAF_Interrupt));
		
		    Disable();
		    unit->au_Bus->ab_Timeout = 0;
		    Enable();
		
		    if (!ata_WaitBusyStatus(unit, &status))
			return TDERR_NotSpecified;

		    if (status & ATAF_ERROR)
			return TDERR_NotSpecified;
		    
		    buffer += cnt << unit->au_SectorShift;
		    *act += cnt << unit->au_SectorShift;

		    dma_StopDMA(unit);

		    CachePostDMA(buffer, &length, 0);

		    count -= cnt;
		    block += cnt;
		}
		else
		{
		    return TDERR_NotSpecified;
		}
	    } while(count);
	    return 0;
	}
    }

    return TDERR_NotSpecified;
}

static ULONG ata_Eject(struct ata_Unit *unit)
{
    ULONG port = unit->au_Bus->ab_Port;

    ata_out(unit->au_DevMask, ata_DevHead, port);
    if (ata_WaitBusy(unit))
    {
	ata_out(ATA_MEDIA_EJECT, ata_Command, port);
	if (ata_WaitBusy(unit))
	{
	    return 0;
	}
    }

    return TDERR_NotSpecified;
}

int ata_DirectScsi(struct SCSICmd *cmd, struct ata_Unit *unit)
{
    UBYTE command[12] = {0};
    ULONG port = unit->au_Bus->ab_Port;
    APTR buffer = cmd->scsi_Data;
    ULONG length = cmd->scsi_Length;
    ULONG size = 0;

    for (cmd->scsi_CmdActual=0; cmd->scsi_CmdActual<12; cmd->scsi_CmdActual++)
    {
       command[cmd->scsi_CmdActual] = cmd->scsi_Command[cmd->scsi_CmdActual];
    }

    cmd->scsi_Actual = 0;

    D(bug("Attempting to send packet!\n"));

    if (atapi_SendPacket(unit, command, sizeof(command), FALSE)) /*cmd->scsi_CmdLength, FALSE))*/
    {
    	D(bug("Command apparently has been sent.. i guess\n"));
	while(1)
	{
	    if ((ata_WaitBusySlow(unit)))
            {
                if (ata_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    UBYTE reason = ata_in(atapi_Reason, port);
    		    
		    D(bug("%s: Current status: %ld, SCSI flags: %ld\n", __PRETTY_FUNCTION__, reason, cmd->scsi_Flags));

                    if (((cmd->scsi_Flags & SCSIF_READ) == SCSIF_READ) &&
                        ((reason & ATAPIF_MASK) == ATAPIF_READ))
                    {
   		        size = ata_in(atapi_ByteCntH, port) << 8 |
                               ata_in(atapi_ByteCntL, port);
		        D(bug("%s: data available for read (%ld bytes, max: %ld bytes)\n", __PRETTY_FUNCTION__, size, length));
			if (size > length)
                        {
			    D(bug("%s: CRITICAL! MORE DATA THAN STORAGE ALLOWS: %ld bytes vs %ld bytes left!\n", __PRETTY_FUNCTION__, size, length));
                            /* damnit!! need to report some critical error here! */
			    size = length;
                        }
		        unit->au_ins(buffer, port, size);
			D(bug("%s: Data read.\n", __PRETTY_FUNCTION__));
                    }   
                    else if (((cmd->scsi_Flags & SCSIF_WRITE) == SCSIF_WRITE) &&
                             ((reason & ATAPIF_MASK) == ATAPIF_WRITE))
                    {
		        size = ata_in(atapi_ByteCntH, port) << 8 |
                               ata_in(atapi_ByteCntL, port);
		        D(bug("%s: device available for write\n", __PRETTY_FUNCTION__));
			if (size > length)
                        {
			    D(bug("%s: CRITICAL! MORE DATA THAN STORAGE ALLOWS: %ld bytes vs %ld bytes left!\n", __PRETTY_FUNCTION__, size, length));
			    size = length;
                            /* damnit!! need to report some critical error here! */
			    size = length;
                        }
		        unit->au_outs(buffer, port, size);
			D(bug("%s: Data written.\n", __PRETTY_FUNCTION__));
                    }
                    else
                    {
		        D(bug("%s: no further data transfer awaits\n", __PRETTY_FUNCTION__));
                        return atapi_ErrCmd();
                    }
               
                    buffer += size;
                    cmd->scsi_Actual += size;
                    length -= size;
		    if (0 == length)
		    {
		       D(bug("%s: User transfer complete, %ld bytes transferred.\n", __PRETTY_FUNCTION__, cmd->scsi_Actual));
		       return atapi_EndCmd(unit);
		    }
                } 
                else
                {
                    return atapi_EndCmd(unit);
                }
            } else return atapi_ErrCmd();
        }
    }

    return atapi_ErrCmd();
/*
    ULONG port = unit->au_Bus->ab_Port;
    UBYTE command = cmd->scsi_Command[0];

    ata_out(unit->au_DevMask, ata_DevHead, port);

    if (ata_WaitBusy(unit))
    {
	ata_out(command, ata_Command, port);
	if (ata_WaitBusy(unit))
	{
	    cmd->scsi_Status = ata_in(ata_Status, port);

	    if (cmd->scsi_Status & ATAF_DATAREQ)
	    {
		unit->au_ins(cmd->scsi_Data, port, cmd->scsi_Length);

		cmd->scsi_Status = ata_in(ata_Status, port);
		if (!(cmd->scsi_Status & ATAF_ERROR))
		{
		    return 0;
		}
	    }
	}
    }
*/    
//    ata_out(ATA_RECALIBRATE, ata_Command, port);
//    ata_WaitBusyLong(unit);
    return HFERR_BadStatus;
}
















/*
    ATAPI commands
*/
static const ULONG ErrorMap[] = {
    CDERR_NotSpecified,
    CDERR_NoSecHdr,
    CDERR_NoDisk,
    CDERR_NoSecHdr,
    CDERR_NoSecHdr,
    CDERR_NOCMD,
    CDERR_NoDisk,
    CDERR_WriteProt,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_ABORTED,
    CDERR_NotSpecified,
    CDERR_NotSpecified,
    CDERR_NoSecHdr,
    CDERR_NotSpecified,
};

static ULONG atapi_ErrCmd()
{
    return CDERR_ABORTED;
}

static ULONG atapi_EndCmd(struct ata_Unit *unit)
{
    unit->au_Flags |= AF_Used;
    
    if (!(ata_in(atapi_Status, unit->au_Bus->ab_Port) & ATAPIF_CHECK))
    {
	return 0;
    }

    return ErrorMap[ata_in(atapi_Error, unit->au_Bus->ab_Port) >> 4];
}

int atapi_SendPacket(struct ata_Unit *unit, APTR packet, ULONG size, BOOL dma)
{
    ULONG port = unit->au_Bus->ab_Port;

    ata_out(unit->au_DevMask, atapi_DevSel, port);
    if (ata_WaitBusy(unit))
    {
        ata_out(dma ? 1 : 0, atapi_Features, port);
        ata_out(0xfe, atapi_ByteCntL, port);
        ata_out(0xff, atapi_ByteCntH, port);
        ata_out(ATA_PACKET, atapi_Command, port);
	if (ata_WaitBusySlow(unit))
	{
	    if (ata_in(atapi_Status, port) & ATAF_DATAREQ)
	    {
		UBYTE stat;
		stat = ata_in(atapi_Reason, port);
		stat &= ATAPIF_MASK;
		if (stat == ATAPIF_COMMAND)
		{
		    unit->au_outs(packet, port, size);
		    return 1;
		}
	    }
	}
    }

    return 0;
}

int atapi_TestUnitOK(struct ata_Unit *unit)
{
    ULONG port = unit->au_Bus->ab_Port;
    ULONG cmd[3] = {0,0,0};
    UBYTE sense;

    if (atapi_SendPacket(unit, cmd, sizeof(cmd), FALSE))
    {
	if (ata_WaitBusySlow(unit))
	{
	    sense = ata_in(ata_Error, port) >> 4;
	    if (sense)
		unit->au_SenseKey = sense;
	    return sense;
	}
    }

    return CDERR_ABORTED;
}

static ULONG atapi_Read(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;

    struct atapi_Read10 cmd = {
	command:    SCSI_READ10,
    };

    cmd.lba[0] = block >> 24;
    cmd.lba[1] = block >> 16;
    cmd.lba[2] = block >> 8;
    cmd.lba[3] = block;
    
    cmd.len[0] = count >> 8;
    cmd.len[1] = count;
    
    *act = 0;

    if (atapi_SendPacket(unit, &cmd, sizeof(cmd), FALSE))
    {
	while(1)
	{
	    if (ata_WaitBusySlow(unit))
            {
                if (ata_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;

                    if ((ata_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_READ)
                        return atapi_ErrCmd();
                    
		    size = ata_in(atapi_ByteCntH, port) << 8 |
                           ata_in(atapi_ByteCntL, port);
                    
		    unit->au_ins(buffer, port, size);

                    buffer += size;
                    *act += size;
                } else return atapi_EndCmd(unit);
            } else return atapi_ErrCmd();
        }
    }

    return atapi_ErrCmd();
}

static ULONG atapi_Write(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    ULONG port = unit->au_Bus->ab_Port;

    struct atapi_Write10 cmd = {
	command:    SCSI_WRITE10,
    };

    cmd.lba[0] = block >> 24;
    cmd.lba[1] = block >> 16;
    cmd.lba[2] = block >> 8;
    cmd.lba[3] = block;
    
    cmd.len[0] = count >> 8;
    cmd.len[1] = count;

    *act = 0;

    if (atapi_SendPacket(unit, &cmd, sizeof(cmd), FALSE))
    {
	while(1)
	{
	    if (ata_WaitBusySlow(unit))
            {
                if (ata_in(atapi_Status, port) & ATAF_DATAREQ)
                {
                    ULONG size;

                    if ((ata_in(atapi_Reason, port) & ATAPIF_MASK) != ATAPIF_WRITE)
                        return atapi_ErrCmd();
                    
		    size = ata_in(atapi_ByteCntH, port) << 8 |
                           ata_in(atapi_ByteCntL, port);
                    
		    unit->au_outs(buffer, port, size);

                    buffer += size;
                    *act += size;
                } else return atapi_EndCmd(unit);
            } else return atapi_ErrCmd();
	}
    }

    return atapi_ErrCmd();
}

static ULONG atapi_Eject(struct ata_Unit *unit)
{
    struct atapi_StartStop cmd = {
	command: SCSI_STARTSTOP,
	immediate: 1,
	flags: ATAPI_SS_EJECT,
    };

    if (atapi_SendPacket(unit, &cmd, sizeof(cmd), FALSE))
    {
	if (ata_WaitBusySlow(unit))
        {
	    return atapi_EndCmd(unit);
	}
    }

    return atapi_ErrCmd();
}

static ULONG atapi_ReadDMA(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    UBYTE status;
    ULONG length = count << unit->au_SectorShift;
    ULONG err = 0;

    struct atapi_Read10 cmd = {
	command:    SCSI_READ10,
    };

    cmd.lba[0] = block >> 24;
    cmd.lba[1] = block >> 16;
    cmd.lba[2] = block >> 8;
    cmd.lba[3] = block;
    
    cmd.len[0] = count >> 8;
    cmd.len[1] = count;
    
    *act = 0;

    dma_SetupPRD(unit, buffer, count, TRUE);

    CachePreDMA(buffer, &length, DMA_ReadFromRAM);

    if (atapi_SendPacket(unit, &cmd, sizeof(cmd), TRUE))
    {
	if (ata_WaitSleepyStatus(unit, &status))
	{
	    *act+= count << unit->au_SectorShift;
	    err = atapi_EndCmd(unit);
	}
	else err = atapi_ErrCmd(unit);
    }

    CachePostDMA(buffer, &length, DMA_ReadFromRAM);

    return err;
}

static ULONG atapi_WriteDMA(struct ata_Unit *unit, ULONG block, ULONG count, APTR buffer, ULONG *act)
{
    UBYTE status;
    ULONG length = count << unit->au_SectorShift;
    ULONG err = 0;

    struct atapi_Write10 cmd = {
	command:    SCSI_WRITE10,
    };

    cmd.lba[0] = block >> 24;
    cmd.lba[1] = block >> 16;
    cmd.lba[2] = block >> 8;
    cmd.lba[3] = block;
    
    cmd.len[0] = count >> 8;
    cmd.len[1] = count;

    *act = 0;

    dma_SetupPRD(unit, buffer, count, FALSE);

    CachePreDMA(buffer, &length, 0);

    if (atapi_SendPacket(unit, &cmd, sizeof(cmd), TRUE))
    {
	if (ata_WaitSleepyStatus(unit, &status))
	{
	    *act+= count << unit->au_SectorShift;
	    err = atapi_EndCmd(unit);
	}
	else err = atapi_ErrCmd(unit);
    }

    CachePostDMA(buffer, &length, 0);

    return err;
}

/* 
 * vim: ts=8 noet 
 */
