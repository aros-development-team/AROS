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
#include <utility/utility.h>
#include <oop/oop.h>

#include <dos/bptr.h>

#include <proto/exec.h>

#include "ata.h"

//---------------------------IO Commands---------------------------------------

/* Invalid comand does nothing, complains only. */
static void cmd_Invalid(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    D(bug("[ATA] Invalid command %d for unit %04x\n", io->io_Command,
	((struct ata_Unit*)io->io_Unit)->au_UnitNum));
    io->io_Error = IOERR_NOCMD;
}

/* Don't need to reset the drive? */
static void cmd_Reset(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    IOStdReq(io)->io_Actual = 0;
}

/* CMD_READ implementation */
static void cmd_Read32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;
    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask = (1 << unit->au_SectorShift) - 1;

    /*
	During this IO call it should be sure that both offset and
	length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
	D(bug("[ATA] offset or length not sector-aligned.\n"));
	cmd_Invalid(io, LIBBASE);
    }
    else
    {
	block >>= unit->au_SectorShift;
	count >>= unit->au_SectorShift;
	ULONG cnt = 0;

	/* Call the Unit's access funtion */
	io->io_Error = unit->au_Read32(unit, block, count,
	    IOStdReq(io)->io_Data, &cnt);
	
	IOStdReq(io)->io_Actual = cnt;
    }
}

/*
    NSCMD_TD_READ64, TD_READ64 implementation. Basically the same, just packs 
    the 64 bit offset in both io_Offset (31:0) and io_Actual (63:32)
*/
static void cmd_Read64(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;
    
    UQUAD block = (IOStdReq(io)->io_Offset) |
	(UQUAD)(IOStdReq(io)->io_Actual) << 32;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask = (1 << unit->au_SectorShift) - 1;

    if ((block & mask) | (count & mask) | (count == 0))
    {
	D(bug("[ATA] offset or length not sector-aligned.\n"));
	cmd_Invalid(io, LIBBASE);
    }
    else
    {
	block >>= unit->au_SectorShift;
	count >>= unit->au_SectorShift;
	ULONG cnt = 0;

	/*
	    If the sum of sector offset and the sector count doesn't overflow
	    the 28-bit LBA address, use 32-bit access for speed and simplicity.
	    Otherwise do the 48-bit LBA addressing.
	*/
	if ((block + count) >= 0x0fffffff)
	    io->io_Error = unit->au_Read32(unit, (block & 0x0fffffff), count,
		IOStdReq(io)->io_Data, &cnt);
	else
	    io->io_Error = unit->au_Read64(unit, block, count,
		IOStdReq(io)->io_Data, &cnt);
		
 	IOStdReq(io)->io_Actual = cnt;
    }
}

/* CMD_WRITE implementation */
static void cmd_Write32(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;
    ULONG block = IOStdReq(io)->io_Offset;
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask = (1 << unit->au_SectorShift) - 1;

    /*
	During this IO call it should be sure that both offset and
	length are already aligned properly to sector boundaries.
    */
    if ((block & mask) | (count & mask))
    {
	D(bug("[ATA] offset or length not sector-aligned.\n"));
	cmd_Invalid(io, LIBBASE);
    }
    else
    {
	block >>= unit->au_SectorShift;
	count >>= unit->au_SectorShift;
	ULONG cnt = 0;

	/* Call the Unit's access funtion */
	io->io_Error = unit->au_Write32(unit, block, count,
	    IOStdReq(io)->io_Data, &cnt);
	
	IOStdReq(io)->io_Actual = cnt;
    }
}

/*
    NSCMD_TD_WRITE64, TD_WRITE64 implementation. Basically the same, just packs 
    the 64 bit offset in both io_Offset (31:0) and io_Actual (63:32)
*/
static void cmd_Write64(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)IOStdReq(io)->io_Unit;
    UQUAD block = IOStdReq(io)->io_Offset | (((UQUAD)(IOStdReq(io)->io_Actual)) << 32);
    ULONG count = IOStdReq(io)->io_Length;
    ULONG mask = (1 << unit->au_SectorShift) - 1;

    if ((block & mask) | (count & mask))
    {
	D(bug("[ATA] offset or length not sector-aligned.\n"));
	cmd_Invalid(io, LIBBASE);
    }
    else
    {
	block >>= unit->au_SectorShift;
	count >>= unit->au_SectorShift;
	ULONG cnt = 0;

	/*
	    If the sum of sector offset and the sector count doesn't overflow
	    the 28-bit LBA address, use 32-bit access for speed and simplicity.
	    Otherwise do the 48-bit LBA addressing.
	*/
	if ((block + count) >= 0x0fffffff)
	    io->io_Error = unit->au_Write32(unit, (block & 0x0fffffff), count,
		IOStdReq(io)->io_Data, &cnt);
	else
	    io->io_Error = unit->au_Write64(unit, block, count,
		IOStdReq(io)->io_Data, &cnt);
 	IOStdReq(io)->io_Actual = cnt;
   }
}


