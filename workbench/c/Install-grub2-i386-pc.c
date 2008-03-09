/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
    $Id: install-pc.c 27903 2008-02-25 21:36:33Z agreppin $
*/
/******************************************************************************


    NAME

        Install-grub2-i386-pc

    SYNOPSIS

        DEVICE/A, UNIT/N/K/A, PARTITIONNUMBER=PN/K/N, GRUB/K/A, FORCELBA/S

    LOCATION

        SYS:C

    FUNCTION

        Installs the GRUB bootloader to the bootblock of the specified disk.

    INPUTS

        DEVICE --  Device name (eg. ata.device)
        UNIT  --  Unit number
        PN  --  Partition number (advice: the first AROS FFS partition)
        GRUB -- Path to GRUB directory.
        FORCELBA --  Force use of LBA mode.

    RESULT

    NOTES
	
    EXAMPLE

        install-pc device ata.device unit 0 PN 1 grub dh0:boot/grub

    BUGS

    SEE ALSO

        Partition, Format
	
    INTERNALS

******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/partition.h>
#include <aros/macros.h>
#include <devices/hardblocks.h>
#include <devices/newstyle.h>
#include <dos/dos.h>
#include <exec/errors.h>
#include <exec/memory.h>
#include <libraries/partition.h>

#define DEBUG 1
#include <aros/debug.h>

/* Defines for grub2 data */
/* Stage 1 pointers */
#define MBR_BPBSTART		0x3
#define MBR_BPBEND		0x3e
#define GRUB_FORCE_LBA		0x41
#define GRUB_STAGE2_SECTOR	0x44
#define MBR_PARTSTART		0x1be
#define MBR_PARTEND		0x1fe

/* Stage 2 pointers */
#define GRUB_KERNEL_MACHINE_INSTALL_DOS_PART 0x14
#define GRUB_KERNEL_MACHINE_INSTALL_BSD_PART 0x18

/* BIOS drive flag */
#define BIOS_HDISK_FLAG		0x80

#define MBR_MAX_PARTITIONS	4
#define MBRT_EXTENDED		0x05
#define MBRT_EXTENDED2		0x0f

struct Volume
{
    struct MsgPort *mp;
    struct IOExtTD *iotd;
    ULONG readcmd;
    ULONG writecmd;
    ULONG startblock;
    ULONG countblock;
    ULONG unitnum;
    UWORD SizeBlock;
    UBYTE flags;
    BYTE partnum;
    ULONG *blockbuffer;
};

#define VF_IS_TRACKDISK (1<<0)
#define VF_IS_RDB       (1<<1)

