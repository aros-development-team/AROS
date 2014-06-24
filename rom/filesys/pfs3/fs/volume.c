/* $Id$ */
/* $Log: volume.c $
 * Revision 11.34  1999/09/11  17:05:14  Michiel
 * bugfix version 18.4
 *
 * Revision 11.33  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 11.32  1999/03/23  05:57:39  Michiel
 * Autoupgrade bug involving MODE_SUPERDELDIR fixed
 *
 * Revision 11.31  1999/03/09  10:32:19  Michiel
 * 00136: 1024 byte sector support
 *
 * Revision 11.30  1999/02/22  16:25:30  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 11.29  1998/09/27  11:26:37  Michiel
 * Dynamic rootblock allocation in GetCurrentRoot
 * New ErrorMsg function that replaces macro
 *
 * Revision 11.28  1998/05/30  18:40:14  Michiel
 * Title of error requesters was wrong
 *
 * Revision 11.27  1998/05/27  21:00:08  Michiel
 * MODE_DATESTAMP is automatically turned on
 *
 * Revision 11.26  1998/05/22  22:59:58  Michiel
 * Datestamps added
 *
 * Revision 11.25  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 11.24  1996/03/29  17:00:11  Michiel
 * bugfix: rootblock was freed with FreeVec instead of with FreeBufmem
 *
 * Revision 11.23  1996/01/30  12:50:39  Michiel
 * Diskinsertsequence didn't used old instead of just read rootblock
 * --- working tree overlap ---
 *
 * Revision 11.22  1996/01/03  10:04:35  Michiel
 * simple change
 *
 * Revision 11.21  1995/11/15  16:00:05  Michiel
 * Rootblock extension detection, loading and automatic creation
 *
 * Revision 11.20  1995/11/07  15:07:18  Michiel
 * Adapted to new datacache (16.2)
 * FreeUnusedResources() checks if volume
 *
 * Revision 11.19  1995/09/01  11:18:05  Michiel
 * CheckCurrentVolumeBack added
 * NormalErrorMsg changed. Extra argument that specifies the
 * number of targets added.
 *
 * Revision 11.18  1995/08/21  04:23:11  Michiel
 * Create deldir if it isn't there
 *
 * Revision 11.17  1995/08/04  04:23:01  Michiel
 * use of MODE_SIZEFIELD added
 *
 * Revision 11.16  1995/07/21  06:58:07  Michiel
 * DELDIR adaptions: MakeVolumeData, FreeVolumeResources
 *
 * Revision 11.15  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 11.14  1995/07/11  09:23:36  Michiel
 * DELDIR stuff
 *
 * Revision 11.13  1995/07/07  14:38:47  Michiel
 * AFSLITE stuff
 *
 * Revision 11.12  1995/07/07  10:14:12  Michiel
 * changed CheckVolume()
 *
 * Revision 11.11  1995/06/19  09:43:19  Michiel
 * softprotect of on diskchange
 *
 * Revision 11.10  1995/06/16  09:59:37  Michiel
 * using Allec & FreeBufMem
 *
 * Revision 11.9  1995/06/15  18:56:53  Michiel
 * pooled mem
 *
 * Revision 11.8  1995/05/20  12:12:12  Michiel
 * Updated messages to reflect Ami-FileLock
 * CUTDOWN version
 * protection update
 *
 * Revision 11.7  1995/03/30  18:55:39  Michiel
 * Initialization of notifylist added to MakeVolumeData()
 *
 * Revision 11.6  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 11.5  1995/01/29  07:34:57  Michiel
 * Raw res read/write and LOCK update
 *
 * Revision 11.4  1995/01/24  09:54:14  Michiel
 * Cache hashing added
 *
 * Revision 11.3  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 11.2  1995/01/15  05:26:44  Michiel
 * fixed FreeVolumeResources bug
 * inhibited trackdisk specific parts
 *
 * Revision 11.1  1995/01/08  16:24:01  Michiel
 * Compiled (new MODE_BIG version)
 *
 * Revision 10.4  1994/11/15  17:52:30  Michiel
 * GURU book update
 *
 * Revision 10.3  1994/10/29  08:55:42  Michiel
 * changed process references to msgport references
 *
 * Revision 10.2  1994/10/27  11:38:17  Michiel
 * Killed old Update() routines, that now reside in new_Update()
 * MakeBlockDirty() now unconditionally sets g->dirty
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

//#define DEBUG 1
//#define DEBUGMODE 1
#define __USE_SYSBASE

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <exec/interrupts.h>
#include <devices/input.h>
#include <devices/timer.h>
#include <dos/filehandler.h>
#include <intuition/intuition.h>
#include <proto/intuition.h>
#include <clib/alib_protos.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include "debug.h"

// own includes
#include "blocks.h"
#include "struct.h"
#include "directory_protos.h"
#include "volume_protos.h"
#include "disk_protos.h"
#include "allocation_protos.h"
#include "anodes_protos.h"
#include "update_protos.h"
#include "lru_protos.h"
#include "ass_protos.h"
#include "init_protos.h"
#include "format_protos.h"

static VOID CreateInputEvent(BOOL inserted, globaldata *g);

/**********************************************************************/
/*                               DEBUG                                */
/**********************************************************************/

