/*
 * storage_device.c
 *
 *  Created on: Oct 27, 2008
 *      Author: misc
 */

/*
 * This file contains exec style device, usbmss.device, which is available as
 * soon as the mass storage class gets initialized. The device translates the
 * trackdisk commands into mass storage methods.
 */
#include <aros/symbolsets.h>
#include <aros/macros.h>

#include <exec/types.h>
#include <exec/nodes.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <devices/scsidisk.h>
#include <devices/newstyle.h>
#include <oop/oop.h>
#include <dos/dos.h>
#include <dos/dosextens.h>

#include <hidd/hidd.h>
#include <usb/usb.h>
#include <usb/storage.h>

#define DEBUG 1

#include <proto/exec.h>
#include <proto/oop.h>
#include <proto/dos.h>
#include <aros/debug.h>

#include "storage.h"
#include LC_LIBDEFS_FILE

#undef SD
#define SD(x) (&LIBBASE->sd)

/* Invalid command - reports error */
static void cmd_Invalid(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	D(bug("[MSS-dev] Invalid command %d for unit %04x\n", io->io_Command, unit->msu_unitNum));
	io->io_Error = IOERR_NOCMD;
}

/* NULL commad - always success. */
static void cmd_NULL(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	IOStdReq(io)->io_Actual = 0;
}

static void cmd_Reset(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	HIDD_USBStorage_Reset(unit->msu_object);
	IOStdReq(io)->io_Actual = 0;
}

static void cmd_Remove(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	if (unit->msu_removeInt)
		io->io_Error = TDERR_DriveInUse;
	else
		unit->msu_removeInt = (struct Interrupt *)IOStdReq(io)->io_Data;
}

static void cmd_AddChangeInt(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	Forbid();
	AddHead(&unit->msu_diskChangeList, (struct Node *)io);
	Permit();

	io->io_Flags &= ~IOF_QUICK;
	unit->msu_unit.unit_flags &= ~UNITF_ACTIVE;
}

static void cmd_RemChangeInt(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	Forbid();
	Remove((struct Node *)io);
	Permit();
}

static void cmd_ChangeNum(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	IOStdReq(io)->io_Actual = unit->msu_changeNum;
}

static void cmd_ProtStatus(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	IOStdReq(io)->io_Actual = ((unit->msu_inquiry[0] & 0x1f) == DG_CDROM) ? -1 : 0;
}

static void cmd_ChangeState(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	IOStdReq(io)->io_Actual = unit->msu_flags & MSF_DiskPresent ? 0:1;
}

static void cmd_GetGeometry(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	if (!(unit->msu_flags & MSF_DiskPresent))
	{
		io->io_Error = TDERR_DiskChanged;
	}
	else if (IOStdReq(io)->io_Length == sizeof(struct DriveGeometry))
	{
		struct DriveGeometry *dg = (struct DriveGeometry *)IOStdReq(io)->io_Data;

		dg->dg_SectorSize       = unit->msu_blockSize;
		dg->dg_TotalSectors		= unit->msu_blockCount;
		dg->dg_Cylinders		= unit->msu_blockCount / (255*63);
		dg->dg_Heads			= 255;
		dg->dg_TrackSectors		= 63;
		dg->dg_CylSectors		= 255*63;
		dg->dg_BufMemType		= MEMF_PUBLIC;
		dg->dg_DeviceType		= unit->msu_inquiry[0] & 0x1f;
		dg->dg_Flags			= (dg->dg_DeviceType == DG_CDROM) ? DGF_REMOVABLE : 0;
		dg->dg_Reserved			= 0;

		IOStdReq(io)->io_Actual = sizeof(struct DriveGeometry);
	}
	else
		io->io_Error = TDERR_NotSpecified;
}

