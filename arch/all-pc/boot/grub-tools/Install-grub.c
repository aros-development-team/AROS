/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$
*/
/******************************************************************************


    NAME

        Install-i386-pc

    SYNOPSIS

        DEVICE/A, UNIT/N/K/A, PARTITIONNUMBER=PN/K/N, GRUB/K/A, FORCELBA/S

    LOCATION

        C:

    FUNCTION

        Installs the GRUB bootloader to the bootblock of the specified disk.

    INPUTS

        DEVICE --  Device name (e.g. ata.device)
        UNIT  --  Unit number
        PN  --  Partition number (advice: the first AROS FFS partition)
        GRUB -- Path to GRUB directory.
        FORCELBA --  Force use of LBA mode.

    RESULT

    NOTES
	
    EXAMPLE

        install-i386-pc DEVICE ata.device UNIT 0 PN 1 grub dh0:boot/grub

    BUGS

    SEE ALSO

        Partition, Sys:System/Format
	
    INTERNALS

******************************************************************************/

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

#define DEBUG 0
#include <aros/debug.h>

/* Defines for grub data */
/* Stage 1 pointers */
#define MBR_BPBEND		    0x3e
#define GRUB_BOOT_DRIVE	    0x40
#define GRUB_FORCE_LBA	    0x41
#define GRUB_STAGE2_SECTOR	 0x44
#define MBR_PARTSTART	    0x1be
#define MBR_PARTEND		    0x1fe
/* Stage 2 pointers */

/* BIOS drive flag */
#define BIOS_HDISK_FLAG	    0x80

#define MBR_MAX_PARTITIONS 4
#define MBRT_EXTENDED      0x05
#define MBRT_EXTENDED2     0x0f

struct Volume {
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
};

#define VF_IS_TRACKDISK (1<<0)
#define VF_IS_RDB       (1<<1)

struct BlockNode {
	ULONG sector;
	UWORD count;
	UWORD seg_adr;
};

const TEXT version[] = "$VER: Install-i386-pc 41.2 (4.6.2009)";

char *template =
	"DEVICE/A,"
	"UNIT/N/K/A,"
	"PARTITIONNUMBER=PN/K/N,"	/* Partition whose boot block we should install stage1 in */
	"GRUB/K/A,"
	"FORCELBA/S";
IPTR myargs[7] = {0,0,0,0,0,0};

struct FileSysStartupMsg *getDiskFSSM(STRPTR path) {
struct DosList *dl;
struct DeviceNode *dn;
char dname[32];
UBYTE i;

D(bug("[install-i386] getDiskFSSM('%s')\n", path));

	for (i=0;(path[i]) && (path[i]!=':');i++)
		dname[i] = path[i];
	if (path[i] == ':')
	{
		dname[i] = 0;
		dl = LockDosList(LDF_READ);
		if (dl)
		{
			dn = (struct DeviceNode *)FindDosEntry(dl, dname, LDF_DEVICES);
			UnLockDosList(LDF_READ);
			if (dn)
			{
				dname[i] = ':';
				dname[i + 1] = '\0';
				if (IsFileSystem(dname))
				{
					return (struct FileSysStartupMsg *)BADDR(dn->dn_Startup);
				}
				else
					Printf("device '%s' doesn't contain a file system\n", dname);
			}
			else
				PrintFault(ERROR_OBJECT_NOT_FOUND, dname);
		}
	}
	else
		Printf("'%s' doesn't contain a device name\n",path);
	return 0;
}

void fillGeometry(struct Volume *volume, struct DosEnvec *de) {
ULONG spc;

D(bug("[install-i386] fillGeometry(%x)\n", volume));

	spc = de->de_Surfaces*de->de_BlocksPerTrack;
	volume->SizeBlock = de->de_SizeBlock;
	volume->startblock = de->de_LowCyl*spc;
	volume->countblock =((de->de_HighCyl-de->de_LowCyl+1)*spc)-1+de->de_Reserved;
}

