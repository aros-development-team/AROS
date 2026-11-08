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
#include <proto/dos.h>
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
	error_t incomplete;	/* first failure preventing a complete scan */
	BOOL retained;		/* repair kept objects with unresolved allocation */
	BOOL setupdone;		/* root and layout metadata are available */
	BOOL readfailed;
	uint32 badblock;		/* last unreadable metadata block in CHECK */

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
static error_t GetRepairBlock(cachedblock_t *block, ULONG blocknr);
static error_t RepairDeldir(void);
static error_t dd_CheckBlock(uint32 bloknr, int seqnr, bool *valid);
static error_t RepairDirTree(void);
static error_t RepairDir(struct direntry *de, c_dirblock_t *parent);
static error_t RepairDirBlock(uint32 bloknr, uint32 anodenr, uint32 parent, bool discardable);
static error_t RepairDirEntry(struct direntry *de, c_dirblock_t *dirblk);
static error_t CheckDataChain(uint32, uint32 *, uint32 *, bool);
static error_t RepairFile(struct direntry *de, c_dirblock_t *parent);
static error_t RepairHardLink(struct direntry *de, c_dirblock_t *dirblok);
static error_t RepairSoftLink(struct direntry *de, c_dirblock_t *dirblok);
static error_t RepairRollover(struct direntry *de, c_dirblock_t *dirblok);
static error_t RemoveDirEntry(struct direntry *de, c_dirblock_t *dirblk);
static error_t DeleteAnode(uint32 anodechain, uint32 node);
static error_t GetExtraFields(struct extrafields *extrafields, struct direntry *de);
static error_t SetExtraFields(struct extrafields *extrafields, struct direntry *from, c_dirblock_t *dirblk);
#ifdef __GNUC__
static error_t RepairLinkChain(struct direntry *de, c_dirblock_t *dirblk) __attribute__((noinline));
#else
static error_t RepairLinkChain(struct direntry *de, c_dirblock_t *dirblk);
#endif
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
static error_t vol_SearchFileSystem(uint32 *result);
static bitmap_t *bg_InitBitmap(int32 start, int32 stop, int32 step);
static void bg_KillBitmap(bitmap_t **bm);
static BOOL bg_IsItemUsed(bitmap_t *bm, uint32 nr);
error_t SearchInDir(uint32 diranodenr, uint32 target, int *result);
static error_t FindInDir(uint32 diranodenr, uint32 target, int *result, uint32 *linkhead, bool *complete);
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

/* These failures say nothing about whether a readable object is corrupt. */
static bool IsOperationalError(error_t error)
{
	return error == e_read_error || error == e_write_error ||
		error == e_out_of_memory || error == e_options_error ||
		error == e_aborted || error == e_alloc_fail || error == e_fatal_error;
}

/* CHECK may skip an unreadable subtree, but must not infer that its
 * references are absent or compare allocation maps built without it. */
static bool SkipCheckReadError(error_t error)
{
	if (mode != check || error != e_read_error || aborting)
		return false;
	if (!ss.incomplete)
		ss.incomplete = error;
	return true;
}

/* Keep uncertain objects without treating their unvisited allocation as free.
 * Never use this for device failures, allocation failures or user aborts. */
static error_t UnresolvedObject(void)
{
	if (mode == repair)
		ss.retained = true;
	else if (!ss.incomplete)
		ss.incomplete = e_keep_object;
	return e_keep_object;
}

static void ReportCheckReadError(error_t error, uint32 blocknr)
{
	if (SkipCheckReadError(error)) {
		ss.readfailed = true;
		ss.badblock = blocknr;
		sprintf(bericht, "unreadable metadata block %lu (check incomplete)",
			(unsigned long)blocknr);
		adderror(bericht);
	}
}

static bool DiscardUnreadableBlock(error_t error, const char *kind, uint32 blocknr)
{
	char question[256];

	if (error != e_read_error || mode != repair)
		return false;
	sprintf(question, "Cannot read %s block %lu.\n"
		"Discard its contents and continue recovery?\n"
		"Files referenced only by this block will be lost.", kind, blocknr);
	return volume.askuser(question, "Discard", "Abort") != 0;
}


static BOOL FileSizeCheck(ULONG low, ULONG high, ULONG numblocks)
{
	ULONG size, oldlow;
	
	oldlow = low;
	low += volume.blocksize - 1;
	if (low < oldlow)
		high++;
	if (high >> volume.blockshift)
		return FALSE;
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
	enum mode savedmode = mode;
	error_t (*writer)(uint8 *, int32, uint32) = volume.writerawblocks;
	error_t error;

	aborting = 0;
	// onbreak(&brk);
	memset(&ss, 0, sizeof(ss));
	opties = opties;

	flags = opties;
	if (opties & (SSF_CHECK|SSF_FIX))
		flags |= SSF_GEN_BMMASK;

	/* init stats */
	memset(&stats, 0, sizeof(stats));
	/* SEARCH uses the CHECK scan, including its non-destructive handling of
	 * damaged entries. Keep simulated cache updates off the device through
	 * exitStandardScan's final flush, also for callers outside the console. */
	if (mode == search)
		mode = check;
	if (mode == check)
		volume.writerawblocks = dev_WriteBlocksDummy;
	error = mainStandardScan(flags);
	volume.writerawblocks = writer;
	mode = savedmode;
	return error;
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
	if (!rext.data)
		return exitStandardScan(e_out_of_memory);
	rext.mode = check;
	if ((error = GetRext()) || aborting)
		return exitStandardScan(error);

	ss.setupdone = true;
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
		error = ss_CheckDirTree();
		if ((error != e_none) || aborting)
			return exitStandardScan(error);
		if (mode == check)
			break;
		ss.pass++; 
	}

	if (ss.stage != finished) {
		if (mode == check && !ss.incomplete)
			ss.incomplete = e_max_pass_exceeded;
		return exitStandardScan(e_max_pass_exceeded);
	}

	volume.status(0, "finishing up", 2);
	volume.progress(0, 1);
	error = exitStandardScan(ss.retained ? e_keep_object : e_none);
	volume.progress(0, 1);
	return error;
}