#ifdef DEBUG
extern BOOL debug;
static UBYTE debugbuf[120];
#define DebugOn debug++
#define DebugOff debug=(debug?debug-1:0)
#define DebugMsg(msg) if(debug) {NormalErrorMsg(msg, NULL); debug=0;}
#define DebugMsgNum(msg, num) sprintf(debugbuf, "%s 0x%08lx.", msg, num); \
			if(debug) {NormalErrorMsg(debugbuf, NULL); debug=0;}
#define DebugMsgName(msg, name) sprintf(debugbuf, "%s >%s<.", msg, name); \
			if(debug) {NormalErrorMsg(debugbuf, NULL); debug=0;}
#else
#define DebugOn
#define DebugOff
#define DebugMsg(m)
#define DebugMsgNum(msg,num)
#define DebugMsgName(msg, name)
#endif


/**********************************************************************/
/*                             NEWVOLUME                              */
/*                             NEWVOLUME                              */
/*                             NEWVOLUME                              */
/**********************************************************************/

/* NewVolume
**
** I  Update diskchangenumber
**      :nochange->exit (tenzij FORCE = TRUE)
**      
** II New state = volume present?
**      :get id of new disk (read rootblock)
**      New disk = current disk?
**          :exit
**
** III Old state = volume present?
**      :diskremoved-sequence
**
** IV New state = volume present ?
**      :diskinserted-sequence
**-----
** II-III-IV ordering is essential because diskremove-sequence has to be done before
**      diskinsert-sequence and diskremove-sequence can only be executed if newvolume <>
**      currentvolume
**-----
** DiskRemove-sequence
**
** I Clear globaldata->currentvolume (prevent infinite recursive loop)
**
** II Changed blocks in cache ?
**      :request old volume (causes recursive call)
**      :update volume
**      :exit
**
** III Locks/files open on volume? 
**      yes: link locks in doslist
**           clear volume task field
**      no:  remove volume from doslist
**           free all volume resources
**-----
** DiskInsert-sequence
**
** I Search new disk in volumelist
**      found:  take over volume and locklist
**      ~found: make new volumestructure
**              link volume in doslist
**
** II Update globaldata->currentvolume
**----
** use FORCE to force a new volume even if the changecount is equal
*/
static BOOL SameDisk(struct rootblock *, struct rootblock *);
static BOOL SameDiskDL(struct rootblock *, struct DeviceList *);
static void TakeOverLocks(struct FileLock *, globaldata *);

void NewVolume (BOOL FORCE, globaldata *g)
{
  BOOL oldstate, newstate, changed;
  struct rootblock *rootblock;

	/* check if something changed */
	changed = UpdateChangeCount (g);
	if (!FORCE && !changed)
		return;

	ENTER("NewVolume");
	FlushDataCache (g);

	/* newstate <=> there is a PFS disk present */
	oldstate = g->currentvolume ? TRUE : FALSE;
	newstate = GetCurrentRoot (&rootblock, g);

	/* undo error enforced softprotect */
	if ((g->softprotect == 1) && g->protectkey == ~0)
		g->softprotect = g->protectkey = 0;

	if (oldstate && !newstate)
		DiskRemoveSequence (g);

	if (newstate)
	{
		if (oldstate && SameDisk (rootblock, g->currentvolume->rootblk))
		{
			FreeBufmem (rootblock, g);  /* @XLVII */
		}
		else
		{
			if (oldstate)
				DiskRemoveSequence (g);
			DiskInsertSequence (rootblock, g);
		}
	}
	else
	{
		g->currentvolume = NULL;    /* @XL */
	}

	MotorOff (g);
	EXIT("NewVolume");
}