void nsdCheck(struct Volume *volume) {
struct NSDeviceQueryResult nsdq;
UWORD *cmdcheck;

D(bug("[install-i386] nsdCheck(%x)\n", volume));

	if (
			(
				(volume->startblock+volume->countblock)*  /* last block */
				((volume->SizeBlock<<2)/512) /* 1 portion (block) equals 512 (bytes) */
			)>8388608)
	{
		nsdq.SizeAvailable=0;
		nsdq.DevQueryFormat=0;
		volume->iotd->iotd_Req.io_Command=NSCMD_DEVICEQUERY;
		volume->iotd->iotd_Req.io_Data=&nsdq;
		volume->iotd->iotd_Req.io_Length=sizeof(struct NSDeviceQueryResult);
		if (DoIO((struct IORequest *)&volume->iotd->iotd_Req)==IOERR_NOCMD)
		{
			Printf("Device doesn't understand NSD-Query\n");
		}
		else
		{
			if (
					(volume->iotd->iotd_Req.io_Actual>sizeof(struct NSDeviceQueryResult)) ||
					(volume->iotd->iotd_Req.io_Actual==0) ||
					(volume->iotd->iotd_Req.io_Actual!=nsdq.SizeAvailable)
				)
			{
				Printf("WARNING wrong io_Actual using NSD\n");
			}
			else
			{
				if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
					Printf("WARNING no trackdisk type\n");
				for (cmdcheck=nsdq.SupportedCommands;*cmdcheck;cmdcheck++)
				{
					if (*cmdcheck == NSCMD_TD_READ64)
						volume->readcmd = NSCMD_TD_READ64;
					if (*cmdcheck == NSCMD_TD_WRITE64)
						volume->writecmd = NSCMD_TD_WRITE64;
				}
				if (
						(volume->readcmd!=NSCMD_TD_READ64) ||
						(volume->writecmd!=NSCMD_TD_WRITE64)
					)
					Printf("WARNING no READ64/WRITE64\n");
			}
		}
	}
}


struct Volume *initVolume(STRPTR device, ULONG unit, ULONG flags, struct DosEnvec *de) {
struct Volume *volume;
LONG error=0;

D(bug("[install-i386] initVolume(%s:%d)\n", device, unit));

