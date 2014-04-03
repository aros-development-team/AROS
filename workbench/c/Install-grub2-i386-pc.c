/*
    Copyright © 1995-2012, The AROS Development Team. All rights reserved.
    $Id$
*/
/******************************************************************************


    NAME

        Install-grub2-i386-pc

    SYNOPSIS

        DEVICE/A, UNIT/N/K/A, PARTITIONNUMBER=PN/K/N, GRUB/K/A, FORCELBA/S

    LOCATION

        C:

    FUNCTION

        Installs the GRUB 2 bootloader to the boot block of the specified
        disk or partition.

    INPUTS

        DEVICE --  Device name (e.g. ata.device)
        UNIT  --  Unit number
        PN  --  Specifies a partition number. If specified, GRUB is installed
            to this partition's boot block. Otherwise, GRUB is installed to
            the disk's boot block.
        GRUB -- Path to GRUB directory.
        FORCELBA --  Force use of LBA mode.

    RESULT

    NOTES
	
    EXAMPLE

        Install-grub2-i386-pc DEVICE ata.device UNIT 0 GRUB DH0:boot/grub

    BUGS

    SEE ALSO

        Partition, SYS:System/Format
	
    INTERNALS

******************************************************************************/

#define DEBUG 0
#include <aros/debug.h>

#include <string.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/partition.h>
#include <proto/utility.h>
#include <aros/macros.h>
#include <devices/hardblocks.h>
#include <devices/newstyle.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <libraries/partition.h>



/* Defines for grub2 data */
/* boot.img pointers */
#define GRUB_BOOT_MACHINE_BPB_START         0x03
#define GRUB_BOOT_MACHINE_BPB_END           0x5a
#define GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC	0x01b8 /* Following grub2 grub-setup sources */
#define GRUB_BOOT_MACHINE_PART_START        0x01be
#define GRUB_BOOT_MACHINE_PART_END          0x01fe
#define GRUB_BOOT_MACHINE_KERNEL_SECTOR	    0x5c
#define GRUB_BOOT_MACHINE_BOOT_DRIVE        0x64
#define GRUB_BOOT_MACHINE_DRIVE_CHECK       0x66

/* core.img pointers */
#define GRUB_DECOMPRESSOR_I386_PC_BOOT_DEVICE 0x18

/* BIOS drive flag */
#define BIOS_HDISK_FLAG		0x80

#define MBR_MAX_PARTITIONS	4
#define MBRT_EXTENDED		0x05
#define MBRT_EXTENDED2		0x0f
#define BLCKLIST_ELEMENTS   14

struct Volume
{
    struct MsgPort *mp;
    struct IOExtTD *iotd;
    ULONG readcmd;
    ULONG writecmd;
    ULONG startblock;
    ULONG countblock;
    CONST_STRPTR device;
    ULONG unitnum;
    UWORD SizeBlock;
    UBYTE flags;
    BYTE partnum;
    ULONG *blockbuffer;
    ULONG dos_id;
};

#define VF_IS_TRACKDISK (1<<0)
#define VF_IS_RDB       (1<<1)

struct BlockNode
{
    ULONG sector_lo;
    ULONG sector_hi;
    UWORD count;
    UWORD seg_adr;
};

const TEXT version[] = "$VER: Install-grub2-i386-pc 41.4 (15.9.2012)";

CONST_STRPTR CORE_IMG_FILE_NAME = "i386-pc/core.img";

STRPTR template =
    (STRPTR) ("DEVICE/A," "UNIT/N/K/A," "PARTITIONNUMBER=PN/K/N," "GRUB/K/A,"
	      "FORCELBA/S");

IPTR myargs[7] = { 0, 0, 0, 0, 0, 0 };

struct FileSysStartupMsg *getDiskFSSM(CONST_STRPTR path)
{
    struct DosList *dl;
    struct DeviceNode *dn;
    TEXT dname[32];
    UBYTE i;

    D(bug("[install] getDiskFSSM('%s')\n", path));

    for (i = 0; (path[i]) && (path[i] != ':'); i++)
	dname[i] = path[i];
    if (path[i] == ':')
    {
	dname[i] = 0;
	dl = LockDosList(LDF_READ);
	if (dl)
	{
	    dn = (struct DeviceNode *) FindDosEntry(dl, dname, LDF_DEVICES);
	    UnLockDosList(LDF_READ);
	    if (dn)
	    {
                dname[i] = ':';
                dname[i + 1] = '\0';
		if (IsFileSystem(dname))
		{
		    return (struct FileSysStartupMsg *) BADDR(dn->dn_Startup);
		}
		else
		    Printf("device '%s' doesn't contain a file system\n",
			   dname);
	    }
	    else
		PrintFault(ERROR_OBJECT_NOT_FOUND, dname);
	}
    }
    else
	Printf("'%s' doesn't contain a device name\n", path);
    return 0;
}

void fillGeometry(struct Volume *volume, struct DosEnvec *de)
{
    ULONG spc;

    D(bug("[install] fillGeometry(%x)\n", volume));

    spc = de->de_Surfaces * de->de_BlocksPerTrack;
    volume->SizeBlock = de->de_SizeBlock;
    volume->startblock = de->de_LowCyl * spc;
    volume->countblock =
	((de->de_HighCyl - de->de_LowCyl + 1) * spc) - 1 + de->de_Reserved;
}