/* pre:
**  globaldata->currentvolume not necessarily present
** post:
**  the old currentvolume is updated en als 'removed' currentvolume == 0
** return waarde = currentdisk back in drive?
** used by NewVolume and ACTION_INHIBIT
*/
void DiskRemoveSequence(globaldata *g)
{
  struct volumedata *oldvolume = g->currentvolume;

	ENTER("DiskRemoveSequence");

	/* -I- update disk 
	** will ask for old volume if there are unsaved changes
	** causes recursive NewVolume call. That's why 'currentvolume'
	** has to be cleared first; UpdateDisk won't be called for the
	** same disk again
	*/
	if(oldvolume && g->dirty)
	{
		RequestCurrentVolumeBack(g);
		UpdateDisk(g);
		return;
	}

	/* disk removed */
	g->currentvolume = NULL;
	FlushDataCache (g);

	/* -II- link locks in doslist 
	** lockentries: link to doslist...
	** fileentries: link them too...
	*/
	Forbid();   /* LockDosList(LDF_VOLUMES|LDF_READ); */
	if(!IsMinListEmpty(&oldvolume->fileentries))
	{
		DB(Trace(1, "DiskRemoveSequence", "there are locks\n"));
		oldvolume->devlist->dl_LockList = MKBADDR(&(((listentry_t *)(HeadOf(&oldvolume->fileentries)))->lock));
		oldvolume->devlist->dl_Task = NULL;
		FreeUnusedResources(oldvolume, g);
	}
	else
	{
		DB(Trace(1, "DiskRemoveSequence", "removing doslist\n"));
		RemDosEntry((struct DosList*)oldvolume->devlist);
		FreeDosEntry((struct DosList*)oldvolume->devlist);
		MinRemove(oldvolume);
		FreeVolumeResources(oldvolume, g);
	}
	Permit();   /* UnLockDosList(LDF_VOLUMES|LDF_READ); */

#ifdef TRACKDISK
	if(g->trackdisk)
	{
		g->request->iotd_Req.io_Command = CMD_CLEAR;
		DoIO(g->request);
	}
#endif

	CreateInputEvent(FALSE, g);

#if ACCESS_DETECT
	g->tdmode = ACCESS_UNDETECTED;
#endif

	EXIT("DiskRemoveSequence");
	return;
}   