/* use CMD_FLUSH to force all IO waiting commands to abort */
static void cmd_Flush(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct IORequest *msg;
    struct ata_Bus *bus = ((struct ata_Unit *)io->io_Unit)->au_Bus;

    Forbid();

    while((msg = (struct IORequest *)GetMsg((struct MsgPort *)bus->ab_MsgPort)))
    {
	msg->io_Error = IOERR_ABORTED;
	ReplyMsg((struct Message *)msg);
    }

    Permit();
}

/*
    Internal command used to check, whether the media in drive has been changed
    since last call. If so, the handlers given by user are called.
*/
static void cmd_TestChanged(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    /*
	It's impossible to check media status in ATA harddrives :)
    */
    if (unit->au_Flags & AF_ATAPI)
    {
	/* Don't bother with nonremovable ATAPI units */
	if (unit->au_Flags & AF_Removable)
	{
	    struct IORequest *msg;
	    UBYTE sense;

	    /* Issue media check */
	    if ((sense = atapi_TestUnitOK(unit)))
	    {
		/* 
		    Clear the unknown flag. We do know already, that there
		    is no media in drive
		*/
	        unit->au_Flags &= ~AF_DiscPresenceUnknown;
		
		/* Media not present */
		if (!(unit->au_Flags & AF_DiscPresent))
		{
		    /*
			Do status change since last call. Do almost
			nothing. Clear the AF_Used flag as the drive cannot
			be really used without media.
		    */
		    unit->au_Flags &= ~AF_Used;
    		    return;
		}
		/* Clear the presence flag */
		unit->au_Flags &= ~AF_DiscPresent;
	    }
	    else
	    {
		/*
		    No more mystery, we know already that there is media in 
		    drive. Clear this mysterious flag.
		*/
		unit->au_Flags &= ~AF_DiscPresenceUnknown;

		/* Media present */
		if (unit->au_Flags & AF_DiscPresent)
		{
		    /* No status change. Do nothing */
		    return;
		}
		/* Set the presence flag */
		unit->au_Flags |= AF_DiscPresent;
	    }
	    /*
		If CPU came here, there was a change in media presence status.
		First, increase the number of changes to satisfy the curiousity
	    */
	    unit->au_ChangeNum++;

	    /*
		And tell the truth to the world :D
	    */
	    Forbid();

	    /* old-fashioned RemoveInt call first */
	    if (unit->au_RemoveInt)
		Cause(unit->au_RemoveInt);

	    /* And now the whole list of possible calls */
	    ForeachNode(&unit->au_SoftList, msg)
	    {
		Cause((struct Interrupt *)IOStdReq(msg)->io_Data);
	    }

	    Permit();
	}
    }
    unit->au_Flags &= ~AF_Used;
}

static void cmd_Update(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    /* Do nothing now. In near future there should be drive cache flush though */
}

static void cmd_Remove(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    if (unit->au_RemoveInt)
	io->io_Error = TDERR_DriveInUse;
    else
	unit->au_RemoveInt = IOStdReq(io)->io_Data;
}

static void cmd_ChangeNum(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    IOStdReq(io)->io_Actual = ((struct ata_Unit *)io->io_Unit)->au_ChangeNum;
}

static void cmd_ChangeState(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    if (unit->au_Flags & AF_DiscPresent)
        IOStdReq(io)->io_Actual = 0;
    else
	IOStdReq(io)->io_Actual = 1;
}

static void cmd_ProtStatus(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    if (unit->au_DevType)
        IOStdReq(io)->io_Actual = -1;
    else
	IOStdReq(io)->io_Actual = 0;

}