static error_t exitStandardScan(error_t error)
{
	error_t write_error = UpdateCache();
	if (write_error)
		error = write_error;

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

	if (error == e_keep_object && mode == repair && !aborting)
		volume.showmsg("INCOMPLETE: unresolved objects retained; existing allocations preserved.\n");
	else if (mode == check && ss.setupdone &&
	    ss.incomplete && error == ss.incomplete && !aborting)
		volume.showmsg("INCOMPLETE\n");
	else if ((error != e_none) || aborting)
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


/* An incomplete namespace can justify adding allocations, never freeing
 * existing ones. Apply this before AllocBuildBlocks uses the generated map.
 * Defer rebuilds while unvisited metadata might occupy their new blocks. */
static error_t PreserveAllocations(void)
{
	rootblock_t *diskroot;
	bitmapblock_t *bitmap;
	uint32 i;
	error_t error;

	diskroot = AllocBufMem(volume.blocksize * rbl->rblkcluster);
	if (!diskroot)
		return e_out_of_memory;
	error = ReadPFS3Metadata((uint8 *)diskroot, ROOTBLOCK + volume.firstblock,
		volume.blocksize * rbl->rblkcluster, PFS3_METADATA_ROOT);
	if (!error) {
		bitmap = (bitmapblock_t *)(diskroot + 1);
		if (bitmap->id != BMBLKID) {
			adderror("reserved bitmap repair deferred: object allocation is unresolved");
			error = e_keep_object;
		} else
			for (i = 0; i < volume.resbitmap->lwsize; i++)
				volume.resbitmap->map[i] &= bitmap->bitmap[i];
	}
	FreeBufMem(diskroot);
	return error;
}

/* Check the partition defined by the rootblock and rootblock extension. */
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
				if ((error = InitReservedBitmap()))
					return error;

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
				if ((error = InitMainBitmap()))
					return error;

		case anodebitmap:

			if (ss.flags & SSF_GEN_ANODEBITMAP) {
				error = InitAnodeBitmap();
				if (IsOperationalError(error) && !SkipCheckReadError(error))
					return error;
			}

		default:

			break;
	}

	/* directory tree */
	error = RepairDirTree();
	if ((error && error != e_keep_object && !SkipCheckReadError(error)) || aborting)
		return error;
	/* Deleted entries must yield to the live namespace, not claim its
	 * anodes first and cause a live file to be removed. */
	if ((error = RepairDeldir()) || aborting)
		return error;
	if (cache.writeerror)
		return cache.writeerror;
	if (ss.incomplete) {
		volume.showmsg("Scan incomplete: metadata not fully checked; allocation-map comparisons skipped.\n");
		return ss.incomplete;
	}
	
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
				if (ss.retained && (error = PreserveAllocations()))
					return error;
				if (!ss.retained && (error = AllocBuildBlocks()))
					return error;

				/* Protect recovered backups already referenced on disk,
				 * even when publishing new metadata must be deferred. */
				if ((error = RepairReservedBitmap()) || aborting)
					return error;
				if (ss.retained && !IsMinListEmpty(&volume.buildblocks)) {
					adderror("metadata rebuild deferred: object allocation is unresolved");
					return e_keep_object;
				}
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
	if (!IsMinListEmpty(&volume.buildblocks)) {
		if (ss.retained) {
			adderror("metadata rebuild deferred: object allocation is unresolved");
			return e_keep_object;
		}
		ss.stage = resbitmap;
	}

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
	if (mode == check && ss.readfailed && ss.badblock == bloknr)
		return e_read_error;

	if ((error = ReadPFS3Metadata((uint8 *)blok->data, realbloknr,
		SIZEOF_RESBLOCK, PFS3_METADATA_RESERVED)))
	{
		ReportCheckReadError(error, bloknr);
		return error;
	}
	else
	{
		blok->mode = check;
		blok->blocknr = bloknr;
		return e_none;
	}
}

error_t vol_GetRawBlock(cachedblock_t *blok, ULONG bloknr)
{
	error_t error;
	ULONG realbloknr = bloknr;

	if ((error = vol_CheckBlockNr(&realbloknr)))
		return error;
	if (mode == check && ss.readfailed && ss.badblock == bloknr)
		return e_read_error;
	if ((error = c_GetBlock((uint8 *)blok->data, realbloknr,
		SIZEOF_RESBLOCK))) {
		ReportCheckReadError(error, bloknr);
		return error;
	}

	blok->mode = check;
	blok->blocknr = bloknr;
	return e_none;
}

/*
 * Repair routines need the contents of an overwritten metadata block so they
 * can take their wrong-ID recovery path.  A syntactically invalid metadata
 * block is therefore readable here. During syntax repair, an unreadable
 * primary also takes the backup-search path; later passes retain the error.
 */
static error_t GetRepairBlock(cachedblock_t *block, ULONG blocknr)
{
	error_t error = volume.getblock(block, blocknr);

	if (error == e_syntax_error)
		error = vol_GetRawBlock(block, blocknr);
	if (error == e_read_error && mode == repair && ss.stage == syntax)
	{
		/* Invalid ID forces the caller to search for a readable backup.
		 * This placeholder must never be treated as recovered metadata. */
		memset(block->data, 0, SIZEOF_RESBLOCK);
		block->blocknr = blocknr;
		block->mode = check;
		return e_none;
	}
	return error;
}

error_t vol_WriteBlock(cachedblock_t *blok)
{
	int status;
	uint32 bloknr;
	cachedblock_t *owner;

	if (blok->mode != build)
	{
		bloknr = blok->blocknr;
		if ((status = vol_CheckBlockNr(&bloknr)))
			return status;

		status = WritePFS3Metadata((uint8 *)blok->data, bloknr,
			SIZEOF_RESBLOCK, PFS3_METADATA_RESERVED);
		if (!status && ss.readfailed && ss.badblock == blok->blocknr)
			ss.readfailed = false;
		return status;
	}

	/* GetResBlock returns caller-owned storage even for pending rebuilds.
	 * Keep the build-list copy authoritative until it is published. */
	owner = GetBuildBlock(blok->data->id, blok->data->indexblock.seqnr);
	if (!owner)
		return e_fatal_error;
	if (owner->data != blok->data)
		memcpy(owner->data, blok->data, SIZEOF_RESBLOCK);
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
			error = WritePFS3Metadata((uint8 *)rbl,
				ROOTBLOCK + volume.firstblock, volume.blocksize,
				PFS3_METADATA_ROOT);
			if (!error)
				fixederror("new rootblock build");
		}
	}
	else
		error = e_aborted;

	return error;
}

/* read from disk and check
 * Returns error (0 = ERROR_NONE = ok).
 */
static error_t RequestMountFile(void)
{
	struct FileRequester *freq;
	char *mfname;
	size_t length;
	bool ok = false;

	freq = AllocAslRequestTags(ASL_FileRequest, ASLFR_SleepWindow, TRUE,
		ASLFR_TitleText, "Select mountfile", ASLFR_InitialFile, "mountme",
		ASLFR_InitialDrawer, "df0:", ASLFR_DoSaveMode, TRUE, TAG_DONE);
	if (freq == NULL)
		return e_out_of_memory;
	while (!ok) {
		if (!AslRequestTags(freq, ASLFR_Window, CheckRepairWnd, TAG_DONE)) {
			volume.showmsg("No mountlist written\n");
			FreeAslRequest(freq);
			return e_aborted;
		}
		length = strlen(freq->fr_Drawer) + strlen(freq->fr_File) + 2;
		mfname = malloc(length);
		if (mfname == NULL) {
			FreeAslRequest(freq);
			return e_out_of_memory;
		}
		strcpy(mfname, freq->fr_Drawer);
		if (!AddPart(mfname, freq->fr_File, length)) {
			free(mfname);
			FreeAslRequest(freq);
			return e_options_error;
		}
		ok = MakeMountFile(mfname);
		free(mfname);
		volume.showmsg(ok ? "Mountlist written\n" : "Could not write mountlist\n");
	}
	FreeAslRequest(freq);
	return e_none;
}

static error_t GetRootBlock(void)
{
	error_t error = e_none;
	uint32 bloknr;
	int okuser;
	bool unreadable = false;

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
	if ((error = ReadPFS3Metadata((uint8 *)rbl,
		ROOTBLOCK + volume.firstblock, volume.blocksize,
		PFS3_METADATA_ROOT)))
	{
		adderror("rootblock could not be loaded");
		if (error != e_read_error || mode != repair)
			return error;
		unreadable = true;
		if (!volume.askuser("The rootblock could not be read.\n"
			"Try recovery from other metadata?\n"
			"Backups may be obsolete and recovery may lose files.",
			"Continue", "Abort"))
			return e_read_error;
		/* Offer the normal root search/reconstruction choices. */
		memset(rbl, 0, MAXRESBLOCKSIZE);
	}

	// check rootblock type
	if (!IsRootBlock(rbl))
	{
		if (!unreadable)
			adderror("not an AFS, PFS-II or PFS-III disk");
		okuser = volume.askuser("Rootblock not found.\n"
			"Select ok to search for misplaced rootblock.", "ok", "cancel");

		bloknr = 0;
		if (okuser && (error = vol_SearchFileSystem(&bloknr)))
			return error;
		if (okuser && bloknr)
		{
			if (unreadable && !volume.askuser(
				"A readable rootblock was found. It may be obsolete.\n"
				"Use this candidate and its partition layout?",
				"Use", "Abort"))
				return e_aborted;
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

			if ((error = RequestMountFile()))
				return error;

			volume.askuser("Now starting to check/repair\n"
				"relocated partition", "CONTINUE", NULL);

			return GetRootBlock();
		}
		else
		{
			if (mode == check)
				return e_rbl_not_found;
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

			if ((error = RequestMountFile()))
				return error;

			volume.askuser("Now starting to check/repair\n"
				"redefined partition", "CONTINUE", NULL);

			return GetRootBlock();

		case e_options_error:
			return error;

		default:
			return error;
	}

	exitblock();
	return e_none;
}

static bool MakeMountFile(char *fname)
{
	FILE *mf;
	bool ok;

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
		ok = !ferror(mf);
		if (fclose(mf) != 0)
			ok = false;
		return ok;
	}
	return false;
}