void DiskInsertSequence(struct rootblock *rootblock, globaldata *g)
{
  struct DosList *doslist;
  struct DosInfo *di;
  struct DeviceList *devlist;
  BOOL found = FALSE, added = FALSE;
  UBYTE diskname[DNSIZE];       // or should it be a DSTR??
  SIPTR locklist;

	ENTER("DiskInsertSequence");

	/* -I- Search new disk in volumelist */

	BCPLtoCString(diskname, rootblock->diskname);
//  doslist = LockDosList(LDF_VOLUMES|LDF_READ);
	Forbid();
	di = BADDR(((struct RootNode *)DOSBase->dl_Root)->rn_Info);
	doslist = BADDR(di->di_DevInfo);
	
	for (doslist = BADDR(di->di_DevInfo);doslist;doslist = BADDR(doslist->dol_Next))
	{
		if (doslist->dol_Type == DLT_VOLUME && SameDiskDL(rootblock, (struct DeviceList *)doslist))
		{
			found = TRUE;
			break;
		}
	}

//  while(!found)
//  {
//      doslist = NextDosEntry(doslist, LDF_VOLUMES);
//
//      if(doslist && (doslist = FindDosEntry(doslist, diskname, LDF_VOLUMES)))
//          found = SameDiskDL(rootblock, (struct DeviceList *)doslist);
//      else
//          break;
//  }

	if (found)
	{
		DB(Trace(1, "DiskInsertSequence", "found\n"));

		devlist = (struct DeviceList *)doslist;
		locklist = (SIPTR)BADDR(devlist->dl_LockList);

		/* take over volume
		** use LOCKTOFILEENTRY(lock)->volume to get volumepointer
		*/
		if(locklist)
		{
		  fileentry_t *fe;

			/* get volumepointer @XLXV */
			fe = LOCKTOFILEENTRY(locklist);
			if(fe->le.type.flags.type == ETF_VOLUME)
				g->currentvolume = fe->le.info.volume.volume;
			else
				g->currentvolume = fe->le.volume;

			/* update rootblock */
			if (g->rootblock) FreeBufmem (g->rootblock, g);
			g->rootblock =
			g->currentvolume->rootblk = rootblock;

			/* take over filelocks
			** lockentries: takeover, change taskfield
			** fileentries: don't change taskfield
			*/
			TakeOverLocks((struct FileLock *)locklist, g);
			devlist = (struct DeviceList *)doslist;
			devlist->dl_LockList = BNULL;
			devlist->dl_Task = g->msgport;

		}
		else
		{
			RemDosEntry(doslist);   // @@ freeing rootblk etc?
			FreeDosEntry(doslist);
			found = FALSE;      // an empty doslistentry is useless to us
		}
	}
//  UnLockDosList(LDF_VOLUMES|LDF_READ);
	Permit();

	if(!found)
	{
		DB(Trace(1, "DiskInsertSequence", "not found %s\n", diskname));

		/* make new doslist entry (MOET eerst (zie blz67 schrift) */
		devlist = (struct DeviceList *)MakeDosEntry(diskname, DLT_VOLUME);
		if(devlist)
		{
			/* devlist invullen. Diskname NIET @XLIX */
			devlist->dl_Task        = g->msgport;
			devlist->dl_VolumeDate.ds_Days   = rootblock->creationday;
			devlist->dl_VolumeDate.ds_Minute = rootblock->creationminute;
			devlist->dl_VolumeDate.ds_Tick   = rootblock->creationtick;
			devlist->dl_LockList    = BNULL; // no locks open yet
			devlist->dl_DiskType    = rootblock->disktype;
			added = AddDosEntry((struct DosList *)devlist);
		}

		/* make new volumestructure for inserted volume */
		g->currentvolume = MakeVolumeData(rootblock, g);
		MinAddHead(&g->volumes, g->currentvolume);
		g->currentvolume->devlist = (struct DeviceList *)devlist;

		/* check if things worked out */
		if(!devlist || !added)      // duplicate disks or out of memory
		{
			ErrorMsg (AFS_ERROR_DOSLIST_ADD, NULL, g);
			if(devlist)
				FreeDosEntry((struct DosList *)devlist);
			FreeVolumeResources(g->currentvolume, g);
			g->currentvolume = NULL;
		}

		CreateInputEvent(TRUE, g);
	}

	/* Reconfigure modules to new volume */
	InitModules (g->currentvolume, FALSE, g);

	/* create rootblockextension if its not there yet */
	if (!g->currentvolume->rblkextension &&
		g->diskstate != ID_WRITE_PROTECTED)
	{
		MakeRBlkExtension (g);
	}

#if DELDIR
	/* upgrade deldir */
	if (rootblock->deldir)
	{
		struct cdeldirblock *ddblk;
		int i, nr;

		/* kill current deldir */
		ddblk = (struct cdeldirblock *)AllocLRU(g);
		if (ddblk)
		{
			if (RawRead ((UBYTE*)&ddblk->blk, RESCLUSTER, rootblock->deldir, g) == 0)
			{
				if (ddblk->blk.id == DELDIRID)
				{
					for (i=0; i<31; i++)
						nr = ddblk->blk.entries[i].anodenr;
						if (nr)
							FreeAnodesInChain(nr, g);
				}
			}
			FreeLRU ((struct cachedblock *)ddblk);
		}

		/* create new deldir */
		SetDeldir(1, g);
		ResToBeFreed(rootblock->deldir, g);
		rootblock->deldir = 0;
		rootblock->options |= MODE_SUPERDELDIR;
	}
#endif

	/* update datestamp and enable */
	rootblock->options |= MODE_DATESTAMP; 
	rootblock->datestamp++;
	g->dirty = TRUE;

	EXIT("DiskInsertSequence");
}

/* check if rootblocks are of the same disk */
static BOOL SameDisk(struct rootblock *disk1, struct rootblock *disk2)
{
  BOOL result;

	result = disk1->creationday    == disk2->creationday &&
			 disk1->creationminute == disk2->creationminute &&
			 disk1->creationtick   == disk2->creationtick &&
			 ddstricmp(disk1->diskname, disk2->diskname);

	return(result);
}

/* checks is devicelist belongs to rootblock */
static BOOL SameDiskDL(struct rootblock *disk1, struct DeviceList *disk2)
{
  BOOL result;

	result = ddstricmp(disk1->diskname, BADDR(disk2->dl_Name)) &&
			 disk1->creationday    == disk2->dl_VolumeDate.ds_Days &&
			 disk1->creationminute == disk2->dl_VolumeDate.ds_Minute &&
			 disk1->creationtick   == disk2->dl_VolumeDate.ds_Tick;

	return(result);
}

/* make and fill in volume structure
 * uses g->geom!
 * returns 0 is fails
 */
struct volumedata *MakeVolumeData (struct rootblock *rootblock, globaldata *g)
{
  struct volumedata *volume;
  struct MinList *list;

	ENTER("MakeVolumeData");

	volume = AllocMemPR (sizeof(struct volumedata), g);

	volume->rootblk = rootblock;
	volume->rootblockchangeflag = FALSE;

	/* lijsten initieren */
	for (list = &volume->fileentries; list <= &volume->notifylist; list++)
		NewList((struct List *)list);

	/* andere gegevens invullen */
	volume->numsofterrors   = 0;
	volume->diskstate       = ID_VALIDATED;