static void cmd_GetNumTracks(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    IOStdReq(io)->io_Actual = ((struct ata_Unit *)io->io_Unit)->au_Cylinders;
}

static void cmd_AddChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;
 
    Forbid();
    AddHead(&unit->au_SoftList, (struct Node *)io);
    Permit();

    io->io_Flags &= ~IOF_QUICK;
    unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;
}

static void cmd_RemChangeInt(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    Forbid();
    Remove((struct Node *)io);
    Permit();
} 

static void cmd_Eject(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;
    
    IOStdReq(io)->io_Error = unit->au_Eject(unit);
}

static void cmd_GetGeometry(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    if (IOStdReq(io)->io_Length == sizeof(struct DriveGeometry))
    {
	struct DriveGeometry *dg = (struct DriveGeometry *)IOStdReq(io)->io_Data;

	dg->dg_SectorSize	= 1 << unit->au_SectorShift;

	if (unit->au_Capacity48 > unit->au_Capacity) {
	    if ((unit->au_Capacity48 >> 32) != 0)
	        dg->dg_TotalSectors	= 0xffffffff;
	    else
		dg->dg_TotalSectors	= unit->au_Capacity48;
	}
	else	dg->dg_TotalSectors	= unit->au_Capacity;

	dg->dg_Cylinders		= unit->au_Cylinders;
	dg->dg_CylSectors		= unit->au_Sectors * unit->au_Heads;
	dg->dg_Heads			= unit->au_Heads;
	dg->dg_TrackSectors		= unit->au_Sectors;
	dg->dg_BufMemType		= MEMF_PUBLIC;
	dg->dg_DeviceType		= unit->au_DevType;
	dg->dg_Flags			= (unit->au_Flags & AF_Removable) ? DGF_REMOVABLE : 0;
	dg->dg_Reserved			= 0;

	IOStdReq(io)->io_Actual = sizeof(struct DriveGeometry);
    }
    else if (IOStdReq(io)->io_Length == 514)
    {
	CopyMemQuick(unit->au_Drive, IOStdReq(io)->io_Data, 512);
    }
    else io->io_Error = TDERR_NotSpecified;
}

static void cmd_DirectScsi(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;

    IOStdReq(io)->io_Actual = sizeof(struct SCSICmd);
    if (!(unit->au_Flags & AF_ATAPI))
    {
	io->io_Error = ata_DirectScsi((struct SCSICmd *)IOStdReq(io)->io_Data, unit);
    }
    else io->io_Error = IOERR_BADADDRESS;
}

//-----------------------------------------------------------------------------

/*
    command translation tables - used to call proper IO functions.
*/

#define N_TD_READ64	0
#define N_TD_WRITE64	1
#define N_TD_SEEK64	2
#define N_TD_FORMAT64	3

#define TD_READ64	24  /* taken from TD64 extension */
#define TD_WRITE64	25
#define TD_SEEK64	26
#define TD_FORMAT64	27

static void (*map64[])(struct IORequest *, LIBBASETYPEPTR) = {
    [N_TD_READ64]   = cmd_Read64,
    [N_TD_WRITE64]  = cmd_Write64,
    [N_TD_SEEK64]   = cmd_Reset,
    [N_TD_FORMAT64] = cmd_Write64
};

static void (*map32[])(struct IORequest *, LIBBASETYPEPTR) = {
    [CMD_INVALID]   = cmd_Invalid,
    [CMD_RESET]	    = cmd_Reset,
    [CMD_READ]	    = cmd_Read32,
    [CMD_WRITE]	    = cmd_Write32,
    [CMD_UPDATE]    = cmd_Update,
    [CMD_CLEAR]	    = cmd_Reset,
    [CMD_STOP]	    = cmd_Reset,
    [CMD_START]	    = cmd_Reset,
    [CMD_FLUSH]	    = cmd_Flush,
    [TD_MOTOR]	    = cmd_Reset,
    [TD_SEEK]	    = cmd_Reset,
    [TD_FORMAT]	    = cmd_Write32,
    [TD_REMOVE]	    = cmd_Remove,
    [TD_CHANGENUM]  = cmd_ChangeNum,
    [TD_CHANGESTATE]= cmd_ChangeState,
    [TD_PROTSTATUS] = cmd_ProtStatus,
    [TD_RAWREAD]    = cmd_Invalid,
    [TD_RAWWRITE]   = cmd_Invalid,
    [TD_GETNUMTRACKS]	    = cmd_GetNumTracks,
    [TD_ADDCHANGEINT]	    = cmd_AddChangeInt,
    [TD_REMCHANGEINT]	    = cmd_RemChangeInt,
    [TD_GETGEOMETRY]= cmd_GetGeometry,
    [TD_EJECT]	    = cmd_Eject,
    [TD_READ64]	    = cmd_Read64,
    [TD_WRITE64]    = cmd_Write64,
    [TD_SEEK64]	    = cmd_Reset,
    [TD_FORMAT64]   = cmd_Write64,
    [HD_SCSICMD]    = cmd_DirectScsi,
    [HD_SCSICMD+1]  = cmd_TestChanged,
};