struct BlockNode
{
    ULONG sector;
    UWORD count;
    UWORD seg_adr;
};

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
		if (IsFileSystem(dname))
		{
		    return (struct FileSysStartupMsg *) BADDR(dn->dn_Startup);
		}
		else
		    printf("device '%s' doesn't contain a file system\n",
			   dname);
	    }
	    else
		PrintFault(ERROR_OBJECT_NOT_FOUND, dname);
	}
    }
    else
	printf("'%s' doesn't contain a device name\n", path);
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
	    printf("Device doesn't understand NSD-Query\n");
	}
	else
	{
	    if ((volume->iotd->iotd_Req.io_Actual >
		 sizeof(struct NSDeviceQueryResult))
		|| (volume->iotd->iotd_Req.io_Actual == 0)
		|| (volume->iotd->iotd_Req.io_Actual != nsdq.SizeAvailable))
	    {
		printf("WARNING wrong io_Actual using NSD\n");
	    }
	    else
	    {
		if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
		    printf("WARNING no trackdisk type\n");
		for (cmdcheck = nsdq.SupportedCommands; *cmdcheck; cmdcheck++)
		{
		    if (*cmdcheck == NSCMD_TD_READ64)
			volume->readcmd = NSCMD_TD_READ64;
		    if (*cmdcheck == NSCMD_TD_WRITE64);
		    volume->writecmd = NSCMD_TD_WRITE64;
		}
		if ((volume->readcmd != NSCMD_TD_READ64) ||
		    (volume->writecmd != NSCMD_TD_WRITE64))
		    printf("WARNING no READ64/WRITE64\n");
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
			volume->unitnum = unit;
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

BOOL isvalidFileSystem(struct Volume * volume, CONST_STRPTR device,
		       ULONG unit)
{
    BOOL retval = FALSE;
    struct PartitionBase *PartitionBase;
    struct PartitionHandle *ph;

    D(bug("[install] isvalidFileSystem(%x, %s, %d)\n", volume, device, unit));

    if (readBlock(volume, 0, volume->blockbuffer, 512))
    {
	printf("Read Error\n");
	return FALSE;
    }

    if (((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00) != 0x444F5300) ||
	((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF) == 0))
    {
	/* first block has no DOS\x so we don't have RDB for sure */
	volume->flags &= ~VF_IS_RDB;
	if (readBlock(volume, 1, volume->blockbuffer, 512))
	{
	    printf("Read Error\n");
	    return FALSE;
	}
	if (((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00) !=
	     0x444F5300)
	    || ((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF) == 0))
	    return FALSE;
    }

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
			printf
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
	    printf("Error OpenRootPartition(%s,%d)\n", device, unit);
	CloseLibrary((struct Library *) PartitionBase);
    }
    else
	printf("Couldn't open partition.library\n");
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
	    printf("stage2 is on an unsupported file system\n");
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
				printf
				    ("partition is not of type AROS (0x30)\n");
			}
			else
			{
			    printf
				("partition %d not found on device %s unit %d\n",
				 *pnum, device, unit);
			}
		    }
		    else
			printf
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
			printf
			    ("partition table type must be either MBR or RDB\n");
		}
		ClosePartitionTable(ph);
	    }
	    else
	    {
#warning "FIXME: GetPartitionAttr() should always work for root partition"
		CopyMem(&ph->de, de, sizeof(struct DosEnvec));
		retval = TRUE;
	    }
	    CloseRootPartition(ph);
	}
	else
	    printf("Error OpenRootPartition(%s,%d)\n", device, unit);
	CloseLibrary((struct Library *) PartitionBase);
    }
    else
	printf("Couldn't open partition.library\n");
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
	    printf("no space for bootblock (RDB is on block 0)\n");
    }
    return NULL;
}

BOOL writeStage1(STRPTR stage1path, struct Volume * volume, struct Volume * s2vol, ULONG block,	/* first block of stage2 file */
		 ULONG unit)
{
    BOOL retval = FALSE;
    LONG error = 0;
    BPTR fh;

    D(bug("[install] writeStage1(%x)\n", volume));

    fh = Open(stage1path, MODE_OLDFILE);
    if (fh)
    {
	if (Read(fh, volume->blockbuffer, 512) == 512)
	{
	    /* install into MBR ? */
	    if ((volume->startblock == 0)
		&& (!(volume->flags & VF_IS_TRACKDISK)))
	    {
#define GRUB_BOOT_MACHINE_BOOT_DRIVE  0x4c
#define GRUB_BOOT_MACHINE_ROOT_DRIVE  0x4d
#define GRUB_BOOT_MACHINE_DRIVE_CHECK 0x4f
		APTR boot_img = volume->blockbuffer;

		UBYTE *boot_drive = (UBYTE *) (boot_img
					       +
					       GRUB_BOOT_MACHINE_BOOT_DRIVE);
		UBYTE *root_drive =
		    (UBYTE *) (boot_img + GRUB_BOOT_MACHINE_ROOT_DRIVE);
		UWORD *boot_drive_check =
		    (UWORD *) (boot_img + GRUB_BOOT_MACHINE_DRIVE_CHECK);

		*boot_drive = 0xff;
		*root_drive = 0xff;
		*boot_drive_check = 0x9090;

		D(bug("[install] writeStage1: Install to HARDDISK\n"));
		// read old MBR
		error = readBlock(volume, 0, s2vol->blockbuffer, 512);

		D(bug
		  ("[install] writeStage1: MBR Buffer @ %x\n",
		   volume->blockbuffer));
		D(bug
		  ("[install] writeStage1: Copying MBR BPB to %x\n",
		   (char *) volume->blockbuffer + 0x04));
		// copy BPB (BIOS Parameter Block)
		CopyMem
		    ((APTR) ((char *) s2vol->blockbuffer + 0x04),
		     (APTR) ((char *) volume->blockbuffer + 0x04),
		     (MBR_BPBEND - MBR_BPBSTART));
		// copy partition table - [Overwrites Floppy boot code]
		D(bug
		  ("[install] writeStage1: Copying MBR Partitions to %x\n",
		   (char *) volume->blockbuffer + MBR_PARTSTART));
		CopyMem((APTR) ((char *) s2vol->blockbuffer + MBR_PARTSTART),
			(APTR) ((char *) volume->blockbuffer + MBR_PARTSTART),
			(MBR_PARTEND - MBR_PARTSTART));

		// Store the stage 2 pointer ..
		ULONG *stage2_sector_start = (ULONG *) (boot_img
							+ GRUB_STAGE2_SECTOR);
		stage2_sector_start[0] = block;
		D(bug("[install] writeStage1: stage2 pointer = %x\n", block));
	    }
	    else
	    {
		D(bug("[install] writeStage1: Install to FLOPPY\n"));
	    }

	    if (error == 0)
	    {
		error = writeBlock(volume, 0, volume->blockbuffer, 512);
		if (error)
		    printf("WriteError %d\n", error);
		else
		    retval = TRUE;
	    }
	    else
		printf("WriteError %d\n", error);
	}
	else
	    printf("%s: Read Error\n", stage1path);
	Close(fh);
    }
    else
	PrintFault(IoErr(), stage1path);
    return retval;
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
    if (Inhibit(devname, DOSTRUE))
	Inhibit(devname, DOSFALSE);
}