	/* these could be put in rootblock @@ see also HD version */
	volume->numblocks       = g->geom->dg_TotalSectors;
	volume->bytesperblock   = g->geom->dg_SectorSize;
	volume->rescluster      = rootblock->reserved_blksize / volume->bytesperblock;

	/* load rootblock extension (if it is present) */
	if (rootblock->extension && (rootblock->options & MODE_EXTENSION))
	{
		struct crootblockextension *rext;

		rext = AllocBufmemR (SIZEOF_CACHEDBLOCK, g);
		memset (rext, 0, sizeof(struct cachedblock));
		if (RawRead ((UBYTE *)&rext->blk, volume->rescluster, rootblock->extension, g) != 0)
		{
			ErrorMsg (AFS_ERROR_READ_EXTENSION, NULL, g);
			FreeBufmem (rext, g);
			rootblock->options ^= MODE_EXTENSION;
		}
		else
		{
			if (rext->blk.id == EXTENSIONID)
			{
				volume->rblkextension = rext;
				rext->volume = volume;
				rext->blocknr = rootblock->extension;
			}
			else
			{
				ErrorMsg (AFS_ERROR_EXTENSION_INVALID, NULL, g);
				FreeBufmem (rext, g);
				rootblock->options ^= MODE_EXTENSION;
			}
		}
	}
	else
	{
		volume->rblkextension = NULL;
	}

	EXIT("MakeVolumeData");
	return volume;
}

/* fill in my process at the task fields of the locks */
static void TakeOverLocks (struct FileLock *locklist, globaldata *g)
{
	while (locklist)
	{
		locklist->fl_Task = g->msgport;
		locklist = (struct FileLock *)BADDR(locklist->fl_Link);
	}
}

/* free all resources (memory) taken by volume accept doslist
** it is assumed all this data can be discarded (not checked here!)
** it is also assumed this volume is no part of any volumelist
*/
void FreeVolumeResources(struct volumedata *volume, globaldata *g)
{
	ENTER("Free volume resources");

	if (volume)
	{
		FreeUnusedResources (volume, g);
#if VERSION23
		if (volume->rblkextension)
			FreeBufmem (volume->rblkextension, g);
#endif
#if DELDIR
	//	if (g->deldirenabled)
	//		FreeBufmem (volume->deldir, g);
#endif
		FreeBufmem (volume->rootblk, g);
		FreeMemP (volume, g);
	}

	EXIT("FreeVolumeResources");
}

void FreeUnusedResources(struct volumedata *volume, globaldata *g)
{
  struct MinList *list;
  struct MinNode *node, *next;

	ENTER("FreeUnusedResources");

	/* check if volume passed */
	if (!volume)
		return;

	/* start with anblks!, fileentries are to be kept! */
	for (list = volume->anblks; list<=&volume->bmindexblks; list++)
	{
		node = (struct MinNode *)HeadOf(list);
		while ((next = node->mln_Succ))
		{
			FlushBlock((struct cachedblock *)node, g);
			FreeLRU((struct cachedblock *)node);
			node = next;
		}
	}
}


/* CreateInputEvent
**
** generate a `disk inserted/removed' event, in order to get Workbench to
** rescan the DosList and update the list of volume icons.
**
** function supplied by Nicola Salmoria
*/
static VOID CreateInputEvent(BOOL inserted, globaldata *g)
{
  struct MsgPort *port;

	port = CreateMsgPort();
	if (port)
	{
	  struct IOStdReq *io;

#ifdef __SASC
		io = CreateStdIO(port);
#else
		io = CreateIORequest(port, sizeof(*io));
#endif
		if (io)
		{
			if (!OpenDevice("input.device",0,(struct IORequest *)io,0))
			{
			  struct InputEvent ie;

				memset(&ie,0,sizeof(struct InputEvent));
				ie.ie_Class = inserted ? IECLASS_DISKINSERTED : IECLASS_DISKREMOVED;
				io->io_Command = IND_WRITEEVENT;
				io->io_Data = &ie;
				io->io_Length = sizeof(struct InputEvent);
				DoIO((struct IORequest *)io);

				CloseDevice((struct IORequest *)io);
			}

#ifdef __SASC
			DeleteStdIO(io);
#else
			DeleteIORequest(io);
#endif
		}

		DeleteMsgPort(port);
	}
}


/**********************************************************************/
/*                        OTHER VOLUMEROUTINES                        */
/*                        OTHER VOLUMEROUTINES                        */
/*                        OTHER VOLUMEROUTINES                        */
/**********************************************************************/