static const UWORD NSDSupported[] = {
    CMD_RESET,
    CMD_READ,
    CMD_WRITE,
    CMD_UPDATE,
    CMD_CLEAR,
    CMD_STOP,
    CMD_START,
    CMD_FLUSH,
    TD_MOTOR,
    TD_SEEK,
    TD_FORMAT,
    TD_REMOVE,
    TD_CHANGENUM,
    TD_CHANGESTATE,
    TD_PROTSTATUS,
    TD_GETNUMTRACKS,
    TD_ADDCHANGEINT,
    TD_REMCHANGEINT,
    TD_GETGEOMETRY,
    TD_EJECT,
    TD_READ64,
    TD_WRITE64,
    TD_SEEK64,
    TD_FORMAT64,
    HD_SCSICMD,
    TD_GETDRIVETYPE,
    NSCMD_DEVICEQUERY,
    NSCMD_TD_READ64,
    NSCMD_TD_WRITE64,
    NSCMD_TD_SEEK64,
    NSCMD_TD_FORMAT64,
    0
};

/*
    Do proper IO actions depending on the request. It's called from the bus
    tasks and from BeginIO in case of immediate commands.
*/
static void HandleIO(struct IORequest *io, LIBBASETYPEPTR LIBBASE)
{
    io->io_Error = 0;
    
    /* Handle few commands directly here */
    switch (io->io_Command)
    {
	/*
	    New Style Devices query. Introduce self as trackdisk and provide list of
	    commands supported
	*/
	case NSCMD_DEVICEQUERY:
	    {
		struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)IOStdReq(io)->io_Data;
		nsdq->DevQueryFormat	= 0;
		nsdq->SizeAvailable	= sizeof(struct NSDeviceQueryResult);
		nsdq->DeviceType	= NSDEVTYPE_TRACKDISK;
		nsdq->DeviceSubType	= 0;
		nsdq->SupportedCommands	= (UWORD *)NSDSupported;
	    }
	    IOStdReq(io)->io_Actual = sizeof(struct NSDeviceQueryResult);
	    break;
	    
	/*
	    New Style Devices report here the 'NSTY' - only if such value is 
	    returned here, the NSCMD_DEVICEQUERY might be called. Otherwice it should 
	    report error.
	*/
	case TD_GETDRIVETYPE:
	    IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
	    break;
	
	/*
	    Call all other commands using the command pointer tables for 32- and
	    64-bit accesses. If requested function is defined call it, otherwise
	    make the function cmd_Invalid.
	*/
	default:
	    if (io->io_Command <= (HD_SCSICMD+1))
	    {
		if (map32[io->io_Command]) 
		    map32[io->io_Command](io, LIBBASE);
		else
		    cmd_Invalid(io, LIBBASE);
	    }
	    else if (io->io_Command >= NSCMD_TD_READ64 && io->io_Command <= NSCMD_TD_READ64)
	    {
		if (map64[io->io_Command - NSCMD_TD_READ64]) 
		    map64[io->io_Command - NSCMD_TD_READ64](io, LIBBASE);
		else
		    cmd_Invalid(io, LIBBASE);
	    }
	    else cmd_Invalid(io, LIBBASE);
	    break;
    }
}


static const ULONG IMMEDIATE_COMMANDS = 0x803ff1e3; // 10000000001111111111000111100011

