/*
    Copyright © 1995-2002, The AROS Development Team. All rights reserved.
    $Id$
*/


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

struct Volume {
   struct MsgPort *mp;
   struct IOExtTD *iotd;
   ULONG readcmd;
   ULONG writecmd;
   ULONG startblock;
   ULONG countblock;
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

struct FileSysStartupMsg *getDiskFSSM(STRPTR path) {
struct DosList *dl;
struct DeviceNode *dn;
char dname[32];
UBYTE i;

	for (i=0;(path[i]) && (path[i]!=':');i++)
		dname[i] = path[i];
	dname[i] = 0;
	dl = LockDosList(LDF_READ);
	if (dl)
	{
		dn = (struct DeviceNode *)FindDosEntry(dl, dname, LDF_DEVICES);
		UnLockDosList(LDF_READ);
		if (dn)
		{
			if (IsFileSystem(dname))
			{
				return (struct FileSysStartupMsg *)BADDR(dn->dn_Startup);
			}
		}
	}
	return 0;
}

void nsdCheck(struct Volume *volume) {
struct NSDeviceQueryResult nsdq;
UWORD *cmdcheck;

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
			printf("Device doesn't understand NSD-Query\n");
		}
		else
		{
			if (
					(volume->iotd->iotd_Req.io_Actual>sizeof(struct NSDeviceQueryResult)) ||
					(volume->iotd->iotd_Req.io_Actual==0) ||
					(volume->iotd->iotd_Req.io_Actual!=nsdq.SizeAvailable)
				)
			{
				printf("WARNING wrong io_Actual using NSD\n");
			}
			else
			{
				if (nsdq.DeviceType != NSDEVTYPE_TRACKDISK)
					printf("WARNING no trackdisk type\n");
				for (cmdcheck=nsdq.SupportedCommands;*cmdcheck;cmdcheck++)
				{
					if (*cmdcheck == NSCMD_TD_READ64)
						volume->readcmd = NSCMD_TD_READ64;
					if (*cmdcheck == NSCMD_TD_WRITE64);
						volume->writecmd = NSCMD_TD_WRITE64;
				}
				if (
						(volume->readcmd!=NSCMD_TD_READ64) ||
						(volume->writecmd!=NSCMD_TD_WRITE64)
					)
					printf("WARNING no READ64/WRITE64\n");
			}
		}
	}
}


struct Volume *initVolume(STRPTR device, ULONG unit, ULONG flags, ULONG size) {
struct Volume *volume;
LONG error=0;