void nsdCheck(struct Volume *volume)
{
    struct NSDeviceQueryResult nsdq;
    UWORD *cmdcheck;

    D(bug("[install] nsdCheck(%x)\n", volume));

    if (((volume->startblock + volume->countblock) *	/* last block */
	 ((volume->SizeBlock << 2) / 512)	/* 1 portion (block) equals 512 (bytes) */
	) > 8388608)
    {
	nsdq.SizeAvailable = 0;
	nsdq.DevQueryFormat = 0;
	volume->iotd->iotd_Req.io_Command = NSCMD_DEVICEQUERY;
	volume->iotd->iotd_Req.io_Data = &nsdq;
	volume->iotd->iotd_Req.io_Length = sizeof(struct NSDeviceQueryResult);
	if (DoIO((struct IORequest *) &volume->iotd->iotd_Req) == IOERR_NOCMD)
	{
	    Printf("Device doesn't understand NSD-Query\n");
	}
	else
	{
	    if ((volume->iotd->iotd_Req.io_Actual >
		 sizeof(struct NSDeviceQueryResult))
		|| (volume->iotd->iotd_Req.io_Actual == 0)
		|| (volume->iotd->iotd_Req.io_Actual != nsdq.SizeAvailable))
	    {
		Printf("WARNING wrong io_Actual using NSD\n");
	    }
	    else
	    {
		if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
		    Printf("WARNING no trackdisk type\n");
		for (cmdcheck = nsdq.SupportedCommands; *cmdcheck; cmdcheck++)
		{
		    if (*cmdcheck == NSCMD_TD_READ64)
			volume->readcmd = NSCMD_TD_READ64;
		    if (*cmdcheck == NSCMD_TD_WRITE64);
		    volume->writecmd = NSCMD_TD_WRITE64;
		}
		if ((volume->readcmd != NSCMD_TD_READ64) ||
		    (volume->writecmd != NSCMD_TD_WRITE64))
		    Printf("WARNING no READ64/WRITE64\n");
	    }
	}
    }
}

struct Volume *initVolume(CONST_STRPTR device, ULONG unit, ULONG flags,
			  struct DosEnvec *de)
{
    struct Volume *volume;
    LONG error = 0;

    D(bug("[install] initVolume(%s:%d)\n", device, unit));

    volume = AllocVec(sizeof(struct Volume), MEMF_PUBLIC | MEMF_CLEAR);
    if (volume)
    {
	volume->mp = CreateMsgPort();
	if (volume->mp)
	{
	    volume->iotd =
		(struct IOExtTD *) CreateIORequest(volume->mp,
						   sizeof(struct IOExtTD));
	    if (volume->iotd)
	    {
		volume->blockbuffer =
		    AllocVec(de->de_SizeBlock << 2, MEMF_PUBLIC | MEMF_CLEAR);
		if (volume->blockbuffer)
		{
		    if (OpenDevice
			(device,
			 unit, (struct IORequest *) volume->iotd, flags) == 0)
		    {
			if (strcmp((const char *) device, TD_NAME) == 0)
			    volume->flags |= VF_IS_TRACKDISK;
			else
			    volume->flags |= VF_IS_RDB;	/* just assume we have RDB */
			volume->readcmd = CMD_READ;
			volume->writecmd = CMD_WRITE;
			volume->device = device;
			volume->unitnum = unit;
            volume->dos_id = 0;
			fillGeometry(volume, de);
			nsdCheck(volume);
			return volume;
		    }
		    else
			error = ERROR_NO_FREE_STORE;
		    FreeVec(volume->blockbuffer);
		}
		else
		    error = ERROR_NO_FREE_STORE;
		DeleteIORequest((struct IORequest *) volume->iotd);
	    }
	    else
		error = ERROR_NO_FREE_STORE;
	    DeleteMsgPort(volume->mp);
	}
	else
	    error = ERROR_NO_FREE_STORE;
	FreeVec(volume);
    }
    else
	error = ERROR_NO_FREE_STORE;

    PrintFault(error, NULL);
    return NULL;
}

void uninitVolume(struct Volume *volume)
{
    D(bug("[install] uninitVolume(%x)\n", volume));

    CloseDevice((struct IORequest *) volume->iotd);
    FreeVec(volume->blockbuffer);
    DeleteIORequest((struct IORequest *) volume->iotd);
    DeleteMsgPort(volume->mp);
    FreeVec(volume);
}

static ULONG _readwriteBlock(struct Volume *volume,
			     ULONG block, APTR buffer, ULONG length,
			     ULONG command)
{
    UQUAD offset;
    ULONG retval = 0;

    volume->iotd->iotd_Req.io_Command = command;
    volume->iotd->iotd_Req.io_Length = length;
    volume->iotd->iotd_Req.io_Data = buffer;
    offset = (UQUAD) (volume->startblock + block) * (volume->SizeBlock << 2);
    volume->iotd->iotd_Req.io_Offset = offset & 0xFFFFFFFF;
    volume->iotd->iotd_Req.io_Actual = offset >> 32;
    retval = DoIO((struct IORequest *) &volume->iotd->iotd_Req);
    if (volume->flags & VF_IS_TRACKDISK)
    {
	    volume->iotd->iotd_Req.io_Command = TD_MOTOR;
	    volume->iotd->iotd_Req.io_Length = 0;
	    DoIO((struct IORequest *) &volume->iotd->iotd_Req);
    }
    return retval;
}

ULONG readBlock(struct Volume * volume, ULONG block, APTR buffer, ULONG size)
{
    D(bug("[install] readBlock(vol:%x, block:%d, %d bytes)\n",
	  volume, block, size));

    return _readwriteBlock(volume, block, buffer, size, volume->readcmd);
}

ULONG writeBlock(struct Volume * volume, ULONG block, APTR buffer, ULONG size)
{
    D(bug("[install] writeBlock(vol:%x, block:%d, %d bytes)\n",
	  volume, block, size));

    return _readwriteBlock(volume, block, buffer, size, volume->writecmd);
}

static BOOL isKnownFs(ULONG dos_id)
{
    switch (dos_id)
    {
        case ID_FFS_DISK:
        case ID_INTER_DOS_DISK:
        case ID_INTER_FFS_DISK:
        case ID_FASTDIR_DOS_DISK:
        case ID_FASTDIR_FFS_DISK:
        case ID_SFS_BE_DISK:
        case ID_SFS_LE_DISK:
        	return TRUE;
    }

    return FALSE;
}