/* See whether the command can be done quick */
BOOL isSlow(ULONG comm)
{
    BOOL slow = TRUE;	/* Assume alwasy slow command */

    /* For commands with numbers <= 31 check the mask */
    if (comm <= 31)
    {
	if (IMMEDIATE_COMMANDS & (1 << comm))
	    slow = FALSE;
    }
    else if (comm == NSCMD_TD_SEEK64) slow = FALSE;

    return slow;
}

/*
    Try to do IO commands. All commands which require talking with ata devices
    will be handled slow, that is they will be passed to bus task which will
    execute them as soon as hardware will be free.
*/
AROS_LH1(void, BeginIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 5, ata)
{
    AROS_LIBFUNC_INIT
    
    struct ata_Unit *unit = (struct ata_Unit *)io->io_Unit;
    
    io->io_Message.mn_Node.ln_Type = NT_MESSAGE;

    /* Disable interrupts for a while to modify message flags */
    Disable();

    /*
	If the command is not-immediate, or presence of disc is still unknown,
	let the bus task do the job.
    */
    if (isSlow(io->io_Command) || (unit->au_Flags & AF_DiscPresenceUnknown))
    {
	unit->au_Unit.unit_flags |= UNITF_ACTIVE | UNITF_INTASK;
	io->io_Flags &= ~IOF_QUICK;
	Enable();
	
	/* Put the message to the bus */
	PutMsg(unit->au_Bus->ab_MsgPort, (struct Message *)io);
    }
    else
    {
	/* Immediate command. Mark unit as active and do the command directly */
	unit->au_Unit.unit_flags |= UNITF_ACTIVE;
	Enable();
	HandleIO(io, LIBBASE);

	unit->au_Unit.unit_flags &= ~UNITF_ACTIVE;

	/*
	    If the command was not intended to be immediate and it was not the
	    TD_ADDCHANGEINT, reply to confirm command execution now.
	*/
        if (!(io->io_Flags & IOF_QUICK) && (io->io_Command != TD_ADDCHANGEINT))
	{
	    ReplyMsg((struct Message *)io);
	}
    }

    AROS_LIBFUNC_EXIT
}

AROS_LH1(LONG, AbortIO,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 6, ata)
{
    AROS_LIBFUNC_INIT

    /* Cannot Abort IO */
    return 0;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetRdskLba,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 7, ata)
{
    AROS_LIBFUNC_INIT

    return Unit(io)->au_RDBSector;

    AROS_LIBFUNC_EXIT
}

AROS_LH1(ULONG, GetBlkSize,
    AROS_LHA(struct IORequest *, io, A1),
    LIBBASETYPEPTR, LIBBASE, 8, ata)
{
    AROS_LIBFUNC_INIT

    return Unit(io)->au_SectorShift;

    AROS_LIBFUNC_EXIT
}

/*
    The daemon of ata.device first opens all ATAPI devices and then enters
    endless loop. Every 3 seconds it tells ATAPI units to check the media
    presence. In case of any state change they will rise user-specified 
    functions.
*/
static void DaemonCode(LIBBASETYPEPTR LIBBASE);

/* Create the daemon task */
int ata_InitDaemonTask(LIBBASETYPEPTR LIBBASE)
{
    struct Task	    *t;
    struct MemList  *ml;
    
    /* Get some memory */
    t = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    ml = AllocMem(sizeof(struct MemList) + sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR);

    if (t && ml)
    {
	UBYTE *sp = AllocMem(STACK_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
	t->tc_SPLower = sp;
	t->tc_SPUpper = sp + STACK_SIZE;
	t->tc_SPReg   = (UBYTE*)t->tc_SPUpper - SP_OFFSET - sizeof(APTR);
	((APTR *)t->tc_SPUpper)[-1] = LIBBASE;

	ml->ml_NumEntries = 2;
	ml->ml_ME[0].me_Addr = t;
	ml->ml_ME[0].me_Length = sizeof(struct Task);
	ml->ml_ME[1].me_Addr = sp;
	ml->ml_ME[1].me_Length = STACK_SIZE;

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry, &ml->ml_Node);

	t->tc_Node.ln_Name = "ATA.daemon";
	t->tc_Node.ln_Type = NT_TASK;
	t->tc_Node.ln_Pri  = TASK_PRI - 1;  /* The daemon should have a little bit lower Pri as handler tasks */
	
	LIBBASE->ata_Daemon = t;

	AddTask(t, DaemonCode, NULL);
    }

    return (t != NULL);
}