	volume = AllocVec(sizeof(struct Volume), MEMF_PUBLIC | MEMF_CLEAR);
	if (volume)
	{
		volume->mp = CreateMsgPort();
		if (volume->mp)
		{
			volume->iotd = (struct IOExtTD *)CreateIORequest(volume->mp, sizeof(struct IOExtTD));
			if (volume->iotd)
			{
				volume->blockbuffer = AllocVec(size<<2, MEMF_PUBLIC | MEMF_CLEAR);
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

void uninitVolume(struct Volume *volume) {

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
QUAD offset;
ULONG retval=0;

	volume->iotd->iotd_Req.io_Command = command;
	volume->iotd->iotd_Req.io_Length = length;
	volume->iotd->iotd_Req.io_Data = buffer;
	offset = (volume->startblock+block)*(volume->SizeBlock<<2);
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

	if (readwriteBlock(volume, 0, volume->blockbuffer, 512, volume->readcmd))
	{
		printf("Read Error\n");
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
			printf("Read Error\n");
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
			struct TagItem tags[2];
			ULONG type;

				tags[1].ti_Tag = TAG_DONE;
				tags[0].ti_Tag = PTT_TYPE;
				tags[0].ti_Data = (STACKIPTR)&type;
				GetPartitionTableAttrs(ph, tags);
				if (type == PHPTT_MBR)
				{
				struct PartitionHandle *pn;
				struct DosEnvec de;

					tags[0].ti_Tag = PT_DOSENVEC;
					tags[0].ti_Data = (STACKIPTR)&de;
					pn = (struct PartitionHandle *)ph->table->list.lh_Head;
					while (pn->ln.ln_Succ)
					{
					ULONG scp;

						GetPartitionAttrs(pn, tags);
						scp = de.de_Surfaces*de.de_BlocksPerTrack;
						if (
								(volume->startblock>=(de.de_LowCyl*scp)) &&
								(volume->startblock<=(((de.de_HighCyl+1)*scp)-1))
							)
							break;
						pn = (struct PartitionHandle *)pn->ln.ln_Succ;
					}
					if (pn->ln.ln_Succ)
					{
						tags[0].ti_Tag = PT_POSITION;
						tags[0].ti_Data = (STACKIPTR)&type;
						GetPartitionAttrs(pn, tags);
						volume->partnum = (UBYTE)type;
						retval = TRUE;
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
						printf("only MBR and RDB partition tables are supported\n");
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
			printf("Error OpenRootPartition(%s,%ld)\n", device, unit);
		CloseLibrary((struct Library *)PartitionBase);
	}
	else
		printf("Couldn't open partition.library\n");
	return retval;
}

void fillGeometry(struct Volume *volume, struct DosEnvec *de) {
ULONG spc;

	spc = de->de_Surfaces*de->de_BlocksPerTrack;
	volume->SizeBlock = de->de_SizeBlock;
	volume->startblock = de->de_LowCyl*spc;
	volume->countblock =((de->de_HighCyl-de->de_LowCyl+1)*spc)-1+de->de_Reserved;
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

	volume = initVolume(device, unit, flags, de->de_SizeBlock);
	if (volume)
	{
		fillGeometry(volume, de);
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
								printf("partition is not of type AROS (0x30)\n");
						}
						else
						{
							printf
							(
								"partition %ld not found on device %s unit %ld\n",
								*pnum, device, unit
							);
						}
					}
					else
						printf("you can only install in partitions which are MBR partitioned\n");
				}
				else
				{
					/* install into MBR */
					tags[0].ti_Tag = PTT_TYPE;
					tags[0].ti_Data = (STACKIPTR)&type;
					GetPartitionTableAttrs(ph, tags);
					if ((type == PHPTT_MBR) || (type == PHPTT_RDB))
					{
						tags[0].ti_Tag = PTT_DOSENVEC;
						tags[0].ti_Data = (STACKIPTR)de;
						GetPartitionTableAttrs(ph, tags);
						retval = TRUE;
					}
					else
						printf("partition table type must be either MBR or RDB\n");
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
			printf("Error OpenRootPartition(%s,%ld)\n", device, unit);
		CloseLibrary((struct Library *)PartitionBase);
	}
	else
		printf("Couldn't open partition.library\n");
	return retval;
}

struct Volume *getBBVolume(STRPTR device, ULONG unit, LONG *partnum) {
struct Volume *volume;
struct DosEnvec de;

	if (isvalidPartition(device, unit, partnum, &de))
	{
		volume = initVolume(device, unit, 0, de.de_SizeBlock);
		volume->partnum = partnum ? *partnum : -1;
		fillGeometry(volume, &de);
		readwriteBlock(volume, 0, volume->blockbuffer, 512, volume->readcmd);
		if (AROS_BE2LONG(volume->blockbuffer[0]) != IDNAME_RIGIDDISK)
			return volume;
		else
			printf("no space for bootblock (RDB is on block 0)\n");
	}
	return 0;
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

#warning "TODO: logical/physical blocks"
	/*
		initialze stage2-blocklist
		(it is NULL-terminated)
	*/
	for (blk_count=-1;blocklist[blk_count].sector!=0;blk_count--)
		blocklist[blk_count].sector = 0;
	/*
		the first block of stage2 will be stored in stage1
		so skip the first filekey in the first loop
	*/
#warning "Block read twice"
	retval=readwriteBlock
		(
			volume, block, volume->blockbuffer, volume->SizeBlock<<2,
			volume->readcmd
		);
	i = volume->SizeBlock - 52;
	first_block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]);
	blk_count=0;
	do
	{
		retval=readwriteBlock
			(
				volume, block, volume->blockbuffer, volume->SizeBlock<<2,
				volume->readcmd
			);
		if (retval)
		{
			printf("ReadError %ld\n", retval);
			return 0;
		}
		while ((i>=6) && (volume->blockbuffer[i]))
		{
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
			}
			else
			{
				blk_count--; /* decrement first */
				if (blocklist[blk_count-1].sector != 0)
				{
					printf("There is no more space to save blocklist in stage2\n");
					return 0;
				}
				blocklist[blk_count].sector = AROS_BE2LONG(volume->blockbuffer[i]);
				blocklist[blk_count].count = 1;
			}
			i--;
		}
		i = volume->SizeBlock - 51;
		block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock - 2]);
	} while (block);
	/*
		blocks in blocklist are relative to the first
		sector of the HD (not partition)
	*/
	i = 0;
	for (count=-1;count>=blk_count;count--)
	{
		blocklist[count].sector += volume->startblock;
		blocklist[count].seg_adr = 0x820 + (i*32);
		i += blocklist[count].count;
	}
	return first_block;
}

