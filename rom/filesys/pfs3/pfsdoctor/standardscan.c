/* $Id$
 * $Log: standardscan.c $
 * Revision 2.10  1999/09/11  16:45:50  Michiel
 * Versie 1.5 with Unformat and Repair nodos
 *
 * Revision 2.9  1999/09/10  22:14:49  Michiel
 * Bugfixes etc (1.4)
 *
 * Revision 2.8  1999/08/01  10:48:21  Michiel
 * less verbose
 *
 * Revision 2.7  1999/05/28  05:07:33  Michiel
 * Fixed bug occuring on empty directory blocks
 * Added rbl.always fix; improved rbl.disksize fix
 * Reduced cachesize
 *
 * Revision 2.6  1999/05/17  10:32:39  Michiel
 * long filename support, verbose fixes
 *
 * Revision 2.5  1999/05/17  09:27:11  Michiel
 * fixed logfile bug
 * made verbose less verbose
 *
 * Revision 2.4  1999/05/07  16:49:00  Michiel
 * bugfixes etc
 *
 * Revision 2.3  1999/05/04  17:59:09  Michiel
 * check mode, logfile, search rootblock implemented
 * bugfixes
 *
 * Revision 2.1  1999/04/30  12:17:58  Michiel
 * Accepts OK disks, bitmapfix and hardlink fix works
 *
 * Revision 1.2  1999/04/22  15:23:50  Michiel
 * compiled
 *
 * Revision 1.1  1999/04/19  22:16:53  Michiel
 * Initial revision
 *
 */

#include <exec/types.h>
#include <exec/ports.h>
#include <devices/trackdisk.h>
#include <libraries/asl.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <proto/asl.h>
#include <proto/exec.h>
#include "pfs3.h"
#include "doctor.h"

long __stack = 30*1024;
extern struct Window *CheckRepairWnd;

/**************************************
 * Globals
 **************************************/

struct
{
	/* state flags */
	uint32 flags;			/* internal flags */
	uint32 opties;			/* options */
	int pass;
	BOOL verbose;
	BOOL unformat;

	enum {syntax, resbitmap, mainbitmap, anodebitmap, finished} stage;

	struct MinList *doubles;
} ss;

volume_t volume;
char bericht[256];
rootblock_t *rbl= NULL;
c_extensionblock_t rext = { 0 };
bool redosyntax;
bool aborting = 0;	// used by break trap

/**************************************
 * Protos
 **************************************/

static error_t mainStandardScan(uint32 flags);
static error_t exitStandardScan(error_t error);
static error_t ss_CheckDirTree(void);
static error_t vol_CheckBlockNr(ULONG *blocknr);
static error_t GetRootBlock(void);
static error_t CheckRootBlock(void);
static error_t GetRext(void);
static error_t RepairBootBlock(void);
static error_t RepairDeldir(void);
static bool dd_CheckBlock(uint32 bloknr, int seqnr);
static error_t RepairDirTree(void);
static error_t RepairDir(struct direntry *de, c_dirblock_t *parent);
static error_t RepairDirBlock(uint32 bloknr, uint32 anodenr, uint32 parent);
static error_t RepairDirEntry(struct direntry *de, c_dirblock_t *dirblk);
static error_t RepairFile(struct direntry *de, c_dirblock_t *parent);
static error_t RepairHardLink(struct direntry *de, c_dirblock_t *dirblok);
static error_t RepairSoftLink(struct direntry *de, c_dirblock_t *dirblok);
static error_t RepairRollover(struct direntry *de, c_dirblock_t *dirblok);
static error_t RemoveDirEntry(struct direntry *de, c_dirblock_t *dirblk);
static error_t DeleteAnode(uint32 anodechain, uint32 node);
static error_t GetExtraFields(struct extrafields *extrafields, struct direntry *de);
static error_t SetExtraFields(struct extrafields *extrafields, struct direntry *from, c_dirblock_t *dirblk);
static error_t RepairLinkChain(struct direntry *de, c_dirblock_t *dirblk);
static error_t CheckAnode(canode_t *anode, uint32 nr, bool fix);
static error_t RepairAnodeTree(void);
static error_t RepairBitmapTree(void);
static error_t InitReservedBitmap(void);
static error_t InitAnodeBitmap(void);
static error_t InitMainBitmap(void);
static error_t RepairReservedBitmap(void);
static error_t RepairMainBitmap(void);
static error_t RepairAnodeBitmap(void);
static error_t MainBlockUsed(uint32 bloknr);
static error_t AnodeUsed(uint32 bloknr);
static error_t bg_ItemUsed(bitmap_t *bm, uint32 nr);
static BOOL IsAnodeUsed(uint32 nr);
static uint32 vol_SearchFileSystem(void);
static bitmap_t *bg_InitBitmap(int32 start, int32 stop, int32 step);
static void bg_KillBitmap(bitmap_t **bm);
static BOOL bg_IsItemUsed(bitmap_t *bm, uint32 nr);
int SearchInDir(uint32 diranodenr, uint32 target);
static void AddExtraFields(struct direntry *direntry, struct extrafields *extra);
static bool MakeMountFile(char *fname);
static error_t BuildRootBlock_hub(void);

/**************************************
 * Break function  					L1
 **************************************/

#ifdef __SASC
int brk(void)
{
	aborting = 1;
	return 0;
}