/* Update the changecount
** returns TRUE if changecount changed
*/
BOOL UpdateChangeCount(globaldata *g)
{
  struct IOExtTD *request = g->request;
  ULONG changecount;
  BOOL result;

	if(g->removable)
	{
		ENTER("UpdateChangeCount");
		request->iotd_Req.io_Command = TD_CHANGENUM;
		if(DoIO((struct IORequest *)request) == 0)
			changecount = request->iotd_Req.io_Actual;  
		else
			changecount = ~0;

		result = (changecount != g->changecount);
		g->changecount = changecount;
		return(result);
	}
	else
	{
		return 0;   /* no change */
	}
}

/* checks if disk is changed. If so calls NewVolume()
** NB: new volume might be NOVOLUME or NOTAFDSDISK
*/
void UpdateCurrentDisk(globaldata *g)
{
	NewVolume(0, g);
}

/* CheckVolume checks if a volume (ve lock) is (still) present.
** If volume==NULL (no disk present) then FALSE is returned (@XLII).
** result: requested volume present/not present TRUE/FALSE
*/
BOOL CheckVolume(struct volumedata *volume, BOOL write, SIPTR *error, globaldata *g)
{
	if(!volume || !g->currentvolume)
	{
		switch(g->disktype)
		{

			case ID_UNREADABLE_DISK:
			case ID_NOT_REALLY_DOS:
				*error = ERROR_NOT_A_DOS_DISK;
				return(DOSFALSE);

			case ID_NO_DISK_PRESENT:
				if(!volume && !g->currentvolume)
				{
					*error = ERROR_NO_DISK;
					return(DOSFALSE);
				}

			default:
				*error = ERROR_DEVICE_NOT_MOUNTED;
				return(DOSFALSE);
		}
	}
	else if(g->currentvolume == volume)
	{
		switch(g->diskstate)
		{
			case ID_WRITE_PROTECTED:
				if(write)
				{
					*error = ERROR_DISK_WRITE_PROTECTED;
					return(DOSFALSE);
				}

			case ID_VALIDATING:
				if(write)
				{
					*error = ERROR_DISK_NOT_VALIDATED;
					return(DOSFALSE);
				}

			case ID_VALIDATED:
				if(write && g->softprotect)
				{
					*error = ERROR_DISK_WRITE_PROTECTED;
					return(DOSFALSE);
				}

			default:
				return(DOSTRUE);
		}
	}
	else
	{
		*error = ERROR_DEVICE_NOT_MOUNTED;
		return(DOSFALSE);
	}
}


/**********************************************************************/
/*                             REQUESTERS                             */
/*                             REQUESTERS                             */
/*                             REQUESTERS                             */
/**********************************************************************/

/*
 * ErrorMsg wrapper function
 */
LONG ErrorMsg(CONST_STRPTR msg, APTR arg, globaldata *g)
{
	LONG rv;
	char charbuffer[256], *b;

	b = stpcpy(charbuffer, "Device ");
	strncpy(b, &g->mountname[1], g->mountname[0]);
	b += g->mountname[0];
	b = stpcpy(b, ":\n");
	b = stpcpy(b, msg);

	rv = (g->ErrorMsg)(charbuffer,arg,1,g);
	if (!g->softprotect) {
		g->softprotect = 1;
		g->protectkey = ~0;
	}
	if (g->currentvolume)
		g->currentvolume->numsofterrors++;
	return rv;
} 


/* All possible ErrorMsg routines
**
** Displays easyrequest with error message
** Intuition library has to be opened 
*/
static const CONST_STRPTR gadgets[] =
{
	"OK", "RETRY|CANCEL"
};

LONG _NormalErrorMsg(CONST_STRPTR melding, APTR arg, ULONG notargets, globaldata *g)
{
  struct EasyStruct req;

	/*
	 * Make sure we don't hold the performance semaphore while displaying
	 * a requester. Note that unlock_device_unit is safe to call multiple
	 * times, and even when not holding a lock. Unlock also is safe to call
	 * before init has been called.
	 */
	unlock_device_unit(g);

	req.es_StructSize   = sizeof(req);
	req.es_Flags        = 0;
	req.es_Title        = "PFS-III Error Requester";
	req.es_TextFormat   = (STRPTR)melding;
	req.es_GadgetFormat = (STRPTR)gadgets[notargets-1];
	return EasyRequestArgs(NULL, &req, NULL, arg);
}


/* Don't show up a error requester. Used by GetCurrentRoot */
static LONG NoErrorMsg(CONST_STRPTR melding, APTR arg, ULONG dummy, globaldata *g)
{
	return 0;
}

/***********************************************************************/
/*                              LOWLEVEL                               */
/*                              LOWLEVEL                               */
/*                              LOWLEVEL                               */
/***********************************************************************/

/* Load the rootblock of the volume that is currently in the drive.
 * Returns FLASE if no disk or no PFS disk and TRU if it is a PFS disk.
 * Sets disktype in globaldata->disktype
 * Should NOT change globaldata->currentvolume
 *
 * Allocates space in 'rootblock' which is freed if an error occurs.
 */