BOOL installStageFiles(struct Volume *s2vol,	/* stage2 volume */
		       CONST_STRPTR stagepath,	/* path to stage* files */
		       ULONG unit,	/* unit stage2 is on */
		       struct Volume *s1vol)	/* boot device for stage 1 */
{
    BOOL retval = FALSE;
    TEXT stagename[256];
    ULONG block;

    D(bug("[install] installStageFiles(%x)\n", s1vol));

    /* Flush GRUB volume's cache */
    flushFS(stagepath);

    struct FileInfoBlock fib;
    ULONG embed_start = 1;	/* embed into MBR for now */
    ULONG embed_end;
    ULONG error, i, len;
    BPTR fh;
    UBYTE *buffer = (UBYTE *) s1vol->blockbuffer;

    AddPart(stagename, stagepath, 256);
    AddPart(stagename, (CONST_STRPTR) "core.img", 256);

    fh = Open(stagename, MODE_OLDFILE);
    if (fh && Examine(fh, &fib))
    {
	block = embed_start;
	embed_end = embed_start + fib.fib_Size / 512 + 1;

	for (i = embed_start; block && i < embed_end; ++i)
	{
	    len = Read(fh, buffer, 512);

	    if (len == 0)
		break;
	    else if (len < 0)
	    {
		block = 0;
		break;
	    }
	    else
	    {
		if (i == embed_start + 1)
		{
		    LONG dos_part = s1vol->partnum;
		    LONG bsd_part = 0; /*?? to fix = RDB part number of DH? */
		    LONG *install_dos_part = (LONG *) (buffer
						       +
						       GRUB_KERNEL_MACHINE_INSTALL_DOS_PART);
		    LONG *install_bsd_part =
			(LONG *) (buffer +
				  GRUB_KERNEL_MACHINE_INSTALL_BSD_PART);

		    if (dos_part == -1)
			dos_part = s2vol->partnum;

		    bug("[install] set dos part = %d\n", dos_part);
		    bug("[install] set bsd part = %d\n", bsd_part);

		    *install_dos_part = dos_part;
		    *install_bsd_part = bsd_part;
		}

		error = writeBlock(s1vol, i, buffer, 512);
		if (error)
		{
		    printf("WriteError %d\n", error);
		    block = 0;
		    break;
		}
	    }
	}
    }
    else
	block = 0;

    if (block)
    {
	AddPart(stagename, stagepath, 256);
	AddPart(stagename, (CONST_STRPTR) "boot.img", 256);
	if (writeStage1(stagename, s1vol, s2vol, block, unit))
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
	    printf("FORCELBA ignored\n");

	if (partnum)
	{
	    printf("PARTITIONNUMBER not supported yet\n");
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
			if (!installStageFiles(grubvol, grubpath,
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
		printf("%s is not on device %s unit %d\n",
		       grubpath, bootDevice, unit);
		ret = RETURN_ERROR;
	    }
	}
	else if (fssm)
	{
	    printf("kernel path must begin with a device name\n");
	    FreeArgs(rdargs);
	    ret = RETURN_ERROR;
	}

	FreeArgs(rdargs);
    }
    else
	PrintFault(IoErr(), (STRPTR) argv[0]);

    return ret;
}