void __regargs __chkabort(void)
{
	/* Check & clear CTRL_C signal */
	if(SetSignal(0L,SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C)
	{
		brk();
	}
}
#endif

static BOOL FileSizeCheck(ULONG low, ULONG high, ULONG numblocks)
{
	ULONG size, oldlow;
	
	oldlow = low;
	low += volume.blocksize - 1;
	if (low < oldlow)
		high++;
	size = low >> volume.blockshift;
	size |= high << (32 - volume.blockshift);
	if (size == numblocks)
		return TRUE;
	return FALSE;
}	

/**************************************
 * StandardScan						L1
 **************************************/

// public interface
error_t StandardScan(uint32 opties)
{
	uint32 flags;

	aborting = 0;
	// onbreak(&brk);
	memset(&ss, sizeof(ss), 0);
	opties = opties;

	flags = opties;
	if (opties & (SSF_CHECK|SSF_FIX))
		flags |= SSF_GEN_BMMASK;

	/* init stats */
	memset(&stats, 0, sizeof(stats));
	return mainStandardScan(flags);
}

// core standardscan 
static error_t mainStandardScan(uint32 flags)
{
	error_t error;

	volume.showmsg("Initializing\n");
	ss.flags = flags;
	ss.stage = syntax;
	ss.pass = stats.pass = 1;
	ss.verbose = flags & SSF_VERBOSE;
	ss.unformat = flags & SSF_UNFORMAT;

	InitFullScan();

	/* unformat .. */
	if (ss.unformat)
	{
		mode = repair; 		// !! niet really correct !! fix this
		volume.showmsg("Unformatting...\n");
		if (!(rbl = (rootblock_t *)AllocBufMem (MAXRESBLOCKSIZE)))
		{
			adderror("couldn't allocate memory for rootblock");
			return exitStandardScan(e_out_of_memory);
		}
		if ((error = BuildRootBlock_hub()) || aborting)
			return exitStandardScan(error);
	}

	/* read rootblock */
	volume.status(0, "rootblock", 100);
	if ((error = GetRootBlock()) || aborting)
		return exitStandardScan(error);

	volume.progress(0, 40);
	if ((error = RepairBootBlock()) || aborting)
		return exitStandardScan(error);

	/* after the rootblock and bootblock the boundaries of the partition are known
	 */

	/* read rootblockextension */
	volume.progress(0, 30);
	rext.data = calloc(1, SIZEOF_RESBLOCK);
	rext.mode = check;
	if ((error = GetRext()) || aborting)
		return exitStandardScan(error);

	GetPFS2Revision(bericht);
	volume.showmsg("Disk formatted with ");
	volume.showmsg(bericht);
	volume.showmsg("\n");

	/* stage 1 */
	volume.progress(0, 30);
	while (ss.stage != finished && ss.pass < MAX_PASS)
	{
		stats.pass = ss.pass;
		sprintf(bericht, "Starting pass %d\n", ss.pass);
		volume.showmsg(bericht);
		volume.updatestats();
		if (mode == check && stats.numerrors > 0)
			break;
		error = ss_CheckDirTree();
		if ((error != e_none) || aborting)
			return exitStandardScan(error);
		if (mode == check)
			break;
		ss.pass++; 
	}

	if (ss.stage != finished)
		return e_max_pass_exceeded;

	volume.status(0, "finishing up", 2);
	volume.progress(0, 1);
	exitStandardScan(e_none);
	volume.progress(0, 1);
	return e_none;
}

static error_t exitStandardScan(error_t error)
{
	UpdateCache();

	if (rbl)
	{
		FreeBufMem(rbl);
		rbl = NULL;
	}
	
	if (rext.mode == check)
		free (rext.data);
	rext.data = NULL;

	KillReservedBitmap();
	KillAnodeBitmap();
	KillMainBitmap();
	ExitFullScan();

	if ((error != e_none) || aborting)
		volume.showmsg("ABORTED\n");
	else if (ss.unformat)
	{
		volume.askuser(
			"Disk has been unformatted. Unformatting can\n"
			"cause problems later. Therefor we recommend\n"
			"to format this partition after backing up  \n"
			"all data\n", "OK", NULL); 
	}
	return error;
}


/* main loop: check the partition defined by the rootblock and
 * rext (rootblock extension)
 */
static error_t ss_CheckDirTree(void)
{
	error_t error;
	int newpassneeded = 0;

	clearstats();
	redosyntax = false;
	switch (ss.stage)
	{
		case syntax:
		case resbitmap:

			/* initialize reserved bitmap generation */
			if (ss.flags & SSF_GEN_RESBITMAP)
				InitReservedBitmap();

			if (rext.mode == check)
				if ((error = ResBlockUsed(rext.blocknr)))
					return error;

			/* anode blok tree */
			/* if anodeindexblok faulty --> create (using DiskScan) */
			if ((error = RepairAnodeTree()) || aborting)
				return error;

			/* bitmap */
			/* check syntax bitmap tree
			 * if block faulty --> create, kill bitmap, ss_makebm = false
			 * and continue */
			if ((error = RepairBitmapTree()) || aborting)
				return error;

		case mainbitmap:

			if (ss.flags & SSF_GEN_MAINBITMAP)
				InitMainBitmap();

		case anodebitmap:

			if (ss.flags & SSF_GEN_ANODEBITMAP)
				InitAnodeBitmap();

		default:

			break;
	}

	/* deldir */
	if ((error = RepairDeldir()) || aborting)
		return error;

	/* directory tree */
	if ((error = RepairDirTree()) || aborting)
		return error;
	
	/* on hardlink conflict */
	if (redosyntax)
	{
		ss.stage = syntax;
		return e_none;
	}

	switch (ss.stage)
	{
		case syntax:
		case resbitmap:

			/* need another pass if blocks were created
			 * (should not be necessary!!)
			 */
			if (!IsMinListEmpty(&volume.buildblocks))
				newpassneeded = 1;

			/* allocating created blocks & reparing reserved bitmap can only
			 * be done after the reserved bitmap has been generated.
			 * ss.flags is set to false if generation was aborted.
			 */
			if (IsBitmapValid(volume.resbitmap))
			{
				if ((error = AllocBuildBlocks()))
					return error;

				if ((error = RepairReservedBitmap()) || aborting)
					return error;
			}
			else
			{
				if (ss.stage < resbitmap)
				{
					ss.stage = resbitmap;
					break;
				}
				else
					return e_res_bitmap_fail;			/* fatal error */
			}

			if (newpassneeded)
				break;
			
		case mainbitmap:
		case anodebitmap:

			/* bitmap generatie has been aborted (disabled during scan)
			 * generate main and anodebitmap
			 */
			if (!IsBitmapValid(volume.mainbitmap))
			{
				if (ss.stage < mainbitmap)
				{
					ss.stage = mainbitmap;
					break;
				}
				else
					return e_main_bitmap_fail;		/* fatal error */
			}

			if (!IsBitmapValid(volume.anodebitmap))
			{
				if (ss.stage < anodebitmap)
				{
					ss.stage = anodebitmap;
					break;
				}
				else
					return e_anode_bitmap_fail;
			}

			if ((error = RepairMainBitmap()) || aborting)
				return error;

			if ((error = RepairAnodeBitmap()) || aborting)
				return error;

			ss.stage = finished;
			break;

		default:
			// case doubledel:

			ss.stage = mainbitmap;
	}

	/* Need another pass if blocks build (created) during
     * scan. Calling allocbuildblocks here is pointless;
     * the reserved bitmap is not valid because of invalid blocks,
     * possibly during non reserved bitmap phase!
	 */
	if (!IsMinListEmpty(&volume.buildblocks))
		ss.stage = resbitmap;

	return e_none;
}


/**************************************
 * Volume functions
 **************************************/

/* functions for in cache struct
 * gets block from reserved area and checks partition
 * borders etc.
 * status/errors: 
 * read/write errors
 * e_none
 * block_outside_reserved
 * block_outside_partition
 */

/* check if a block is in partition range */
static error_t vol_CheckBlockNr(ULONG *blocknr)
{
	if (*blocknr > volume.lastreserved) {

		adderror("block outside reserved area");
		return e_block_outside_reserved;
	}

	*blocknr += volume.firstblock;
	if (*blocknr > volume.lastblock || *blocknr < volume.firstblock) {

		adderror("block outside partition");
		return e_block_outside_partition;
	}

	return e_none;
}
	

error_t vol_GetBlock(cachedblock_t *blok, ULONG bloknr)
{
	error_t error;
	ULONG realbloknr;

	realbloknr = bloknr;
	if ((error = vol_CheckBlockNr(&realbloknr)))
		return error;

	if ((error = c_GetBlock((uint8 *)blok->data, realbloknr, SIZEOF_RESBLOCK)))
	{
		return error;
	}
	else
	{
		blok->mode = check;
		blok->blocknr = bloknr;
		return e_none;
	}
}

error_t vol_WriteBlock(cachedblock_t *blok)
{
	int status;
	uint32 bloknr;

	if (blok->mode != build)
	{
		bloknr = blok->blocknr;
		if ((status = vol_CheckBlockNr(&bloknr)))
			return status;

		return c_WriteBlock((uint8 *)blok->data, bloknr, SIZEOF_RESBLOCK);
	}

	return e_none;
}


/**************************************
 * GetRootBlock
 **************************************/

// public functions

static error_t BuildRootBlock_hub(void)
{
	error_t error;

	if (volume.askuser(
		"Press ok to rebuild rootblock or cancel to exit\n"
		"WARNING: make sure correct accessmode (direct-scsi/td64/nsd)\n"
		"is selected", "ok", "cancel"))
	{
		if (!(error = BuildRootBlock(rbl)))
		{
			fixederror("new rootblock build");
			c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
		}
	}
	else
		error = e_aborted;

	return error;
}

/* read from disk and check
 * Returns error (0 = ERROR_NONE = ok).
 */
static error_t GetRootBlock(void)
{
	error_t error = e_none;
	uint32 bloknr;
	int okuser;
	struct FileRequester *freq;
	char mfname[FNSIZE], *t;
	bool ok = false;

	enterblock(ROOTBLOCK);
	volume.showmsg("Checking rootblock\n");
	if (rbl)
		FreeBufMem(rbl);

	if (!(rbl = (rootblock_t *)AllocBufMem (MAXRESBLOCKSIZE)))
	{
		adderror("couldn't allocate memory for rootblock");
		return e_out_of_memory;
	}

	// read rootblock 
	if ((error = c_GetBlock ((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize)))
	{
		adderror("rootblock could not be loaded");
		return error;
	}

	// check rootblock type
	if (!IsRootBlock(rbl))
	{
		adderror("not an AFS, PFS-II or PFS-III disk");
		okuser = volume.askuser("Rootblock not found.\n"
			"Select ok to search for misplaced rootblock.", "ok", "cancel");

		if (okuser && (bloknr = vol_SearchFileSystem()))
		{
			volume.showmsg("\nRootblock found. Repartitioning .. \n");
			if ((error = Repartition(bloknr)))
				return error;

			volume.askuser(
				"Partition is misplaced. Probable cause of this problem\n"
				"is the use of the 'Get Drive Definition' option in\n"
				"HDToolbox after the disk was formatted\n\n"
				"Now a mountfile will be needed to mount the partition.\n"
				"Press CONTINUE to select location to store the\n"
				"mountlist", "CONTINUE", NULL);

			freq = AllocAslRequestTags(ASL_FileRequest, ASLFR_SleepWindow, TRUE, 
				ASLFR_TitleText, "Select mountfile", ASLFR_InitialFile, "mountme",
				ASLFR_InitialDrawer, "df0:", ASLFR_DoSaveMode, TRUE, TAG_DONE);

			do
			{
				if (AslRequestTags(freq, ASLFR_Window, CheckRepairWnd, TAG_DONE))
				{
					t = stpcpy(mfname, freq->fr_Drawer);
					t = stpcpy(t, freq->fr_File);
					if ((ok = MakeMountFile(mfname)))
						volume.showmsg("Mountlist written\n");
				}
				else
				{
					volume.showmsg("no mountlist written\n");
				}
			} while (!ok);

			FreeAslRequest(freq);	
			volume.askuser("Now starting to check/repair\n"
				"relocated partition", "CONTINUE", NULL);

			return GetRootBlock();
		}
		else
		{
			okuser = volume.askuser("Rootblock not found.\n"
			"Select ok to build a new one.\n"
			"WARNING: Make sure this is a PFS partition.\n"
			"Non-PFS disks will be destroyed by this operation\n",
			"ok", "cancel");

			if (okuser)
				return BuildRootBlock_hub();
			else
			{
				aborting = 1;
				return e_aborted;
			}
		}
	}
	
	error = CheckRootBlock();

	switch (error)
	{
		case e_none:
			break;

		case e_repartition:
			if ((error = Repartition(volume.firstblock+ROOTBLOCK)))
				return error;

			volume.askuser(
				"The partition information stored in the RDB does\n"
				"not match the partition information used when the\n"
				"disk was formatted. Probable cause of this problem\n"
				"is the use of the 'Get Drive Definition' option in\n"
				"HDToolbox after the disk was formatted\n\n"
				"Now a mountfile will be needed to mount the partition.\n"
				"Press CONTINUE to select location to store the\n"
				"mountlist", "CONTINUE", NULL);

			freq = AllocAslRequestTags(ASL_FileRequest, ASLFR_SleepWindow, TRUE, 
				ASLFR_TitleText, "Select mountfile", ASLFR_InitialFile, "mountme",
				ASLFR_InitialDrawer, "df0:", ASLFR_DoSaveMode, TRUE, TAG_DONE);

			do
			{
				if (AslRequestTags(freq, ASLFR_Window, CheckRepairWnd, TAG_DONE))
				{
					t = stpcpy(mfname, freq->fr_Drawer);
					t = stpcpy(t, freq->fr_File);
					if ((ok = MakeMountFile(mfname)))
						volume.showmsg("Mountlist written\n");
				}
				else
				{
					volume.showmsg("no mountlist written\n");
				}
			} while (!ok);

			FreeAslRequest(freq);	
			volume.askuser("Now starting to check/repair\n"
				"redefined partition", "CONTINUE", NULL);

			return GetRootBlock();

		case e_options_error:
			return error;

		default:
			return BuildRootBlock_hub();
	}

	exitblock();
	return e_none;
}

static bool MakeMountFile(char *fname)
{
	FILE *mf;

	if ((mf = fopen(fname, "w")))
	{
		fprintf(mf, "/****************************************/\n");
		fprintf(mf, "/* PFSDoctor generated mountlist        */\n");
		fprintf(mf, "/****************************************/\n");
		fprintf(mf, "REPAIRED:\n");
		fprintf(mf, "\tDevice         = %s\n", volume.execdevice);
		fprintf(mf, "\tUnit           = %d\n", volume.execunit);
		fprintf(mf, "\tBuffers        = 300\n");
		fprintf(mf, "\tBufMemType     = %lu\n", volume.dosenvec->de_BufMemType);
		fprintf(mf, "\tFlags          = %lu\n", volume.fssm->fssm_Flags);
		fprintf(mf, "\tDosType        = %#lx\n", volume.dosenvec->de_DosType);
		fprintf(mf, "\tGlobVec        = -1\n");
		fprintf(mf, "\tInterleave     = %ld\n", volume.dosenvec->de_Interleave);
		fprintf(mf, "\tReserved       = 2\n");
		fprintf(mf, "\tLowCyl         = %lu\n", volume.firstblock);
		fprintf(mf, "\tHighCyl        = %lu\n", volume.lastblock);
		fprintf(mf, "\tMount          = 1\n");
		fprintf(mf, "\tSurfaces       = 1\n");
		fprintf(mf, "\tBlocksPerTrack = 1\n");
		fprintf(mf, "\tPriority       = 10\n");
		fprintf(mf, "\tStackSize      = 600\n");
		fprintf(mf, "\tMaxTransfer    = %lu\n", volume.dosenvec->de_MaxTransfer);
		fprintf(mf, "\tMask           = %#lx\n", volume.dosenvec->de_Mask);
		fclose(mf);
		return true;
	}
	return false;
}

static uint32 vol_SearchFileSystem(void)
{
	return SearchFileSystem(
			(volume.firstblock > 1024) ? volume.firstblock - 1024 : 0,
			volume.firstblock + 1024);
}

bool IsRootBlock(rootblock_t *r)
{
	ULONG modemask;

	// check rootblock type
	if (r->disktype != ID_PFS_DISK && r->disktype != ID_PFS2_DISK) {
		if (ss.verbose)
			volume.showmsg("Unexpected rootblock id 0x%08lx\n", r->disktype);
		return false;
	}

	// check options
	// require non-null options to accept rootblock as such,
	// otherwise it could be a bootblock 
	modemask = MODE_HARDDISK + MODE_SPLITTED_ANODES + MODE_DIR_EXTENSION;
	if ((r->options & modemask) != modemask) {
		if (ss.verbose)
			volume.showmsg("Unexpected rootblock options 0x%08lx\n", r->options);
		return false;
	}

	return true;
}


/* check loaded rootblock
 */
static error_t CheckRootBlock(void)
{
	int error = 0;
	bool dirty = false;
	ULONG modemask, resblocksize;

	modemask = MODE_HARDDISK + MODE_SPLITTED_ANODES + MODE_DIR_EXTENSION
			 + MODE_DELDIR + MODE_SIZEFIELD + MODE_EXTENSION + MODE_DATESTAMP
			 + MODE_SUPERINDEX + MODE_SUPERDELDIR + MODE_EXTROVING + MODE_LONGFN
			 + MODE_LARGEFILE;

	if (rbl->options & ~modemask)
	{
		adderror("Unknown options enabled. Ask for an upgrade.");
		return e_options_error;
	}

	// check the fields
	if (!rbl->diskname[0])
	{
		if (mode == check)
			adderror("volume has no name");
		else
		{
			fixederror("volume had no name");
			rbl->diskname[0] = strlen("doctor");
			strcpy((char *)&rbl->diskname[1], "doctor");
			dirty = true;
		}
	}

	strncpy (volume.diskname, (char *)&rbl->diskname[1], rbl->diskname[0]);
	volume.diskname[rbl->diskname[0]+1] = 0;
	volume.showmsg("Checking disk ");
	volume.showmsg(volume.diskname);
	volume.showmsg("\n");
	volume.showmsg("Reserved blocksize: %lu\n", rbl->reserved_blksize);

	resblocksize = 1024;
	if (rbl->disktype == ID_PFS2_DISK) {
		if (volume.disksize > MAXDISKSIZE4K) {
			adderror("too large (>1.6TB) partition size");
			return e_options_error;
		}
		if (volume.disksize > MAXDISKSIZE1K) {
			resblocksize = 2048;
			if (volume.disksize > MAXDISKSIZE2K)
				resblocksize = 4096;
		}
	} else {
		if (volume.disksize > MAXDISKSIZE1K) {
			adderror("too large (>104GB) partition size");
			return e_options_error;
		}
	}

	if (rbl->reserved_blksize != resblocksize)
	{
		if (rbl->reserved_blksize == 1024 || rbl->reserved_blksize == 2048 || rbl->reserved_blksize == 4096) {
			adderror("reserved blocksize is valid but does not match partition size. Driver size limit problem?");
			return e_options_error;
		}
		sprintf(bericht, "wrong reserved blocksize of %d", rbl->reserved_blksize);
		fixederror(bericht);
		rbl->reserved_blksize = resblocksize;
		dirty = true;
	}

	volume.rescluster = resblocksize / volume.blocksize;

	// size must match, else repartition
	if (rbl->options & MODE_SIZEFIELD)
	{
		if(rbl->disksize != volume.disksize)
		{
			int32 difference;

			/* uses rule: if difference less then 10%, assume mistake
			 * in RDB, otherwise assume mistake in rootblock
			 */
			adderror("wrong disksize");
			volume.showmsg("Rootblock size: %lu, driver reported size: %lu\n", rbl->disksize, volume.disksize);
			difference = abs((int32)rbl->disksize - (int32)volume.disksize);
			if (difference < volume.disksize/10)
			{
				error = e_repartition;
			}
			else if (mode != check)
			{
				rbl->disksize = volume.disksize;
				dirty = true;
				fixederror("set disksize");
			}
		}
	}

	if (rbl->alwaysfree < volume.disksize/40 || 
		rbl->alwaysfree > volume.disksize/15)
	{
		fixederror("allocation block buffer out of range");
		rbl->alwaysfree = volume.disksize/20;
		dirty = true;
	}

	if (dirty)
		return c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);

	return error;
}


/**************************************
 * GetRootBlockExtension
 **************************************/

/* check rootblock extension. Do 'easy fix'. Returns
 * false if failure
 */
static error_t GetRext(void)
{
	error_t error;
	uint32 bloknr, oldbloknr;

	bloknr = rbl->extension;
	enterblock(bloknr);
	volume.showmsg("Checking rext\n");

	if (bloknr)
	{
		if ((error = volume.getblock((cachedblock_t *)&rext, bloknr)))
			return error;
	}

	if (!bloknr || rext.data->id != EXTENSIONID)
	{
		adderror("extension block has wrong id");
		if (mode == repair)
		{
			volume.status(1, "Searching rext", 256);
			oldbloknr = bloknr;
			bloknr = SearchBlock(EXTENSIONID, 0, bloknr, 0, 0, 0);
			if (bloknr)
			{
				sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, bloknr);
				fixederror(bericht);
				rbl->extension = bloknr;
				return GetRext();
			}
			else
			{
				volume.showmsg("No replacement block found\n");
				free (rext.data);
				if ((error = BuildRext(&rext)))
					adderror("building new rootblockextension failed");
				else
					fixederror("new rootblockextension created");
				return error;
			}
		}
		else
		{
			return e_fatal_error;
		}
	}

	// ResBlockUsed(bloknr); in initreservedbitmap 

	/* check if enabled */
	if (!(rbl->options & MODE_EXTENSION))
	{
		rbl->options |= MODE_EXTENSION;
		volume.writeblock((cachedblock_t *)&rext);
		if (mode == check)
			adderror("MODE_EXTENSION is disabled");
		else
			fixederror("MODE_EXTENSION was disabled");
	}

	/* fnsize */
	if (rbl->options & MODE_LONGFN)
	{
		if (rext.data->fnsize < 30 || rext.data->fnsize > 108)
		{
			sprintf(bericht, "illegal filename size of %d", rext.data->fnsize);
			if (mode == check)
				adderror(bericht);
			else
			{
				if (rext.data->fnsize < 30)
				{
					fixederror("reset filename size to 30");
					rext.data->fnsize = 30;
				}
				else
				{
					fixederror("reset filename size to 100");
					rext.data->fnsize = 100;
				}
				volume.writeblock((cachedblock_t *)&rext);
			}
		}
		volume.fnsize = rext.data->fnsize - 1;
	}
	else
	{
		volume.fnsize = 32 - 1;
	}

	/* tbd fields */
	if (rext.data->postponed_op[0] ||
		rext.data->postponed_op[1] ||
		rext.data->postponed_op[2] ||
		rext.data->postponed_op[3])
	{
		if (mode == check)
			adderror("rootblock extension TBD not empty");
		else
		{
			rext.data->postponed_op[0] =
			rext.data->postponed_op[1] =
			rext.data->postponed_op[2] =
			rext.data->postponed_op[3] = 0;
			volume.writeblock((cachedblock_t *)&rext);
			fixederror("rootblock extension TBD not empty");
		}
	}

	exitblock();
	return e_none;
}