void installStageFiles
	(
		struct Volume *s2vol, /* stage2 volume */
		STRPTR stagepath,     /* path to stage* files */
		ULONG unit,           /* unit stage2 is on */
		struct Volume *s1vol  /* device on which stage1 will be stored */
	)
{
char stagename[256];
struct FileInfoBlock fib;
BPTR fh, fh2;
ULONG block,retval;
ULONG error=0;
STRPTR errstr=NULL;

	AddPart(stagename, stagepath, 256);
	AddPart(stagename, "stage2", 256);
	fh = Open(stagename,MODE_OLDFILE);
	if (fh)
	{
		if (Examine(fh, &fib))
		{
			if (Read(fh, s1vol->blockbuffer, 512) == 512)
			{
				/*
					get and store all blocks of stage2 in first block of stage2
					first block of stage2 will be returned
				*/ 
				block=collectBlockList
					(
						s2vol,
						fib.fib_DiskKey,
						(struct BlockNode *)&s1vol->blockbuffer[128]
					);
				if (block)
				{
					if (Seek(fh, 0, OFFSET_BEGINNING)!=-1)
					{
						/* write back first block */
						if (Write(fh, s1vol->blockbuffer, 512)==512)
						{
						char *menuname;
						ULONG i;
						UBYTE *bootfrom;

							/* read second stage2 block */
							Read(fh, s1vol->blockbuffer, 512);
							/* set partition number where stage2 is on */
							bootfrom = &s1vol->blockbuffer[2];
							bootfrom[0] = 0xFF;
							bootfrom[1] = 0xFF;
							bootfrom[2] = s2vol->partnum;
							bootfrom[3] = 0;
							/* get ptr to version string */
							menuname = ((char *)s1vol->blockbuffer+18);
							/* skip version string */
							while (*menuname++);
							/* now we are at path to menu.lst */
							/* copy new path */
							i=0;
							if (s2vol->flags & VF_IS_RDB)
							{
								/* we have an RDB so use devicename */
								*menuname++ = '/';
								while ((stagepath[i]) && (stagepath[i]!=':'))
									*menuname++ = stagepath[i++];
							}
							else
							{
								while ((stagepath[i]) && (stagepath[i]!=':'))
									i++;
							}	
							i++; /* skip colon */
							*menuname++ = '/';
							/* append path */
							while (stagepath[i])
								*menuname++ = stagepath[i++];
							if (menuname[-1]!='/')
								*menuname++ = '/';
							*menuname=0;
							strcat(menuname, "menu.lst");
							/* write second stage2 block back */
							if (Seek(fh, -512, OFFSET_CURRENT) != -1)
								Write(fh, s1vol->blockbuffer, 512);
							else
								printf("Seek Error: menu.lst path couldn't be written\n");
							AddPart(stagename, stagepath, 256);
							AddPart(stagename, "stage1", 256);
							fh2 = Open(stagename, MODE_OLDFILE);
							if (fh2)
							{
								if (Read(fh2, s1vol->blockbuffer, 512) == 512)
								{
									s1vol->blockbuffer[17]=block;
									retval = 0;
									/* install into MBR? */
									if (
											(s1vol->startblock == 0) &&
											(!(s1vol->flags & VF_IS_TRACKDISK))
										)
									{
										/* add stage2 partition startblock */
										s1vol->blockbuffer[17] += s2vol->startblock;
										/* read ol MBR */
										retval = readwriteBlock
											(
												s1vol, 0,
												s2vol->blockbuffer, 512, s1vol->readcmd
											);
										/* copy BPB (BIOS Parameter Block)*/
										CopyMem
											(
												(APTR)((char *)s2vol->blockbuffer+0x3),
												(APTR)((char *)s1vol->blockbuffer+0x3),
												0x3B
											);
										/* copy partition table */
										CopyMem
											(
												(APTR)((char *)s2vol->blockbuffer+0x1BE),
												(APTR)((char *)s1vol->blockbuffer+0x1BE),
												0x40
											);
										/* store the drive num stage2 is stored on */
										((char *)s1vol->blockbuffer)[0x40] = unit+0x80;
									}
									if (retval==0)
									{
										retval = readwriteBlock
											(
												s1vol, 0,
												s1vol->blockbuffer, 512, s1vol->writecmd
											);
										if (retval)
											printf("WriteError %ld\n", retval);
									}
									else
										printf("WriteError %ld\n", retval);
								}
								else
									error = IoErr();
								Close(fh2);
							}
							else
							{
								error = IoErr();
								errstr = stagename;
							}
						}
						else
							error = IoErr();
					}
					else
						error = IoErr();
				}
			}
			else
			{
				error = IoErr();
				errstr = stagename;
			}
		}
		else
		{
			error = IoErr();
			errstr = stagename;
		}
		Close(fh);
	}
	else
	{
		error = IoErr();
		errstr = stagename;
	}
	if (error)
		PrintFault(error, errstr);
}

