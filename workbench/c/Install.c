#include <strings.h>
#include <stdio.h>

#include <proto/dos.h>
#include <proto/exec.h>

#include <devices/newstyle.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <exec/errors.h>
#include <exec/io.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <exec/ports.h>

struct Volume {
	STRPTR drivename;
	struct MsgPort *mp;
	struct IOExtTD *iotd;
	ULONG readcommand;
	ULONG writecommand;
	ULONG startblock;
	ULONG countblock;
	UWORD SizeBlock;
	UBYTE flags;
	ULONG *blockbuffer;
};

#define VF_IS_TRACKDISK	(1<<0)
#define VF_MOVE_BB		(1<<1)

struct BlockNode {
	ULONG sector;
	UWORD count;
	UWORD seg_adr;
};

ULONG stage2_firstblock[128];

int readwriteBlock
	(
		struct Volume *volume,
		ULONG block, APTR buffer, ULONG length,
		ULONG command
	)
{
QUAD offset;
ULONG retval;

	volume->iotd->iotd_Req.io_Command = command;
	volume->iotd->iotd_Req.io_Length = length;
	volume->iotd->iotd_Req.io_Data = buffer;
	offset = (volume->startblock+block)*(volume->SizeBlock*4);
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

ULONG collectBlockList(struct Volume *volume, ULONG block, struct BlockNode *blocklist) {
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
			volume, block, volume->blockbuffer, volume->SizeBlock*4,
			volume->readcommand
		);
	i = volume->SizeBlock - 52;
	first_block = AROS_BE2LONG(volume->blockbuffer[volume->SizeBlock-51]);
	blk_count=0;
	do {
		retval=readwriteBlock
			(
				volume, block, volume->blockbuffer, volume->SizeBlock*4,
				volume->readcommand
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

void installStageFiles(struct Volume *volume) {
char stagename[256];
struct FileInfoBlock fib;
BPTR fh, fh2;
ULONG block,retval;

	
	strcpy(stagename, (char *)volume->drivename);
	strcat(stagename, "Boot/grub/stage2");
	fh = Open(stagename,MODE_OLDFILE);
	if (fh)
	{
		if (Examine(fh, &fib))
		{
			if (Read(fh, stage2_firstblock, 512) == 512)
			{
				if (volume->flags & VF_MOVE_BB)
				{
					readwriteBlock
						(
							volume, 1, volume->blockbuffer, 512,
							volume->writecommand
						);
				}
				if (
						(
							block=collectBlockList
								(
									volume, fib.fib_DiskKey,
									(struct BlockNode *)&stage2_firstblock[128]
								)
						)
					)
				{
					strcpy(stagename, (char *)volume->drivename);
					strcat(stagename, "Boot/grub/stage1");
					fh2 = Open(stagename, MODE_OLDFILE);
					if (fh2)
					{
						if (Read(fh2, volume->blockbuffer, 512) == 512)
						{
							volume->blockbuffer[17]=block;
							retval = readwriteBlock
								(
									volume, 0,
									volume->blockbuffer, 512, volume->writecommand
								);
							if (retval)
								printf("WriteError %ld\n", retval);
							else
							{
								if (Seek(fh, 0, OFFSET_BEGINNING)==-1)
									PrintFault(IoErr(), NULL);
								else
								{
									if (Write(fh, stage2_firstblock, 512)!=512)
										PrintFault(IoErr(), NULL);
								}
							}
						}
						Close(fh2);
					}
					else
						PrintFault(IoErr(), stagename);
				}
			}
			else
				PrintFault(IoErr(), stagename);
		}
		else
			PrintFault(IoErr(), stagename);
		Close(fh);
	}
	else
		PrintFault(IoErr(), stagename);
}

void nsdCheck(struct Volume *volume) {
struct NSDeviceQueryResult nsdq;
UWORD *cmdcheck;
	if (
			(
				(volume->startblock+volume->countblock)*  /* last block */
				(volume->SizeBlock*4/512)	/* 1 portion (block) equals 512 (bytes) */
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
						volume->readcommand = NSCMD_TD_READ64;
					if (*cmdcheck == NSCMD_TD_WRITE64);
						volume->writecommand = NSCMD_TD_WRITE64;
				}
				if (
						(volume->readcommand!=NSCMD_TD_READ64) ||
						(volume->writecommand!=NSCMD_TD_WRITE64)
					)
					printf("WARNING no READ64/WRITE64\n");
			}
		}
	}
}

struct Volume *initVolume(STRPTR drivename) {
struct Volume *volume=0;
struct DosList *dl;
struct FileSysStartupMsg *fssm;
struct DosEnvec *de;
struct DeviceNode *dn;
char dname[32];
UBYTE i;
ULONG error,retval;

	for (i=0;(drivename[i]) && (drivename[i]!=':');i++)
		dname[i]=drivename[i];
	dname[i]=0;
	dl = LockDosList(LDF_READ | LDF_DEVICES);
	if (dl)
	{
		dn = (struct DeviceNode *)FindDosEntry(dl, dname, LDF_DEVICES);
		UnLockDosList(LDF_READ | LDF_DEVICES);
		if (dn)
		{
			if (IsFileSystem(drivename))
			{
				fssm = (struct FileSysStartupMsg *)BADDR(dn->dn_Startup);
				volume = AllocVec(sizeof(struct Volume), MEMF_PUBLIC | MEMF_CLEAR);
				if (volume)
				{
					volume->drivename = drivename;
					volume->mp = CreateMsgPort();
					if (volume->mp)
					{
						volume->iotd = (struct IOExtTD *)CreateIORequest(volume->mp, sizeof(struct IOExtTD));
						if (volume->iotd)
						{
							de = fssm->fssm_Environ;
							volume->SizeBlock = de->de_SizeBlock;
							volume->blockbuffer = AllocVec(volume->SizeBlock*4, MEMF_PUBLIC | MEMF_CLEAR);
							if (volume->blockbuffer)
							{
								if (
										OpenDevice
											(
												AROS_BSTR_ADDR(fssm->fssm_Device),
												fssm->fssm_Unit,
												(struct IORequest *)&volume->iotd->iotd_Req,
												fssm->fssm_Flags
											)==0
									)
								{
									if (strcmp(AROS_BSTR_ADDR(fssm->fssm_Device), "trackdisk.device") == 0)
										volume->flags |= VF_IS_TRACKDISK;
									volume->startblock = 
										de->de_LowCyl*
										de->de_Surfaces*
										de->de_BlocksPerTrack;
									volume->countblock =
										(
											(
												de->de_HighCyl-de->de_LowCyl+1
											)*de->de_Surfaces*de->de_BlocksPerTrack
										)-1+de->de_Reserved;
									volume->readcommand = CMD_READ;
									volume->writecommand = CMD_WRITE;
									nsdCheck(volume);
									retval = readwriteBlock
										(
											volume, 0,
											volume->blockbuffer, 512, volume->readcommand
										);
									if (retval == 0)
									{
										if ((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)!=0x444F5300)
										{
											retval = readwriteBlock
												(
													volume, 1,
													volume->blockbuffer, 512, volume->readcommand
												);
										}
										else
											volume->flags |= VF_MOVE_BB;
										if (
												((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)==0x444F5300) &&
												((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFF)>0)
											)
										{
											return volume;
										}
										else
											error = ERROR_OBJECT_WRONG_TYPE;
									}
									else
										error = ERROR_UNKNOWN;
								}
								else
									error = ERROR_UNKNOWN;
								FreeVec(volume->blockbuffer);
							}
							else
								error = ERROR_NO_FREE_STORE;
						}
						else
							error = ERROR_NO_FREE_STORE;
						DeleteMsgPort(volume->mp);
					}
					else
						error = ERROR_NO_FREE_STORE;
					FreeVec(volume);
					volume = 0;
				}
				else
					error = ERROR_NO_FREE_STORE;
			}
			else
				error = IoErr();
		}
		else
			error = ERROR_OBJECT_NOT_FOUND;
	}
	else
		error = ERROR_UNKNOWN;
	PrintFault(error, NULL);
	return 0;
}

void uninitVolume(struct Volume *volume) {

	FreeVec(volume->blockbuffer);
	CloseDevice((struct IORequest *)&volume->iotd->iotd_Req);
	DeleteIORequest((struct IORequest *)volume->iotd);
	DeleteMsgPort(volume->mp);
	FreeVec(volume);
}

void checkBootCode(struct Volume *volume) {
	printf("CHECK not implemented yet\n");
}

void removeBootCode(struct Volume *volume) {
ULONG retval;

	retval = readwriteBlock
		(
			volume, 1,
			volume->blockbuffer, 512, volume->readcommand
		);
	if (retval)
		printf("ReadError %ld\n", retval);
	else
	{
		if ((AROS_BE2LONG(volume->blockbuffer[0]) & 0xFFFFFF00)==0x444F5300)
		{
			retval = readwriteBlock
				(
					volume, 0,
					volume->blockbuffer, 512, volume->writecommand
				);
			if (retval)
				printf("WriteError %ld\n", retval);
		}
	}
}

int main(void) {
LONG myargs[4]={0,0,0,0};
struct RDArgs *rdargs;
struct Volume *volume;

	rdargs = ReadArgs("DRIVE/A,NOBOOT/S,CHECK/S,FFS/S",myargs,NULL);
	if (rdargs)
	{
		volume = initVolume((STRPTR)myargs[0]);
		if (volume)
		{
			if (myargs[1])
				removeBootCode(volume);
			else if (myargs[2])
				checkBootCode(volume);
			else
				installStageFiles(volume);
			uninitVolume(volume);
		}
		FreeArgs(rdargs);
	}
	else
		PrintFault(IoErr(), NULL);
	return 0;
}