/*
    The daemon tries to send HD_SCSICMD+1 command (internal testchanged 
    command) to all ATAPI devices in the system. They should already handle
    the command further.
*/
void DaemonCode(LIBBASETYPEPTR LIBBASE)
{
    struct MsgPort *mp;		// Message port used with timer.device
    struct MsgPort *myport;	// Message port used with ata.device
    struct timerequest *tr;	// timer's time request message
    struct IOStdReq *ios[MAX_BUS*MAX_UNIT]; // Placeholer for unit messages
    int count = 0,b,d;

    D(bug("[%s] You woke up DAEMON\n",FindTask(NULL)->tc_Node.ln_Name));

    /* Prepare message ports and timer.device's request */
    mp = CreateMsgPort();
    myport = CreateMsgPort();
    tr = (struct timerequest *)CreateIORequest(mp, sizeof(struct timerequest));

    /* Look through all buses in the system */   
    for (b=0; b < MAX_BUS; b++)
    {
	/* Is there anything on the bus? */
	if (LIBBASE->ata_Buses[b])
	{
	    /* Yeah, bus is active. Are there devices? */
	    for (d=0; d < MAX_UNIT; d++)
	    {
		/* Is a device ATAPI? */
		if (LIBBASE->ata_Buses[b]->ab_Dev[d] == DEV_ATAPI)
		{
		    /* Atapi device found. Create IOStdReq for it */
		    ios[count] = (struct IOStdReq *)
			CreateIORequest(myport, sizeof(struct IOStdReq));
			
		    ios[count]->io_Command = HD_SCSICMD + 1;
		    
		    /*
			And force OpenDevice call. Don't use direct call as it's unsafe 
			and not nice at all.
		    */
		    AROS_LVO_CALL3(void, 
			AROS_LCA(struct IORequest *, (struct IORequest *)(ios[count]), A1),
			AROS_LCA(ULONG, (b << 8) | d, D0),
			AROS_LCA(ULONG, 0, D1),
			LIBBASETYPEPTR, LIBBASE, 1, ata);
			
		    /* increase amount of ATAPI devices in system */
		    count++;
		}
	    }
	}
    }

    /* Are there any ATAPI devices found? */
    if (count)
    {
	/* Ok, open the timer.device */
        OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)tr, 0);

        /* Endless lop */
	for (;;)
        {
	    /* call separate IORequest for every ATAPI device */
	    for (b=0; b < count; b++)
		DoIO((struct IORequest *)ios[b]);

	    /* And then hide and wait ;) */
	    tr->tr_node.io_Command = TR_ADDREQUEST;
	    tr->tr_time.tv_secs = 3;
	    tr->tr_time.tv_micro = 0;
	    DoIO((struct IORequest *)tr);
	}
    }
    else
    {
	/* Well, when there are no ATAPI device, daemon is useless. Say goodbay and quit then */
	D(bug("[%s] Deamon useless (no ATAPI devices in system). Bye\n",FindTask(NULL)->tc_Node.ln_Name));
	DeleteMsgPort(myport);
	DeleteMsgPort(mp);
	DeleteIORequest((struct IORequest *)tr);
    }
}

/*
    The Bus task goes here. If you for any reason would put anything below, 
    remember that the SysBase refers now to the LIBBASE, which should be the
    base of ata.device.
*/
#ifdef SysBase
#undef SysBase
#endif
#define SysBase (LIBBASE->ata_SysBase)
#undef LIBBASE
#define LIBBASE (bus->ab_Base)

/*
    As I duplicated task names are not really welcomed, use different names
    for all buses.
*/
static char *TaskNames[] = {
    "ATA.0",
    "ATA.1",
    "ATA.2",
    "ATA.3",
};

static void TaskCode(struct ata_Bus *);