static error_t vol_SearchFileSystem(uint32 *result)
{
	return SearchFileSystem(
			(volume.firstblock > 1024) ? volume.firstblock - 1024 : 0,
			volume.firstblock + 1024, result);
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

	memcpy(volume.diskname, &rbl->diskname[1], min(rbl->diskname[0], sizeof(rbl->diskname) - 1));
	volume.diskname[min(rbl->diskname[0], sizeof(rbl->diskname) - 1)] = 0;
	volume.showmsg("Checking disk ");
	volume.showmsg("%s", volume.diskname);
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

	if (dirty) {
		error_t write_error = WritePFS3Metadata((uint8 *)rbl,
			ROOTBLOCK + volume.firstblock, volume.blocksize,
			PFS3_METADATA_ROOT);
		if (write_error)
			return write_error;
	}

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
		if ((error = GetRepairBlock((cachedblock_t *)&rext, bloknr)))
			return error;
	}

	if (!bloknr || rext.data->id != EXTENSIONID)
	{
		adderror("extension block has wrong id");
		if (mode == repair)
		{
			volume.status(1, "Searching rext", 256);
			oldbloknr = bloknr;
			if ((error = SearchBlock(EXTENSIONID, 0, bloknr, 0, 0, 0, &bloknr)))
				return error;
			if (bloknr)
			{
				sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, bloknr);
				fixederror(bericht);
				rbl->extension = bloknr;
				if ((error = WritePFS3Metadata((uint8 *)rbl,
					ROOTBLOCK + volume.firstblock, volume.blocksize, PFS3_METADATA_ROOT)))
					return error;
				return GetRext();
			}
			else
			{
				volume.showmsg("No replacement block found\n");
				free (rext.data);
				if ((error = BuildRext(&rext))) {
					adderror("building new rootblockextension failed");
					return error;
				}
				fixederror("new rootblockextension created");
				/* Apply the same volume setup as for a recovered extension. */
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
		if (mode == repair && (error = WritePFS3Metadata((uint8 *)rbl,
			ROOTBLOCK + volume.firstblock, volume.blocksize, PFS3_METADATA_ROOT)))
			return error;
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
			/* Bound the diagnostic scan too, without rejecting recoverable
			 * names just because the stored limit has been lost. */
			fixederror(bericht);
			rext.data->fnsize = 108;
			if (mode != check && (error = volume.writeblock((cachedblock_t *)&rext)))
				return error;
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
	bootbl.data = calloc(1, 2 * volume.blocksize);
	if ((error = ReadPFS3Metadata((uint8 *)bootbl.data,
		BOOTBLOCK + volume.firstblock, 2 * volume.blocksize,
		PFS3_METADATA_BOOT)))
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
			if ((error = WritePFS3Metadata((uint8 *)bootbl.data,
				BOOTBLOCK + volume.firstblock, 2 * volume.blocksize,
				PFS3_METADATA_BOOT)))
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
	bool remove, valid;
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
						if ((error = dd_CheckBlock(rext.data->deldir[i], i, &valid))) {
							exitblock();
							return error;
						}
						if (valid)
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
				if ((error = dd_CheckBlock(rbl->deldir, 0, &valid)))
					return error;
				if (valid)
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

	if (rootdirty) {
		error = WritePFS3Metadata((uint8 *)rbl, ROOTBLOCK + volume.firstblock,
			volume.blocksize, PFS3_METADATA_ROOT);
		if (error)
			return error;
	}
	if (rextdirty)
		return volume.writeblock((cachedblock_t *)&rext);

	return e_none;
}


static error_t dd_CheckBlock(uint32 bloknr, int seqnr, bool *valid)
{
	error_t error;
	c_deldirblock_t ddblk;
	int i;
	uint32 ddnr, blocks, remaining;
	canode_t delnode;
	struct deldirentry *dde;
	ULONG size, high;

	*valid = false;
	volume.progress(0, 1);
	
	/* get block */
	ddblk.data = calloc(1, SIZEOF_RESBLOCK);
	if (!ddblk.data)
		return e_out_of_memory;
	if ((error = volume.getblock((cachedblock_t *)&ddblk, bloknr)))
	{
		adderror("couldn't read deldirblock");
		free (ddblk.data);
		if (SkipCheckReadError(error))
			return e_none;
		if (DiscardUnreadableBlock(error, "deleted-file directory", bloknr))
			return e_none;
		return IsOperationalError(error) ? error : e_none;
	}

	/* check id */
	if (ddblk.data->id != DELDIRID)
	{
		adderror("deldirblock error");
		free (ddblk.data);
		return e_none;
	}

	if (ddblk.data->seqnr != seqnr)
	{
		adderror("deldirblock error");
		free (ddblk.data);
		return e_none;
	}

	// check entries
	for (i=0; i<31; i++)
	{
		blocks = 0;
		dde = &ddblk.data->entries[i];
		if (!dde->anodenr)
			continue;

		/* Live files have already claimed their anodes.  Never cut a
		 * deleted chain: it may overlap a live file or another old entry. */
		error = CheckDataChain(dde->anodenr, &blocks, &remaining, true);
		if (SkipCheckReadError(error))
			continue;
		if (IsOperationalError(error)) {
			free(ddblk.data);
			return error;
		}
		size = GetDDFileSize(dde, &high);
		if (error || !FileSizeCheck(size, high, blocks))
			goto invalid_entry;

		for (ddnr = dde->anodenr; ddnr && remaining;
		     ddnr = delnode.next, remaining--) {
			error = ReadAnode(&delnode, ddnr, false);
			if (SkipCheckReadError(error))
				goto next_entry;
			if (!error)
				error = AnodeUsed(ddnr);
			if (error) {
				if (!IsOperationalError(error)) {
					/* Earlier nodes may already be marked in this pass. */
					if (mode == repair)
						redosyntax = true;
					goto invalid_entry;
				}
				free(ddblk.data);
				return error;
			}
		}
		continue;

 invalid_entry:
		if (mode == check)
			adderror("delfile anode error");
		else {
			fixederror("delfile anode error");
			dde->anodenr = 0;
			if ((error = volume.writeblock((cachedblock_t *)&ddblk))) {
				free(ddblk.data);
				return error;
			}
		}
 next_entry:
		;
	}

	free(ddblk.data);
	*valid = true;
	return e_none;
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


/* Find an anode chain's unique extent before visiting its entries.  Brent's
 * check uses constant space; only a proven cycle needs the extra walks below.
 * On a read failure, stop identifies the unreadable node so CHECK can
 * still visit its readable prefix.
 * The first node is supplied and retained for the common one-block directory. */
/* Do not retain preflight locals in every recursive RepairDir frame. */
#ifdef __GNUC__
static error_t CheckAnodeChain(canode_t *, uint32 *, uint32 *) __attribute__((noinline));
#endif
static error_t CheckAnodeChain(canode_t *first, uint32 *length, uint32 *stop)
{
	canode_t node = *first, other;
	uint32 checkpoint = first->nr, distance = 0, power = 1;
	uint32 lead, tail, i, cyclelength = 0;
	error_t error;

	*length = 0;
	*stop = 0;
	for (;;) {
		if (aborting)
			return e_aborted;
		++*length;
		if (!node.next) {
			*stop = 0;
			return e_none;
		}
		++distance;
		if (node.next == checkpoint)
			break;
		if (distance == power) {
			checkpoint = node.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
		if ((error = ReadAnode(&node, *stop = node.next, true))) {
			if (mode == repair && (error == e_number_error || error == e_not_found)) {
				/* Keep the readable chain prefix when its successor is
				 * structurally absent. Propagate failed reads instead of cutting. */
				node.next = 0;
				if ((error = WriteAnode(&node, node.nr))) {
					addfileerror("cannot cut invalid anode tail; object retained");
					return IsOperationalError(error) ? error : e_fatal_error;
				}
				if (node.nr == first->nr)
					*first = node;
				fixedfileerror("removed invalid anode chain tail");
				*stop = 0;
				return e_none;
			}
			goto read_failed;
		}
	}

	/* Locate the entry to the cycle, then its closing edge.  Cutting the
	 * detection edge instead could orphan distinct nodes. */
	cyclelength = distance;
	*length = 0;
	lead = tail = first->nr;
	for (i = 0; i < cyclelength; i++) {
		if ((error = ReadAnode(&node, *stop = lead, true)))
			goto read_failed;
		lead = node.next;
	}
	while (tail != lead) {
		if ((error = ReadAnode(&node, *stop = tail, true)) ||
		    (error = ReadAnode(&other, *stop = lead, true)))
			goto read_failed;
		tail = node.next;
		lead = other.next;
		++*length;
	}
	/* The node immediately before lead reached the cycle entry closes it. */
	if (*length)
		node = other;
	*length += cyclelength;
	if (mode == repair) {
		node.next = 0;
		if ((error = WriteAnode(&node, node.nr))) {
			addfileerror("cannot cut anode chain cycle; object retained");
			return IsOperationalError(error) ? error : e_fatal_error;
		}
		if (node.nr == first->nr)
			*first = node;
		fixedfileerror("removed anode chain cycle");
	} else
		addfileerror("cyclic anode chain");
	*stop = 0;
	return e_none;

read_failed:
	if (mode == repair && !IsOperationalError(error)) {
		addfileerror("cannot inspect anode chain; object retained");
		return e_fatal_error;
	}
	/* A transient failure may occur on a later lap. Stop at its first
	 * occurrence in the real walk, not at the probe's iteration count. */
	*length = ~(uint32)0;
	return error;
}

#ifdef __GNUC__
static bool IsDirectoryAncestor(uint32) __attribute__((noinline));
#endif
static bool IsDirectoryAncestor(uint32 nr)
{
	struct dirstack *ancestor;
	for (ancestor = currentobject ? currentobject->parent : NULL;
	     ancestor; ancestor = ancestor->parent)
		if (ancestor->anodenr == nr)
			return true;
	return false;
}

static error_t RepairDir(struct direntry *de, c_dirblock_t *parent)
{
	uint32 mainanodenr, anodenr, parentanodenr, remaining, stop;
	BOOL firstnode = TRUE;
	canode_t dirnode;
	error_t error = e_none, chainerror;

	if (de && (!de->anode || IsDirectoryAncestor(de->anode))) {
		addfileerror(de->anode ? "directory reference points to an ancestor" : "missing directory anode");
		return e_remove;
	}

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
		error = RepairLinkChain(de, parent);
		if (IsOperationalError(error) && !SkipCheckReadError(error))
			return error;
	}

	parentanodenr = parent ? parent->data->anodenr : 0;
	mainanodenr = de ? de->anode : ANODE_ROOTDIR;
	if ((error = ReadAnode(&dirnode, mainanodenr, true))) {
		if (IsOperationalError(error))
			return error;
		if (mode != repair && !ss.incomplete)
			ss.incomplete = error;
		return (mode != repair || error == e_number_error || error == e_not_found)
			? e_remove : e_fatal_error;
	}
	chainerror = CheckAnodeChain(&dirnode, &remaining, &stop);
	if (chainerror && mode != repair && !IsOperationalError(chainerror)) {
		addfileerror("invalid directory anode chain; checking readable prefix");
		if (!ss.incomplete)
			ss.incomplete = chainerror;
		chainerror = e_none;
	}
	if (chainerror && !SkipCheckReadError(chainerror))
		return chainerror;
	for (anodenr = mainanodenr; anodenr && remaining && anodenr != stop;
	     anodenr = dirnode.next, remaining--)
	{
		/* get anode */
		if (!firstnode && (error = ReadAnode(&dirnode, anodenr, true)))
			return IsOperationalError(error) ? error : e_fatal_error;

		firstnode = FALSE;
		/* check anode */
		if (dirnode.clustersize != 1)
		{
			fixedfileerror("directory anode has illegal clustersize");
			dirnode.clustersize = 1;
			SaveAnode(&dirnode, anodenr);
		}

		enterblock(dirnode.blocknr);
		error = RepairDirBlock(dirnode.blocknr, mainanodenr, parentanodenr,
			de != NULL || anodenr != mainanodenr || dirnode.next != 0);
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

				if (mode == check) {
					addfileerror("corrupt directory block");
					if (!ss.incomplete)
						ss.incomplete = e_syntax_error;
					error = e_none;
					break;
				}
				if (mode != repair)
					addfileerror("corrupt directory block");
				error = DeleteAnode(mainanodenr, anodenr);
				if (error && error != e_empty) {
					/* Keep the directory if its chain cannot be shortened. */
					if (mode == repair && !IsOperationalError(error)) {
						addfileerror("cannot shorten directory anode chain; directory retained");
						return e_fatal_error;
					}
					return error;
				}
				if (mode == repair)
					fixedfileerror("removed corrupt directory block");
				if (error == e_empty)
					return e_remove;
				/* Deleting the head copied its successor into the head. */
				if (anodenr == mainanodenr)
					dirnode.next = mainanodenr;
				break;

			case e_none:

				ResBlockUsed(dirnode.blocknr);
				AnodeUsed(anodenr);
				break;

			default:
			
				if (error != e_keep_object)
					KillReservedBitmap();
		}
		
		if ((error != e_none) || aborting)
			return error;
	}

	if (anodenr && !chainerror && mode == repair) {
		addfileerror("directory chain changed during repair; scan stopped");
		return e_fatal_error;
	}
	return chainerror;
}

/* The entry used to reach this block is evidence of its parent. Preserve
 * the contents when the old parent is stale, but do not choose between live
 * owners or guess which entries an uncertain directory lookup would retain. */
#ifdef __GNUC__
static error_t CheckDirParent(c_dirblock_t *, uint32) __attribute__((noinline));
#endif
static error_t CheckDirParent(c_dirblock_t *dirblk, uint32 parent)
{
	int type = 0;
	bool complete;
	error_t error;

	if (dirblk->data->parent == parent)
		return e_none;
	addfileerror("invalid directory parent");
	if (dirblk->data->anodenr != ANODE_ROOTDIR) {
		error = FindInDir(dirblk->data->parent, dirblk->data->anodenr,
			&type, NULL, &complete);
		if (error)
			return error;
		if (type || !complete) {
			addfileerror("directory owner is ambiguous; contents retained");
			return UnresolvedObject();
		}
	}

	dirblk->data->parent = parent;
	/* The corrected copy is fully traversed in CHECK too. */
	if (mode == check)
		return e_none;
	if ((error = volume.writeblock((cachedblock_t *)dirblk)))
		return error;
	fixedfileerror("corrected directory parent");
	return e_none;
}

/* The normal metadata reader rejects an invalid entry chain before converting
 * anything. Recover only its bounded prefix in a private buffer; the caller
 * must still check the directory identity before writing it back. */
/* Keep this exceptional path's locals out of recursive directory frames. */
#ifdef __GNUC__
static error_t RecoverDirPrefix(c_dirblock_t *, uint32) __attribute__((noinline));
#endif
static error_t RecoverDirPrefix(c_dirblock_t *dirblk, uint32 bloknr)
{
	uint8 *data = (uint8 *)dirblk->data;
	uint32 offset, next;
	error_t error;

	if ((error = vol_GetRawBlock((cachedblock_t *)dirblk, bloknr)))
		return error;
	if (PFS3DiskBlockId(data) != DBLKID)
		return e_syntax_error;

	for (offset = sizeof(struct dirblock); offset < SIZEOF_RESBLOCK; offset += next) {
		next = data[offset];
		if (!next)
			return e_syntax_error;
		/* Leave room for the terminating zero, including at the block end. */
		if (next < sizeof(struct direntry) || next >= SIZEOF_RESBLOCK - offset) {
			memset(data + offset, 0, SIZEOF_RESBLOCK - offset);
			return PFS3MetadataToHostForRecovery(data, SIZEOF_RESBLOCK,
				PFS3_METADATA_RESERVED) ? e_none : e_syntax_error;
		}
	}
	return e_syntax_error;
}

static error_t RepairDirBlock(uint32 bloknr, uint32 anodenr, uint32 parent, bool discardable)
{
	c_dirblock_t dirblk;
	struct direntry *entry;
	uint8 *end;
	bool truncated = false;
	error_t error;

	/* get block */
	dirblk.data = calloc(1, SIZEOF_RESBLOCK);
	if (!dirblk.data)
		return e_out_of_memory;
	error = volume.getblock((cachedblock_t *)&dirblk, bloknr);
	if (error == e_syntax_error) {
		error = RecoverDirPrefix(&dirblk, bloknr);
		truncated = error == e_none;
	}
	if (error)
	{
		free (dirblk.data);
		if (SkipCheckReadError(error))
			return e_none;
		if (discardable && DiscardUnreadableBlock(error, "directory", bloknr))
			return e_remove;
		return IsOperationalError(error) ? error : e_remove;
	}

	/* check dirblock */
	if (dirblk.data->id != DBLKID || dirblk.data->anodenr != anodenr)
	{
		free (dirblk.data);
		return e_remove;
	}
	if ((error = CheckDirParent(&dirblk, parent))) {
		free(dirblk.data);
		return error;
	}
	
	if (truncated) {
		if (mode == repair) {
			if ((error = volume.writeblock((cachedblock_t *)&dirblk))) {
				free(dirblk.data);
				return error;
			}
			fixedfileerror("removed invalid directory entry tail; readable prefix retained");
		} else {
			addfileerror("invalid directory entry tail; checking readable prefix");
			if (!ss.incomplete)
				ss.incomplete = e_syntax_error;
		}
	}
	entry = (struct direntry *)dirblk.data->entries;
	end = (uint8 *)dirblk.data + SIZEOF_RESBLOCK;

	while ((uint8 *)entry < end && entry->next)
	{
		if (entry->next < sizeof(*entry) || entry->next >= end - (uint8 *)entry) {
			free(dirblk.data);
			return e_fatal_error;
		}
		if ((error = RepairDirEntry(entry, &dirblk)))
		{
			if (error == e_keep_object || SkipCheckReadError(error)) {
				entry = NEXTENTRY(entry);
				continue;
			}
			if (IsOperationalError(error)) {
				free(dirblk.data);
				return error;
			}
			if (mode == repair)
			{
				redosyntax = true;
				fixedfileerror("removing corrupt directory entry");
				if ((error = RemoveDirEntry(entry, &dirblk))) {
					free(dirblk.data);
					return error;
				}
			}
			else
			{
				addfileerror("corrupt directory entry");
				if (entry->next & 1) {
					if (!ss.incomplete)
						ss.incomplete = e_syntax_error;
					break;
				}
				entry = NEXTENTRY(entry);
			}
		}
		else
			entry = NEXTENTRY(entry);
	}

	if ((uint8 *)entry >= end) {
		free(dirblk.data);
		return e_fatal_error;
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

/* Keep the low size word and infer only the missing high word, from a
 * complete, bounded allocation chain. No rounding up to allocated bytes. */
#ifdef __GNUC__
static error_t RecoverSizeHigh(struct direntry *, uint32 *) __attribute__((noinline));
#endif
static error_t RecoverSizeHigh(struct direntry *de, uint32 *high)
{
	uint32 blocks, length, lowblocks, difference;
	error_t error;

	if ((error = CheckDataChain(de->anode, &blocks, &length, false))) {
		if (IsOperationalError(error))
			return error;
		goto uncertain;
	}
	lowblocks = (de->fsize >> volume.blockshift) +
		((de->fsize & (volume.blocksize - 1)) != 0);
	if (blocks < lowblocks)
		goto uncertain;
	difference = blocks - lowblocks;
	if (difference & (((uint32)1 << (32 - volume.blockshift)) - 1))
		goto uncertain;
	*high = difference >> (32 - volume.blockshift);
	return e_none;

uncertain:
	/* Let ordinary chain/size validation reject this object, without
	 * preventing unrelated repairs on the volume. */
	*high = 0;
	return e_none;
}

/* The handler requires valid name/comment/extra-field packing, not merely
 * a traversable entry chain. Preserve names and comments when only the packed
 * fields are unusable; otherwise let normal entry removal handle the damage. */
#ifdef __GNUC__
static error_t CheckDirEntryLayout(struct direntry *, c_dirblock_t *) __attribute__((noinline));
#endif
static error_t CheckDirEntryLayout(struct direntry *de, c_dirblock_t *dirblk)
{
	struct extrafields extra;
	uint8 *data = (uint8 *)de;
	uint32 offset, high = 0;
	error_t error;

	if (PFS3GetExtraFields(de, &extra) != (UWORD)~0)
		return e_none;
	if (mode != repair) {
		if (!ss.incomplete)
			ss.incomplete = e_direntry_error;
	}
	offset = offsetof(struct direntry, startofname) + de->nlength;
	if (offset >= de->next) {
		addfileerror("invalid filename");
		return e_direntry_error;
	}
	offset = (sizeof(*de) + de->nlength + data[offset]) & ~1U;
	if (offset > de->next - sizeof(UWORD)) {
		addfileerror("invalid filenote");
		return e_direntry_error;
	}
	if (mode != repair)
		return e_direntry_error;
	if ((rbl->options & MODE_LARGEFILE) && de->type == ST_FILE) {
		if ((error = RecoverSizeHigh(de, &high)))
			return error;
		if (high && offset > de->next - 2 * sizeof(UWORD)) {
			addfileerror("no room for recovered file size; file retained");
			return UnresolvedObject();
		}
	}
	memset(data + de->next - sizeof(UWORD), 0, sizeof(UWORD));
	if (high) {
		data[de->next - 4] = high >> 8;
		data[de->next - 3] = high;
		data[de->next - 2] = 4; /* packed fsizex flag, stored big endian */
		fixedfileerror("recovered high file size from allocation chain");
	}
	fixedfileerror("removed malformed directory extra fields");
	return volume.writeblock((cachedblock_t *)dirblk);
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

		if ((error = CheckDirEntryLayout(de, dirblk)))
			return error;

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

/* Validate and count a data chain before either file walker uses it.  The
 * normal path reads each node once; only cycles need a second traversal.
 * Deleted-file chains must not be changed: their anodes may now be live. */
#ifdef __GNUC__
static error_t CheckDataChain(uint32, uint32 *, uint32 *, bool) __attribute__((noinline));
#endif
static error_t CheckDataChain(uint32 head, uint32 *blocks, uint32 *length, bool deleted)
{
	canode_t node, first;
	uint32 nr, checkpoint, distance, power, limit = ~(uint32)0, stop;
	error_t error;
	bool oversized;

again:
	*blocks = *length = 0;
	oversized = false;
	checkpoint = head;
	distance = 0;
	power = 1;
	if (!head)
		return e_anode_error;
	for (nr = head; nr && *length < limit; nr = node.next) {
		if (aborting)
			return e_aborted;
		if ((error = CheckAnode(&node, nr, !deleted)))
			return error;
		if (deleted && IsAnodeUsed(nr))
			return e_double_allocation;
		if (!*length)
			first = node;
		if (node.clustersize &&
		    (node.blocknr <= rbl->lastreserved || node.blocknr >= volume.disksize ||
		     node.clustersize > volume.disksize - node.blocknr))
			return e_anode_error;
		if (node.clustersize > volume.disksize - *blocks)
			oversized = true;
		else
			*blocks += node.clustersize;
		++*length;
		if (limit != ~(uint32)0)
			continue;
		if (node.next && node.next == checkpoint) {
			if (deleted)
				return e_anode_error;
			/* This preflight locates the true closing edge, preserving
			 * every distinct node, and leaves CHECK's disk untouched. */
			if ((error = CheckAnodeChain(&first, &limit, &stop)))
				return error;
			goto again;
		}
		if (++distance == power) {
			checkpoint = node.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
	}
	return oversized ? e_anode_error : e_none;
}

static error_t RepairFile(struct direntry *de, c_dirblock_t *parent)
{
	uint32 blokken, remaining;
	struct extrafields extra;
	canode_t filenode;
	error_t error;
	uint32 anodenr, bl;
	ULONG size, high;

	stats.numfiles++;
	if ((error = GetExtraFields(&extra, de)))
		return error;

	if ((error = CheckDataChain(de->anode, &blokken, &remaining, false))) {
		if (!IsOperationalError(error))
			addfileerror("invalid file anode chain");
		return IsOperationalError(error) ? error : e_remove;
	}

	/* check anode chain */
	if (ss.stage == syntax)
	{
		if (de->type != ST_ROLLOVERFILE && (extra.virtualsize || extra.rollpointer))
		{
			extra.virtualsize = 0;
			extra.rollpointer = 0;
			if ((error = SetExtraFields(&extra, de, parent)))
				return error;
			fixedfileerror("dangling rollover");
		}

		size = GetDEFileSize(de, &extra, &high);
		if (!FileSizeCheck(size, high, blokken))
		{
			addfileerror("invalid filesize");
			return e_remove;
		}
	}

	if (extra.link) {
		error = RepairLinkChain(de, parent);
		if (IsOperationalError(error))
			return error;
	}

	/* bitmap gen */
	for (anodenr = de->anode; anodenr && remaining;
	     anodenr = filenode.next, remaining--)
	{
		if ((error = ReadAnode(&filenode, anodenr, true)))
			break;
		if ((error = AnodeUsed(anodenr)))
			break;

		for (bl = filenode.blocknr; bl < filenode.blocknr + filenode.clustersize; bl++)
			if ((error = MainBlockUsed(bl)))
				break;
		if (error)
			break;
	}

	if (error != e_none)
	{
		/* invalidate bitmaps to force generation in next stage. */
		if (volume.mainbitmap)
			InvalidBitmap(volume.mainbitmap);
		if (volume.anodebitmap)
			InvalidBitmap(volume.anodebitmap);
		return error;
	}

	return e_none;	
}

/* A skipped target directory is unknown, not evidence of a dangling link. */
#ifdef __GNUC__
static error_t FindLinkTarget(uint32, uint32, int *, uint32 *) __attribute__((noinline));
#endif
static error_t FindLinkTarget(uint32 directory, uint32 object, int *type, uint32 *head)
{
	bool complete;
	error_t error = FindInDir(directory, object, type, head, &complete);
	if (error)
		return error;
	if (!*type && !complete) {
		addfileerror("cannot inspect hardlink target; link retained");
		return UnresolvedObject();
	}
	return e_none;
}

/* A surviving link must belong to its object's backlink chain.  Otherwise
 * the handler cannot safely delete it or preserve it when moving the object.
 * Keep the exceptional lookup's storage out of the recursive directory frame.
 */
#ifdef __GNUC__
static error_t CheckLinkMembership(uint32 directory, uint32 object, uint32 link)
    __attribute__((noinline));
#endif

static error_t CheckLinkMembership(uint32 directory, uint32 object, uint32 link)
{
	uint32 head = 0, checkpoint, distance = 0, power = 1;
	canode_t node;
	error_t error;
	int type;

	if ((error = FindLinkTarget(directory, object, &type, &head)))
		return error;
	checkpoint = head;
	while (head) {
		if (aborting)
			return e_aborted;
		if (head == link)
			return e_none;
		if ((error = ReadAnode(&node, head, false))) {
			if (IsOperationalError(error))
				return error;
			goto uncertain;
		}
		if (node.next && node.next == checkpoint)
			goto uncertain;
		if (++distance == power) {
			checkpoint = node.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
		head = node.next;
	}
	addfileerror("hardlink missing from object link chain");
	return e_remove;

uncertain:
	addfileerror("cannot verify hardlink membership; link retained");
	return UnresolvedObject();
}

/* Repair a hardlink and verify both directions of its object reference. */
static error_t RepairHardLink(struct direntry *de, c_dirblock_t *dirblok)
{
	struct extrafields extra;
	canode_t linknode;
	error_t error, usederror;
	int type;

	stats.numhardlink++;
	if ((error = GetExtraFields(&extra, de)))
		return e_remove;

	if (!extra.link)
	{
		addfileerror("invalid hardlink");
		return e_remove;
	}

	/* get linknode */
	if ((error = ReadAnode(&linknode, de->anode, true)))
		return IsOperationalError(error) ? error : e_remove;

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
	if ((error = FindLinkTarget(linknode.clustersize, extra.link, &type, NULL)))
		goto account;
	switch (type)
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
				if ((error = volume.writeblock((cachedblock_t *)dirblok)))
					return error;
			}
			break;

		default:
			if (de->type == ST_LINKFILE)
			{
				fixedfileerror("invalid hardlink");
				de->type = ST_LINKDIR;
				if ((error = volume.writeblock((cachedblock_t *)dirblok)))
					return error;
			}
			break;
	}

	if (ss.stage == syntax)
		error = CheckLinkMembership(linknode.clustersize, extra.link, de->anode);
account:
	if (error && error != e_keep_object)
		return error;
	usederror = AnodeUsed(de->anode);
	return usederror ? usederror : error;
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
		if ((error = SetExtraFields(&extra, de, dirblok)))
			return error;
	}

	return RepairFile(de, dirblok);
}


/*
 * Helper functions
 */

static error_t RemoveDirEntry(struct direntry *de, c_dirblock_t *dirblk)
{
	uint8 *dest, *source;
	uint32 movelen, removed = de->next;

	dest = (uint8 *)de;
	source = dest + removed;
	movelen = ((uint8 *)(dirblk->data) + SIZEOF_RESBLOCK) - source;
	if (movelen > 0 && movelen < SIZEOF_RESBLOCK)
	{
		memmove(dest, source, movelen);
		memset(dest + movelen, 0, removed);
		return volume.writeblock((cachedblock_t *)dirblk);
	}
	else
		return e_fatal_error;

}


/* remove node from anodechain
 */
static error_t DeleteAnode(uint32 anodechain, uint32 node)
{
	canode_t prev_node, next_node;
	uint32 previous, checkpoint = anodechain, distance = 0, power = 1;
	error_t error;

	if ((error = ReadAnode(&next_node, node, false)))
		return error;
	if (next_node.next == node)
		goto cycle;
	if (anodechain == node)
	{
		if (!next_node.next)
			return e_empty;
		if ((error = ReadAnode(&next_node, next_node.next, false)))
			return error;
		if (next_node.next == node)
			goto cycle;
		return WriteAnode(&next_node, node);
	}

	for (previous = anodechain; previous; previous = prev_node.next)
	{
		if ((error = ReadAnode(&prev_node, previous, false)))
			return error;
		if (prev_node.next == node)
		{
			if (next_node.next == previous)
				goto cycle;
			prev_node.next = next_node.next;
			return WriteAnode(&prev_node, previous);
		}
		/* Brent's cycle check needs no extra reads or chain-sized storage. */
		if (prev_node.next == checkpoint)
			goto cycle;
		if (++distance == power) {
			checkpoint = prev_node.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
	}
	return e_anode_error;

cycle:
	adderror("cyclic anode chain");
	return e_anode_error;
}

/* Get the directory entry extension fields
 */
static error_t GetExtraFields(struct extrafields *extrafields, struct direntry *de)
{
	if (PFS3GetExtraFields(de, extrafields))
		addfileerror("unknown extrafield flags found (ignored)");
	return e_none;
}

/* Set extrafields. Requirement is that the new directory entry fits.
 * Callers must only shrink the entry or check room before growing it.
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
	PFS3AddExtraFields(direntry, extra);
}


/* A replacement head can need one more packed word. Refuse growth when
 * either the entry byte or the block's terminating zero would be lost. */
static error_t SetLinkHead(struct extrafields *extra, uint32 head,
	struct direntry *de, c_dirblock_t *dirblk)
{
	int growth = 2 * ((head >> 16 != 0) + ((head & 0xffff) != 0)
		- (extra->link >> 16 != 0) - ((extra->link & 0xffff) != 0));
	struct direntry *end;

	if (growth > 0) {
		for (end = de; end->next; end = NEXTENTRY(end))
			;
		if (de->next + growth > 254 ||
			(uint8 *)end + growth >= (uint8 *)dirblk->data + SIZEOF_RESBLOCK) {
			addfileerror("cannot redirect hardlink chain: directory entry has no room");
			return UnresolvedObject();
		}
	}
	extra->link = head;
	return SetExtraFields(extra, de, dirblk);
}

static error_t LinkChainError(error_t error)
{
	if (error == e_keep_object || IsOperationalError(error))
		return error;
	addfileerror("invalid hardlink chain; object retained");
	return UnresolvedObject();
}

/* Repair the hardlinks chain referring to a file
 */
static error_t RepairLinkChain(struct direntry *de, c_dirblock_t *dirblk)
{
	struct extrafields extra = { 0 } ;
	canode_t linknode;
	error_t error;
	int type;
	uint32 parent, checkpoint, distance, power, length, stop;

restart:
	distance = 0;
	power = 1;
	/* check extra fields */
	if ((error = GetExtraFields(&extra, de)))
		return error;

	/* check if object has a linkchain */
	if (!extra.link)
		return e_none;

	if ((error = ReadAnode(&linknode, extra.link, false)))
		return LinkChainError(error);

	parent = dirblk->data->anodenr;
	checkpoint = extra.link;
	for (;;)
	{
		if (aborting)
			return e_aborted;
		if (linknode.next && linknode.next == checkpoint) {
			if (mode != repair) {
				addfileerror("cyclic hardlink chain");
				return UnresolvedObject();
			}
			if ((error = ReadAnode(&linknode, extra.link, false)) ||
			    (error = CheckAnodeChain(&linknode, &length, &stop)))
				return LinkChainError(error);
			redosyntax = true;
			goto restart;
		}
		// check objectdir (clustersize) against actual objectdir
		if (linknode.clustersize != parent)
		{
			fixedfileerror("invalid hardlink");
			linknode.clustersize = parent;
			if ((error = WriteAnode(&linknode, linknode.nr)))
				return error;
		}

		// check linkdir left to HardLink::Check
		if ((error = SearchInDir(linknode.blocknr, linknode.nr, &type)))
			return error;
		if (!type)
		{
			if (linknode.nr == extra.link) {
				/* Link entries name their own anodes. Copying the successor
				 * into the head (as for a directory chain) orphans them. */
				error = SetLinkHead(&extra, linknode.next, de, dirblk);
			} else
				error = DeleteAnode(extra.link, linknode.nr);
			if (error)
				return LinkChainError(error);
			fixedfileerror("dangling hardlink");
		}

		if (!linknode.next)
			return e_none;

		if (++distance == power) {
			checkpoint = linknode.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
		if ((error = ReadAnode(&linknode, linknode.next, false)))
			return LinkChainError(error);
	}
}

/* Get & check an anode
 */
static error_t CheckAnode(canode_t *anode, uint32 nr, bool fix)
{
	error_t error = ReadAnode(anode, nr, fix);
	/* Missing/corrupt anodes are empty; unreadable ones are unknown. */
	if (error)
		return IsOperationalError(error) ? error : e_empty;

	if (anode->clustersize == 0 && anode->blocknr == 0 && anode->next == 0)
		return e_empty;

	return e_none;
}

/* Search file by anodenr in a directory
 * return filetype or 0 for not found
 */
error_t SearchInDir(uint32 diranodenr, uint32 target, int *result)
{
	return FindInDir(diranodenr, target, result, NULL, NULL);
}

static error_t FindInDir(uint32 diranodenr, uint32 target, int *result, uint32 *linkhead, bool *complete)
{
	uint32 mainanode = diranodenr, checkpoint = diranodenr, distance = 0, power = 1;
	uint32 entryanode;
	canode_t anode;
	c_dirblock_t dirblk;
	struct direntry *de;
	error_t error, read_error = e_none;

	if (complete)
		*complete = true;
	dirblk.data = calloc(1, SIZEOF_RESBLOCK);
	if (!dirblk.data)
		return e_out_of_memory;
	while (diranodenr)
	{
		error = ReadAnode(&anode, diranodenr, false);
		if (error) {
			if (complete && error != e_number_error && error != e_not_found)
				*complete = false;
			if (IsOperationalError(error))
				goto failed;
			break;
		}
		if (complete && !anode.clustersize && !anode.blocknr && !anode.next)
			break; /* a free anode cannot own directory entries */
		if (complete && (anode.blocknr < rbl->firstreserved ||
		    anode.blocknr > rbl->lastreserved))
			goto next_anode; /* file data cannot be a reserved dirblock */
		error = volume.getblock((cachedblock_t *)&dirblk, anode.blocknr);
		if (error == e_syntax_error) {
			if (complete && mode != repair)
				*complete = false;
			error = RecoverDirPrefix(&dirblk, anode.blocknr);
		}
		if (aborting) {
			error = e_aborted;
			goto failed;
		}
		/* In REPAIR only the readable prefix can survive. A bad ID or
		 * discarded tail cannot own a surviving entry; a block belonging
		 * to another directory still leaves ownership uncertain. */
		if (complete) {
			if (error) {
				if (mode != repair || IsOperationalError(error))
					*complete = false;
			} else if (dirblk.data->id == DBLKID) {
				if (dirblk.data->anodenr != mainanode)
					*complete = false;
			} else if (mode != repair)
				*complete = false;
		}
		if (error == e_read_error)
			read_error = error;
		else if (IsOperationalError(error))
			goto failed;
		else if (!error && dirblk.data->id == DBLKID && dirblk.data->anodenr == mainanode) {
			for (de = FIRSTENTRY(&dirblk); de->next; de = NEXTENTRY(de)) {
				/* Repair removes odd-sized entries; later entries can still
				 * be found before compaction, including on 68000/010. */
				if (de->next & 1)
					continue;
				if ((IPTR)de & 1)
					CopyMem((uint8 *)de + offsetof(struct direntry, anode), &entryanode, sizeof(entryanode));
				else
					entryanode = de->anode;
				if (entryanode == target) {
					*result = de->type;
					if (linkhead) {
						struct extrafields extra;
						PFS3GetExtraFields(de, &extra);
						*linkhead = extra.link;
					}
					free(dirblk.data);
					return e_none;
				}
			}
		}
	next_anode:
		if (anode.next && anode.next == checkpoint)
			break; /* every distinct block in this cycle was searched */
		if (++distance == power) {
			checkpoint = anode.next;
			distance = 0;
			power = power < 0x80000000UL ? power * 2 : ~(uint32)0;
		}
		diranodenr = anode.next;
	}
	error = read_error;
	if (!error)
		*result = 0;
 failed:
	free(dirblk.data);
	return error;
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
					WritePFS3Metadata((uint8 *)rbl,
						ROOTBLOCK + volume.firstblock,
						volume.blocksize, PFS3_METADATA_ROOT);
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
	if (!ablk.data)
		return e_out_of_memory;
	if (*bloknr)
	{
		if ((error = GetRepairBlock((cachedblock_t *)&ablk, *bloknr)))
		{
			free (ablk.data);
			return SkipCheckReadError(error) ? e_none : error;
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
				if ((error = SearchBlock(ABLKID, seqnr, oldbloknr, 0, 0, 0, bloknr)))
				{
					*bloknr = oldbloknr;
					free(ablk.data);
					exitblock();
					return error;
				}
				if (*bloknr)
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
			if ((error = RepairBitmapIndex(&rbl->idx.large.bitmapindex[i], i)) || aborting)
				return error;
			
			if (rbl->idx.large.bitmapindex[i] != blknr)
				WritePFS3Metadata((uint8 *)rbl,
					ROOTBLOCK + volume.firstblock,
					volume.blocksize, PFS3_METADATA_ROOT);
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
	if (GetBuildBlock(BMBLKID, seqnr))
		return e_none;
	bmblk.data = calloc(1, SIZEOF_RESBLOCK);
	if (!bmblk.data)
		return e_out_of_memory;

	if (*bloknr)
	{
		if ((error = GetRepairBlock((cachedblock_t *)&bmblk, *bloknr)))
		{
			free (bmblk.data);
			return SkipCheckReadError(error) ? e_none : error;
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
				if ((error = SearchBlock(BMBLKID, seqnr, oldbloknr, 0, 0, 0, bloknr)))
				{
					*bloknr = oldbloknr;
					free(bmblk.data);
					exitblock();
					return error;
				}
				if (*bloknr)
				{
					sprintf(bericht, "replaced block %lu with backup %lu", oldbloknr, *bloknr);
					fixederror(bericht);
				}
				else
				{
					volume.showmsg("No replacement block found\n");
					free (bmblk.data);
					*bloknr = oldbloknr;
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
		if (!iblk.data)
			return e_out_of_memory;

		/* read or build indexblock
		 */
		if (*bloknr && (error = GetRepairBlock((cachedblock_t *)&iblk, *bloknr)))
		{
			free (iblk.data);
			exitblock();
			return SkipCheckReadError(error) ? e_none : error;
		}

		/* check id */
		if (ss.stage == syntax)
		{
			if (iblk.data->id != bloktype || iblk.data->seqnr != seqnr)
			{
				if (!*bloknr)
					adderror("missing index block");
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
					if ((error = SearchBlock(bloktype, seqnr, oldbloknr, 0, 0, 0, bloknr)))
					{
						*bloknr = oldbloknr;
						free(iblk.data);
						exitblock();
						return error;
					}
					if (*bloknr)
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
						*bloknr = oldbloknr;
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
			error = repairchild((uint32 *)&iblk.data->index[i], seqnr*LONGS_PER_BMB + i);
			if (IsOperationalError(error) || aborting) {
				if (iblk.mode == check)
					free(iblk.data);
				exitblock();
				return error ? error : e_aborted;
			}
			if (error) {
				iblk.data->index[i] = 0;
				error = e_none;
			}

			/* save changes */
			if (child_blk_nr != iblk.data->index[i] && iblk.mode == check) {
				error = volume.writeblock((cachedblock_t *)&iblk);
				if (error) {
					free(iblk.data);
					exitblock();
					return error;
				}
			}
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
	error_t error;
	uint32 reservedsize, bloknr, cluster;

	KillReservedBitmap();
	if (rbl->firstreserved != 2)
	{
		fixederror("illegal firstreserved");
		rbl->firstreserved = 2;
	}

	// reservedsize cannot be smaller then 0 and is not allowed to take more than
    // 1/8th of the partition.
	volume.lastreserved = volume.disksize - 1;
	reservedsize = rbl->lastreserved < rbl->firstreserved ? 0 :
		(rbl->lastreserved - rbl->firstreserved) / volume.rescluster + 1;

	if (reservedsize <=0 || reservedsize > volume.disksize/8)
	{
		sprintf(bericht, "illegal reserved area size of %lu blocks", reservedsize);
		adderror(bericht);
		
		if ((error = SearchLastReserved(&volume, &bloknr)))
			return error;
		if (!bloknr)
			return e_fatal_error;

		fixederror(NULL);
		rbl->lastreserved = volume.lastreserved = bloknr;
	}

	volume.lastreserved = rbl->lastreserved;
	reservedsize = (rbl->lastreserved - rbl->firstreserved) / volume.rescluster + 1;

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
	if (!iblk.data)
		return e_out_of_memory;

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

		for (i=INDEX_PER_BLOCK - 1; i && !iblk.data->index[i]; i--);
	}
	else
	{
		for (s=0, i=MAXSMALLINDEXNR; !rbl->idx.small.indexblocks[i] && i; i--);
	}

	s = s*INDEX_PER_BLOCK*INDEX_PER_BLOCK*ANODES_PER_BLOCK;
	s += (++i * INDEX_PER_BLOCK * ANODES_PER_BLOCK) - 1;
	volume.anodebitmap = bg_InitBitmap(0, s, 1);
	if (!volume.anodebitmap) {
		free(iblk.data);
		return e_out_of_memory;
	}
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

	volume.status(0, "validating reserved", volume.resbitmap->lwsize/0xff);

	/* first save current rootblock */
	if ((error = WritePFS3Metadata((uint8 *)rbl,
		ROOTBLOCK + volume.firstblock, volume.blocksize,
		PFS3_METADATA_ROOT)))
		return error;		
	
	/* now get reserved bitmap with rootblock in memory */
	if (!(lrb = (rootblock_t *)AllocBufMem(volume.blocksize * rbl->rblkcluster)))
		return e_out_of_memory;

	if ((error = ReadPFS3Metadata((uint8 *)lrb,
		ROOTBLOCK + volume.firstblock,
		volume.blocksize * rbl->rblkcluster, PFS3_METADATA_ROOT)))
	{
		adderror("reserved bitmap could not be loaded");
		FreeBufMem(lrb);
		return error;
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
		error = WritePFS3Metadata((uint8 *)lrb,
			ROOTBLOCK + volume.firstblock,
			volume.blocksize * lrb->rblkcluster,
			PFS3_METADATA_ROOT);
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
	bool dirty = false;
	bitmapblock_t *buffer;
	cachedblock_t *built;
	uint32 nuba=0, ubna=0;

	if (ss.verbose)
		volume.showmsg("checking main bitmap\n");

	gbm = volume.mainbitmap;
	buffer = calloc(1, SIZEOF_RESBLOCK);
	if (!buffer)
		return e_out_of_memory;
	volume.status(0, "validating main", gbm->lwsize/INDEX_PER_BLOCK);

	for (bmseqnr = 0; bmseqnr <= gbm->lwsize/INDEX_PER_BLOCK; bmseqnr++)
	{
		bmb.data = buffer;
		volume.progress(0, 1);

		error = GetResBlock((cachedblock_t *)&bmb, BMBLKID, bmseqnr, true);
		if (IsOperationalError(error) || aborting) {
			free(buffer);
			return error ? error : e_aborted;
		}
		built = GetBuildBlock(BMBLKID, bmseqnr);
		if (built)
			bmb = *(c_bitmapblock_t *)built;
		else if (error) {
			if ((error = BuildBitmapBlock(&bmb, bmseqnr))) {
				free(buffer);
				return error;
			}
		}

		if (ss.retained && bmb.mode == build)
			memset(bmb.data->bitmap, 0, LONGS_PER_BMB * sizeof(uint32));

		// check block
		for (i=0; i<INDEX_PER_BLOCK; i++, k++)
		{
			if (ss.retained && k < gbm->lwsize)
				gbm->map[k] &= bmb.data->bitmap[i];
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
							free(buffer);
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
		error = WritePFS3Metadata((uint8 *)rbl,
			ROOTBLOCK + volume.firstblock, volume.blocksize,
			PFS3_METADATA_ROOT);
	
	free(buffer);
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
		if (ss.retained && !anodeused)
			continue;
		anodenr = (i/ANODES_PER_BLOCK << 16) + i%ANODES_PER_BLOCK;

		error = CheckAnode(&anode, anodenr, false);
		if (IsOperationalError(error))
			return error;

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
	if (!nr || !volume.anodebitmap) return FALSE;
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

	/* Outside the generated range is unavailable to deleted files. */
	if (nr < bm->start || nr > bm->stop)
		return TRUE;

	nr -= bm->start;
	nr /= bm->step;
	index = nr/32;
	bit = nr%32;

	if (bm->map[index] & (0x80000000UL >> bit))
		return FALSE;
	else
		return TRUE;
}