struct RDArgs *MyReadArgs(STRPTR template, IPTR *args, struct RDArgs *rdargs) {
struct RDArgs *retval;

	retval = ReadArgs(template, args, rdargs);
	if (args[0] && args[1] && args[3])
	{
		return retval;
	}
	FreeArgs(retval);
	SetIoErr(ERROR_REQUIRED_ARG_MISSING);
	return NULL;
}

int main(int argc, char **argv) {
char *template = "DEVICE/K/A,UNIT/N/K/A,PARTITIONNUMBER=PN/K/N,GRUB/K/A";
IPTR myargs[4] = {0,0,0,0};
struct RDArgs *rdargs;
struct Volume *grubvol;
struct Volume *bbvol;
struct FileSysStartupMsg *fssm;

	rdargs = MyReadArgs(template, myargs, NULL);
	if (rdargs)
	{
		fssm = getDiskFSSM((STRPTR)myargs[3]);
		if (fssm)
		{
			if (
					(strcmp(AROS_BSTR_ADDR(fssm->fssm_Device),(char*)myargs[0])==0)//&&
//					(fssm->fssm_Unit == *((LONG *)myargs[1]))
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
							if the partition directly contains an filesystem
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
								(grubvol, (STRPTR)myargs[3], fssm->fssm_Unit, bbvol);
						}
						else
							printf("Read Error: %ld\n", retval);
						uninitVolume(bbvol);
					}
					uninitVolume(grubvol);
				}
			}
			else
			{
				printf
				(
					"%s is not on device %s unit %ld\n",
					(STRPTR)myargs[3], (STRPTR)myargs[0], *((LONG *)myargs[1])
				);
			}
		}
		else
			PrintFault(ERROR_OBJECT_WRONG_TYPE, argv[0]);
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), argv[0]);
	return 0;
}