/*
    Make a task for given bus alive.
*/
int ata_InitBusTask(struct ata_Bus *bus, int bus_num)
{
    struct Task	    *t;
    struct MemList  *ml;
    
    /*
	Need some memory. I don't know however, wheter it wouldn't be better
	to take some RAM from device's memory pool.
    */
    t = AllocMem(sizeof(struct Task), MEMF_PUBLIC | MEMF_CLEAR);
    ml = AllocMem(sizeof(struct MemList) + 2*sizeof(struct MemEntry), MEMF_PUBLIC | MEMF_CLEAR);

    if (t && ml)
    {
	/* Setup stack and put the pointer to the bus as the only parameter */
	UBYTE *sp = AllocMem(STACK_SIZE, MEMF_PUBLIC | MEMF_CLEAR);
	t->tc_SPLower = sp;
	t->tc_SPUpper = sp + STACK_SIZE;
	t->tc_SPReg   = (UBYTE*)t->tc_SPUpper - SP_OFFSET - sizeof(APTR);
	((APTR *)t->tc_SPUpper)[-1] = bus;

	/* Message port receiving all the IO requests */
	bus->ab_MsgPort = AllocMem(sizeof(struct MsgPort), MEMF_PUBLIC | MEMF_CLEAR);
	NEWLIST(&bus->ab_MsgPort->mp_MsgList);
	bus->ab_MsgPort->mp_Node.ln_Type = NT_MSGPORT;
	bus->ab_MsgPort->mp_Flags	 = PA_SIGNAL;
	bus->ab_MsgPort->mp_SigBit	 = SIGBREAKB_CTRL_F;
	bus->ab_MsgPort->mp_SigTask	 = t;
	bus->ab_MsgPort->mp_Node.ln_Name = TaskNames[bus_num];

	/* Tell the System, which memory regions are to be freed upon a task completion */
	ml->ml_NumEntries = 3;
	ml->ml_ME[0].me_Addr = t;
	ml->ml_ME[0].me_Length = sizeof(struct Task);
	ml->ml_ME[1].me_Addr = sp;
	ml->ml_ME[1].me_Length = STACK_SIZE;
	ml->ml_ME[2].me_Addr = bus->ab_MsgPort;
	ml->ml_ME[2].me_Length = sizeof(struct MsgPort);

	NEWLIST(&t->tc_MemEntry);
	AddHead(&t->tc_MemEntry, &ml->ml_Node);

	t->tc_Node.ln_Name = TaskNames[bus_num];
	t->tc_Node.ln_Type = NT_TASK;
	t->tc_Node.ln_Pri  = TASK_PRI;
	
	bus->ab_Task = t;

	/* Wake up the task */
	AddTask(t, TaskCode, NULL);
    }

    return (t != NULL);
}

/*
    Bus task body. It doesn't really do much. It recives simply all IORequests 
    in endless lopp and calls proper handling function. The IO is Semaphore-
    protected within a bus.
*/
static void TaskCode(struct ata_Bus *bus)
{
    ULONG sig;
    struct IORequest *msg;
    
    D(bug("[%s] Task started (IO: 0x%x)\n",bus->ab_Task->tc_Node.ln_Name,
	bus->ab_Port));

    /* 
	Prepare timer.device in case some IO commands will try to wait using it
	instead of busy loop delays.
    */
    bus->ab_TimerMP = CreateMsgPort();
    bus->ab_TimerIO = (struct timerequest *)
	CreateIORequest(bus->ab_TimerMP, sizeof(struct timerequest));

    OpenDevice("timer.device", UNIT_VBLANK, (struct IORequest *)bus->ab_TimerIO, 0);

    sig = 1L << bus->ab_MsgPort->mp_SigBit;

    /* Wait forever and process messages */
    for (;;)
    {
	Wait(sig);
	
	/* Even if you get new signal, do not process it until Unit is not active */
	if (!(bus->ab_Flags & UNITF_ACTIVE))
	{
	    bus->ab_Flags |= UNITF_ACTIVE;
	    
	    /* Empty the request queue */
	    while ((msg = (struct IORequest *)GetMsg(bus->ab_MsgPort)))
	    {
		/* And do IO's */
		ObtainSemaphore(&bus->ab_Lock);
		HandleIO(msg, LIBBASE);
		ReleaseSemaphore(&bus->ab_Lock);
		/* TD_ADDCHANGEINT doesn't require reply */
		if (msg->io_Command != TD_ADDCHANGEINT)
		{
		    ReplyMsg((struct Message *)msg);
	    	}
	    }

	    bus->ab_Flags &= ~(UNITF_INTASK | UNITF_ACTIVE);
	}
    }
}


