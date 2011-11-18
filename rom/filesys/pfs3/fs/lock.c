/* $Id: lock.c 11.4 1999/02/22 16:33:43 Michiel Exp Michiel $ */
/* $Log: lock.c $
 * Revision 11.4  1999/02/22  16:33:43  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 11.3  1995/12/21  12:05:55  Michiel
 * bugfix: anodechain wasn't freed after closing file
 *
 * Revision 11.2  1995/12/07  15:25:38  Michiel
 * rollover support and bugfixes
 *
 * Revision 11.1  1995/10/03  11:19:27  Michiel
 * merged develop: anodechain
 *
 * Revision 10.15  1995/08/04  11:51:19  Michiel
 * Some memory-allocation fail conditions tests added
 *
 * Revision 10.14  1995/07/21  06:54:46  Michiel
 * MakeListEntry() adapted for DELDIR
 *
 * Revision 10.13  1995/07/18  06:56:13  Michiel
 * changed multiuser access functions
 *
 * Revision 10.12  1995/06/15  18:56:53  Michiel
 * pooled mem
 *
 * Revision 10.11  1995/06/07  19:17:19  Michiel
 * added multiuser routines
 *
 * Revision 10.10  1995/06/04  06:04:31  Michiel
 * minor
 *
 * Revision 10.9  1995/03/24  16:31:07  Michiel
 * MakeListEntry adapted for soflinks & bugfixed
 *
 * Revision 10.8  1995/02/28  18:26:58  Michiel
 * MakeListEntry resolves links now
 *
 * Revision 10.7  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 10.6  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 10.5  1994/11/15  17:52:30  Michiel
 * GURU book update
 *
 * Revision 10.4  1994/10/29  08:55:03  Michiel
 * changed process references to msgport references
 *
 * Revision 10.3  1994/10/28  06:06:40  Michiel
 * use new listentry field 'anodenr'
 * this was essential for AccessConflict()
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

//#define DEBUG 1
#define __USE_SYSBASE

#include "debug.h"

// own includes
#include "blocks.h"
#include "struct.h"
#include "lock_protos.h"
#include "directory_protos.h"
#include "volume_protos.h"
#include "anodes_protos.h"
#include "disk_protos.h"

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
/*                          MAKE/REMOVE/FREE                          */
/*                          MAKE/REMOVE/FREE                          */
/*                          MAKE/REMOVE/FREE                          */
/**********************************************************************/

/* MakeListEntry
**
** Allocated filentry structure and fill it with data from objectinfo and
** listtype. The result should be freed with FreeListEntry.
**
** input : - info: objectinfo of object 
**		 - type: desired type (readlock, writelock, readfe, writefe)
**
** result: the fileentry, or NULL if failure
*/
struct listentry *MakeListEntry (union objectinfo *info, listtype type, SIPTR *error, globaldata *g)
{
  listentry_t *listentry;
  union objectinfo newinfo;
  ULONG size;
  struct extrafields extrafields;
#if DELDIR
  struct deldirentry *dde = 0;
#endif

	ENTER("MakeListEntry");

	// alloceren fileentry
	switch (type.flags.type)
	{
		case ETF_FILEENTRY:	size = sizeof(fileentry_t); break;
		case ETF_VOLUME:
		case ETF_LOCK:		size = sizeof(lockentry_t); break;
		default:			listentry = NULL; return listentry;
	}

	DB(Trace(1,"MakeListEntry","size = %lx\n",size));

#if DELDIR
	if (IsDelDir(*info) || IsVolume(*info) || IsDir(*info))
#else
	if (IsVolume(*info) || IsDir(*info))
#endif
		type.flags.dir = 1;

	/* softlinks cannot directly be opened */
#if DELDIR
	if (info->deldir.special > SPECIAL_DELFILE && info->file.direntry->type == ST_SOFTLINK)
#else	
	if (!IsVolume(*info) && info->file.direntry->type == ST_SOFTLINK)
#endif
	{
		*error = ERROR_IS_SOFT_LINK;
		return NULL;
	}

	if (!(listentry = AllocMemP (size, g)))
	{
		*error = ERROR_NO_FREE_STORE;
		return NULL;
	}