BOOL isvalidFileSystem(struct Volume * volume, CONST_STRPTR device,
		       ULONG unit)
{
    BOOL retval = FALSE;
    struct PartitionBase *PartitionBase;
    struct PartitionHandle *ph;
    ULONG dos_id;

    D(bug("[install] isvalidFileSystem(%x, %s, %d)\n", volume, device, unit));

    if (readBlock(volume, 0, volume->blockbuffer, 512))
    {
	Printf("Read Error\n");
	return FALSE;
    }

    dos_id = AROS_BE2LONG(volume->blockbuffer[0]);

    if (!isKnownFs(dos_id))
    {
      	/* first block has no DOS\x so we don't have RDB for sure */
	    volume->flags &= ~VF_IS_RDB;
	    if (readBlock(volume, 1, volume->blockbuffer, 512))
	    {
	        Printf("Read Error\n");
	        return FALSE;
	    }

	    dos_id = AROS_BE2LONG(volume->blockbuffer[0]);

	    if (!isKnownFs(dos_id))
	        return FALSE;
        else
            volume->dos_id = dos_id;
    }
    else
        volume->dos_id = dos_id;

    volume->partnum = -1;

    PartitionBase =
	(struct PartitionBase *) OpenLibrary((CONST_STRPTR)
					     "partition.library", 1);
    if (PartitionBase)
    {
	ph = OpenRootPartition(device, unit);
	if (ph)
	{
	    if (OpenPartitionTable(ph) == 0)
	    {
		struct TagItem tags[3];
		IPTR type;

		tags[1].ti_Tag = TAG_DONE;
		tags[0].ti_Tag = PTT_TYPE;
		tags[0].ti_Data = (STACKIPTR) & type;
		GetPartitionTableAttrs(ph, tags);
		if (type == PHPTT_MBR)
		{
		    struct PartitionHandle *pn;
		    struct DosEnvec de;
		    struct PartitionHandle *extph = NULL;
		    struct PartitionType ptype = { };

		    tags[0].ti_Tag = PT_DOSENVEC;
		    tags[0].ti_Data = (STACKIPTR) & de;
		    tags[1].ti_Tag = PT_TYPE;
		    tags[1].ti_Data = (STACKIPTR) & ptype;
		    tags[2].ti_Tag = TAG_DONE;
		    pn = (struct PartitionHandle *) ph->table->list.lh_Head;
		    while (pn->ln.ln_Succ)
		    {
			ULONG scp;

			GetPartitionAttrs(pn, tags);
			if (ptype.id[0] == MBRT_EXTENDED
			    || ptype.id[0] == MBRT_EXTENDED2)
			    extph = pn;
			else
			{
			    scp = de.de_Surfaces * de.de_BlocksPerTrack;
			    if ((volume->startblock >= (de.de_LowCyl * scp))
				&& (volume->startblock <=
				    (((de.de_HighCyl + 1) * scp) - 1)))
				break;
			}
			pn = (struct PartitionHandle *) pn->ln.ln_Succ;
		    }
		    if (pn->ln.ln_Succ)
		    {
			tags[0].ti_Tag = PT_POSITION;
			tags[0].ti_Data = (STACKIPTR) & type;
			tags[1].ti_Tag = TAG_DONE;
			GetPartitionAttrs(pn, tags);
			volume->partnum = (UBYTE) type;
			retval = TRUE;
			D(bug
			  ("[install] Primary partition found: partnum=%d\n",
			   volume->partnum));
		    }
		    else if (extph != NULL)
		    {
			if (OpenPartitionTable(extph) == 0)
			{
			    tags[0].ti_Tag = PTT_TYPE;
			    tags[0].ti_Data = (STACKIPTR) & type;
			    tags[1].ti_Tag = TAG_DONE;
			    GetPartitionTableAttrs(extph, tags);
			    if (type == PHPTT_EBR)
			    {
				tags[0].ti_Tag = PT_DOSENVEC;
				tags[0].ti_Data = (STACKIPTR) & de;
				tags[1].ti_Tag = TAG_DONE;
				pn = (struct PartitionHandle *) extph->table->
				    list.lh_Head;
				while (pn->ln.ln_Succ)
				{
				    ULONG offset, scp;

				    offset = extph->de.de_LowCyl
					* extph->de.de_Surfaces
					* extph->de.de_BlocksPerTrack;
				    GetPartitionAttrs(pn, tags);
				    scp =
					de.de_Surfaces * de.de_BlocksPerTrack;
				    if ((volume->startblock >=
					 offset + (de.de_LowCyl * scp))
					&& (volume->startblock <=
					    offset +
					    (((de.de_HighCyl + 1) * scp) -
					     1)))
					break;
				    pn = (struct PartitionHandle *) pn->ln.
					ln_Succ;
				}
				if (pn->ln.ln_Succ)
				{
				    tags[0].ti_Tag = PT_POSITION;
				    tags[0].ti_Data = (STACKIPTR) & type;
				    GetPartitionAttrs(pn, tags);
				    volume->partnum =
					MBR_MAX_PARTITIONS + (UBYTE) type;
				    retval = TRUE;
				    D(bug
				      ("[install] Logical partition found: partnum=%d\n",
				       (int) volume->partnum));
				}
			    }
			    ClosePartitionTable(extph);
			}
		    }
		}
		else
		{
		    if (type == PHPTT_RDB)
		    {
			/* just use whole hard disk */
			retval = TRUE;
		    }
		    else
			Printf
			    ("only MBR and RDB partition tables are supported\n");
		}
		ClosePartitionTable(ph);
	    }
	    else
	    {
		/* just use whole hard disk */
		retval = TRUE;
	    }
	    CloseRootPartition(ph);
	}
	else
	    Printf("Error OpenRootPartition(%s,%lu)\n", device, (long)unit);
	CloseLibrary((struct Library *) PartitionBase);
    }
    else
	Printf("Couldn't open partition.library\n");
    return retval;
}

struct Volume *getGrubStageVolume(CONST_STRPTR device, ULONG unit,
				  ULONG flags, struct DosEnvec *de)
{
    struct Volume *volume;

    volume = initVolume(device, unit, flags, de);

    D(bug("[install] getGrubStageVolume(): volume=%x\n", volume));

    if (volume)
    {
	if (isvalidFileSystem(volume, device, unit))
	    return volume;
	else
	{
	    Printf("stage2 is on an unsupported file system\n");
	    PrintFault(ERROR_OBJECT_WRONG_TYPE, NULL);
	}
	uninitVolume(volume);
    }
    return 0;
}

BOOL isvalidPartition(CONST_STRPTR device, ULONG unit, LONG * pnum,
		      struct DosEnvec * de)
{
    struct PartitionBase *PartitionBase;
    struct PartitionHandle *ph;
    ULONG type;
    BOOL retval = FALSE;

    D(bug
      ("[install] isvalidPartition(%s:%d, part:%d)\n", device, unit, pnum));