/* check loaded rootblock extension
 */
bool GetPFS2Revision(char *vstring)
{
	char buf[8];
	uint16 ver = rext.data->pfs2version >> 16;
	uint16 rev = rext.data->pfs2version & 0xffff;

	if (!rext.data)
		return false;

	if (ver < 16)
		strcpy(buf, "AFS");
	else
		if (ver == 16)
			if (rev < 22)
				strcpy(buf, "AFS");
			else
				strcpy(buf, "PFS-II");
		else if (ver >= 18)
			strcpy(buf, "PFS-III");
		else
			strcpy(buf, "PFS-II");

	sprintf(vstring, "%s %d.%d", buf, ver, rev);

	return true;
}



/**************************************
 * RepairBootBlock
 **************************************/

/* read and check bootblock (not store)
 */
static error_t RepairBootBlock(void)
{
	error_t error;
	cachedblock_t bootbl;

	enterblock(BOOTBLOCK);

	// read bootblock 
	bootbl.data = calloc(1, MAXRESBLOCKSIZE);
	if ((error = volume.getblock ((cachedblock_t *)&bootbl, BOOTBLOCK)))
	{
		adderror("bootblock could not be loaded");
		free (bootbl.data);
		return error;
	}

	// check bootblock type
	if (bootbl.data->bootblock.disktype != ID_PFS_DISK)
	{
		if (mode == check)
			adderror("bootblock has wrong ID");
		else
		{
			bootbl.data->bootblock.disktype = ID_PFS_DISK;
			fixederror("bootblock had wrong ID");
			if ((error = volume.writeblock((cachedblock_t *)&bootbl)))
			{
				adderror("bootblock could not be written");
				free (bootbl.data);
				return error;
			}
		}
	}

	free(bootbl.data);
	exitblock();
	return e_none;
}