#ifdef TRACKDISK
static void UpdateDosEnvec(globaldata *g);
#endif

BOOL GetCurrentRoot(struct rootblock **rootblock, globaldata *g)
{
  BOOL changestate;
  ULONG error;
  struct IOExtTD *request = g->request;
  int rblsize;

	ENTER("GetCurrentRoot");

	if (g->removable)
	{
		/* init */
		UpdateChangeCount(g);

		/* added %9.5.1
		** ESSENTIAL for diskspare.device
		** if not present problems with interleaving FFS disks
		*/
#ifdef TRACKDISK
		if (g->trackdisk)
		{
			g->request->iotd_Req.io_Command = CMD_CLEAR;
			DoIO(g->request);
		}
#endif

		/* Get volumepresentstate */
		request->iotd_Req.io_Command = TD_CHANGESTATE;
		if (DoIO((struct IORequest *)request) == 0)
		{
			/* changestate <=> there is a disk present */
			changestate = !request->iotd_Req.io_Actual;
			if (!changestate)
			{
				if ((g->disktype == ID_NOT_REALLY_DOS) || (g->disktype == ID_UNREADABLE_DISK))
					CreateInputEvent(FALSE, g);
				g->disktype = ID_NO_DISK_PRESENT;
				return DOSFALSE;
			}
		}
	}

	/* get drive geometry table (V4.3) */
	GetDriveGeometry(g);
	
	/* Get diskprotection @XLI */
	request->iotd_Req.io_Command = TD_PROTSTATUS;
	if (DoIO((struct IORequest *)request) == 0)
	{
		if (request->iotd_Req.io_Actual)
			g->diskstate = ID_WRITE_PROTECTED;
		else
			g->diskstate = ID_VALIDATED;
	}
	else
	{
		g->diskstate = ID_VALIDATED;
		DB(Trace(1, "GetCurrentRoot", "TD_PROTSTATUS failed\n"));
	}

	/* check if disk is PFS disk */
	*rootblock = AllocBufmemR (BLOCKSIZE, g);
	g->ErrorMsg = NoErrorMsg;   // prevent readerrormsg

#if ACCESS_DETECT
	/* detect best access mode, td32, td64, nsd or directscsi */
	if (g->tdmode == ACCESS_UNDETECTED) {
		if (!detectaccessmode((UBYTE*)*rootblock, g))
			goto nrd_error;
	}
#endif

	error = RawRead((UBYTE *)*rootblock, 1, BOOTBLOCK1, g);
	g->ErrorMsg = _NormalErrorMsg;

	if (!error)
	{
		if ((*rootblock)->disktype == ID_PFS_DISK || (*rootblock)->disktype == ID_PFS2_DISK)
		{
			g->disktype = ID_PFS_DISK;
			error = RawRead((UBYTE *)*rootblock, 1, ROOTBLOCK, g);
			if (!error)
			{
				/* check size and read all rootblock blocks */
				// 17.10: with 1024 byte blocks rblsize can be 1!
				rblsize = (*rootblock)->rblkcluster;
				if (rblsize < 1 || rblsize > 64)
					goto nrd_error;

				FreeBufmem(*rootblock, g);
				*rootblock = AllocBufmemR (rblsize << BLOCKSHIFT, g);
				error = RawRead((UBYTE *)*rootblock, rblsize, ROOTBLOCK, g);
			}

			/* size check */
			if (((*rootblock)->options & MODE_SIZEFIELD) &&
				(g->geom->dg_TotalSectors != (*rootblock)->disksize))
			{
				goto nrd_error;
			}

			return DOSTRUE;
		}

nrd_error:
		g->disktype = ID_NOT_REALLY_DOS;
		CreateInputEvent(TRUE, g);
		FreeBufmem (*rootblock, g);
		*rootblock = NULL;
		return DOSFALSE;
	}
	else if (error == TDERR_DiskChanged)
	{
		if ((g->disktype == ID_NOT_REALLY_DOS) || (g->disktype == ID_UNREADABLE_DISK))
			CreateInputEvent(FALSE, g);
		g->disktype = ID_NO_DISK_PRESENT;
		FreeBufmem (*rootblock, g);
		*rootblock = NULL;
		return DOSFALSE;
	}
	else
	{
		if (g->diskstate != ID_WRITE_PROTECTED)
			g->diskstate = ID_VALIDATING;

		g->disktype = ID_UNREADABLE_DISK;
		CreateInputEvent(TRUE, g);
		FreeBufmem (*rootblock, g);
		*rootblock = NULL;
		return DOSFALSE;
	}
}