    PartitionBase =
	(struct PartitionBase *) OpenLibrary((CONST_STRPTR)
					     "partition.library", 1);
    if (PartitionBase)
    {
	ph = OpenRootPartition(device, unit);
	if (ph)
	{
	    struct TagItem tags[2];

	    tags[1].ti_Tag = TAG_DONE;
	    /* is there a partition table? */
	    if (OpenPartitionTable(ph) == 0)
	    {
		if (pnum)
		{
		    /* install into partition bootblock */
		    tags[0].ti_Tag = PTT_TYPE;
		    tags[0].ti_Data = (STACKIPTR) & type;
		    GetPartitionTableAttrs(ph, tags);
		    if (type == PHPTT_MBR)
		    {
			struct PartitionHandle *pn;

			/* search for partition */
			tags[0].ti_Tag = PT_POSITION;
			tags[0].ti_Data = (STACKIPTR) & type;
			pn = (struct PartitionHandle *) ph->table->list.
			    lh_Head;
			while (pn->ln.ln_Succ)
			{
			    GetPartitionAttrs(pn, tags);
			    if (type == *pnum)
				break;
			    pn = (struct PartitionHandle *) pn->ln.ln_Succ;
			}
			if (pn->ln.ln_Succ)
			{
			    struct PartitionType ptype;

			    /* is it an AROS partition? */
			    tags[0].ti_Tag = PT_TYPE;
			    tags[0].ti_Data = (STACKIPTR) & ptype;
			    GetPartitionAttrs(pn, tags);
			    if (ptype.id[0] == 0x30)
			    {
				tags[0].ti_Tag = PT_DOSENVEC;
				tags[0].ti_Data = (STACKIPTR) de;
				GetPartitionAttrs(pn, tags);
				retval = TRUE;
			    }
			    else
				Printf
				    ("partition is not of type AROS (0x30)\n");
			}
			else
			{
			    Printf
				("partition %ld not found on device %s unit %lu\n",
				 (long)*pnum, device, (long)unit);
			}
		    }
		    else
			Printf
			    ("you can only install in partitions which are MBR partitioned\n");
		}
		else
		{
		    /* install into MBR */
		    tags[0].ti_Tag = PTT_TYPE;
		    tags[0].ti_Data = (STACKIPTR) & type;
		    GetPartitionTableAttrs(ph, tags);
		    if ((type == PHPTT_MBR) || (type == PHPTT_RDB))
		    {
			tags[0].ti_Tag = PT_DOSENVEC;
			tags[0].ti_Data = (STACKIPTR) de;
			GetPartitionAttrs(ph, tags);
			retval = TRUE;
		    }
		    else
			Printf
			    ("partition table type must be either MBR or RDB\n");
		}
		ClosePartitionTable(ph);
	    }
	    else
	    {
		/* FIXME: GetPartitionAttr() should always work for root partition */
		CopyMem(&ph->de, de, sizeof(struct DosEnvec));
		retval = TRUE;
	    }
	    CloseRootPartition(ph);
	}
	else
	    Printf("Error OpenRootPartition(%s,%lu)\n", device, (long)unit);
	CloseLibrary((struct Library *) PartitionBase);
    }
    else
	Printf("Couldn't open partition.library\n");
    return retval;
}

struct Volume *getBBVolume(CONST_STRPTR device, ULONG unit, LONG * partnum)
{
    struct Volume *volume;
    struct DosEnvec de;

    D(bug("[install] getBBVolume(%s:%d, %d)\n", device, unit, partnum));

    if (isvalidPartition(device, unit, partnum, &de))
    {
	volume = initVolume(device, unit, 0, &de);
	volume->partnum = partnum ? *partnum : -1;
	readBlock(volume, 0, volume->blockbuffer, 512);
	if (AROS_BE2LONG(volume->blockbuffer[0]) != IDNAME_RIGIDDISK)
	{
	    /* Clear the boot sector region! */
	    memset(volume->blockbuffer, 0x00, 446);
	    return volume;
	}
	else
	    Printf("no space for bootblock (RDB is on block 0)\n");
    }
    return NULL;
}

/* Convert a unit number into a drive number as understood by GRUB */
UWORD getDriveNumber(CONST_STRPTR device, ULONG unit)
{
    struct PartitionHandle *ph;
    ULONG i;
    UWORD hd_count = 0;

    for (i = 0; i < unit; i++)
    {
        ph = OpenRootPartition(device, i);
        if (ph != NULL)
        {
            hd_count++;
            CloseRootPartition(ph);
        }
    }

    return hd_count;
}

BOOL writeBootIMG(STRPTR bootimgpath, struct Volume * bootimgvol, struct Volume * coreimgvol, 
                ULONG block /* first block of core.img file */, ULONG unit)
{
    BOOL retval = FALSE;
    LONG error = 0;
    BPTR fh;

    D(bug("[install] writeBootIMG(%x)\n", bootimgvol));

    fh = Open(bootimgpath, MODE_OLDFILE);
    if (fh)
    {
	    if (Read(fh, bootimgvol->blockbuffer, 512) == 512)
	    {
	        /* install into MBR ? */
	        if ((bootimgvol->startblock == 0)
		    && (!(bootimgvol->flags & VF_IS_TRACKDISK)))
	        {
		        APTR boot_img = bootimgvol->blockbuffer;

		        UBYTE *boot_drive = 
                    (UBYTE *) (boot_img + GRUB_BOOT_MACHINE_BOOT_DRIVE);
		        UWORD *boot_drive_check =
		            (UWORD *) (boot_img + GRUB_BOOT_MACHINE_DRIVE_CHECK);

                if (unit == bootimgvol->unitnum)
                    *boot_drive = 0xFF;
                else
                    *boot_drive = getDriveNumber(coreimgvol->device, unit)
                        | BIOS_HDISK_FLAG;
		        *boot_drive_check = 0x9090;

		        D(bug("[install] writeBootIMG: Install to HARDDISK\n"));

		        /* read old MBR */
		        error = readBlock(bootimgvol, 0, coreimgvol->blockbuffer, 512);

		        D(bug("[install] writeBootIMG: MBR Buffer @ %x\n", bootimgvol->blockbuffer));
		        D(bug("[install] writeBootIMG: Copying MBR BPB to %x\n",
		           (char *) bootimgvol->blockbuffer + GRUB_BOOT_MACHINE_BPB_START));
		        /* copy BPB (BIOS Parameter Block) */
		        CopyMem
		            ((APTR) ((char *) coreimgvol->blockbuffer + GRUB_BOOT_MACHINE_BPB_START),
		             (APTR) ((char *) bootimgvol->blockbuffer + GRUB_BOOT_MACHINE_BPB_START),
		             (GRUB_BOOT_MACHINE_BPB_END - GRUB_BOOT_MACHINE_BPB_START));

		        /* copy partition table - [Overwrites Floppy boot code] */
		        D(bug("[install] writeBootIMG: Copying MBR Partitions to %x\n",
		           (char *) bootimgvol->blockbuffer + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC));
		        CopyMem((APTR) ((char *) coreimgvol->blockbuffer + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC),
			        (APTR) ((char *) bootimgvol->blockbuffer + GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC),
			        (GRUB_BOOT_MACHINE_PART_END - GRUB_BOOT_MACHINE_WINDOWS_NT_MAGIC));

		        /* Store the core.img pointer .. */
		        ULONG * coreimg_sector_start = (ULONG *) (boot_img
							        + GRUB_BOOT_MACHINE_KERNEL_SECTOR);
		        coreimg_sector_start[0] = block;
		        D(bug("[install] writeBootIMG: core.img pointer = %ld\n", block));
	        }
	        else
	        {
		        D(bug("[install] writeBootIMG: Install to FLOPPY\n"));
	        }

	        if (error == 0)
	        {
		        error = writeBlock(bootimgvol, 0, bootimgvol->blockbuffer, 512);
                
		        if (error)
		            Printf("WriteError %lu\n", (long)error);
		        else
		            retval = TRUE;
	        }
	        else
		        Printf("WriteError %lu\n", (long)error);
	    }
	    else
	        Printf("%s: Read Error\n", bootimgpath);
	    Close(fh);
    }
    else
	    PrintFault(IoErr(), bootimgpath);

    return retval;
}