static void cmd_Read32(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	uint32_t block = IOStdReq(io)->io_Offset;
	uint32_t count = IOStdReq(io)->io_Length;

	if (!(unit->msu_flags & MSF_DiskPresent))
	{
		io->io_Error = TDERR_DiskChanged;
	}
	else
	{
//		D(bug("[%s] Read32(%08x, %08x)\n", FindTask(NULL)->tc_Node.ln_Name, IOStdReq(io)->io_Offset, IOStdReq(io)->io_Length));

		if (!IOStdReq(io)->io_Data)
		{
			D(bug("[%s] No buffer!\n", FindTask(NULL)->tc_Node.ln_Name));
			io->io_Error = IOERR_BADADDRESS;
		}
		else if ((block & ((1 <<  unit->msu_blockShift) - 1)) || (count & ((1 <<  unit->msu_blockShift) - 1)))
		{
			D(bug("[%s] offset or length not sector-aligned\n", FindTask(NULL)->tc_Node.ln_Name));
			cmd_Invalid(io, dev, unit);
		}
		else
		{
			block >>= unit->msu_blockShift;
			count >>= unit->msu_blockShift;

			if (block + count - 1> unit->msu_blockCount)
			{
				D(bug("[%s] ERROR! Requested read outside the available area\n", FindTask(NULL)->tc_Node.ln_Name));
				io->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				if (!HIDD_USBStorage_Read(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
				{
					D(bug("[%s] READ ERROR: block=0x%08x count=0x%08x\n", FindTask(NULL)->tc_Node.ln_Name, block, count));

					HIDD_USBStorage_Reset(unit->msu_object);

					if (!HIDD_USBStorage_Read(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
					{
						IOStdReq(io)->io_Error = TDERR_NotSpecified;
						IOStdReq(io)->io_Actual = 0;

						return;
					}
				}
				IOStdReq(io)->io_Actual = IOStdReq(io)->io_Length;
			}
		}
	}
}

static void cmd_Read64(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	uint64_t block = (uint64_t)(IOStdReq(io)->io_Offset) | ((uint64_t)(IOStdReq(io)->io_Actual) << 32);
	uint32_t count = IOStdReq(io)->io_Length;

	if (!(unit->msu_flags & MSF_DiskPresent))
	{
		io->io_Error = TDERR_DiskChanged;
	}
	else
	{

//		D(bug("[%s] Read64(%04x%08x, %08x)\n", FindTask(NULL)->tc_Node.ln_Name, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, IOStdReq(io)->io_Length));

		io->io_Error = 0;

		if (!IOStdReq(io)->io_Data)
		{
			D(bug("[%s] No buffer!\n", FindTask(NULL)->tc_Node.ln_Name));
			io->io_Error = IOERR_BADADDRESS;
		}
		else if ((block & ((1 <<  unit->msu_blockShift) - 1)) || (count & ((1 <<  unit->msu_blockShift) - 1)))
		{
			D(bug("[%s] offset or length not sector-aligned\n", FindTask(NULL)->tc_Node.ln_Name));
			cmd_Invalid(io, dev, unit);
		}
		else
		{
			block >>= unit->msu_blockShift;
			count >>= unit->msu_blockShift;

			if (block + count - 1> unit->msu_blockCount)
			{
				D(bug("[%s] ERROR! Requested read outside the available area\n", FindTask(NULL)->tc_Node.ln_Name));
				io->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				if (!HIDD_USBStorage_Read(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
				{
					D(bug("[%s] READ ERROR: block=0x%08x count=0x%08x\n", FindTask(NULL)->tc_Node.ln_Name, (int32_t)block, count));

					HIDD_USBStorage_Reset(unit->msu_object);

					if (!HIDD_USBStorage_Read(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
					{
						IOStdReq(io)->io_Error = TDERR_NotSpecified;
						IOStdReq(io)->io_Actual = 0;

						return;
					}
				}
				IOStdReq(io)->io_Actual = IOStdReq(io)->io_Length;
			}
		}
	}
}

static void cmd_Write32(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	uint32_t block = IOStdReq(io)->io_Offset;
	uint32_t count = IOStdReq(io)->io_Length;

	if (!(unit->msu_flags & MSF_DiskPresent))
	{
		io->io_Error = TDERR_DiskChanged;
	}
	else
	{

	//	D(bug("[%s] Write32(%08x, %08x)\n", FindTask(NULL)->tc_Node.ln_Name, IOStdReq(io)->io_Offset, IOStdReq(io)->io_Length));

		if (!IOStdReq(io)->io_Data)
		{
			D(bug("[%s] No buffer!\n", FindTask(NULL)->tc_Node.ln_Name));
			io->io_Error = IOERR_BADADDRESS;
		}
		else if ((block & ((1 <<  unit->msu_blockShift) - 1)) || (count & ((1 <<  unit->msu_blockShift) - 1)))
		{
			D(bug("[%s] offset or length not sector-aligned\n", FindTask(NULL)->tc_Node.ln_Name));
			cmd_Invalid(io, dev, unit);
		}
		else
		{
			block >>= unit->msu_blockShift;
			count >>= unit->msu_blockShift;

			if (block + count - 1> unit->msu_blockCount)
			{
				D(bug("[%s] ERROR! Requested write outside the available area\n", FindTask(NULL)->tc_Node.ln_Name));
				io->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				if (!HIDD_USBStorage_Write(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
				{
					D(bug("[%s] WRITE ERROR: block=0x%08x count=0x%08x\n", FindTask(NULL)->tc_Node.ln_Name, block, count));

					HIDD_USBStorage_Reset(unit->msu_object);

					if (!HIDD_USBStorage_Write(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
					{
						IOStdReq(io)->io_Error = TDERR_NotSpecified;
						IOStdReq(io)->io_Actual = 0;

						return;
					}
				}
				IOStdReq(io)->io_Actual = IOStdReq(io)->io_Length;
			}
		}
	}
}

static void cmd_Write64(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	uint64_t block = (uint64_t)(IOStdReq(io)->io_Offset) | ((uint64_t)(IOStdReq(io)->io_Actual) << 32);
	uint32_t count = IOStdReq(io)->io_Length;

	if (!(unit->msu_flags & MSF_DiskPresent))
	{
		io->io_Error = TDERR_DiskChanged;
	}
	else
	{
//		D(bug("[%s] Write64(%04x%08x, %08x)\n", FindTask(NULL)->tc_Node.ln_Name, IOStdReq(io)->io_Actual, IOStdReq(io)->io_Offset, IOStdReq(io)->io_Length));

		io->io_Error = 0;

		if (!IOStdReq(io)->io_Data)
		{
			D(bug("[%s] No buffer!\n", FindTask(NULL)->tc_Node.ln_Name));
			io->io_Error = IOERR_BADADDRESS;
		}
		else if ((block & ((1 <<  unit->msu_blockShift) - 1)) || (count & ((1 <<  unit->msu_blockShift) - 1)))
		{
			D(bug("[%s] offset or length not sector-aligned\n", FindTask(NULL)->tc_Node.ln_Name));
			cmd_Invalid(io, dev, unit);
		}
		else
		{
			block >>= unit->msu_blockShift;
			count >>= unit->msu_blockShift;

			if (block + count - 1 > unit->msu_blockCount)
			{
				D(bug("[%s] ERROR! Requested write outside the available area\n", FindTask(NULL)->tc_Node.ln_Name));
				io->io_Error = IOERR_BADADDRESS;
			}
			else
			{
				if (!HIDD_USBStorage_Write(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
				{
					D(bug("[%s] WRITE ERROR: block=0x%08x count=0x%08x\n", FindTask(NULL)->tc_Node.ln_Name, (int32_t)block, count));

					HIDD_USBStorage_Reset(unit->msu_object);

					if (!HIDD_USBStorage_Write(unit->msu_object, unit->msu_lun, IOStdReq(io)->io_Data, block, count))
					{
						IOStdReq(io)->io_Error = TDERR_NotSpecified;
						IOStdReq(io)->io_Actual = 0;

						return;
					}
				}
				IOStdReq(io)->io_Actual = IOStdReq(io)->io_Length;
			}
		}
	}
}

static void cmd_DirectSCSI(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit)
{
	struct SCSICmd *cmd = (struct SCSICmd *)IOStdReq(io)->io_Data;

	IOStdReq(io)->io_Error = 0;
	int i;

	D(bug("[%s] DirectSCSI(", FindTask(NULL)->tc_Node.ln_Name));
	for (i=0; i < cmd->scsi_CmdLength; i++)
	{
		D(bug("%02x%c", cmd->scsi_Command[i], i < (cmd->scsi_CmdLength-1) ? ',':')'));
	}
	D(bug("\n"));

	if (cmd)
	{
		cmd->scsi_Actual = 0;
		cmd->scsi_Status = 0;

		/*
		   Handle the USB SCSI quirks. The code is bad because it assumes the
		   cmd->scsi_Command is writable
		*/
		switch(cmd->scsi_Command[0])
		{
			case 0x12:	/* INQUIRY: USB devices deliver the first 36 bytes only */
				if (cmd->scsi_Command[4] > 36)
				{
					cmd->scsi_Command[4] = 36;
					cmd->scsi_Length = 36;
				}
				break;

			case 0x5a:	/* MODE SENSE 10: Only 8 bytes are delivered */
				if (((cmd->scsi_Command[7] << 8) | cmd->scsi_Command[8]) > 8)
				{
					cmd->scsi_Command[7] = 0;
					cmd->scsi_Command[8] = 8;
					cmd->scsi_Length = 8;
				}
				break;

			case 0x03:	/* REQUEST SENSE: USB devices deliver up to 18 bytes only*/
				if (cmd->scsi_Command[4] > 18)
				{
					cmd->scsi_Command[4] = 18;
					cmd->scsi_Length = 18;
				}
				break;

			case 0xec:	/* Hack to issue ATA Inquiry call. Ignore it. */
				if (cmd->scsi_CmdLength == 1)
				{
					IOStdReq(io)->io_Error = IOERR_NOCMD;
					return;
				}
				break;
		}

		if (!HIDD_USBStorage_DirectSCSI(unit->msu_object, unit->msu_lun, cmd->scsi_Command, cmd->scsi_CmdLength, cmd->scsi_Data, cmd->scsi_Length, cmd->scsi_Flags & SCSIF_READ))
		{
			int i;
			char *c = cmd->scsi_Command;
			D(bug("[MSS-dev] DirectSCSI failed: "));

			for (i=0; i < cmd->scsi_CmdLength; i++)
				D(bug("%02x ", c[i]));

			D(bug("data=%08x len=%08x\n", cmd->scsi_Data, cmd->scsi_Length));

			IOStdReq(io)->io_Error = HFERR_BadStatus;

			cmd->scsi_Status = 2;

			if (cmd->scsi_Flags & SCSIF_AUTOSENSE)
			{
				if (cmd->scsi_SenseData && cmd->scsi_SenseLength > 0)
				{
					uint8_t sense[18];

					if (HIDD_USBStorage_RequestSense(unit->msu_object, unit->msu_lun, sense, 18))
					{
						if (cmd->scsi_SenseLength >= 18)
						{
							CopyMem(sense, cmd->scsi_SenseData, 18);
							cmd->scsi_SenseActual = 18;
						}
						else
						{
							CopyMem(sense, cmd->scsi_SenseData, cmd->scsi_SenseLength);
							cmd->scsi_SenseActual = cmd->scsi_SenseLength;
						}
					}
				}
			}
		}
		else
		{
			IOStdReq(io)->io_Error = 0;
			cmd->scsi_Actual = cmd->scsi_Length;
			IOStdReq(io)->io_Actual = IOStdReq(io)->io_Length;
		}
	}
}

static const uint16_t supported_commands[] = {
		CMD_RESET, CMD_READ, CMD_WRITE, CMD_UPDATE, CMD_CLEAR, CMD_START, CMD_STOP, CMD_FLUSH,
		TD_MOTOR, TD_SEEK, TD_FORMAT, TD_REMOVE, TD_CHANGENUM, TD_CHANGESTATE, TD_PROTSTATUS,
		TD_GETNUMTRACKS, TD_ADDCHANGEINT, TD_REMCHANGEINT, TD_GETGEOMETRY, TD_EJECT,
		TD_READ64, TD_WRITE64, TD_SEEK64, TD_FORMAT64,
		HD_SCSICMD, TD_GETDRIVETYPE,
		NSCMD_DEVICEQUERY, NSCMD_TD_READ64, NSCMD_TD_WRITE64, NSCMD_TD_SEEK64, NSCMD_TD_FORMAT64,
		0
};

static void (*map32[HD_SCSICMD+1])(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit) = {
		[CMD_INVALID]		= cmd_Invalid,
		[CMD_RESET]     	= cmd_Reset,
		[CMD_READ]      	= cmd_Read32,
		[CMD_WRITE]     	= cmd_Write32,
		[CMD_UPDATE]    	= cmd_Reset,
		[CMD_CLEAR]     	= cmd_Reset,
		[CMD_STOP]      	= cmd_Reset,
		[CMD_START]     	= cmd_Reset,
		[CMD_FLUSH]     	= cmd_Reset, /* Implement flush! */
		[TD_MOTOR]      	= cmd_NULL,
		[TD_SEEK]       	= cmd_NULL,
		[TD_FORMAT]     	= cmd_Write32,
		[TD_REMOVE]     	= cmd_Remove,
		[TD_CHANGENUM]  	= cmd_ChangeNum,
		[TD_CHANGESTATE]	= cmd_ChangeState,
		[TD_PROTSTATUS] 	= cmd_ProtStatus,
		[TD_RAWREAD]    	= cmd_Invalid,
		[TD_RAWWRITE]   	= cmd_Invalid,
		[TD_GETNUMTRACKS]   = cmd_Invalid,
		[TD_ADDCHANGEINT]   = cmd_AddChangeInt,
		[TD_REMCHANGEINT]   = cmd_RemChangeInt,
		[TD_GETGEOMETRY]	= cmd_GetGeometry,
		[TD_EJECT]      	= cmd_Invalid,
		[TD_READ64]     	= cmd_Read64,
		[TD_WRITE64]    	= cmd_Write64,
		[TD_SEEK64]     	= cmd_NULL,
		[TD_FORMAT64]   	= cmd_Write64,
		[HD_SCSICMD]    	= cmd_DirectSCSI,
};

static void (*map64[4])(struct IORequest *io, mss_device_t *dev, mss_unit_t *unit) = {
		[NSCMD_TD_READ64 - 0xc000] 		= cmd_Read64,
		[NSCMD_TD_WRITE64 - 0xc000] 	= cmd_Write64,
		[NSCMD_TD_SEEK64 - 0xc000] 		= cmd_NULL,
		[NSCMD_TD_FORMAT64 - 0xc000]	= cmd_Write64,
};

/*
 * IO handler. It may be either called directly, or through the mass storage
 * unit task.
 */
void HandleIO(struct IORequest *io, mss_device_t *device, mss_unit_t *unit)
{
	unit->msu_unit.unit_flags |= UNITF_ACTIVE;

	io->io_Error = 0;

	/* Some commands will be handled directly */
	switch (io->io_Command)
	{
	/*
			New Style Devices query. Introduce self as trackdisk and provide list of
			commands supported
	 */
	case NSCMD_DEVICEQUERY:
	{
		struct NSDeviceQueryResult *nsdq = (struct NSDeviceQueryResult *)IOStdReq(io)->io_Data;
		nsdq->DevQueryFormat    = 0;
		nsdq->SizeAvailable     = sizeof(struct NSDeviceQueryResult);
		nsdq->DeviceType        = NSDEVTYPE_TRACKDISK;
		nsdq->DeviceSubType     = 0;
		nsdq->SupportedCommands = (uint16_t *)supported_commands;
		IOStdReq(io)->io_Actual = sizeof(struct NSDeviceQueryResult);
		break;
	}

	/*
			New Style Devices report here the 'NSTY' - only if such value is
			returned here, the NSCMD_DEVICEQUERY might be called. Otherwice it should
			report error.
	 */
	case TD_GETDRIVETYPE:
		IOStdReq(io)->io_Actual = DRIVE_NEWSTYLE;
		break;

	default:
		if (io->io_Command <= HD_SCSICMD)
		{
			if (map32[io->io_Command])
				map32[io->io_Command](io, device, unit);
			else
				cmd_Invalid(io, device, unit);
		}
		else if (io->io_Command >= NSCMD_TD_READ64 && io->io_Command <= NSCMD_TD_FORMAT64)
		{
			if (map64[io->io_Command - NSCMD_TD_READ64])
				map64[io->io_Command - NSCMD_TD_READ64](io, device, unit);
			else
				cmd_Invalid(io, device, unit);
		}
		break;
	}

	unit->msu_unit.unit_flags &= ~(UNITF_ACTIVE);
}



/*
 * Exec device functions
 */

static
AROS_LH3 (void, dev_OpenLib,
		AROS_LHA(struct IORequest *, ioreq, A1),
		AROS_LHA(ULONG, unitnum, D0),
		AROS_LHA(ULONG, flags, D1),
		mss_device_t *, lh, 1, Storage)
		{
	AROS_LIBFUNC_INIT

	mss_unit_t *unit, *found = NULL;

	ioreq->io_Error = IOERR_OPENFAIL;

	D(bug("[MSS-dev] Open(%p, %x, %x)\n", ioreq, unitnum, flags));

	/* Lock the whole mass storage class and search for a requested unit */
	ObtainSemaphore(&lh->mss_static->Lock);
	ForeachNode(&lh->mss_static->unitList, unit)
	{
		if (unit->msu_unitNum == unitnum)
		{
			found = unit;
			break;
		}
	}
	if (found)
	{
		ioreq->io_Device = &lh->mss_device;
		ioreq->io_Unit = (struct Unit *)found;
		ioreq->io_Error = 0;

		found->msu_unit.unit_OpenCnt++;
	}
	ReleaseSemaphore(&lh->mss_static->Lock);

	D(bug("[MSS-dev] %sfound unit %d at %p\n", found?"":"not ", unitnum, found));

	AROS_LIBFUNC_EXIT
		}

static
AROS_LH1(BPTR, dev_CloseLib,
		AROS_LHA(struct IORequest *, ioreq, A1),
		mss_device_t *, lh, 2, Storage)
		{
	AROS_LIBFUNC_INIT

	/* Release the unit */
	ObtainSemaphore(&lh->mss_static->Lock);
	mss_unit_t *unit = (mss_unit_t *)ioreq->io_Unit;

	ioreq->io_Unit = (struct Unit *)~0;
	if (unit && unit->msu_unit.unit_OpenCnt)
		unit->msu_unit.unit_OpenCnt--;

	ReleaseSemaphore(&lh->mss_static->Lock);

	return NULL;

	AROS_LIBFUNC_EXIT
		}

static
AROS_LH1 (BPTR, dev_ExpungeLib,
		AROS_LHA(mss_device_t *, extralh, D0),
		mss_device_t *, lh, 3, Storage)
		{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
		}

static
AROS_LH0 (LIBBASETYPEPTR, dev_ExtFuncLib,
		mss_device_t *, lh, 4, Storage)
		{
	AROS_LIBFUNC_INIT
	return NULL;
	AROS_LIBFUNC_EXIT
		}

/*
 * Helper function used to test, whether the specified command is slow or not.
 * Slow commands will be transfered to the unit task, all others will be handled
 * immediately.
 *
 * NOTE: Every command performing *any* USB transfer/operation is considered slow.
 */
static const uint64_t immediate_cmds = (
		(1 << CMD_INVALID) |
		(1 << CMD_CLEAR) |
		(1 << TD_MOTOR) |
		(1 << TD_SEEK) |
		(1 << TD_RAWREAD) |
		(1 << TD_RAWWRITE) |
		(1 << TD_CHANGENUM) |
		(1 << TD_CHANGESTATE) |
		(1 << TD_ADDCHANGEINT) |
		(1 << TD_REMCHANGEINT) |
		(1 << TD_GETGEOMETRY) |
		(1 << TD_SEEK64) |
		(1 << TD_GETDRIVETYPE)
);

static BOOL isSlow(uint32_t cmd)
{
	BOOL slow = TRUE;

	if (cmd < 32)
	{
		if (immediate_cmds & (1 << cmd))
			slow = FALSE;
	}
	else if (cmd == NSCMD_TD_SEEK64 || cmd == NSCMD_DEVICEQUERY)
		slow = FALSE;

//	D(bug("[MSS-dev] isSlow(%04x) = %d\n", cmd, slow));

	return slow;
}

static
AROS_LH1(void, dev_BeginIO,
		AROS_LHA(struct IORequest *, io, A1),
		mss_device_t *, mss_dev, 5, Storage)
		{
	AROS_LIBFUNC_INIT

//	D(bug("[MSS-dev] BeginIO. CMD=%04x\n", io->io_Command));

	if (io)
	{
		mss_unit_t *unit = (mss_unit_t *)io->io_Unit;

		if (unit && unit != (void *)0xffffffff)
		{
			if (isSlow(io->io_Command))
			{
//				D(bug("[MSS-dev] Forwarding IO call to device task\n"));
				unit->msu_unit.unit_flags |= UNITF_INTASK;
				io->io_Flags &= ~IOF_QUICK;

				PutMsg(&unit->msu_unit.unit_MsgPort, &io->io_Message);

				unit->msu_unit.unit_flags &= ~UNITF_INTASK;
			}
			else
				HandleIO(io, mss_dev, unit);
		}
	}

	AROS_LIBFUNC_EXIT
		}

static
AROS_LH1(LONG, dev_AbortIO,
		AROS_LHA(struct IORequest *, io, A1),
		mss_device_t *, mss_dev, 6, Storage)
		{
	AROS_LIBFUNC_INIT

	/* Cannot Abort IO */
	return 0;

	AROS_LIBFUNC_EXIT
		}

static
AROS_LH1(ULONG, dev_GetRdskLba,
		AROS_LHA(struct IORequest *, io, A1),
		mss_device_t *, mss_dev, 7, Storage)
		{
	AROS_LIBFUNC_INIT

	return 0;

	AROS_LIBFUNC_EXIT
		}

static
AROS_LH1(ULONG, dev_GetBlkSize,
		AROS_LHA(struct IORequest *, io, A1),
		mss_device_t *, mss_dev, 8, Storage)
		{
	AROS_LIBFUNC_INIT

	mss_unit_t *unit = (mss_unit_t *)io->io_Unit;

	if (unit->msu_blockSize)
		return AROS_LEAST_BIT_POS(unit->msu_blockSize);
	else
		return 9;

	AROS_LIBFUNC_EXIT
		}

static const APTR Storage_dev_FuncTable[]=
{
		&AROS_SLIB_ENTRY(dev_OpenLib,Storage,1),
		&AROS_SLIB_ENTRY(dev_CloseLib,Storage,2),
		&AROS_SLIB_ENTRY(dev_ExpungeLib,Storage,3),
		&AROS_SLIB_ENTRY(dev_ExtFuncLib,Storage,4),
		&AROS_SLIB_ENTRY(dev_BeginIO,Storage,5),
		&AROS_SLIB_ENTRY(dev_AbortIO,Storage,6),
		&AROS_SLIB_ENTRY(dev_GetRdskLba,Storage,7),
		&AROS_SLIB_ENTRY(dev_GetBlkSize,Storage,8),
		(void *)-1
};

static int StorageDev_Init(LIBBASETYPEPTR LIBBASE)
{
	D(bug("[MSS] USB Mass Storage device layer init\n"));

	mss_device_t *dev = (mss_device_t *)
	((intptr_t)AllocPooled(SD(LIBBASE)->MemPool,
			sizeof(mss_device_t) + sizeof(Storage_dev_FuncTable)) + sizeof(Storage_dev_FuncTable));

	dev->mss_static = SD(LIBBASE);

	dev->mss_device.dd_Library.lib_Node.ln_Name = "usbmss.device";
	dev->mss_device.dd_Library.lib_Node.ln_Type = NT_DEVICE;
	dev->mss_device.dd_Library.lib_Node.ln_Pri = 0;
	dev->mss_device.dd_Library.lib_Version = 1;
	dev->mss_device.dd_Library.lib_Revision = 0;

	MakeFunctions(dev, Storage_dev_FuncTable, NULL);
	AddDevice((struct Device *)dev);

	return TRUE;
}

static int StorageDev_Expunge(LIBBASETYPEPTR LIBBASE)
{
	return FALSE;
}

ADD2INITLIB(StorageDev_Init, 1)
ADD2EXPUNGELIB(StorageDev_Expunge, 1)