	/* go after link and fetch the fileinfo of the real object
	 * (stored in 'newinfo'
	 */
#if DELDIR
	if (info->deldir.special > SPECIAL_DELFILE && (
#else
	if (!IsVolume(*info) && (
#endif
		(info->file.direntry->type == ST_LINKFILE) ||
		(info->file.direntry->type == ST_LINKDIR)))
	{
		struct canode linknode;

		/* The clustersize of the linknode (direntry.anode) 
		 * actually is the anodenr of the directory the linked to
		 * object is in. The object can be found by searching for
		 * 'anode == objectid'. This objectid can be found in
		 * the extrafields
		 */
		GetExtraFields(info->file.direntry, &extrafields);
		GetAnode(&linknode, info->file.direntry->anode, g);
		if (!FetchObject(linknode.clustersize, extrafields.link, &newinfo, g))
		{
			*error = ERROR_OBJECT_NOT_FOUND;
			return NULL;
		}
	}
	else
	{
		newinfo = *info;
	}

	// general
	listentry->type = type;
#if DELDIR
	switch (newinfo.delfile.special)
	{
		case 0:
			listentry->anodenr = ANODE_ROOTDIR; break;

		case SPECIAL_DELDIR:
			listentry->anodenr = 0; break;

		case SPECIAL_DELFILE:
			dde = GetDeldirEntryQuick(newinfo.delfile.slotnr, g);	
			listentry->anodenr = dde->anodenr;
			break;

		default:
			listentry->anodenr = newinfo.file.direntry->anode; break;
	}
#else
	listentry->anodenr = (newinfo.file.direntry) ? (newinfo.file.direntry->anode) : ANODE_ROOTDIR;
#endif

	listentry->info = newinfo;
	listentry->lock.fl_Access = (type.flags.access & 2) ? EXCLUSIVE_LOCK : SHARED_LOCK;
	listentry->lock.fl_Task   = g->msgport;
	listentry->lock.fl_Volume = MKBADDR(g->currentvolume->devlist);
	listentry->volume = g->currentvolume;

	// type specific
	switch (type.flags.type)
	{
		case ETF_VOLUME:
			listentry->lock.fl_Key    = 0; 
			// listentry->lock.fl_Volume = MKBADDR(newinfo.volume.volume->devlist);
			// listentry->volume		  = newinfo.volume.volume;
			break;

		case ETF_LOCK:
			/* every dirlock MUST have a different fl_Key (DOPUS!) */
			listentry->lock.fl_Key = listentry->anodenr;
			// listentry->lock.fl_Volume = MKBADDR(newinfo.file.dirblock->volume->devlist);
			// listentry->volume = newinfo.file.dirblock->volume;
			break;

		case ETF_FILEENTRY:
#define fe ((fileentry_t *)listentry)
			listentry->lock.fl_Key = listentry->anodenr;
			// listentry->lock.fl_Volume = MKBADDR(MKBADDR(newinfo.file.dirblock->volume->devlist);
			// listentry->volume = newinfo.file.dirblock->volume;
			fe->originalsize = IsDelFile(newinfo) ? dde->size :
					newinfo.file.direntry->size;

			/* Get anodechain. If it fails anodechain will become NULL. This has to be
			 * taken into account by functions that use the chain
			 */
			fe->anodechain = GetAnodeChain (listentry->anodenr, g);
			fe->currnode = &fe->anodechain->head;

#if ROLLOVER
			/* Rollover file: set offset to rollfileoffset */
			/* check for rollover files */
			if (IsRollover(newinfo))
			{
				GetExtraFields(newinfo.file.direntry, &extrafields);
				SeekInFile(fe,extrafields.rollpointer,OFFSET_BEGINNING,error,g);
			}
#endif /* ROLLOVER */
#undef fe
			break;

		default:
			listentry = NULL;
			return listentry;
	}

	return listentry;
}


/* AddListEntry
**
** Checks if the listentry causes access conflicts
** Adds the entry to the locklist 
*/
BOOL _AddListEntry (listentry_t *entry, globaldata *g)
{
  struct volumedata *volume;

	DB(Trace(1,"AddListEntry","fe = %lx\n", entry->volume->fileentries.mlh_Head));

	if (entry==NULL)
		return DOSFALSE;

	if (AccessConflict(entry))
	{
		DB(Trace(1,"AddListEntry","found accessconflict!"));
		return DOSFALSE;
	}

	volume = entry->volume;

	/* add to head of list; als link locks using BPTRs */
	if (!IsMinListEmpty(&volume->fileentries))
		entry->lock.fl_Link = MKBADDR(&(((listentry_t *)HeadOf(&volume->fileentries))->lock));
	else
		entry->lock.fl_Link = BNULL;

	MinAddHead (&volume->fileentries, entry);

	return DOSTRUE; 
}

/* RemoveListEntry
**
** removes 'entry' from the list and frees entry with FreeFileEntry 
**
** also think about empty lists: kill volume if empty and not present
** also	makes lock-links
*/
void RemoveListEntry (listentry_t *entry, globaldata *g)
{
  struct volumedata *volume;
  struct MinList *previous;

	/* get volume */
	volume = entry->volume;

	/* remove from list */
	MinRemove (entry);

	/* update FileLock link */
	previous = (struct MinList *)entry->prev;
	if (!IsHead(entry))
	{
		if (!IsTail(entry))
				((listentry_t *)previous)->lock.fl_Link = 
					MKBADDR(&(((listentry_t *)entry)->next->lock));
		else
			((listentry_t *)previous)->lock.fl_Link = BNULL;
	}
	entry->lock.fl_Task = NULL;
	FreeListEntry (entry, g);

	if (g->currentvolume != volume)
	{
		/* check if last lock; yes:kill (only if not current disk)
		*/
		if (IsMinListEmpty (&volume->fileentries))
		{
			DB(Trace(1,"RemoveListEntry", "killing volumedata\n"));

//			LockDosList (LDF_VOLUMES|LDF_READ);
			Forbid ();
			RemDosEntry ((struct DosList *)volume->devlist);
			FreeDosEntry ((struct DosList *)volume->devlist);
			FreeVolumeResources (volume, g);
//			UnLockDosList (LDF_VOLUMES|LDF_READ);
			Permit ();
		}
		/* update doslist->dl_LockList if necessary
		*/
		else if (previous == (struct MinList *)&volume->fileentries)
		{
//			LockDosList (LDF_VOLUMES|LDF_READ);
			Forbid ();
			volume->devlist->dl_LockList = MKBADDR(&(((fileentry_t *)HeadOf(&volume->fileentries))->le.lock));
//			UnLockDosList (LDF_VOLUMES|LDF_READ);
			Permit ();
		}
	}
}

void FreeListEntry(listentry_t *entry, globaldata *g)
{
#define fe ((fileentry_t *)entry)
	if (IsFileEntry(entry) && fe->anodechain)
		DetachAnodeChain(fe->anodechain, g);
	FreeMemP(entry, g);
#undef fe
}

/**********************************************************************/
/*                               CHANGE                               */
/*                               CHANGE                               */
/*                               CHANGE                               */
/**********************************************************************/

BOOL _ChangeAccessMode(listentry_t *file, LONG mode, SIPTR *error, globaldata *g)
{
  UWORD oldmode, newmode;

	// -I- make dosmode compatible with listtype-mode
	switch(mode)
	{
		case MODE_READWRITE:
		case MODE_NEWFILE:
		case EXCLUSIVE_LOCK:

			newmode = ET_EXCLWRITE;	break;

		default:			

			newmode = ET_SHAREDREAD; break;
	}

	// -II- check if mode is already correct		
	oldmode = file->type.flags.access;
	if (oldmode == newmode) 
		return TRUE;
	
	// -III- operation always ok if oldmode exclusive or newmode shared
	if (oldmode>1 || newmode<=1)
	{
		file->type.flags.access = newmode;
		file->lock.fl_Access = (newmode > 1) ? EXCLUSIVE_LOCK : SHARED_LOCK;
		return TRUE;
	}

	// -IV- otherwise: check for accessconflict
	MinRemove(file);	// prevents access conflict on itself 
	file->type.flags.access = newmode;
	file->lock.fl_Access = (newmode > 1) ? EXCLUSIVE_LOCK : SHARED_LOCK;
	if (!AccessConflict(file))
	{
		MinAddHead(&file->volume->fileentries, file);
		return DOSTRUE;
	}
	else
	{
		file->type.flags.access = oldmode;
		file->lock.fl_Access = (oldmode > 1) ? EXCLUSIVE_LOCK : SHARED_LOCK;
		MinAddHead(&file->volume->fileentries, file);
		*error = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}
}

/**********************************************************************/
/*                               CHECK                                */
/*                               CHECK                                */
/*                               CHECK                                */
/**********************************************************************/

/* AccessConflict
**
** input : - [entry]: the object to be granted access
**    This object should contain valid references
**
** result: TRUE = accessconflict; FALSE = no accessconflict
**    All locks on same ANODE are checked. So a lock on a link can
**    be denied if the linked to file is locked exclusively.
**
** Because UpdateReference always updates all references to a dirblock,
** and the match object is valid, a flushed reference CANNOT point to
** the same dirblock. If it is a link, it CAN reference the same
** object
**
** Returns FALSE if there is an exclusive lock or if there is
** write access on a shared lock
**
*/
BOOL AccessConflict (listentry_t *entry)
{
  ULONG anodenr;
  listentry_t *fe;
  struct volumedata *volume;

	DB(Trace(1,"Accessconflict","entry %lx\n",entry));

	// -I- get anodenr
	anodenr =  entry->anodenr;
	volume  = entry->volume;

	// -II- zoek locks naar zelfde object
	for(fe = HeadOf(&volume->fileentries); fe->next; fe=fe->next) 
	{
		if(fe->type.flags.type == ETF_VOLUME)
		{
			if(entry->type.flags.type == ETF_VOLUME &&
			   (!SHAREDLOCK(fe->type) || !SHAREDLOCK(entry->type)))
			{
				DB(Trace(1,"Accessconflict","on volume\n"));
				return(TRUE);
			}
		}	
		else if(fe->anodenr == anodenr)
		{
			// on of the two wants or has an exclusive lock?
			if(!SHAREDLOCK(fe->type) || !SHAREDLOCK(entry->type))
			{
				DB(Trace(1,"Accessconflict","exclusive lock\n"));
				return(TRUE);
			}

			// new & old shared lock, both write? 
			else if(fe->type.flags.access == ET_SHAREDWRITE &&
					entry->type.flags.access == ET_SHAREDWRITE)
			{
				DB(Trace(1,"Accessconflict","two write locks\n"));
				return(TRUE);
			}
		}
	}
	
	return FALSE;	// no conflicting locks
}

/* ScanLockList
 *
 * checks <list> for a lock on the file with anode anodenr
 * ONLY FOR LOCKS TO CURRENTVOLUME
 */
BOOL ScanLockList (listentry_t *list, ULONG anodenr)
{
	for (;list->next; list=list->next)
	{
		if (list->anodenr == anodenr)
			return TRUE;
	}
	return FALSE;
}


#if MULTIUSER

/*****************************************************************************/
/* muFS routines  															 */
/*****************************************************************************/

/*
 * Check if access allowed 
 * protection is the LW protection of the object
 * flags are flags of GetRelationshipA
 * returns error
 */

ULONG muFS_CheckReadAccess (ULONG protection, ULONG flags, globaldata *g)
{
	if (flags & (muRelF_UID_MATCH | muRelF_NO_OWNER))
	{
		if (protection & FIBF_READ)
			return ERROR_READ_PROTECTED;
	}
	else if (flags & (muRelF_GID_MATCH))
	{
		if (!(protection & FIBF_GRP_READ))
			return ERROR_READ_PROTECTED;
	}
	else if (!(protection & FIBF_OTR_READ))
		return ERROR_READ_PROTECTED;

	return NULL;
}

ULONG muFS_CheckWriteAccess (ULONG protection, ULONG flags, globaldata *g)
{
	if (flags & (muRelF_UID_MATCH | muRelF_NO_OWNER))
	{
		if (protection & FIBF_WRITE)
			return ERROR_WRITE_PROTECTED;
	}
	else if (flags & (muRelF_GID_MATCH))
	{
		if (!(protection & FIBF_GRP_WRITE))
			return ERROR_WRITE_PROTECTED;
	}
	else if (!(protection & FIBF_OTR_WRITE))
		return ERROR_WRITE_PROTECTED;

	return NULL;
}

ULONG muFS_CheckDeleteAccess (ULONG protection, ULONG flags, globaldata *g)
{
	if (flags & (muRelF_UID_MATCH | muRelF_NO_OWNER))
	{
		if (protection & FIBF_DELETE)
			return ERROR_DELETE_PROTECTED;
	}
	else if (flags & (muRelF_GID_MATCH))
	{
		if (!(protection & FIBF_GRP_DELETE))
			return ERROR_DELETE_PROTECTED;
	}
	else if (!(protection & FIBF_OTR_DELETE))
		return ERROR_DELETE_PROTECTED;

	return NULL;
}

#endif