/* Collects the list of blocks that a file occupies on FFS filesystem */
ULONG collectBlockListFFS(struct Volume *volume, ULONG block, struct BlockNode *blocklist)
{
    ULONG retval, first_block;
    WORD blk_count,count;
    UWORD i;

    D(bug("[install] collectBlockListFFS(%x, %ld, %x)\n", volume, block, blocklist));


    /* Clear the core.img sector pointers region! */
    memset((UBYTE*)&blocklist[-BLCKLIST_ELEMENTS],0x00, BLCKLIST_ELEMENTS*sizeof(struct BlockNode)); 

	/*
		The number of first block of core.img will be stored in boot.img
		so skip the first filekey in the first loop
	*/

	retval = _readwriteBlock(volume, block, volume->blockbuffer, volume->SizeBlock<<2,
			volume->readcmd);

    if (retval)
    {
        D(bug("[install] collectBlockListFFS: ERROR reading block (error: %ld\n", retval));
		Printf("ReadError %lu\n", (long)retval);
		return 0;
	}

	i = volume->SizeBlock - 52;
	first_block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]);
	blk_count=0;
	
    D(bug("[install] collectBlockListFFS: First block @ %ld, i:%d\n", first_block, i));

	
	do
	{
		retval = _readwriteBlock(volume, block, volume->blockbuffer, volume->SizeBlock<<2,
				volume->readcmd);
		if (retval)
		{
            D(bug("[install] collectBlockListFFS: ERROR reading block (error: %ld)\n", retval));
			Printf("ReadError %lu\n", (long)retval);
			return 0;
		}
        
        D(bug("[install] collectBlockListFFS: read block %ld, i = %d\n", block, i));
		while ((i>=6) && (volume->blockbuffer[i]))
		{
            D(bug("[install] collectBlockListFFS: i = %d\n", i));
			/*
				if current sector follows right after last sector
				then we don't need a new element
			*/
			if ((blocklist[blk_count].sector_lo) &&
					((blocklist[blk_count].sector_lo+blocklist[blk_count].count)==
						AROS_BE2LONG(volume->blockbuffer[i])))
			{
				blocklist[blk_count].count += 1;
                D(bug("[install] collectBlockListFFS: sector %d follows previous - increasing count of block %d to %d\n",
                    i, blk_count, blocklist[blk_count].count));
			}
			else
			{
				blk_count--; /* decrement first */
                D(bug("[install] collectBlockListFFS: store new block (%d)\n", blk_count));
    
				if ((blk_count-1) <= -BLCKLIST_ELEMENTS)
				{
                    D(bug("[install] collectBlockListFFS: ERROR: out of block space at sector %d, block %d\n",
                        i, blk_count));
					Printf("There is no more space to save blocklist in core.img\n");
					return 0;
				}
                D(bug("[install] collectBlockListFFS: storing sector pointer for %d in block %d\n", 
                    i, blk_count));
				blocklist[blk_count].sector_lo = AROS_BE2LONG(volume->blockbuffer[i]);
				blocklist[blk_count].sector_hi = 0;
				blocklist[blk_count].count = 1;
			}
			i--;
		}
		i = volume->SizeBlock - 51;
		block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock - 2]);
        D(bug("[install] collectBlockListFFS: next block %d, i = %d\n", block, i));
	} while (block);


	/*
		blocks in blocklist are relative to the first
		sector of the HD (not partition)
	*/
    
    D(bug("[install] collectBlockListFFS: successfully updated pointers for %d blocks\n", blk_count));

	i = 0;
	for (count=-1;count>=blk_count;count--)
	{
		blocklist[count].sector_lo += volume->startblock;
		blocklist[count].seg_adr = 0x820 + (i*32);
		i += blocklist[count].count;
        D(bug("[install] collectBlockListFFS: correcting block %d for partition start\n", count));
        D(bug("[install] collectBlockListFFS: sector : %ld seg_adr : %x\n", 
            blocklist[count].sector_lo, blocklist[count].seg_adr));
	}

    first_block += volume->startblock;
    D(bug("[install] collectBlockListFFS: corrected first block for partition start: %ld\n", first_block));
    
	return first_block;
}