/**************************************
 * RepairDeldir
 **************************************/

/* Check plus fix deldir
 */
static error_t RepairDeldir(void)
{
	int i;
	bool remove;
	uint32 ddsize;
	error_t error;
	bool rextdirty = false, rootdirty = false;

	volume.showmsg("Checking deldir\n");

	/* check deldir version */
	if (rbl->options & MODE_DELDIR)
	{
		if (rbl->options & MODE_SUPERDELDIR)
		{
			ddsize = rext.data->deldirsize;
			volume.status(0, "Deldir", ddsize);

			for (i=0; i<32; i++)
			{
				if (rext.data->deldir[i])
				{
					enterblock(rext.data->deldir[i]);
					if (ss.verbose)
					{
						sprintf(bericht, "DELDIR %d\n", i);
						volume.showmsg(bericht);
					}

					remove = false;
					if (i <= ddsize)
					{
						remove = true;
						if (dd_CheckBlock(rext.data->deldir[i], i))
						{
							if ((error = ResBlockUsed(rext.data->deldir[i])))
								remove = true;
							else
								remove = false;
						}
					}

					if (remove && mode == repair)
					{
						sprintf(bericht, "deldir block %d removed\n", i);
						fixederror(bericht);
						rext.data->deldir[i] = 0;
						ddsize = min(ddsize, i+1);
						rextdirty = true;
					}
					exitblock();
				}
				else
				{
					ddsize = min(ddsize, i+1);
				}
			}

			if (ddsize != rext.data->deldirsize)
			{
				rext.data->deldirsize = ddsize;
				rextdirty = true;
			}

			if (rbl->deldir)
			{
				if (mode == check)
					adderror("reference to old deldir");
				else
				{
					rbl->deldir = 0;
					rootdirty = true;
					fixederror("reference to old deldir removed");
				}
			}
		}
		else		/* old mode */
		{
			if (!rbl->deldir)
			{
				if (mode == check)
					adderror("deldir enabled, but missing");
				else
				{
					fixederror("deldir enabled, but missing");
					rbl->options &= ~MODE_DELDIR;
					rootdirty = true;
				}
			}
			else
			{
				remove = false;
				if (dd_CheckBlock(rbl->deldir, 0))
				{
					if ((error = ResBlockUsed(rbl->deldir)))
						remove = true;
				}
				else
					remove = true;

				if (remove && mode == repair)
				{
					fixederror("deldir block removed\n");
					rbl->deldir = 0;
					rbl->options &= ~MODE_DELDIR;
					rootdirty = true;
				}
			}

		}
	}
	else	/* deldir disabled */
	{
		if (rext.data->deldirsize)
		{
			if (mode == check)
				adderror("deldir size incorrect");
			else
			{
				fixederror("deldir size incorrect");
				rext.data->deldirsize = 0;
				rextdirty = true;
			}
		}

		if (mode == repair)
		{
			for (i=0; i<32; i++)
			{
				if (rext.data->deldir[i])
				{
					fixederror("deldir block removed");
					rext.data->deldir[i] = 0;
					rextdirty = true;
				}
			}
		}

		if (rbl->deldir)
		{
			if (mode == check)
				adderror("illegal deldir reference");
			else
			{
				fixederror("deldir block removed");
				rbl->deldir = 0;
				rootdirty = true;
			}
		}
	}

	if (rootdirty)
		c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
	if (rextdirty)
		volume.writeblock((cachedblock_t *)&rext);

	return e_none;
}


static bool dd_CheckBlock(uint32 bloknr, int seqnr)
{
	error_t error;
	c_deldirblock_t ddblk;
	int i, ddnr;
	int32 blocks;
	canode_t delnode;
	struct deldirentry *dde;
	ULONG size, high;

	volume.progress(0, 1);
	
	/* get block */
	ddblk.data = calloc(1, SIZEOF_RESBLOCK);
	if ((error = volume.getblock((cachedblock_t *)&ddblk, bloknr)))
	{
		adderror("couldn't read deldirblock");
		free (ddblk.data);
		return false;
	}

	/* check id */
	if (ddblk.data->id != DELDIRID)
	{
		adderror("deldirblock error");
		free (ddblk.data);
		return false;
	}

	if (ddblk.data->seqnr != seqnr)
	{
		adderror("deldirblock error");
		free (ddblk.data);
		return false;
	}

	// check entries
	for (i=0; i<31; i++)
	{
		blocks = 0;
		dde = &ddblk.data->entries[i];
		if (!dde->anodenr)
			continue;

		/* syntax fix */
		if (ss.stage == syntax)
		{
			for (ddnr = dde->anodenr; ddnr; ddnr = delnode.next)
			{
				if (!GetAnode (&delnode, ddnr, false) || IsAnodeUsed(ddnr))
				{
					if (mode == check)
						adderror("delfile anode error");
					else
					{
						fixederror("delfile anode error");
						dde->anodenr = 0;
						volume.writeblock((cachedblock_t *)&ddblk);
					}
					break;
				}

				blocks += (uint32)(delnode.clustersize);
			}
			
			size = GetDDFileSize(dde, &high);
			if (!FileSizeCheck(size, high, blocks))
			{
				sprintf(bericht, "delfile anode %#lx error", dde->anodenr);
				if (mode == check)
					adderror(bericht);
				else
				{
					fixederror(bericht);
					dde->anodenr = 0;
					volume.writeblock((cachedblock_t *)&ddblk);
				}
			}
		}

		if (!dde->anodenr)
			continue;

		for (ddnr = dde->anodenr; ddnr; ddnr = delnode.next)
		{
			GetAnode(&delnode, ddnr, false);
			/* mark anodes as used */
			if (ss.stage <= anodebitmap)
				AnodeUsed(ddnr);

			/* for dde only the anodes need to be checked */
			// if (ss.stage == doubledel && isAnodeDouble(ddnr))
			//	dde->anodenr = 0;
		}
	}

	free(ddblk.data);
	return true;
}


/**************************************
 * Directory tree checker			L2
 **************************************/

struct dirstack
{
	struct dirstack *parent;
	char 	name[FNSIZE];
	uint32	anodenr;
	char	*objecttype;
};

#define pushdirstack(item) \
	(item)->parent = currentobject; \
	currentobject = (item)

#define popdirstack() \
	currentobject = currentobject->parent

struct dirstack *currentobject = NULL;
char objectname[1024] = { 0 };
char dirtype[] = "DIR ";
char filetype[] = "FILE ";
char hardlinktype[] = "HARDLINK ";
char softlinktype[] = "SOFTLINK ";
char rollovertype[] = "ROLLOVER ";
char unknowntype[] = "OBJECT ";

static void GetObjectName(void)
{
	struct dirstack *n, *ds;
	char *b = objectname;

	objectname[0] = 0;
	if (!currentobject)
		return;
	ds = NULL;
	while (ds != currentobject)
	{
		for (n = currentobject; n->parent != ds; n=n->parent);
		if (n->parent && n->parent->parent)
			b = stpcpy(b, "/");
		b = stpcpy(b, n->name);
		ds = n;
	}
}

static void fixedfileerror(char *errortxt)
{
	if (currentobject)
	{
		GetObjectName();
		volume.showmsg(currentobject->objecttype);
		volume.showmsg(objectname);
		volume.showmsg("\n");
	}
	if (mode == check)
		adderror(errortxt);
	else
		fixederror(errortxt);
}

static void addfileerror(char *errortxt)
{
	if (currentobject)
	{
		GetObjectName();
		volume.showmsg(currentobject->objecttype);
		volume.showmsg(objectname);
		volume.showmsg("\n");
	}
	adderror(errortxt);
}

/* Check directory tree. Track bitmap if needed.
 * Returns fatal error (0 = e_none. continue and errors fixed)
 */
static error_t RepairDirTree(void)
{
	struct dirstack stackbottom = {
		NULL, "/", ANODE_ROOTDIR, dirtype };
	error_t error;

	volume.status(0, "root", 256);
	volume.showmsg("Starting directory check\n");
	pushdirstack(&stackbottom);
	error = RepairDir(NULL, NULL);	
	popdirstack();
	return error;
}


static error_t RepairDir(struct direntry *de, c_dirblock_t *parent)
{
	uint32 mainanodenr, anodenr, parentanodenr;
	canode_t dirnode;
	error_t error = e_none;

	stats.numdirs++;
	if (de)
	{
		volume.status(0, currentobject->name, 0);
		if (ss.verbose)
		{
			volume.showmsg("DIR ");
			volume.showmsg(currentobject->name);
			volume.showmsg("\n");
		}

		/* repair linkchain, remove if necessary */
		RepairLinkChain(de, parent);
	}

	parentanodenr = parent ? parent->data->anodenr : 0;
	mainanodenr = de ? de->anode : ANODE_ROOTDIR;
	for (anodenr = mainanodenr; anodenr; anodenr=dirnode.next)
	{
		/* get anode */
		if (!GetAnode(&dirnode, anodenr, true))
			return e_remove;

		/* check anode */
		if (dirnode.clustersize != 1)
		{
			fixedfileerror("directory anode has illegal clustersize");
			dirnode.clustersize = 1;
			SaveAnode(&dirnode, anodenr);
		}

		enterblock(dirnode.blocknr);
		error = RepairDirBlock(dirnode.blocknr, mainanodenr, parentanodenr);
		exitblock();
		switch (error)
		{
			case e_empty:
				
				// BUG: an empty leading dirblock cannot be freed like
				// that, the anodes have to moved!
				//
				// if (!dirnode.next)
				// {
					ResBlockUsed(dirnode.blocknr);
					AnodeUsed(anodenr);
					error = e_none;
					break;
				// }
				//
				// fixedfileerror("empty directory block");
				
			case e_remove:

				if (error != e_empty)
				{
					if (mode == repair)
						fixedfileerror("removing corrupt directory block");
					else
						addfileerror("corrupt directory block");
				}
				
				DeleteAnode(mainanodenr, anodenr);
				break;

			case e_none:

				ResBlockUsed(dirnode.blocknr);
				AnodeUsed(anodenr);
				break;

			default:
			
				KillReservedBitmap();
		}
		
		if ((error != e_none) || aborting)
			return error;
	}

	return e_none;
}