/* Get drivegeometry from diskdevice.
** If TD_GETGEOMETRY fails the DOSENVEC values are taken
** Dosenvec is taken into consideration
*/
void GetDriveGeometry(globaldata *g)
{
  IPTR *env = (IPTR *)g->dosenvec;
  struct DriveGeometry *geom = g->geom;
  UBYTE error = 1;

#ifdef TRACKDISK
  struct IOExtTD *request = g->request;

	if(g->trackdisk)
	{
		request->iotd_Req.io_Data = geom;
		request->iotd_Req.io_Command = TD_GETGEOMETRY;
		request->iotd_Req.io_Length = sizeof(struct DriveGeometry);
		if(!(error = DoIO((struct IORequest *)request)))
			UpdateDosEnvec(g);
	}
#endif

	if(error || !g->trackdisk)
	{
		geom->dg_SectorSize     = env[DE_SIZEBLOCK] << 2;
		geom->dg_Cylinders      = env[DE_UPPERCYL] - env[DE_LOWCYL] + 1;
		geom->dg_CylSectors     = env[DE_NUMHEADS] * env[DE_BLKSPERTRACK];
		geom->dg_TotalSectors   = g->geom->dg_Cylinders * 
									  g->geom->dg_CylSectors;
		geom->dg_Heads          = env[DE_NUMHEADS];
		geom->dg_TrackSectors   = env[DE_BLKSPERTRACK];
		geom->dg_BufMemType     = env[DE_MEMBUFTYPE];
		geom->dg_DeviceType     = DG_UNKNOWN;
		geom->dg_Flags          = 0;
	}

	g->firstblock = g->dosenvec->de_LowCyl * geom->dg_CylSectors;
	g->lastblock = (g->dosenvec->de_HighCyl + 1) *  geom->dg_CylSectors - 1;
#if LIMIT_MAXTRANSFER
	/* A600/A1200/A4000 ROM scsi.device ATA spec max transfer bug workaround */
	g->maxtransfer = min(g->dosenvec->de_MaxTransfer, LIMIT_MAXTRANSFER);
#else
	g->maxtransfer = g->dosenvec->de_MaxTransfer;
#endif
	DB(Trace(1,"GetDriveGeometry","firstblk %ld lastblk %ld\n",g->firstblock,g->lastblock));
}


#ifdef TRACKDISK
/* UpdateDosEnvec
** Adapt dosenvec to the current drivegeometry
** DO NOT call this function if you use partitions (ONLY called by GetDriveGeometry) 
*/
static void UpdateDosEnvec(globaldata *g)
{
  ULONG *env;
  struct DriveGeometry *geom;
  ULONG size;

//  LockDosList(LDF_DEVICES|LDF_READ);
	Forbid();

	geom = g->geom;
	env = (ULONG *)g->dosenvec;
	size = env[DE_TABLESIZE];

	if(size > DE_SIZEBLOCK)
		env[DE_SIZEBLOCK] = geom->dg_SectorSize >> 2;

	if(size > DE_NUMHEADS)
		env[DE_NUMHEADS] = geom->dg_Heads;

	if(size > DE_BLKSPERTRACK)
		env[DE_BLKSPERTRACK] = geom->dg_TrackSectors;

	if(size > DE_LOWCYL)
		env[DE_LOWCYL]  = 0;

	if(size > DE_UPPERCYL)
		env[DE_UPPERCYL] = geom->dg_Cylinders - 1;

//  UnLockDosList(LDF_DEVICES|LDF_READ);
	Permit();
}
#endif


/* Get the currentvolume back in the drive
*/
void RequestCurrentVolumeBack(globaldata *g)
{
  UBYTE volumename[DNSIZE];
  BOOL ready = FALSE;
  struct rootblock *rootblock;
  struct volumedata *volume = g->currentvolume;

	ENTER("GetCurrentVolumeBack");

	BCPLtoCString(volumename, volume->rootblk->diskname);

	while(!ready)
	{
		ready = GetCurrentRoot(&rootblock, g) && SameDisk(volume->rootblk, rootblock);
		if(!ready)
			ErrorReport(ABORT_BUSY, REPORT_VOLUME, (IPTR)MKBADDR(volume->devlist), NULL);
	}

	if (rootblock)
		FreeBufmem (rootblock, g);
}   

BOOL CheckCurrentVolumeBack (globaldata *g)
{
  BOOL ready = FALSE;
  struct rootblock *rootblock;
  struct volumedata *volume = g->currentvolume;

	ready = GetCurrentRoot(&rootblock, g) && SameDisk(volume->rootblk, rootblock);
	if (rootblock)
		FreeBufmem (rootblock, g);
	return ready;
}