/* Collects the list of blocks that a file occupies on SFS filesystem */
ULONG collectBlockListSFS(struct Volume *volume, ULONG objectnode, struct BlockNode *blocklist)
{
    ULONG retval, first_block = 0;
    WORD blk_count = 0, count = 0;
    ULONG block_objectnoderoot = 0, block_sfsobjectcontainer = 0, block_extentbnoderoot = 0;
    ULONG nextblock = 0, searchedblock = 0;
    WORD i = 0;
    UBYTE * tmpBytePtr = NULL;

    D(bug("[install] collectBlockListSFS(startblock: %ld, objectnode: %ld)\n", volume->startblock, objectnode));
    D(bug("[install] collectBlockListSFS(%ld, %d, %d)\n", volume->countblock, volume->SizeBlock, volume->partnum));

    /* Clear the core.img sector pointers region! */
    memset((UBYTE*)&blocklist[-BLCKLIST_ELEMENTS],0x00, BLCKLIST_ELEMENTS*sizeof(struct BlockNode)); 

    /* Description of actions:
     * 1. Load SFS root block
     * 2. From root block find the block containing root of objectnodes
     * 3. Traverse the tree of objectnodes until block of objectdescriptor is found
     * 4. Search the objectdescriptor for entry matching given objectnode from entry read the
     *    first block of file
     * 5. Having first file block, find the extentbnode for that block and read number 
     *    of blocks. Put first block and number of blocks into BlockList.
     * 6. If the file has more blocks than this exntentbnode hold, find first file
     *    block in next extentbnode. Go to step 5.
     * Use the SFS source codes for reference. They operate on structures not pointers
     * and are much easier to understand.
     */
 
    /* Read root block */
	retval = _readwriteBlock(volume, 0, volume->blockbuffer, volume->SizeBlock<<2,
			volume->readcmd);

    if (retval)
    {
        D(bug("[install] collectBlockListSFS: ERROR reading root block (error: %ld)\n", retval));
		Printf("ReadError %lu\n", (long)retval);
		return 0;
	}

    /* Get block pointers from root block */
    block_objectnoderoot = AROS_BE2LONG(volume->blockbuffer[28]); /* objectnoderoot - 29th ULONG */
    block_extentbnoderoot = AROS_BE2LONG(volume->blockbuffer[27]); /* extentbnoderoot - 28th ULONG */
    
    D(bug("[install] collectBlockListSFS: objectnoderoot: %ld, extentbnoderoot %ld\n", 
        block_objectnoderoot, block_extentbnoderoot));



    /* Find the SFSObjectContainer block for given objectnode */
    /* Reference: SFS, nodes.c, function findnode */
    nextblock = block_objectnoderoot;
    D(bug("[install] collectBlockListSFS: searching in nextblock %d for sfsobjectcontainer for objectnode %ld\n", 
        nextblock, objectnode));
    while(1)
    {
        _readwriteBlock(volume, nextblock, volume->blockbuffer, volume->SizeBlock<<2,
			volume->readcmd);
    
        /* If nodes == 1, we are at the correct nodecontainer, else go to next nodecontainer */
        if (AROS_BE2LONG(volume->blockbuffer[4]) == 1)
        {
            /* read entry from position: be_node + sizeof(fsObjectNode) * (objectnode - be_nodenumber) */
            tmpBytePtr = (UBYTE*)volume->blockbuffer;
            ULONG index = 20 + 10 * (objectnode - AROS_BE2LONG(volume->blockbuffer[3]));
            block_sfsobjectcontainer = AROS_BE2LONG(((ULONG*)(tmpBytePtr + index))[0]);
            D(bug("[install] collectBlockListSFS: leaf found in nextblock %ld, sfsobjectcontainer block is %ld \n", 
                nextblock, block_sfsobjectcontainer));
            break;
        }
        else
        {
            UWORD containerentry = 
                (objectnode - AROS_BE2LONG(volume->blockbuffer[3]))/AROS_BE2LONG(volume->blockbuffer[4]);
            nextblock = AROS_BE2LONG(volume->blockbuffer[containerentry + 5]) >> 4; /* 9-5 (2^9 = 512) */;
            D(bug("[install] collectBlockListSFS: check next block %ld\n", nextblock));
        }
    }

    if (block_sfsobjectcontainer == 0)
    {
        D(bug("[install] collectBlockListSFS: SFSObjectContainer not found\n"));
		Printf("SFSObjectContainer not found\n");
		return 0;
    }



    /* Find the SFSObject in SFSObjectContainer for given objectnode */
    first_block = 0;
    while((block_sfsobjectcontainer != 0) && (first_block == 0))
    {
        /* Read next SFS container block */
	    retval = _readwriteBlock(volume, block_sfsobjectcontainer, volume->blockbuffer, volume->SizeBlock<<2,
			    volume->readcmd);

        if (retval)
        {
            D(bug("[install] collectBlockListSFS: ERROR reading block (error: %ld)\n", retval));
		    Printf("ReadError %lu\n", (long)retval);
		    return 0;
	    }

        /* Iterate over SFS objects and match the objectnode */
        /* 
         * The first offset comes from :
         * sizeof(sfsblockheader) = uint32 + uint32 + uint32 (field of sfsobjectcontainer)
         * parent, next, previous = uint32 + uint32 + uint32 (fields of sfsobjectcontainers)
         */
        tmpBytePtr = ((UBYTE*)volume->blockbuffer) + 12 + 12; /* tmpBytePtr points to first object in container */

        while (AROS_BE2LONG(((ULONG*)(tmpBytePtr + 4))[0]) > 0) /* check on the objectnode field */
        {
            
            /* Compare objectnode */
            if (AROS_BE2LONG(((ULONG*)(tmpBytePtr + 4))[0]) == objectnode)
            {
                /* Found! */
                first_block = AROS_BE2LONG(((ULONG*)(tmpBytePtr + 12))[0]); /* data */
                D(bug("[install] collectBlockListSFS: first block is %ld\n", first_block));
                break;
            }
            
            /* Move to next object */
            /* Find end of name and end of comment */
            tmpBytePtr += 25; /* Point to name */
            count = 0;
	        for (i = 2; i > 0; tmpBytePtr++, count++)
		        if (*tmpBytePtr == '\0')
			        i--;

            /* Correction for aligment */
            if ((count & 0x01) == 0 )
                tmpBytePtr++;
        }
        
        /* Move to next sfs object container block */
        block_sfsobjectcontainer =  AROS_BE2LONG(volume->blockbuffer[4]); /* next field */       
        
    }

    if (first_block == 0)
    {
        D(bug("[install] collectBlockListSFS: First block not found\n"));
		Printf("First block not found\n");
		return 0;
    }



    /* First file block found. Find all blocks of file */
    searchedblock = first_block;
    blk_count = 0;

    while(1)
    {
        nextblock = block_extentbnoderoot;
        UBYTE * BNodePtr = NULL;
        
        while(1)
        {
            /* Find the extentbnode for this block */

            D(bug("[install] collectBlockListSFS: searching in nextblock %d for extentbnode for block %ld\n", 
                nextblock, searchedblock));

            UBYTE * BTreeContainerPtr = NULL;
            BNodePtr = NULL;

            _readwriteBlock(volume, nextblock, volume->blockbuffer, volume->SizeBlock<<2,
			    volume->readcmd);
            
            BTreeContainerPtr = (UBYTE*)(volume->blockbuffer + 3); /* Starts right after the header */

            D(bug("[install] collectBlockListSFS: tree container nodecount: %d\n", 
                AROS_BE2WORD(((UWORD*)BTreeContainerPtr)[0])));

            for (i = AROS_BE2WORD(((UWORD*)BTreeContainerPtr)[0]) - 1; i >=0; i--) /* Start from last element */
            {
                /* Read the BNode */
                tmpBytePtr = BTreeContainerPtr + 4 + i * BTreeContainerPtr[3];

                if (AROS_BE2LONG(((ULONG*)(tmpBytePtr))[0]) <= searchedblock) /* Check on the key field */
                {
                    BNodePtr = tmpBytePtr;
                    break;
                }
            }
            
            /* Fail if BNodePtr still NULL */
            if (BNodePtr == NULL)
            {
                D(bug("[install] collectBlockListSFS: Failed to travers extentbnode tree.\n"));
		        Printf("Failed to travers extentbnode tree.\n");
		        return 0;
            }

            /* If we are at the leaf, stop */
            if (BTreeContainerPtr[2])
                break;

            /* Else search further */
            nextblock = AROS_BE2LONG(((ULONG*)(BNodePtr))[1]); /* data / next field */
        }

        /* Found. Add BlockList entry */
        D(bug("[install] collectBlockListSFS: extentbnode for block %ld found. Block count: %d\n", 
            searchedblock, AROS_BE2WORD(((UWORD*)(BNodePtr + 12))[0])));

        /* Add blocklist entry */
        blk_count--;

        /* Check if we still have spece left to add data to BlockList */
		if ((blk_count-1) <= -BLCKLIST_ELEMENTS)
		{
            D(bug("[install] collectBlockListSFS: ERROR: out of block space\n"));
			Printf("There is no more space to save blocklist in core.img\n");
			return 0;
		}

	    blocklist[blk_count].sector_lo = searchedblock;
	    blocklist[blk_count].sector_hi = 0;
	    blocklist[blk_count].count = AROS_BE2WORD(((UWORD*)(BNodePtr + 12))[0]);

        /* Handling of special situations */
        if (searchedblock == first_block)
        { 
            /* Writting first pack of blocks. Pointer needs to point to second file block */
	        blocklist[blk_count].sector_lo++;
	        blocklist[blk_count].count--;
            if (blocklist[blk_count].count == 0)
            {
                /* This means that the first pack of blocks contained only one block - first block */
                /* Since the first blocklist needs to start at second file block, 'reset' the blk_count */
                /* so that next iteration will overwrite the current results */
                blk_count++;
            }
        }
        
        /* Are there more blocks to read? */
        if (AROS_BE2LONG(((ULONG*)(BNodePtr))[1]) == 0)
        {
            D(bug("[install] collectBlockListSFS: All core.img blocks found!\n"));
            break;
        }
        else
            searchedblock = AROS_BE2LONG(((ULONG*)(BNodePtr))[1]); /* data / next field */
    }


    /* Correct blocks for volume start */
	
    /* Blocks in blocklist are relative to the first sector of the HD (not partition) */
	i = 0;
	for (count=-1;count>=blk_count;count--)
	{
		blocklist[count].sector_lo += volume->startblock;
		blocklist[count].seg_adr = 0x820 + (i*32);
		i += blocklist[count].count;
        D(bug("[install] collectBlockListFFS: correcting block %d for partition start\n", count));
        D(bug("[install] collectBlockListFFS: sector : %ld seg_adr : %x\n", 
            blocklist[count].sector_lo, blocklist[count].seg_adr));
	}

    first_block += volume->startblock;

    return first_block;
}