static error_t RepairDirBlock(uint32 bloknr, uint32 anodenr, uint32 parent)
{
	c_dirblock_t dirblk;
	struct direntry *entry;
	error_t error;

	/* get block */
	dirblk.data = calloc(1, SIZEOF_RESBLOCK);
	if ((error = volume.getblock((cachedblock_t *)&dirblk, bloknr)))
	{
		free (dirblk.data);
		return e_remove;
	}

	/* check dirblock */
	if (dirblk.data->id != DBLKID || dirblk.data->anodenr != anodenr ||
		dirblk.data->parent != parent)
	{
		free (dirblk.data);
		return e_remove;
	}
	
	entry = (struct direntry *)dirblk.data->entries;

	while (entry->next)
	{
		if ((error = RepairDirEntry(entry, &dirblk)))
		{
			if (mode == repair)
			{
				redosyntax = true;
				fixedfileerror("removing corrupt directory entry");
				RemoveDirEntry(entry, &dirblk);
			}
			else
			{
				addfileerror("corrupt directory entry");
				entry = NEXTENTRY(entry);
			}
		}
		else
			entry = NEXTENTRY(entry);
	}

	/* has to be done after main check, because an empty block can
     * arise
	 */
	error = e_none;
	if (mode == repair)
	{
		entry = (struct direntry *)dirblk.data->entries;
		if (!entry->next)
			error = e_empty;
	}

	free (dirblk.data);
	return error;
}

static error_t RepairDirEntry(struct direntry *de, c_dirblock_t *dirblk)
{
	struct dirstack dirstack;
	error_t error = e_none;

	volume.progress(0, 1);
	if (!de || !de->next)
		return e_empty;

	if (ss.stage == syntax)
	{
		if (de->next & 1)
		{
			addfileerror("odd directory entry length");
			return e_direntry_error;
		}

		if (de->nlength + offsetof(struct direntry, nlength) > de->next)
		{
			addfileerror("invalid filename");
			return e_direntry_error;
		}

		if (de->nlength > volume.fnsize)
		{
			addfileerror("filename too long");
			return e_direntry_error;
		}
	
		if (*FILENOTE(de) + de->nlength + offsetof(struct direntry, nlength) > de->next)
		{
			addfileerror("invalid filenote");
			return e_direntry_error;
		}
	}

	pushdirstack(&dirstack);
	strncpy(dirstack.name, (char *)&de->startofname, de->nlength);
	dirstack.name[de->nlength] = 0;
	dirstack.anodenr = de->anode;
	dirstack.objecttype = unknowntype;

	switch (de->type)
	{
		case ST_USERDIR:

			dirstack.objecttype = dirtype;
			error = RepairDir(de, dirblk);
			break;

		case ST_SOFTLINK:
	
			dirstack.objecttype = softlinktype;
			error = RepairSoftLink(de, dirblk);
			break;

		case ST_LINKFILE:
		case ST_LINKDIR:

			dirstack.objecttype = hardlinktype;
			error = RepairHardLink(de, dirblk);
			if (error == e_remove)
				redosyntax = true;
			break;

		case ST_ROLLOVERFILE:

			dirstack.objecttype = rollovertype;
			error = RepairRollover(de, dirblk);
			break;

		case ST_FILE:

			dirstack.objecttype = filetype;
			error = RepairFile(de, dirblk);
			break;

		case ST_PIPEFILE:
		case ST_ROOT:
		default:

			addfileerror("invalid filetype");
			error = e_direntry_error;
			break;
	}

	popdirstack();
	return error;
}

static error_t RepairFile(struct direntry *de, c_dirblock_t *parent)
{
	int32 blokken;
	struct extrafields extra;
	canode_t filenode;
	error_t error;
	uint32 anodenr, bl;
	ULONG size, high;

	stats.numfiles++;
	if ((error = GetExtraFields(&extra, de)))
		return error;

	/* check anode chain */
	if (ss.stage == syntax)
	{
		if (de->type != ST_ROLLOVERFILE && (extra.virtualsize || extra.rollpointer))
		{
			extra.virtualsize = 0;
			extra.rollpointer = 0;
			SetExtraFields(&extra, de, parent);
			fixedfileerror("dangling rollover");
		}
		
		blokken = 0;
		for (anodenr = de->anode; anodenr; anodenr = filenode.next)
		{
			if ((error = CheckAnode(&filenode, anodenr, true)))
				return e_remove;

			blokken += filenode.clustersize;
		}

		size = GetDEFileSize(de, &extra, &high);
		if (!FileSizeCheck(size, high, blokken))
		{
			addfileerror("invalid filesize");
			return e_remove;
		}
	}

	if (extra.link)
		RepairLinkChain(de, parent);

	/* bitmap gen */
	for (anodenr = de->anode; anodenr; anodenr = filenode.next)
	{
		GetAnode(&filenode, anodenr, true);
		if ((error = AnodeUsed(anodenr)))
			break;

		for (bl = filenode.blocknr; bl < filenode.blocknr + filenode.clustersize; bl++)
			if ((error = MainBlockUsed(bl)))
				break;
	}

	if (error != e_none)
	{
		/* invalidate bitmaps to force generation in next stage. */
		InvalidBitmap(volume.mainbitmap);
		InvalidBitmap(volume.anodebitmap);
		return error;
	}

	return e_none;	
}

/* Repair hardlink. If a hardlink is removed that has consequences for
 * the linkchain and the linked to file.
 */
static error_t RepairHardLink(struct direntry *de, c_dirblock_t *dirblok)
{
	struct extrafields extra;
	canode_t linknode;
	error_t error;

	stats.numhardlink++;
	if ((error = GetExtraFields(&extra, de)))
		return e_remove;

	if (!extra.link)
	{
		addfileerror("invalid hardlink");
		return e_remove;
	}

	/* get linknode */
	if (!GetAnode (&linknode, de->anode, true))
		return e_remove;

	/* check linknode linkdir (anode.blocknr) reference (objectdir is
	 * checked by CheckLinkChain) */
	if (linknode.blocknr != dirblok->data->anodenr)
	{
		addfileerror("invalid linknode directory reference");
		return e_remove;
	}

	/* Check object reference 
	 * Find anodenr in objectdir, with problem of possible
     * corruption. Loose directories are not detected.
	 * It is not checked if the link itself is infact a link, 
     * just that it exists.
	 */
	switch (SearchInDir(linknode.clustersize, extra.link))
	{
		case 0:
			addfileerror("dangling hardlink");
			return e_remove;

		case ST_FILE:
		case ST_ROLLOVERFILE:
		case ST_SOFTLINK:
		case ST_LINKFILE:
			if (de->type == ST_LINKDIR)
			{
				fixedfileerror("invalid hardlink");
				de->type = ST_LINKFILE;
				volume.writeblock((cachedblock_t *)dirblok);
			}
			break;

		default:
			if (de->type == ST_LINKFILE)
			{
				fixedfileerror("invalid hardlink");
				de->type = ST_LINKDIR;
				volume.writeblock((cachedblock_t *)dirblok);
			}
			break;
	}

	if ((error = AnodeUsed(de->anode)))
		return error;

	return e_none;
}


static error_t RepairSoftLink(struct direntry *de, c_dirblock_t *dirblok)
{
	stats.numsoftlink++;
	if (de->fsize > 108)
		return e_invalid_softlink;

	return RepairFile(de, dirblok);
}

static error_t RepairRollover(struct direntry *de, c_dirblock_t *dirblok)
{
	struct extrafields extra;
	error_t error;

	stats.numrollover++;
	if ((error = GetExtraFields(&extra, de)))
		return error;

	if (extra.virtualsize > de->fsize ||
		extra.rollpointer > de->fsize)
	{
		fixedfileerror("invalid rollover fields");
		extra.virtualsize = 0;
		extra.rollpointer = 0;
		SetExtraFields(&extra, de, dirblok);
	}

	return RepairFile(de, dirblok);
}


/*
 * Helper functions
 */

static error_t RemoveDirEntry(struct direntry *de, c_dirblock_t *dirblk)
{
	uint8 *dest, *source;
	uint32 movelen;

	dest = (uint8 *)de;
	source = dest + de->next;
	movelen = ((uint8 *)(dirblk->data) + SIZEOF_RESBLOCK) - dest;
	if (movelen > 0 && movelen < SIZEOF_RESBLOCK)
	{
		memmove(dest, source, movelen);
		volume.writeblock((cachedblock_t *)dirblk);
	}
	else
		return e_fatal_error;

	return e_none;
}


/* remove node from anodechain
 */
static error_t DeleteAnode(uint32 anodechain, uint32 node)
{
	canode_t prev_node, next_node;
	uint32 previous;

	/* node is head */
	if (anodechain == node)
	{
		if (!GetAnode(&next_node, node, false))
			return e_anode_error;

		if (!next_node.next || !GetAnode(&next_node, next_node.next, false))
			return e_anode_error;

		SaveAnode(&next_node, node);
	}
	else
	{
		/* find previous */
		previous = anodechain;
		while (true)
		{
			if (!GetAnode(&prev_node, previous, false))
				return e_anode_error;

			if (prev_node.next == node) 
				break;		// we found it

			previous = prev_node.next;
		}

		GetAnode(&next_node, prev_node.next, false);
		prev_node.next = next_node.next;
		SaveAnode(&prev_node, prev_node.nr);
	}
	
	return e_none;
}

/* Get the directory entry extension fields
 */
static error_t GetExtraFields(struct extrafields *extrafields, struct direntry *de)
{
	uint16 *extra = (uint16 *)extrafields;
	uint16 *fields = (uint16 *)(((uint8 *)de)+de->next);
	uint16 flags, i;

	// extract extrafields from directoryentry
	flags = *(--fields);
	for (i=0; i < sizeof(struct extrafields)/2; i++, flags>>=1)
		*(extra++) = (flags&1) ? *(--fields) : 0;

	// check flags field
	if (flags)
		addfileerror("unknown extrafield flags found (ignored)");

	// patch protection lower 8 bits
	extrafields->prot |= de->protection;
	return e_none;
}