	volume = AllocVec(sizeof(struct Volume), MEMF_PUBLIC | MEMF_CLEAR);
	if (volume)
	{
		volume->mp = CreateMsgPort();
		if (volume->mp)
		{
			volume->iotd = (struct IOExtTD *)CreateIORequest(volume->mp, sizeof(struct IOExtTD));
			if (volume->iotd)
			{
				volume->blockbuffer = AllocVec(de->de_SizeBlock<<2, MEMF_PUBLIC | MEMF_CLEAR);
				if (volume->blockbuffer)
				{
					if (
							OpenDevice
							(
								device,
								unit,
								(struct IORequest *)volume->iotd,
								flags
							) == 0
						)
					{
						if (strcmp(device, "trackdisk.device")==0)
							volume->flags |= VF_IS_TRACKDISK;
						else
							volume->flags |= VF_IS_RDB; /* just assume we have RDB */
						volume->readcmd = CMD_READ;
						volume->writecmd = CMD_WRITE;
						volume->device = device;
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
				DeleteIORequest((struct IORequest *)volume->iotd);
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
	return 0;
}

void uninitVolume(struct Volume *volume)
{

D(bug("[install-i386] uninitVolume(%x)\n", volume));

	CloseDevice((struct IORequest *)volume->iotd);
	FreeVec(volume->blockbuffer);
	DeleteIORequest((struct IORequest *)volume->iotd);
	DeleteMsgPort(volume->mp);
	FreeVec(volume);
}

ULONG readwriteBlock
	(
		struct Volume *volume,
		ULONG block, APTR buffer, ULONG length,
		ULONG command
	)
{
UQUAD offset;
ULONG retval=0;

D(bug("[install-i386] readwriteBlock(vol:%x, block:%d, %d bytes)\n", volume, block, length));

	volume->iotd->iotd_Req.io_Command = command;
	volume->iotd->iotd_Req.io_Length = length;
	volume->iotd->iotd_Req.io_Data = buffer;
	offset = (UQUAD)(volume->startblock+block)*(volume->SizeBlock<<2);
	volume->iotd->iotd_Req.io_Offset = offset & 0xFFFFFFFF;
	volume->iotd->iotd_Req.io_Actual = offset>>32;
	retval = DoIO((struct IORequest *)&volume->iotd->iotd_Req);
	if (volume->flags & VF_IS_TRACKDISK)
	{
		volume->iotd->iotd_Req.io_Command = TD_MOTOR;
		volume->iotd->iotd_Req.io_Length = 0;
		DoIO((struct IORequest *)&volume->iotd->iotd_Req);
	}
	return retval;
}

BOOL isvalidFileSystem(struct Volume *volume, STRPTR device, ULONG unit) {
BOOL retval = FALSE;
struct PartitionBase *PartitionBase;
struct PartitionHandle *ph;

D(bug("[install-i386] isvalidFileSystem(%x, %s, %d)\n", volume, device, unit));

	if (readwriteBlock(volume, 0, volume->blockbuffer, 512, volume->readcmd))
	{
		Printf("Read Error\n");
		return FALSE;
	}
	if (
			((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)!=0x444F5300) ||
			((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF) == 0)
		)
	{
		/* first block has no DOS\x so we don't have RDB for sure */
		volume->flags &= ~VF_IS_RDB;
		if (readwriteBlock(volume, 1, volume->blockbuffer, 512, volume->readcmd))
		{
			Printf("Read Error\n");
			return FALSE;
		}
		if (
				((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)!=0x444F5300) ||
				((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF) == 0)
			)
			return FALSE;
	}
	volume->partnum = -1;
	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
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
				tags[0].ti_Data = (STACKIPTR)&type;
				GetPartitionTableAttrs(ph, tags);
				if (type == PHPTT_MBR)
				{
				struct PartitionHandle *pn;
				struct DosEnvec de;
				struct PartitionHandle *extph = NULL;
				struct PartitionType ptype = {{0}};

					tags[0].ti_Tag = PT_DOSENVEC;
					tags[0].ti_Data = (STACKIPTR)&de;
					tags[1].ti_Tag = PT_TYPE;
					tags[1].ti_Data = (STACKIPTR)&ptype;
					tags[2].ti_Tag = TAG_DONE;
					pn = (struct PartitionHandle *)ph->table->list.lh_Head;
					while (pn->ln.ln_Succ)
					{
					ULONG scp;

						GetPartitionAttrs(pn, tags);
						if (ptype.id[0] == MBRT_EXTENDED || ptype.id[0] == MBRT_EXTENDED2)
							extph = pn;
						else
						{
							scp = de.de_Surfaces*de.de_BlocksPerTrack;
							if (
									(volume->startblock>=(de.de_LowCyl*scp)) &&
									(volume->startblock<=(((de.de_HighCyl+1)*scp)-1))
								)
								break;
						}
						pn = (struct PartitionHandle *)pn->ln.ln_Succ;
					}
					if (pn->ln.ln_Succ)
					{
						tags[0].ti_Tag = PT_POSITION;
						tags[0].ti_Data = (STACKIPTR)&type;
						tags[1].ti_Tag = TAG_DONE;
						GetPartitionAttrs(pn, tags);
						volume->partnum = (UBYTE)type;
						retval = TRUE;
						D(bug("[install-i386] Primary partition found: partnum=%ld\n", volume->partnum));
					}
					else if (extph != NULL)
					{
						if (OpenPartitionTable(extph) == 0)
						{
							tags[0].ti_Tag = PTT_TYPE;
							tags[0].ti_Data = (STACKIPTR)&type;
							tags[1].ti_Tag = TAG_DONE;
							GetPartitionTableAttrs(extph, tags);
							if (type == PHPTT_EBR)
							{
								tags[0].ti_Tag = PT_DOSENVEC;
								tags[0].ti_Data = (STACKIPTR)&de;
								tags[1].ti_Tag = TAG_DONE;
								pn = (struct PartitionHandle *)extph->table->list.lh_Head;
								while (pn->ln.ln_Succ)
								{
								ULONG offset, scp;

									offset = extph->de.de_LowCyl
										* extph->de.de_Surfaces
										* extph->de.de_BlocksPerTrack;
									GetPartitionAttrs(pn, tags);
									scp = de.de_Surfaces*de.de_BlocksPerTrack;
									if (
											(volume->startblock>=offset+(de.de_LowCyl*scp)) &&
											(volume->startblock<=offset+(((de.de_HighCyl+1)*scp)-1))
										)
										break;
									pn = (struct PartitionHandle *)pn->ln.ln_Succ;
								}
								if (pn->ln.ln_Succ)
								{
									tags[0].ti_Tag = PT_POSITION;
									tags[0].ti_Data = (STACKIPTR)&type;
									GetPartitionAttrs(pn, tags);
									volume->partnum = MBR_MAX_PARTITIONS + (UBYTE)type;
									retval = TRUE;
									D(bug("[install-i386] Logical partition found: partnum=%ld\n", volume->partnum));
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
						Printf("only MBR and RDB partition tables are supported\n");
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
		CloseLibrary((struct Library *)PartitionBase);
	}
	else
		Printf("Couldn't open partition.library\n");
	return retval;
}

struct Volume *getGrubStageVolume
	(
		STRPTR device,
		ULONG unit,
		ULONG flags,
		struct DosEnvec *de
	)
{
struct Volume *volume;

	volume = initVolume(device, unit, flags, de);

D(bug("[install-i386] getGrubStageVolume(): volume=%x\n", volume));

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

BOOL isvalidPartition
	(
		STRPTR device,
		ULONG unit,
		LONG *pnum,
		struct DosEnvec *de
	)
{
struct PartitionBase *PartitionBase;
struct PartitionHandle *ph;
ULONG type;
BOOL retval=FALSE;

D(bug("[install-i386] isvalidPartition(%s:%d, part:%d)\n", device, unit, pnum));

	PartitionBase = (struct PartitionBase *)OpenLibrary("partition.library", 1);
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
					tags[0].ti_Data = (STACKIPTR)&type;
					GetPartitionTableAttrs(ph, tags);
					if (type == PHPTT_MBR)
					{
					struct PartitionHandle *pn;

						/* search for partition */
						tags[0].ti_Tag = PT_POSITION;
						tags[0].ti_Data = (STACKIPTR)&type;
						pn = (struct PartitionHandle *)ph->table->list.lh_Head;
						while (pn->ln.ln_Succ)
						{
							GetPartitionAttrs(pn, tags);
							if (type == *pnum)
								break;
							pn = (struct PartitionHandle *)pn->ln.ln_Succ;
						}
						if (pn->ln.ln_Succ)
						{
						struct PartitionType ptype;

							/* is it an AROS partition? */
							tags[0].ti_Tag = PT_TYPE;
							tags[0].ti_Data = (STACKIPTR)&ptype;
							GetPartitionAttrs(pn, tags);
							if (ptype.id[0] == 0x30)
							{
								tags[0].ti_Tag = PT_DOSENVEC;
								tags[0].ti_Data = (STACKIPTR)de;
								GetPartitionAttrs(pn, tags);
								retval = TRUE;
							}
							else
								Printf("partition is not of type AROS (0x30)\n");
						}
						else
						{
							Printf
							(
								"partition %ld not found on device %s unit %lu\n",
								(long)*pnum, device, (long)unit
							);
						}
					}
					else
						Printf("you can only install in partitions which are MBR partitioned\n");
				}
				else
				{
					/* install into MBR */
					tags[0].ti_Tag = PTT_TYPE;
					tags[0].ti_Data = (STACKIPTR)&type;
					GetPartitionTableAttrs(ph, tags);
					if ((type == PHPTT_MBR) || (type == PHPTT_RDB))
					{
						tags[0].ti_Tag = PT_DOSENVEC;
						tags[0].ti_Data = (STACKIPTR)de;
						GetPartitionAttrs(ph, tags);
						retval = TRUE;
					}
					else
						Printf("partition table type must be either MBR or RDB\n");
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
		CloseLibrary((struct Library *)PartitionBase);
	}
	else
		Printf("Couldn't open partition.library\n");
	return retval;
}

struct Volume *getBBVolume(STRPTR device, ULONG unit, LONG *partnum) {
struct Volume *volume;
struct DosEnvec de;

D(bug("[install-i386] getBBVolume(%s:%d, %d)\n", device, unit, partnum));

	if (isvalidPartition(device, unit, partnum, &de))
	{
		volume = initVolume(device, unit, 0, &de);
		volume->partnum = partnum ? *partnum : -1;
		readwriteBlock(volume, 0, volume->blockbuffer, 512, volume->readcmd);
		if (AROS_BE2LONG(volume->blockbuffer[0]) != IDNAME_RIGIDDISK)
		{
		   memset(volume->blockbuffer,0x00, 446); /* Clear the boot sector region! */
			return volume;
	   }
		else
			Printf("no space for bootblock (RDB is on block 0)\n");
	}
	return NULL;
}

ULONG collectBlockList
	(
		struct Volume *volume,
		ULONG block,
		struct BlockNode *blocklist
	)
{
ULONG retval, first_block;
WORD blk_count,count;
UWORD i;

D(bug("[install-i386] collectBlockList(%x, %d, %x)\n", volume, block, blocklist));

	/* TODO: logical/physical blocks */
	/*
		initialze stage2-blocklist
		(it is NULL-terminated)
	*/
//	for (blk_count=-1;blocklist[blk_count].sector!=0;blk_count--)
//		blocklist[blk_count].sector = 0;

   memset((char *)&blocklist[-20],0x00, 20*sizeof(struct BlockNode)); /* Clear the stage2 sector pointers region! */
D(bug("[install-i386] collectBlockList: Cleared sector list (20 entries) [start: %x, end %x]\n", &blocklist[-20], &blocklist[-1]));

	/*
		the first block of stage2 will be stored in stage1
		so skip the first filekey in the first loop
	*/
	/* FIXME: Block read twice */
	retval=readwriteBlock
		(
			volume, block, volume->blockbuffer, volume->SizeBlock<<2,
			volume->readcmd
		);
   if (retval)
	{
D(bug("[install-i386] collectBlockList: ERROR reading block (error: %ld\n", retval));
			Printf("ReadError %lu\n", (long)retval);
			return 0;
		}

	i = volume->SizeBlock - 52;
	first_block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]);
	blk_count=0;
	
D(bug("[install-i386] collectBlockList: First block @ %x, i:%d\n", first_block, i));

	
	do
	{
		retval=readwriteBlock
			(
				volume, block, volume->blockbuffer, volume->SizeBlock<<2,
				volume->readcmd
			);
		if (retval)
		{
D(bug("[install-i386] collectBlockList: ERROR reading block (error: %ld)\n", retval));
			Printf("ReadError %lu\n", (long)retval);
			return 0;
		}
D(bug("[install-i386] collectBlockList: read block %lx, i = %d\n", block, i));
		while ((i>=6) && (volume->blockbuffer[i]))
		{
D(bug("[install-i386] collectBlockList: i = %d\n", i));
			/*
				if current sector follows right after last sector
				then we don't need a new element
			*/
			if (
					(blocklist[blk_count].sector) &&
					((blocklist[blk_count].sector+blocklist[blk_count].count)==
						AROS_BE2LONG(volume->blockbuffer[i]))
				)
			{
				blocklist[blk_count].count += 1;
D(bug("[install-i386] collectBlockList: sector %d follows previous - increasing count of block %d to %d\n", i, blk_count, blocklist[blk_count].count));
			}
			else
			{
				blk_count--; /* decrement first */
D(bug("[install-i386] collectBlockList: store new block (%d)\n", blk_count));
				if (blocklist[blk_count-1].sector != 0)
				{
D(bug("[install-i386] collectBlockList: ERROR: out of block space at sector %d, block %d\n", i, blk_count));
					Printf("There is no more space to save blocklist in stage2\n");
					return 0;
				}
D(bug("[install-i386] collectBlockList: storing sector pointer for %d in block %d\n", i, blk_count));
				blocklist[blk_count].sector = AROS_BE2LONG(volume->blockbuffer[i]);
				blocklist[blk_count].count = 1;
			}
			i--;
		}
		i = volume->SizeBlock - 51;
		block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock - 2]);
D(bug("[install-i386] collectBlockList: next block %d, i = %d\n", block, i));
	} while (block);
	/*
		blocks in blocklist are relative to the first
		sector of the HD (not partition)
	*/
D(bug("[install-i386] collectBlockList: successfully updated pointers for %d blocks\n", blk_count));

	i = 0;
	for (count=-1;count>=blk_count;count--)
	{
		blocklist[count].sector += volume->startblock;
		blocklist[count].seg_adr = 0x820 + (i*32);
		i += blocklist[count].count;
D(bug("[install-i386] collectBlockList: correcting block %d for partition start\n", count));
D(bug("[install-i386] collectBlockList: sector : %x seg_adr : %x\n", blocklist[count].sector, blocklist[count].seg_adr));
	}
	return first_block;
}

void copyRootPath(char *dst, char *rpdos, BOOL isRDB) {

D(bug("[install-i386] copyRootPath()\n"));

	if (isRDB)
	{
		/* we have an RDB so use devicename */
		*dst++ = '/';
		while ((*rpdos) && (*rpdos!=':'))
			*dst++ = *rpdos++;
	}
	else
	{
		while ((*rpdos) && (*rpdos!=':'))
			rpdos++;
	}
	rpdos++; /* skip colon */
	*dst++ = '/';
	/* append path */
	while (*rpdos)
		*dst++ = *rpdos++;
	if (dst[-1] == '/')
		dst[-1] = 0;
	else
		*dst = 0;
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

UBYTE *memstr(UBYTE *mem, UBYTE *str, LONG len) {
UBYTE *next;
UBYTE *search;
LONG left;

	while (len)
	{
		len--;
		if (*mem++ == *str)
		{
			next = mem;
			search = str+1;
			left = len;
			while ((*search) && (left) && (*next++ == *search++))
				left--;
			if (*search == 0)
				return mem-1;
		}
	}
	return 0;
}

BOOL writeStage2
	(
		BPTR fh,
		UBYTE *buffer,
		STRPTR grubpath,
		struct Volume *volume
	)
{
BOOL retval = FALSE;
char *menuname;

D(bug("[install-i386] writeStage2(%x)\n", volume));

	if (Seek(fh, 0, OFFSET_BEGINNING) != -1)
	{
		/* write back first block */
		if (Write(fh, buffer, 512)==512)
		{
			/* read second stage2 block */
			if (Read(fh, buffer, 512) == 512)
			{
				/* set partition number where stage2 is on */
				buffer[8] = 0xFF;
				buffer[9] = 0xFF;
				buffer[10] = volume->partnum;
				buffer[11] = 0;
				/* get ptr to version string */
				menuname = buffer+18;
				while (*menuname++); /* skip version string */
				copyRootPath(menuname, grubpath, volume->flags & VF_IS_RDB);
				strcat(menuname, "/menu.lst");
				/* write second stage2 block back */
				if (Seek(fh, -512, OFFSET_CURRENT) != -1)
				{
					if (Write(fh, buffer, 512) == 512)
					{
						retval = TRUE;
					}
					else
						Printf("%s: Write Error\n", menuname);
				}
				else
					Printf("%s: Seek Error\n", menuname);
			}
			else
				Printf("Read Error\n");
		}
		else
			Printf("Write Error\n");
	}
	else
		PrintFault(IoErr(), NULL);
	return retval;
}

ULONG changeStage2
	(
		STRPTR grubpath,     /* path of grub dir */
		struct Volume *volume, /* volume stage2 is on */
		ULONG *buffer          /* a buffer of at least 512 bytes */
	)
{
ULONG block = 0;
struct FileInfoBlock fib;
BPTR fh;
char stage2path[256];

D(bug("[install-i386] changeStage2(%x)\n", volume));

	AddPart(stage2path, grubpath, 256);
	AddPart(stage2path, "stage2", 256);
	fh = Open(stage2path, MODE_OLDFILE);
	if (fh)
	{
		if (ExamineFH(fh, &fib))
		{
			if (Read(fh, buffer, 512) == 512)
			{
				/*
					get and store all blocks of stage2 in first block of stage2
					first block of stage2 will be returned
				*/
				block = collectBlockList
					(volume, fib.fib_DiskKey, (struct BlockNode *)&buffer[128]);

				if (block)
				{
					if (!writeStage2(fh, (UBYTE *)buffer, grubpath, volume))
						block = 0;
				}
			}
			else
				Printf("%s: Read Error\n", stage2path);
		}
		else
			PrintFault(IoErr(), stage2path);
		Close(fh);
	}
	else
		PrintFault(IoErr(), stage2path);
	return block;
}

BOOL writeStage1
	(
		STRPTR stage1path,
		struct Volume *volume,
		struct Volume *s2vol,
		ULONG block,           /* first block of stage2 file */
		ULONG unit
	)
{
BOOL retval = FALSE;
LONG error = 0;
BPTR fh;

D(bug("[install-i386] writeStage1(%x)\n", volume));

	fh = Open(stage1path, MODE_OLDFILE);
	if (fh)
	{
		if (Read(fh, volume->blockbuffer, 512) == 512)
		{
			/* install into MBR ? */
			if ((volume->startblock == 0) && (!(volume->flags & VF_IS_TRACKDISK)))
			{
D(bug("[install-i386] writeStage1: Install to HARDDISK\n"));
				// read old MBR
				error = readwriteBlock
					(volume, 0,	s2vol->blockbuffer, 512, volume->readcmd);

D(bug("[install-i386] writeStage1: MBR Buffer @ %x\n", volume->blockbuffer));
D(bug("[install-i386] writeStage1: Copying MBR BPB to %x\n", (char *)volume->blockbuffer + 0x04));
				// copy BPB (BIOS Parameter Block)
				CopyMem
					(
						(APTR)((char *)s2vol->blockbuffer + 0x04),
						(APTR)((char *)volume->blockbuffer + 0x04),
						(MBR_BPBEND - 4)
					);
				// copy partition table - [Overwrites Floppy boot code]
D(bug("[install-i386] writeStage1: Copying MBR Partitions to %x\n", (char *)volume->blockbuffer + MBR_PARTSTART));
				CopyMem
					(
						(APTR)((char *)s2vol->blockbuffer + MBR_PARTSTART),
						(APTR)((char *)volume->blockbuffer + MBR_PARTSTART),
						(MBR_PARTEND - MBR_PARTSTART)
					);
				// store the drive num stage2 is stored on
				((char *)volume->blockbuffer)[GRUB_BOOT_DRIVE] =
                    getDriveNumber(volume->device, unit) | BIOS_HDISK_FLAG;
				// Store the stage 2 pointer ..
				ULONG * stage2_sector_start = (ULONG *)((char *)volume->blockbuffer + GRUB_STAGE2_SECTOR);
D(bug("[install-i386] writeStage1: writing stage2 pointer @ %x\n", stage2_sector_start));
	  			stage2_sector_start[0] = block;
D(bug("[install-i386] writeStage1: stage2 pointer = %x\n", stage2_sector_start[0]));	  			
	 			stage2_sector_start[0] += s2vol->startblock;
D(bug("[install-i386] writeStage1: + offset [%d] = %x\n", s2vol->startblock, stage2_sector_start[0]));	
	 			
	 			if (myargs[4]!=0)
	 			{
D(bug("[install-i386] writeStage1: Forcing LBA\n"));
   	 			 ((char *)volume->blockbuffer)[GRUB_FORCE_LBA] = 1;
   	 		}
   	 	   else
   	 	   {
D(bug("[install-i386] writeStage1: NOT Forcing LBA\n"));
   	 	   	 ((char *)volume->blockbuffer)[GRUB_FORCE_LBA] = 0;
   	 	   }
			}
			else
			{
D(bug("[install-i386] writeStage1: Install to FLOPPY\n"));
			}

			if (error == 0)
			{
				error = readwriteBlock
					(volume, 0, volume->blockbuffer, 512, volume->writecmd);
				if (error)
					Printf("WriteError %lu\n", (long)error);
				else
					retval = TRUE;
			}
			else
				Printf("WriteError %lu\n", (long)error);
		}
		else
			Printf("%s: Read Error\n", stage1path);
		Close(fh);
	}
	else
		PrintFault(IoErr(), stage1path);
	return retval;
}

/* Flushes the cache on the volume containing the specified path. */
VOID flushFS(CONST TEXT *path)
{
char devname[256];
UWORD i;

	for (i = 0; path[i] != ':'; i++)
		devname[i] = path[i];
	devname[i++] = ':';
	devname[i] = '\0';
	if (Inhibit(devname, DOSTRUE))
		Inhibit(devname, DOSFALSE);
}

BOOL installStageFiles
	(
		struct Volume *s2vol, /* stage2 volume */
		STRPTR stagepath,     /* path to stage* files */
		ULONG unit,           /* unit stage2 is on */
		struct Volume *s1vol  /* device on which stage1 will be stored */
	)
{
BOOL retval = FALSE;
char stagename[256];
ULONG block;

D(bug("[install-i386] installStageFiles(%x)\n", s1vol));

	/* Flush GRUB volume's cache */
	flushFS(stagepath);

	block = changeStage2(stagepath, s2vol, s1vol->blockbuffer);
	if (block)
	{
		AddPart(stagename, stagepath, 256);
		AddPart(stagename, "stage1", 256);
		if (writeStage1(stagename, s1vol, s2vol, block, unit))
			retval = TRUE;
	}
	return retval;
}

int main(int argc, char **argv) {

struct RDArgs *rdargs;
struct Volume *grubvol;
struct Volume *bbvol;
struct FileSysStartupMsg *fssm;

D(bug("[install-i386] main()\n"));

	rdargs = ReadArgs(template, myargs, NULL);
	if (rdargs)
	{
D(bug("[install-i386] FORCELBA = %d\n",myargs[4]));

		fssm = getDiskFSSM((STRPTR)myargs[3]);
		if (fssm != NULL)
		{
			if (
					(strcmp(AROS_BSTR_ADDR(fssm->fssm_Device),(char*)myargs[0])==0)
				)
			{
				grubvol = getGrubStageVolume
					(
						AROS_BSTR_ADDR(fssm->fssm_Device),
						fssm->fssm_Unit,
						fssm->fssm_Flags,
						(struct DosEnvec *)BADDR(fssm->fssm_Environ)
					);
				if (grubvol)
				{
					bbvol=getBBVolume
						(
							(STRPTR)myargs[0],
							*((LONG *)myargs[1]),
							(LONG *)myargs[2]
						);
					if (bbvol)
					{
					ULONG retval=0;
						/*
							getBBVolume() read block 0
							if the partition directly contains a filesystem
							(currently only DOS\x is supported) we have
							to move block 0 to block 1 to make space for stage1
						*/
						if (
								(grubvol->startblock == bbvol->startblock) &&
								((AROS_BE2LONG(bbvol->blockbuffer[0]) & 0xFFFFFF00)==0x444F5300) 
							)
						{
							grubvol->flags &= ~VF_IS_RDB;
							retval = readwriteBlock
								(bbvol, 0, bbvol->blockbuffer, 512, bbvol->readcmd);
						}
						if (retval == 0)
						{
								installStageFiles
								(
									grubvol,
									(STRPTR)myargs[3], /* grub path (stage1/2) */
									fssm->fssm_Unit,
									bbvol
								);
						}
						else
							Printf("Read Error: %lu\n", (long)retval);
						uninitVolume(bbvol);
					}
					uninitVolume(grubvol);
				}
			}
			else
			{
				Printf
				(
					"%s is not on device %s unit %lu\n",
					(STRPTR)myargs[3], (STRPTR)myargs[0], (long)*((ULONG *)myargs[1])
				);
			}
		}
		else
			if (fssm)
				Printf("kernel path must begin with a device name\n");
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), argv[0]);
	return 0;
}