/* Flushes the cache on the volume containing the specified path. */
VOID flushFS(CONST_STRPTR path)
{
    TEXT devname[256];
    UWORD i;

    for (i = 0; path[i] != ':'; i++)
	    devname[i] = path[i];
    devname[i++] = ':';
    devname[i] = '\0';

    /* Try to flush 10 times. 5 seconds total */
    
    /* Failsafe in case first Inhibit fails in some way (was needed
     * for SFS because non flushed data was failing Inhibit) */
     
    for (i = 0; i < 10; i++)
    {
        if (Inhibit(devname, DOSTRUE))
        {
            Inhibit(devname, DOSFALSE);
            break;
        }
        else
            Delay(25);
    }
}

BOOL writeCoreIMG(BPTR fh, UBYTE *buffer, struct Volume *volume)
{
    BOOL retval = FALSE;
    LONG error;

    D(bug("[install] writeCoreIMG(%x)\n", volume));

	if (Seek(fh, 0, OFFSET_BEGINNING) != -1)
	{
        D(bug("[install] writeCoreIMG - write first block\n"));

		/* write back first block */
		if (Write(fh, buffer, 512) == 512)
		{
            

			/* read second core.img block */
			if (Read(fh, buffer, 512) == 512)
			{
				/* set partition number where core.img is on */
		        /* FIXME: set RDB part number of DH? */
		        UBYTE *install_boot_device = 
                    buffer + GRUB_DECOMPRESSOR_I386_PC_BOOT_DEVICE;

		        D(bug("[install] set dos part = %d\n", volume->partnum));

		        install_boot_device[0] = 0;
		        install_boot_device[1] = 0;
		        install_boot_device[2] = volume->partnum;

				/* write second core.img block back */
				if (Seek(fh, -512, OFFSET_CURRENT) != -1)
				{
					if (Write(fh, buffer, 512) == 512)
					{
						retval = TRUE;
					}
					else
						Printf("Write Error\n");
				}
				else
					Printf("Seek Error\n");
			}
			else
				Printf("Read Error\n");
		}
		else
			Printf("Write Error\n");
	}
	else
    {
		error = IoErr();
        Printf("Seek Error\n");
		PrintFault(error, NULL);
    }
	return retval;
}