/* Set extrafields. Requirement is that the new directory entry fits.
 * To make sure it does, extrafields are removed only, not added.
 */
static error_t SetExtraFields(struct extrafields *extrafields, struct direntry *from,
	 c_dirblock_t *dirblk)
{
	uint8 entrybuffer[MAX_ENTRYSIZE];
	struct direntry *to;
	uint8 *dest, *start, *end;
	uint32 movelen;
	int diff;

	to = (struct direntry *)entrybuffer;
	memcpy(to, from, from->next);
	AddExtraFields(to, extrafields);

	/* make room for new direntry */
	diff = to->next - from->next;
	dest = (uint8 *)from + to->next;
	start = (uint8 *)from + from->next;
	end = (uint8 *)dirblk->data + SIZEOF_RESBLOCK;
	movelen = (diff > 0) ? (end - dest) : (end - start);
	memmove(dest, start, movelen);

	/* add new direntry */
	memcpy((uint8 *)from, to, to->next);

	return volume.writeblock((cachedblock_t *)dirblk);
}

static void AddExtraFields(struct direntry *direntry, struct extrafields *extra)
{
	UWORD offset, *dirext;
	UWORD array[16], i = 0, j = 0;
	UWORD flags = 0, orvalue;
	UWORD *fields = (UWORD *)extra;

	/* patch protection lower 8 bits */
	extra->prot &= 0xffffff00;
	offset = (sizeof(struct direntry) + (direntry->nlength) + *FILENOTE(direntry)) & 0xfffe;
	dirext = (UWORD *)((UBYTE *)(direntry) + (UBYTE)offset);

	orvalue = 1;
	/* fill packed field array */
	for (i = 0; i < sizeof(struct extrafields) / 2; i++)
	{
		if (*fields)
		{
			array[j++] = *fields++;
			flags |= orvalue;
		}
		else
		{
			fields++;
		}

		orvalue <<= 1;
	}

	/* add fields to direntry */
	i = j;
	while (i)
		*dirext++ = array[--i];
	*dirext++ = flags;

	direntry->next = offset + 2 * j + 2;
}


/* Repair the hardlinks chain referring to a file
 */
static error_t RepairLinkChain(struct direntry *de, c_dirblock_t *dirblk)
{
	struct extrafields extra;
	canode_t linknode;
	error_t error;
	uint32 parent;

	/* check extra fields */
	if ((error = GetExtraFields(&extra, de)))
		return error;

	/* check if object has a linkchain */
	if (!extra.link)
		return e_none;

	if (!GetAnode(&linknode, extra.link, false))
		return e_anode_error;

	parent = dirblk->data->anodenr;
	for (;;)
	{
		// check objectdir (clustersize) against actual objectdir
		if (linknode.clustersize != parent)
		{
			fixedfileerror("invalid hardlink");
			linknode.clustersize = parent;
			SaveAnode(&linknode, linknode.nr);
		}

		// check linkdir left to HardLink::Check
		if (!SearchInDir(linknode.blocknr, linknode.nr))
		{
			fixedfileerror("dangling hardlink");
			if ((error = DeleteAnode(extra.link, linknode.nr))
				 && (linknode.nr == extra.link))
			{
				extra.link = 0;
				SetExtraFields(&extra, de, dirblk);
			}
		}

		if (!linknode.next)
			return e_none;

		if (!GetAnode (&linknode, linknode.next, false))
			return e_anode_error;
	}
}

/* Get & check an anode
 */
static error_t CheckAnode(canode_t *anode, uint32 nr, bool fix)
{
	/* if an anodeblock does not exist, it is considered empty */
	if (!GetAnode(anode, nr, fix))
		return e_empty;

	if (anode->clustersize == 0 && anode->blocknr == 0 && anode->next == 0)
		return e_empty;

	return e_none;
}

/* Search file by anodenr in a directory
 * return filetype or 0 for not found
 */
int SearchInDir(uint32 diranodenr, uint32 target)
{
	canode_t anode;
	c_dirblock_t dirblk;
	struct direntry *de;
	error_t error = e_none;
	int ftype;

	/* Get directory and find object */
	dirblk.data = calloc(1, SIZEOF_RESBLOCK);
	while (error == e_none && diranodenr)
	{
		if (!GetAnode(&anode, diranodenr, false))
			break;
		
		if ((error = volume.getblock((cachedblock_t *)&dirblk, anode.blocknr)) || aborting)
			break;

		de = FIRSTENTRY(&dirblk);
		while (de->next)
		{
			if (de->anode == target)
			{
				ftype = de->type;
				free (dirblk.data);
				return ftype;
			}

			de = NEXTENTRY(de);
		}

		diranodenr = anode.next;
	}

	free (dirblk.data);
	return 0;
}

/**************************************
 * Anode tree checker				L2 
 **************************************/

/* Check anode tree.
 * Returns fatal errors (e_none = continue)
 */
static error_t RepairAnodeTree(void)
{
	int i;
	error_t error;
	uint32 child_blk_nr;

	volume.showmsg("Checking anodetree\n");
	if (rbl->options & MODE_SUPERINDEX)
	{
		volume.status(0, "Anodetree", 100);

		for (i=0; i<MAXSUPER+1; i++)
		{
			if ((child_blk_nr = rext.data->superindex[i]))
			{
				if ((error = RepairSuperIndex(&rext.data->superindex[i], i)) || aborting)
					return error;

				if (rext.data->superindex[i] != child_blk_nr)
					volume.writeblock((cachedblock_t *)&rext);
			}
		}
	}
	else
	{
		volume.status(0, "Anodetree", 100);

		for (i=0; i<MAXSMALLINDEXNR+1; i++)
		{
			if ((child_blk_nr = rbl->idx.small.indexblocks[i]))
			{
				if ((error = RepairAnodeIndex(&rbl->idx.small.indexblocks[i], i)) || aborting)
					return error;

				if (rbl->idx.small.indexblocks[i] != child_blk_nr)
					c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
			}
		}
	}

	return e_none;
}

error_t RepairSuperIndex(uint32 *bloknr, uint32 seqnr)
{
	return RepairIndexBlock(SBLKID, RepairAnodeIndex, bloknr, seqnr);
}

error_t RepairAnodeIndex(uint32 *bloknr, uint32 seqnr)
{
	volume.progress(0, 0);	// reset progress bar
	return RepairIndexBlock(IBLKID, RepairAnodeBlock, bloknr, seqnr);
}

/* missing: check of first, reserved anodes.
 * if blocknr is 0 --> search and build
 */
error_t RepairAnodeBlock(uint32 *bloknr, uint32 seqnr)
{
	error_t error = e_none;
	c_anodeblock_t ablk;
	uint32 oldbloknr;

	// get anodeblock
	ablk.data = calloc(1, SIZEOF_RESBLOCK);
	if (*bloknr)
	{
		if ((error = volume.getblock((cachedblock_t *)&ablk, *bloknr)))
		{
			free (ablk.data);
			return error;
		}

		enterblock(*bloknr);
	}

	// check syntax block
	if (ss.stage == syntax)
	{
		if (ablk.data->id != ABLKID || ablk.data->seqnr != seqnr)
		{
			if (*bloknr)
			{
				sprintf(bericht, "anodeblock seqnr %lu blocknr %lu has an incorrect id", seqnr, *bloknr);
				adderror(bericht);
			}

			if (mode == repair)
			{
				volume.status(1, "Searching anode block", 256);
				oldbloknr = *bloknr;
				if ((*bloknr = SearchBlock(ABLKID, seqnr, oldbloknr, 0, 0, 0)))
				{
					sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, *bloknr);
					fixederror(bericht);
				}
				else
				{
					/* remove corrupt block */
					fixederror("no replacement found - removing block");
					free (ablk.data);
					exitblock();
					return e_remove;
				}
			}
		}
	}

	ResBlockUsed(*bloknr);
	free (ablk.data);
	exitblock();
	return error;
}

/**************************************
 * Bitmap tree checker				L2 
 **************************************/

/* Check bitmap tree.
 * Returns fatal errors (e_none = continue)
 */
static error_t RepairBitmapTree(void)
{
	error_t error = e_none;
	uint32 i, num;
	uint32 blknr;

	volume.status(0, "Bitmap", 100);
	volume.showmsg("Checking bitmap\n");

	num = ((volume.lastblock - volume.firstblock - rbl->lastreserved - 1) + 31) / 32;
	num = (num + INDEX_PER_BLOCK*INDEX_PER_BLOCK - 1)/(INDEX_PER_BLOCK * INDEX_PER_BLOCK);
	for (i=0; i < ((rbl->options & MODE_SUPERINDEX) ? (MAXBITMAPINDEX + 1) : (MAXSMALLBITMAPINDEX + 1)); i++)
	{
		blknr = rbl->idx.large.bitmapindex[i];
		if (i < num)
		{
			/* force bitmapindexblock to be searched and created */
			if (!blknr)
				rbl->idx.large.bitmapindex[i] = rbl->lastreserved;

			if ((error = RepairBitmapIndex(&rbl->idx.large.bitmapindex[i], i)) || aborting)
				return error;
			
			if (rbl->idx.large.bitmapindex[i] != blknr)
				c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
		}
	}

	return e_none;
}


/* change blocknr if necessary
 */
error_t RepairBitmapIndex(uint32 *bloknr, uint32 seqnr)
{
	volume.progress(0, 0);	// reset progress bar
	return RepairIndexBlock(BMIBLKID, RepairBitmapBlock, bloknr, seqnr);
}

error_t RepairBitmapBlock(uint32 *bloknr, uint32 seqnr)
{
	error_t error = e_none;
	c_bitmapblock_t bmblk;
	uint32 oldbloknr;

	// get anodeblock
	bmblk.data = calloc(1, SIZEOF_RESBLOCK);
	if (GetBuildBlock(BMBLKID, seqnr))
		return e_none;

	if (*bloknr)
	{
		if ((error = volume.getblock((cachedblock_t *)&bmblk, *bloknr)))
		{
			free (bmblk.data);
			return error;
		}
	}

	enterblock(*bloknr);

	// check syntax block
	if (ss.stage == syntax)
	{
		if (bmblk.data->id != BMBLKID || bmblk.data->seqnr != seqnr)
		{
			if (*bloknr)
			{
				sprintf(bericht, "bitmapblock seqnr %lu blocknr %lu has an incorrect id", seqnr, *bloknr);
				adderror(bericht);
			}

			if (mode == repair)
			{
				volume.status(1, "Searching bitmap block", 256);
				oldbloknr = *bloknr;
				if ((*bloknr = SearchBlock(BMBLKID, seqnr, oldbloknr, 0, 0, 0)))
				{
					sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, *bloknr);
					fixederror(bericht);
				}
				else
				{
					volume.showmsg("No replacement block found\n");
					free (bmblk.data);
					error = BuildBitmapBlock(&bmblk, seqnr);
					exitblock();
					if (error)
						adderror("building new bitmapblock failed");
					else
						fixederror("new bitmapblock created");
					return error;
				}
			}
		}
	}

	ResBlockUsed(*bloknr);
	free (bmblk.data);
	exitblock();
	return error;
}

/**************************************
 * Check index blocks.
 * Returns fatal errors (e_none = continue)
 */
error_t RepairIndexBlock(uint16 bloktype, error_t (*repairchild)(uint32 *, uint32),
	uint32 *bloknr, uint32 seqnr)
{
	error_t error = e_none;
	c_indexblock_t iblk = { 0 };
	cachedblock_t *cblk;
	int i;
	uint32 child_blk_nr, oldbloknr;

	enterblock(*bloknr);

	if ((cblk = GetBuildBlock(bloktype, seqnr)))
		iblk = *(c_indexblock_t *)cblk;

	if (!iblk.data)
	{
		iblk.data = calloc(1, SIZEOF_RESBLOCK);

		/* read or build indexblock
		 */
		if ((error = volume.getblock((cachedblock_t *)&iblk, *bloknr)))
		{
			free (iblk.data);
			return error;
		}

		/* check id */
		if (ss.stage == syntax)
		{
			if (iblk.data->id != bloktype || iblk.data->seqnr != seqnr)
			{
				if (*bloknr)
				{
					sprintf(bericht,
						"indexblock %lu blocknr %lu type %x has incorrect id of %x, %lu",
						seqnr, *bloknr,	bloktype, iblk.data->id, iblk.data->seqnr);
					adderror(bericht);
				}

				if (mode == repair)
				{
					volume.status(1, "Searching index block", 256);
					oldbloknr = *bloknr;
					if ((*bloknr = SearchBlock(bloktype, seqnr, oldbloknr, 0, 0, 0)))
					{
						sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, *bloknr);
						fixederror(bericht);
						free (iblk.data);
						return RepairIndexBlock(bloktype, repairchild, bloknr, seqnr);
					}
					else
					{
						volume.showmsg("No replacement block found\n");
						free (iblk.data);
						if ((error = BuildIndexBlock(&iblk, bloktype, seqnr)))
						{
							adderror("building new indexblock failed");
							exitblock();
							return error;
						}
						fixederror("new indexblock created");
					}
				}
				else
				{
					/* check mode failure */
					ResBlockUsed(*bloknr);
					free(iblk.data);
					return e_none;
				}
			}
		}
	}

	for (i=0; i<LONGS_PER_BMB; i++)
	{
		if ((child_blk_nr = iblk.data->index[i]))
		{
			volume.progress(0, 1);
			switch (repairchild((uint32 *)&iblk.data->index[i], seqnr*LONGS_PER_BMB + i))
			{
				case e_none:
					break;

				default:
					/* kill block */
					iblk.data->index[i] = 0;
					break;
			}

			if (aborting)
				return error;

			/* save changes */
			if (child_blk_nr != iblk.data->index[i] && iblk.mode == check)
				volume.writeblock((cachedblock_t *)&iblk);
		}
	}
	
	exitblock();
	if (iblk.mode == check)
	{
		/* not necessary (allowed) for generated blocks */
		ResBlockUsed(*bloknr);
		free (iblk.data);
	}
	return error;
}


/**************************************
 * Bitmap Generator					L3
 **************************************/

// public data

// enable flags

// public functions

static error_t InitReservedBitmap(void)
{
	uint32 reservedsize, bloknr, cluster;

	KillReservedBitmap();
	if (rbl->firstreserved != 2)
	{
		fixederror("illegal firstreserved");
		rbl->firstreserved = 2;
	}

	// reservedsize cannot be smaller then 0 and is not allowed to take more than
    // 1/8th of the partition.
	volume.lastreserved = rbl->lastreserved;
	reservedsize = rbl->lastreserved/volume.rescluster - rbl->firstreserved + 1;

	if (reservedsize <=0 || reservedsize > volume.disksize/8)
	{
		sprintf(bericht, "illegal reserved area size of %lu blocks", reservedsize);
		adderror(bericht);
		
		if (!(bloknr = SearchLastReserved(&volume)))
			return e_fatal_error;

		fixederror(NULL);
		rbl->lastreserved = volume.lastreserved = bloknr;
	}

	if ((rext.data->reserved_roving + 31)/32 > reservedsize)
	{
		rext.data->reserved_roving = 0;
		volume.writeblock((cachedblock_t *)&rext);
	}

	cluster = 1 + ((reservedsize > 125*32) ? (reservedsize - 125*32 + 256*32 - 1)/(256*32) : 0);
	// cluster is rounded up to closest reserved blocksize
	cluster = (((cluster + rbl->reserved_blksize / 1024 - 1) & ~(rbl->reserved_blksize / 1024 - 1)) * 1024)>>volume.blockshift;
	if (rbl->rblkcluster != cluster)
	{
		rbl->rblkcluster = cluster;
		if (mode == check)
		fixederror("wrong rootblock cluster size");
	}
	
	volume.resbitmap = bg_InitBitmap(rbl->firstreserved, rbl->lastreserved, volume.rescluster);
	if (!volume.resbitmap)
		return e_fatal_error;

	/* mark rootblock cluster as used */
	for (bloknr = 2; bloknr < 2+cluster; bloknr += volume.rescluster)
		ResBlockUsed(bloknr);

	return e_none;
}

/* assumes valid anodetree syntax
 */
static error_t InitAnodeBitmap(void)
{
	c_indexblock_t iblk;
	int i, s;
	error_t error;

	KillAnodeBitmap();
	iblk.data = calloc(1, SIZEOF_RESBLOCK);

	/* determine number of anodebitmap blocks
	 * If any anode or indexblocks are generated, the anodebitmap has to be
     * destroyed and regenerated.
	 */
	if (rbl->options & MODE_SUPERINDEX)
	{
		for (s=MAXSUPER; !rext.data->superindex[s] && s; s--);

		if ((error = GetResBlock((cachedblock_t *)&iblk, SBLKID, s, false)))
		{
			free (iblk.data);
			return error;
		}

		for (i=INDEX_PER_BLOCK - 1; !iblk.data->index[i]; i--);
	}
	else
	{
		for (s=0, i=MAXSMALLINDEXNR; !rbl->idx.small.indexblocks[i] && i; i--);
	}

	s = s*INDEX_PER_BLOCK*INDEX_PER_BLOCK*ANODES_PER_BLOCK;
	s += (++i * INDEX_PER_BLOCK * ANODES_PER_BLOCK) - 1;
	volume.anodebitmap = bg_InitBitmap(0, s, 1);
	for (i=0; i<ANODE_ROOTDIR; i++)
		AnodeUsed(i);
	
	free (iblk.data);
	return e_none;
}

/* Initialise generation of main bitmap
 */
static error_t InitMainBitmap(void)
{
	KillMainBitmap();
	volume.mainbitmap = bg_InitBitmap(rbl->lastreserved+1, 
			volume.lastblock-volume.firstblock, 1);

	if (!volume.mainbitmap)
		return e_fatal_error;
	else
		return e_none;
}

void KillAnodeBitmap(void)
{
	bg_KillBitmap(&volume.anodebitmap);
}

void KillMainBitmap(void)
{
	bg_KillBitmap(&volume.mainbitmap);
}

void KillReservedBitmap(void)
{
	bg_KillBitmap(&volume.resbitmap);
}

/* Compare generated reserved bitmap with the actual bitmap
 * and fix differences.
 * Pre:
 *	- syntax disk is correct
 *	- rootblock read
 *	- reserved bitmap generated AND valid
 */