ULONG updateCoreIMG(CONST_STRPTR grubpath,     /* path of grub dir */
		            struct Volume *volume, /* volume core.img is on */
		            ULONG *buffer          /* a buffer of at least 512 bytes */)
{
    ULONG block = 0;
    struct FileInfoBlock fib;
    BPTR fh;
    TEXT coreimgpath[256];

    D(bug("[install] updateCoreIMG(%x)\n", volume));

	AddPart(coreimgpath, grubpath, 256);
	AddPart(coreimgpath, CORE_IMG_FILE_NAME, 256);
	fh = Open(coreimgpath, MODE_OLDFILE);
	if (fh)
	{
		if (ExamineFH(fh, &fib))
		{
			if (Read(fh, buffer, 512) == 512)
			{
				/*
					Get and store all blocks of core.img in first block of core.img.
					First block of core.img will be returned.
                    List of BlockNode starts at 512 - sizeof(BlockNode). List grows downwards.
                    buffer is ULONG, buffer[128] is one pointer after first element(upwards).
                    collectBlockList assumes it receives one pointer after first element(upwards).
				*/
    
                if (volume->dos_id == ID_SFS_BE_DISK)
                {
                    D(bug("[install] core.img on SFS file system\n"));
				    block = collectBlockListSFS
					    (volume, fib.fib_DiskKey, (struct BlockNode *)&buffer[128]);
                }
                else 
                if ((volume->dos_id == ID_FFS_DISK) || (volume->dos_id == ID_INTER_DOS_DISK) ||
                    (volume->dos_id == ID_INTER_FFS_DISK) || (volume->dos_id == ID_FASTDIR_DOS_DISK) ||
                    (volume->dos_id == ID_FASTDIR_FFS_DISK))
                {
                    D(bug("[install] core.img on FFS file system\n"));
                    block = collectBlockListFFS
					    (volume, fib.fib_DiskKey, (struct BlockNode *)&buffer[128]);
                }
                else
                {
                    block = 0;
                    D(bug("[install] core.img on unsupported file system\n"));
                    Printf("Unsupported file system\n");
                }

                D(bug("[install] core.img first block: %ld\n", block));
    
				if (block)
				{
					if (!writeCoreIMG(fh, (UBYTE *)buffer, volume))
						block = 0;
				}
			}
			else
				Printf("%s: Read Error\n", coreimgpath);
		}
		else
			PrintFault(IoErr(), coreimgpath);

		Close(fh);
        
	}
	else
		PrintFault(IoErr(), coreimgpath);
	return block;
}

/* Installs boot.img to MBR and updates core.img */
BOOL installGrubFiles(struct Volume *coreimgvol,	/* core.img volume */
		       CONST_STRPTR grubpath,	/* path to grub files */
		       ULONG unit,	/* unit core.img is on */
		       struct Volume *bootimgvol)	/* boot device for boot.img */
{
    BOOL retval = FALSE;
    TEXT bootimgpath[256];
    ULONG block;

    D(bug("[install] installStageFiles(%x)\n", bootimgvol));

    /* Flush GRUB volume's cache */
    flushFS(grubpath);

    block = updateCoreIMG(grubpath, coreimgvol, bootimgvol->blockbuffer);

    if (block)
    {
	    AddPart(bootimgpath, grubpath, 256);
	    AddPart(bootimgpath, (CONST_STRPTR) "i386-pc/boot.img", 256);
	    if (writeBootIMG(bootimgpath, bootimgvol, coreimgvol, block, unit))
	        retval = TRUE;
    }
    else
	    bug("failed %d\n", IoErr());

    return retval;
}

int main(int argc, char **argv)
{
    struct RDArgs *rdargs;
    struct Volume *grubvol;
    struct Volume *bbvol;
    struct FileSysStartupMsg *fssm;
    int ret = RETURN_OK;

    D(bug("[install] main()\n"));

    rdargs = ReadArgs(template, myargs, NULL);
    if (rdargs)
    {
	    CONST_STRPTR bootDevice = (CONST_STRPTR) myargs[0];
	    LONG unit = *(LONG *) myargs[1];
	    LONG *partnum = (LONG *) myargs[2];
	    CONST_STRPTR grubpath = (CONST_STRPTR) myargs[3];

	    D(bug("[install] FORCELBA = %d\n", myargs[4]));
	    if (myargs[4])
	        Printf("FORCELBA ignored\n");

	    if (partnum)
	    {
	        Printf("PARTITIONNUMBER not supported yet\n");
	        FreeArgs(rdargs);
	        return RETURN_ERROR;
	    }

	    fssm = getDiskFSSM(grubpath);
	    if (fssm != NULL)
	    {
	        CONST_STRPTR grubDevice = AROS_BSTR_ADDR(fssm->fssm_Device);

	        if (!strcmp((const char *) grubDevice, (const char *) bootDevice))
	        {
		        struct DosEnvec *dosEnvec;
		        dosEnvec = (struct DosEnvec *) BADDR(fssm->fssm_Environ);

		        grubvol = getGrubStageVolume(grubDevice, fssm->fssm_Unit,
					             fssm->fssm_Flags, dosEnvec);
		        if (grubvol)
		        {

		            bbvol = getBBVolume(bootDevice, unit, partnum);
		            if (bbvol)
		            {
			            if (!installGrubFiles(grubvol, grubpath,
					                   fssm->fssm_Unit, bbvol))
			                ret = RETURN_ERROR;

			            uninitVolume(bbvol);
		            }
		            else
		            {
			            D(bug("getBBVolume failed miserably\n"));
			            ret = RETURN_ERROR;
		            }

		            uninitVolume(grubvol);
		        }
	        }
	        else
	        {
		        Printf("%s is not on device %s unit %ld\n",
		           grubpath, bootDevice, (long)unit);
		        ret = RETURN_ERROR;
	        }
	    }
	    else if (fssm)
	    {
	        Printf("kernel path must begin with a device name\n");
	        FreeArgs(rdargs);
	        ret = RETURN_ERROR;
	    }

	    FreeArgs(rdargs);
    }
    else
	    PrintFault(IoErr(), (STRPTR) argv[0]);

    return ret;
}