static error_t RepairReservedBitmap(void)
{
	rootblock_t *lrb;
	bitmapblock_t *bitmap;
	uint32 i, j, mask, blknr, blocksfree = 0;
	bool dirty = false;
	error_t error;
	uint32 nuba=0, ubna=0;
	uint8 *t;

	volume.status(0, "validating reserved", volume.resbitmap->lwsize/0xff);

	/* first save current rootblock */
	if ((error = c_WriteBlock ((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize)))
		return error;		
	
	/* now get reserved bitmap with rootblock in memory */
	if (!(lrb = (rootblock_t *)AllocBufMem(volume.blocksize * rbl->rblkcluster)))
		return e_out_of_memory;

	t = (uint8 *)lrb;
	for (i=0; i<rbl->rblkcluster; i++)
	{
		if ((error = c_GetBlock (t, ROOTBLOCK + volume.firstblock + i, volume.blocksize)))
		{
			adderror("reserved bitmap could not be loaded");
			FreeBufMem(lrb);
			return error;
		}
		t += volume.blocksize;
	}

	bitmap = (bitmapblock_t *)(lrb + 1);
	if (bitmap->id != BMBLKID)
	{
		/* this is also done at BUILD ROOTBLOCK */
		if (mode == check)
			adderror("reserved bitmap id is wrong");
		else
			fixederror("reserved bitmap id was wrong");

		memset(bitmap, 0, rbl->rblkcluster*volume.blocksize - sizeof(*lrb));
		bitmap->id = BMBLKID;
	}

	/* now we can check */
	for (i=0; i<volume.resbitmap->lwsize; i++)
	{
		if (!(i&0xff))
		{
			if (aborting)
				break;
			volume.progress(0, 1);
		}

		for (j=0; j<32; j++)
		{
			mask = 0x80000000 >> j;
			blknr = (i*32 + j)*volume.resbitmap->step + volume.resbitmap->start;

			if (blknr <= volume.resbitmap->stop)
			{
				if ((volume.resbitmap->map[i] & mask) !=
					(bitmap->bitmap[i] & mask))
				{
					if (volume.resbitmap->map[i] & mask)
					{
						nuba++;
						sprintf(bericht, "reserved block %lu not used but allocated", blknr);
						if (ss.verbose)
							fixederror(bericht);
						lrb->reserved_free++;
					}
					else
					{
						ubna++;
						sprintf(bericht, "reserved block %lu used but not allocated", blknr);
						if (ss.verbose)
							fixederror(bericht);
						lrb->reserved_free--;
					}
					
					bitmap->bitmap[i] ^= mask;
					dirty = true;
				}
					
				if (bitmap->bitmap[i] & mask)
					blocksfree++;
			}
		}
	}

	if (nuba > 0)
	{
		sprintf(bericht, "%lu reserved blocks not used but allocated", nuba);
		if (mode == check)
			fixederror(bericht);
	}

	if (ubna > 0)
	{
		sprintf(bericht, "%lu reserved blocks used but not allocated", ubna);
		if (mode == check)
			fixederror(bericht);
	}

	// check blocks free
	if (lrb->reserved_free != blocksfree && !aborting)
	{
		if (mode == check)
			fixederror("wrong number of reserved blocks free");
		lrb->reserved_free = blocksfree;
		dirty = true;
	}

	error = e_none;
	if (dirty && !aborting)
	{
		t = (uint8 *)lrb;
		for (i=0; i<lrb->rblkcluster; i++)
		{
			error = c_WriteBlock (t, ROOTBLOCK + volume.firstblock + i, volume.blocksize);
			t += volume.blocksize;
		}
		memcpy(rbl, lrb, SIZEOF_RESBLOCK);
	}
	
	FreeBufMem(lrb);
	return error;
}

/* Compare generated bitmap with actual bitmap and fix found
 * differences. Pre conditions:
 * - syntax disk correct
 * - main bitmap generated and VALID
 * - rootblock and rext loaded
 */
static error_t RepairMainBitmap(void)
{
	uint32 k=0, i, j, bmseqnr; 
	uint32 mask, blknr, blocksfree = 0;
	c_bitmapblock_t bmb;
	bitmap_t *gbm;
	error_t error = e_none;
	bool dirty;
	bool build = 0;
	uint32 nuba=0, ubna=0;

	if (ss.verbose)
		volume.showmsg("checking main bitmap\n");

	gbm = volume.mainbitmap;
	bmb.data = calloc(1, SIZEOF_RESBLOCK);
	volume.status(0, "validating main", gbm->lwsize/INDEX_PER_BLOCK);

	for (bmseqnr = 0; bmseqnr <= gbm->lwsize/INDEX_PER_BLOCK; bmseqnr++)
	{
		volume.progress(0, 1);

		if ((error = GetResBlock((cachedblock_t *)&bmb, BMBLKID, bmseqnr, true)) || aborting)
		{
			free (bmb.data);
			if ((error = BuildBitmapBlock(&bmb, bmseqnr)))
				return error;
			build = 1;
		}

		// check block
		for (i=0; i<INDEX_PER_BLOCK; i++, k++)
		{
			for (j=0; j<32; j++)
			{
				mask = 0x80000000 >> j;
				blknr = k*32 + j + gbm->start;

				if (blknr <= gbm->stop)
				{
					if ((bmb.data->bitmap[i] & mask) !=
						(gbm->map[k] & mask))
					{
						if (gbm->map[k] & mask)
						{
							nuba++;
							// sprintf(bericht, "block %d not used but allocated", blknr);
							// if (ss.verbose)
							//	fixederror(bericht);
							rbl->blocksfree++;
						}
						else
						{
							ubna++;
							// sprintf(bericht, "block %d used but not allocated", blknr);
							// if (ss.verbose)
							//	fixederror(bericht);
							rbl->blocksfree--;
						}

						bmb.data->bitmap[i] ^= mask;
						dirty = true;
						if ((error = volume.writeblock ((cachedblock_t *)&bmb)))
						{
							free (bmb.data);
							return error;
						}						
					}

					if (bmb.data->bitmap[i] & mask)
						blocksfree++;
				}
			}
		}
	}

	if (nuba > 0)
	{
		sprintf(bericht, "%lu data blocks not used but allocated", nuba);
		fixederror(bericht);
	}

	if (ubna > 0)
	{
		sprintf(bericht, "%lu data blocks used but not allocated", ubna);
		fixederror(bericht);
	}

	// check blocks free
	if (rbl->blocksfree != blocksfree)
	{
		fixederror("wrong number of blocks free");
		rbl->blocksfree = blocksfree;
		dirty = true;
	}

	if (dirty)
		error = c_WriteBlock ((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
	
	if (!build)
		free (bmb.data);
	return error;
}

/* Compare anodes with generated anode bitmap. Fix found
 * differences. Pre conditions:
 * - syntax disk correct
 * - anode bitmap generated and VALID
 * - rootblock and rext loaded
 */
static error_t RepairAnodeBitmap(void)
{
	uint32 i, anodenr;
	int anodeused;
	canode_t anode;
	bitmap_t *gbm = volume.anodebitmap;
	error_t error;
	uint32 nuba=0, ubna=0;

	if (ss.verbose)
		volume.showmsg("checking anode bitmap\n");
	volume.status(0, "validating anodes", (gbm->stop - gbm->start)/32);

	for (i=gbm->start; i<=gbm->stop; i++)
	{
		if (!(i%32))
		{
			if (aborting)
				break;
			volume.progress(0,1);
		}

		anodeused = !(gbm->map[i/32] & (0x80000000UL >> i%32));
		anodenr = (i/ANODES_PER_BLOCK << 16) + i%ANODES_PER_BLOCK;

		error = CheckAnode(&anode, anodenr, false);

		if ((error == e_empty) == anodeused)
		{
			if (anodeused)
			{
				if (ss.verbose)
				{
					sprintf(bericht, "anode %#lx used but not allocated", anodenr);
					fixederror(bericht);
				}

				ubna++;
				if (mode == repair)
				{
					anode.clustersize = 0;
					anode.blocknr     = 0xffffffff;
					anode.next        = 0;
					SaveAnode(&anode, anodenr);
				}
			}
			else
			{
				if (ss.verbose)
				{
					sprintf(bericht, "anode %#lx not used but allocated", anodenr);
					fixederror(bericht);
				}

				nuba++;
				if (mode == repair)
				{
					anode.clustersize =
					anode.blocknr     =
					anode.next        = 0;
					SaveAnode(&anode, anodenr);
				}
			}
		}
	}

	if (nuba > 0)
	{
		sprintf(bericht, "%lu anodes not used but allocated", nuba);
		fixederror(bericht);
	}

	if (ubna > 0)
	{
		sprintf(bericht, "%lu anodes used but not allocated", ubna);
		fixederror(bericht);
	}

	return e_none;
}

error_t ResBlockUsed(uint32 bloknr)
{
	error_t error = e_none;

	if (ss.stage <= resbitmap)
	{
		error = bg_ItemUsed(volume.resbitmap, bloknr);
		if (error == e_double_allocation)
		{
			sprintf(bericht, "reserved block %lu is double allocated", bloknr);
			adderror(bericht);
		}
	}
	return error;
}

static error_t MainBlockUsed(uint32 bloknr)
{
	error_t error = e_none;

	if (ss.stage <= mainbitmap)
	{
		error = bg_ItemUsed(volume.mainbitmap, bloknr);
		if (error == e_double_allocation)
		{
			sprintf(bericht, "block %lu is double allocated", bloknr);
			addfileerror(bericht);
		}
	}
	return error;
}

static error_t AnodeUsed(uint32 nr)
{
	error_t error = e_none;

	if (ss.stage <= anodebitmap)
	{
		nr = (nr >> 16) * ANODES_PER_BLOCK + (uint16)nr;
		error = bg_ItemUsed(volume.anodebitmap, nr);
		if (error == e_double_allocation)
		{
			sprintf(bericht, "anode %#lx is double allocated", nr);
			addfileerror(bericht);
		}
	}
	return error;
}

static BOOL IsAnodeUsed(uint32 nr)
{
	if (!nr) return FALSE;
	nr = (nr >> 16) * ANODES_PER_BLOCK + (uint16)nr;
	return bg_IsItemUsed(volume.anodebitmap, nr);
}

/* private functions
 */

/* initialise a bitmap for generation. The range is start to stop with
 * stepsize step
 */
static bitmap_t *bg_InitBitmap(int32 start, int32 stop, int32 step)
{
	bitmap_t *mybitmap;

	if (!(mybitmap = calloc(1, sizeof(bitmap_t))))
		return NULL;

	mybitmap->start = start;
	mybitmap->stop = stop;
	mybitmap->step = step;
	mybitmap->lwsize = (((stop-start+step)/step)+31)/32;

	if (!(mybitmap->map = malloc(4 * mybitmap->lwsize)))
	{
		free(mybitmap);
		volume.showmsg("DOCTOR ERROR: out of memory\n");
		return NULL;
	}

	memset(mybitmap->map, 0xff, mybitmap->lwsize*4);
	mybitmap->valid = true;
	return(mybitmap);
}

static void bg_KillBitmap(bitmap_t **bm)
{
	if (*bm)
	{
		if ((*bm)->map)
			free((*bm)->map);
		free(*bm);
	}

	*bm = NULL;
}

static error_t bg_ItemUsed(bitmap_t *bm, uint32 nr)
{
	uint32 index, bit;

	if (!bm)
		return e_none;

	if (nr < bm->start || nr > bm->stop)
		return e_outside_bitmap_error;

	nr -= bm->start;
	nr /= bm->step;
	index = nr/32;
	bit = nr%32;

	// check & set bit
	if (bm->map[index] & (0x80000000UL >> bit))
	{
		bm->map[index] ^= (0x80000000UL >> bit);
		return e_none;
	}
	else
	{
		return e_double_allocation;
	}
}

static BOOL bg_IsItemUsed(bitmap_t *bm, uint32 nr)
{
	uint32 index, bit;

	nr -= bm->start;
	nr /= bm->step;
	index = nr/32;
	bit = nr%32;

	if (bm->map[index] & (0x80000000UL >> bit))
		return FALSE;
	else
		return TRUE;
}

