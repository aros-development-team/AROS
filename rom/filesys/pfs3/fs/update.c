/* $Id$ */
/* $Log: update.c $
 * Revision 12.12  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 12.11  1999/02/22  16:25:30  Michiel
 * Changes for increasing deldir capacity
 *
 * Revision 12.10  1998/09/27  11:26:37  Michiel
 * Adapted for supermode
 * ErrorMsg param
 *
 * Revision 12.9  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 12.8  1998/05/29  19:31:18  Michiel
 * fixed bug 108
 *
 * Revision 12.7  1998/05/22  22:59:58  Michiel
 * Datestamps added
 *
 * Revision 12.6  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 12.5  1996/03/07  10:03:38  Michiel
 * enforcer hit in CheckUpdate fixed (Working tree)
 *
 * Revision 12.4  1996/01/30  12:49:46  Michiel
 * update of rext->curranseqnr added
 *
 * Revision 12.3  1996/01/03  10:02:59  Michiel
 * reserved rovingpointer added
 *
 * Revision 12.2  1995/11/15  15:56:33  Michiel
 * Rootblock extension update added
 * volume_data datestamp added
 * RemoveEmptyXBlock now calls ResToBeFreed to prevent state overlap
 * CommitReservedToBeFreed() added
 *
 * Revision 12.1  1995/11/07  15:04:07  Michiel
 * Real atomic update:
 * ReservedToBeFreed cache (CommitReservedToBeFreed())
 * Changed update order
 *
 * Revision 11.14  1995/11/02  10:00:31  Michiel
 * using UpdateDataCache (16.2)
 *
 * Revision 11.13  1995/10/03  09:55:22  Michiel
 * rootblock is datestamped at Update()
 *
 * Revision 11.12  1995/07/21  06:55:32  Michiel
 * DELDIR adaptions
 *
 * Revision 11.11  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 11.10  1995/07/11  09:23:36  Michiel
 * DELDIR stuff
 *
 * Revision 11.9  1995/05/20  12:12:12  Michiel
 * Updated messages to reflect Ami-FileLock
 * CUTDOWN version
 * protection update
 *
 * Revision 11.8  1995/04/12  19:22:15  Michiel
 * fixed the 'cache inconsistency' bug, caused by
 * an Update() at the wrong moment
 *
 * Revision 11.7  1995/02/28  18:36:55  Michiel
 * Use of blocks_dirty added
 * Update order changed (see 12.6 bugfix)
 *
 * Revision 11.6  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 11.5  1995/02/09  04:54:20  Michiel
 * Bugfix: Makeblockdirty should unlock block to prevent
 * pfs to run out of buffers
 *
 * Revision 11.4  1995/01/29  07:34:57  Michiel
 * Hash table implemented
 *
 * Revision 11.3  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 11.2  1995/01/08  16:22:44  Michiel
 * *** empty log message ***
 *
 * Revision 11.1  1995/01/03  17:16:19  Michiel
 * New update algorithm based on reallocation at makeblockdirty time
 * combined with bitmap-reserved allocation
 *
 * Revision 10.4  1994/12/05  08:06:20  Michiel
 * Last version of old algorithm
 *
 * Revision 10.3  1994/11/15  17:52:30  Michiel
 * Bugfix: added a DirectUpdate of anblks after indexblk reallocation
 *
 * Revision 10.2  1994/10/27  11:36:07  Michiel
 * Fixed bug that caused unneeded "Abandoned savewrite .." message
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

#define __USE_SYSBASE

/*
 * includes
 */

#include <exec/types.h>
#include <ctype.h>
#include "debug.h"

#include "blocks.h"
#include "struct.h"
#include "update_protos.h"
#include "volume_protos.h"
#include "disk_protos.h"
#include "allocation_protos.h"
#include "anodes_protos.h"
#include "lru_protos.h"
#include "ass_protos.h"
#include "init_protos.h"

/*
 * prototypes
 */

static void RemoveEmptyABlocks(struct volumedata *volume, globaldata *g);
static void RemoveEmptyDBlocks(struct volumedata *volume, globaldata *g);
static void RemoveEmptyIBlocks(struct volumedata *volume, globaldata *g);
static void RemoveEmptySBlocks(struct volumedata *volume, globaldata *g);
static ULONG GetAnodeOfDBlk(struct cdirblock *blk, struct canode *anode, globaldata *g);
static BOOL IsFirstDBlk(struct cdirblock *blk, globaldata *g);
static BOOL IsEmptyABlk(struct canodeblock *ablk, globaldata *g);
static BOOL IsEmptyIBlk(struct cindexblock *blk, globaldata *g);
static BOOL UpdateList (struct cachedblock *blk, globaldata *g);
static void CommitReservedToBeFreed (globaldata *g);
static BOOL UpdateDirtyBlock (struct cachedblock *blk, globaldata *g);

/**********************************************************************/
/*                             UPDATEDISK                             */
/*                             UPDATEDISK                             */
/*                             UPDATEDISK                             */
/**********************************************************************/

#define IsFirstABlk(blk) (blk->blk.seqnr == 0)
#define IsFirstIBlk(blk) (blk->blk.seqnr == 0)
#define IsEmptyDBlk(blk) (FIRSTENTRY(blk)->next == 0)

/* indicates current state of update */
#define updateok g->updateok

/*
 * Snapshot to disk.
 * --> remove empty blocks
 * --> update freelist
 * --> save reserved blocks at new location, free the old
 *
 * Result FALSE/TRUE done nothing/volume updated
 */
BOOL UpdateDisk (globaldata *g)
{
  struct DateStamp time;
  struct volumedata *volume = g->currentvolume;
  BOOL success;
  ULONG i;

	ENTER("UpdateDisk");

	/*
	 * Do update
	 */
	if (volume && g->dirty && !g->softprotect)
	{
		/*
		 * For performance reasons avoid concurrent access to same physical
		 * device. Note that the lock can be broken safely, it's only used
		 * to avoid excessive seeking due to competing updates.
		 */
		lock_device_unit(g);

		g->uip = TRUE;
		updateok = TRUE;
		UpdateDataCache (g);            /* flush DiskRead DiskWrite cache */

#if VERSION23
		/* make sure rootblockextension is reallocated */
		if (volume->rblkextension)
			MakeBlockDirty ((struct cachedblock *)volume->rblkextension, g);
#endif

		/* commit user space free list */
		UpdateFreeList(g);

		/* remove empty dir, anode, index and superblocks */
		RemoveEmptyDBlocks(volume, g);
		RemoveEmptyABlocks(volume, g);
		RemoveEmptyIBlocks(volume, g);
		RemoveEmptySBlocks(volume, g);

		/* update anode, dir, index and superblocks (not changed by UpdateFreeList) */
		for (i=0; i<=HASHM_DIR; i++)
			updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->dirblks[i]), g);
		for (i=0; i<=HASHM_ANODE; i++)
			updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->anblks[i]), g);
		updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->indexblks), g);
		updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->superblks), g);
#if DELDIR
		updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->deldirblks), g);
#endif

#if VERSION23
		if (volume->rblkextension)
		{
			struct crootblockextension *rext = volume->rblkextension;

			/* reserved roving and anode roving */
			rext->blk.reserved_roving = alloc_data.res_roving;
			rext->blk.rovingbit = alloc_data.rovingbit;
			rext->blk.curranseqnr = andata.curranseqnr;

			/* volume datestamp */
			DateStamp (&time);
			rext->blk.volume_date[0] = (UWORD)time.ds_Days;
			rext->blk.volume_date[1] = (UWORD)time.ds_Minute;
			rext->blk.volume_date[2] = (UWORD)time.ds_Tick;
			rext->blk.datestamp = volume->rootblk->datestamp;

			updateok &= UpdateDirtyBlock ((struct cachedblock *)rext, g);
		}
#endif


		/* commit reserved to be freed list */
		CommitReservedToBeFreed(g);

		/* update bitmap and bitmap index blocks */
		updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->bmblks), g);
		updateok &= UpdateList ((struct cachedblock *)HeadOf(&volume->bmindexblks), g);

		/* update root (MUST be done last) */
		if (updateok)
		{
			RawWrite((UBYTE *)volume->rootblk, volume->rootblk->rblkcluster, ROOTBLOCK, g);
			volume->rootblk->datestamp++;
			volume->rootblockchangeflag = FALSE;

			/* make sure update is really done */
			g->request->iotd_Req.io_Command = CMD_UPDATE;
			DoIO((struct IORequest *)g->request);
			success = TRUE;
		}
		else
		{
			ErrorMsg (AFS_ERROR_UPDATE_FAIL, NULL, g);
			success = FALSE;
		}

		g->uip = FALSE;

		unlock_device_unit(g);
	}
	else
	{
		if (volume && g->dirty && g->softprotect)
			ErrorMsg (AFS_ERROR_UPDATE_FAIL, NULL, g);

		success = FALSE;
	}

	g->dirty = FALSE;
	g->blocks_dirty = 0;

	EXIT("UpdateDisk");
	return success;
}

/*
 * Empty Dirblocks 
 */
static void RemoveEmptyDBlocks(struct volumedata *volume, globaldata *g)
{
  struct cdirblock *blk, *next;
  struct canode anode;
  ULONG previous, i;

	for (i=0; i<=HASHM_DIR; i++)
	{
		for (blk = HeadOf(&volume->dirblks[i]); (next=blk->next); blk=next)
		{
			if (IsEmptyDBlk(blk) && !IsFirstDBlk(blk, g) && !ISLOCKED(blk) )
			{
				previous = GetAnodeOfDBlk(blk, &anode, g);
				RemoveFromAnodeChain(&anode, previous, blk->blk.anodenr, g);
				MinRemove(blk);
				FreeReservedBlock(blk->blocknr, g);
				ResToBeFreed(blk->oldblocknr, g);
				FreeLRU((struct cachedblock *)blk);
			}
		}
	}
}

static ULONG GetAnodeOfDBlk(struct cdirblock *blk, struct canode *anode, globaldata *g)
{
  ULONG prev = 0;

	GetAnode(anode, blk->blk.anodenr, g);
	while (anode->blocknr != blk->blocknr && anode->next)   //anode.next purely safety
	{
		prev = anode->nr;
		GetAnode(anode, anode->next, g);
	}

	return prev;
}

static BOOL IsFirstDBlk(struct cdirblock *blk, globaldata *g)
{
  BOOL first;
  struct canode anode;

	GetAnode(&anode, blk->blk.anodenr, g);
	first = (anode.blocknr == blk->blocknr);

	return first;
}


/*
 * Empty block check
 */
static void RemoveEmptyIBlocks(struct volumedata *volume, globaldata *g)
{
  struct cindexblock *blk, *next;

	for (blk = HeadOf(&volume->indexblks); (next=blk->next); blk=next)
	{
		if (blk->changeflag && !IsFirstIBlk(blk) && IsEmptyIBlk(blk,g) && !ISLOCKED(blk) )
		{
			volume->rootblk->idx.small.indexblocks[blk->blk.seqnr] = 0;
			volume->rootblockchangeflag = TRUE;
			MinRemove(blk);
			FreeReservedBlock(blk->blocknr, g);
			ResToBeFreed(blk->oldblocknr, g);
			FreeLRU((struct cachedblock *)blk);
		}
	}
}

static void RemoveEmptySBlocks(struct volumedata *volume, globaldata *g)
{
  struct cindexblock *blk, *next;

	for (blk = HeadOf(&volume->superblks); (next=blk->next); blk=next)
	{
		if (blk->changeflag && !IsFirstIBlk(blk) && IsEmptyIBlk(blk, g) && !ISLOCKED(blk) )
		{
			volume->rblkextension->blk.superindex[blk->blk.seqnr] = 0;
			volume->rblkextension->changeflag = TRUE;
			MinRemove(blk);
			FreeReservedBlock(blk->blocknr, g);
			ResToBeFreed(blk->oldblocknr, g);
			FreeLRU((struct cachedblock *)blk);
		}
	}
}

static void RemoveEmptyABlocks(struct volumedata *volume, globaldata *g)
{
  struct canodeblock *blk, *next;
  ULONG indexblknr, indexoffset, i;
  struct cindexblock *index;

	for (i=0; i<=HASHM_ANODE; i++)
	{
		for (blk = HeadOf(&volume->anblks[i]); (next=blk->next); blk=next)
		{
			if (blk->changeflag && !IsFirstABlk(blk) && IsEmptyABlk(blk, g) && !ISLOCKED(blk) )
			{
				indexblknr  = ((struct canodeblock *)blk)->blk.seqnr / andata.indexperblock;
				indexoffset = ((struct canodeblock *)blk)->blk.seqnr % andata.indexperblock;

				/* kill the block */
				MinRemove(blk);
				FreeReservedBlock(blk->blocknr, g);
				ResToBeFreed(blk->oldblocknr, g);
				FreeLRU((struct cachedblock *)blk);

				/* and remove the reference (this one should already be in the cache) */
				index = GetIndexBlock(indexblknr, g);
				index->blk.index[indexoffset] = 0;
				index->changeflag = TRUE;
			}
		}
	}
}

static BOOL IsEmptyABlk(struct canodeblock *ablk, globaldata *g)
{
  struct anode *anodes;
  ULONG j; 
  BOOL found = 0;

	/* zoek bezette anode */
	anodes = ablk->blk.nodes;
	for(j=0; j<andata.anodesperblock && !found; j++)
		found |= (anodes[j].blocknr != 0);

	found = !found;
	return found;       /* not found -> empty */
}

static BOOL IsEmptyIBlk(struct cindexblock *blk, globaldata *g)
{
  ULONG *index, i;
  BOOL found;

	index = blk->blk.index;
	for(i=0; i<andata.indexperblock; i++)
	{
		found = index[i] != 0;
		if (found)
			break;
	}

	found = !found;
	return found;
}

static BOOL UpdateList (struct cachedblock *blk, globaldata *g)
{
  ULONG error;
  struct cbitmapblock *blk2;

	if (!updateok)
		return FALSE;

	while (blk->next)
	{
		if (blk->changeflag)
		{
			FreeReservedBlock (blk->oldblocknr, g);
			blk2 = (struct cbitmapblock *)blk;
			blk2->blk.datestamp = blk2->volume->rootblk->datestamp;
			blk->oldblocknr = 0;
			error = RawWrite ((UBYTE *)&blk->data, RESCLUSTER, blk->blocknr, g);
			if (error)
				goto update_error;

			blk->changeflag = FALSE;
		}

		blk=blk->next;
	}

	return TRUE;

  update_error:
	ErrorMsg (AFS_ERROR_UPDATE_FAIL, NULL, g);
	return FALSE;
}

static BOOL UpdateDirtyBlock (struct cachedblock *blk, globaldata *g)
{
  ULONG error;

	if (!updateok)
		return FALSE;

	if (blk->changeflag)
	{
		FreeReservedBlock (blk->oldblocknr, g);
		blk->oldblocknr = 0;
		error = RawWrite ((UBYTE *)&blk->data, RESCLUSTER, blk->blocknr, g);
		if (error)
		{
			ErrorMsg (AFS_ERROR_UPDATE_FAIL, NULL, g);
			return FALSE;
		}
	}

	blk->changeflag = FALSE;
	return TRUE;
}

static void CommitReservedToBeFreed (globaldata *g)
{
	int i;
	for (i=0;i<alloc_data.rtbf_index;i++)
	{
		if (alloc_data.reservedtobefreed[i])
		{
			FreeReservedBlock (alloc_data.reservedtobefreed[i], g);
			alloc_data.reservedtobefreed[i] = 0;
		}
	}

	alloc_data.rtbf_index = 0;
}

/* Check if an update is needed (see atomic.fw)
 * rtbf_threshold indicates how full the rtbf cache is allowed to be,
 * normally RTBF_THRESHOLD, RTBF_POSTPONED_TH for intermediate checks.
 */
void CheckUpdate (ULONG rtbf_threshold, globaldata *g)
{
	if (g->currentvolume &&
	   ((alloc_data.rtbf_index > rtbf_threshold) ||
	    (alloc_data.tobefreed_index > TBF_THRESHOLD) ||
		(g->rootblock->reserved_free < RESFREE_THRESHOLD + 5 + alloc_data.tbf_resneed)))
	{
		UpdateDisk (g);

		if (g->rootblock->reserved_free <= RESFREE_THRESHOLD)
			alloc_data.res_alert = TRUE;
		else
			alloc_data.res_alert = FALSE;
	}
}


/**********************************************************************/
/*                            MAKEBLOCKDIRTY                          */
/*                            MAKEBLOCKDIRTY                          */
/*                            MAKEBLOCKDIRTY                          */
/**********************************************************************/

static void UpdateBlocknr(struct cachedblock *blk, ULONG newblocknr, globaldata *g);
static void UpdateABLK(struct cachedblock *, ULONG, globaldata *);
static void UpdateDBLK(struct cachedblock *, ULONG, globaldata *);
static void UpdateIBLK(struct cachedblock *, ULONG, globaldata *);
static void UpdateSBLK(struct cachedblock *, ULONG, globaldata *);
static void UpdateBMBLK(struct cachedblock *blk, ULONG newblocknr, globaldata *g);
static void UpdateBMIBLK(struct cachedblock *blk, ULONG newblocknr, globaldata *g);
#if VERSION23
static void UpdateRBlkExtension (struct cachedblock *blk, ULONG newblocknr, globaldata *g);
#endif
#if DELDIR
static void UpdateDELDIR (struct cachedblock *blk, ULONG newblocknr, globaldata *g);
#endif

/* --> part of update
 * marks a directory or anodeblock dirty. Nothing happens if it already
 * was dirty. If it wasn't, the block will be reallocated and marked dirty.
 * If the reallocation fails, an error is displayed.
 *
 * result: TRUE = was clean; FALSE: was already dirty
 * 
 * LOCKing the block until next packet proves to be too restrictive,
 * so unlock afterwards.
 */
BOOL MakeBlockDirty (struct cachedblock *blk, globaldata *g)
{
  ULONG blocknr;
  UWORD oldlock;

	if (!(blk->changeflag))
	{
		g->dirty = TRUE;
		oldlock = blk->used;
		LOCK(blk);

		blocknr = AllocReservedBlock (g);
		if (blocknr)
		{
			blk->oldblocknr = blk->blocknr;
			blk->blocknr = blocknr;
			UpdateBlocknr (blk, blocknr, g);
		}
		else
		{
#ifdef BETAVERSION
			ErrorMsg(AFS_BETA_WARNING_2, NULL, g);
#endif
			blk->changeflag = TRUE;
			g->blocks_dirty++;
		}

		blk->used = oldlock;    // unlock block
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void UpdateBlocknr (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
	switch (((UWORD *)blk->data)[0])
	{
		case DBLKID:    /* dirblock */
			UpdateDBLK (blk, newblocknr, g);
			break;

		case ABLKID:    /* anodeblock */
			UpdateABLK (blk, newblocknr, g);
			break;

		case IBLKID:    /* indexblock */
			UpdateIBLK (blk, newblocknr, g);
			break;

		case BMBLKID:   /* bitmapblock */
			UpdateBMBLK (blk, newblocknr, g);
			break;

		case BMIBLKID:  /* bitmapindexblock */
			UpdateBMIBLK (blk, newblocknr, g);
			break;

#if VERSION23
		case EXTENSIONID:   /* rootblockextension */
			UpdateRBlkExtension (blk, newblocknr, g);
			break;
#endif

#if DELDIR
		case DELDIRID:  /* deldir */
			UpdateDELDIR (blk, newblocknr, g);
			break;
#endif

		case SBLKID:	/* superblock */
			UpdateSBLK (blk, newblocknr, g);
			break;
	}
}


static void UpdateDBLK (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
  struct cdirblock *dblk = (struct cdirblock *)blk;
  struct canode anode;
  ULONG oldblocknr = dblk->oldblocknr;

	LOCK(blk);

	/* get old anode (all 1-block anodes) */
	GetAnode (&anode, dblk->blk.anodenr, g);
	while ((anode.blocknr != oldblocknr) && anode.next) //anode.next purely safety
		GetAnode (&anode, anode.next, g);

	/* change it.. */
	if (anode.blocknr != oldblocknr)
	{
		DB(Trace(4, "UpdateDBLK", "anode.blocknr=%ld, dblk->blocknr=%ld\n",
			anode.blocknr, dblk->blocknr));
		ErrorMsg (AFS_ERROR_CACHE_INCONSISTENCY, NULL, g);
	}

	/* This must happen AFTER anode correction, because Update() could be called,
	 * causing trouble (invalid checkpoint: dirblock uptodate, anode not)
	 */
	blk->changeflag = TRUE;
	g->blocks_dirty++;
	anode.blocknr = newblocknr;
	SaveAnode(&anode, anode.nr, g);

	ReHash(blk, g->currentvolume->dirblks, HASHM_DIR);
}

static void UpdateABLK (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
  struct cindexblock *index;
  ULONG indexblknr, indexoffset, temp;

	blk->changeflag = TRUE;
	g->blocks_dirty++;

	temp = ((struct canodeblock *)blk)->blk.seqnr;
	indexblknr  = temp / andata.indexperblock;
	indexoffset = temp % andata.indexperblock;

	/* this one should already be in the cache */
	index = GetIndexBlock(indexblknr, g);
	index->blk.index[indexoffset] = newblocknr;
	MakeBlockDirty ((struct cachedblock *)index, g);
	ReHash(blk, g->currentvolume->anblks, HASHM_ANODE);
}

static void UpdateIBLK(struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
  struct cindexblock *superblk;
  ULONG temp;

	blk->changeflag = TRUE;
	g->blocks_dirty++;
	if (g->supermode)
	{
		temp = divide (((struct cindexblock *)blk)->blk.seqnr, andata.indexperblock);
		superblk = GetSuperBlock (temp /* & 0xffff */, g);
		superblk->blk.index[temp >> 16] = newblocknr;
		MakeBlockDirty ((struct cachedblock *)superblk, g);
	}
	else
	{
		blk->volume->rootblk->idx.small.indexblocks[((struct cindexblock *)blk)->blk.seqnr] = newblocknr;
		blk->volume->rootblockchangeflag = TRUE;
	}
}

static void UpdateSBLK(struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
	blk->changeflag = TRUE;
	g->blocks_dirty++;
	blk->volume->rblkextension->blk.superindex[((struct cindexblock *)blk)->blk.seqnr] = newblocknr;
}

static void UpdateBMBLK (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
  struct cindexblock *indexblock;
  struct cbitmapblock *bmb = (struct cbitmapblock *)blk;
  ULONG temp;

	blk->changeflag = TRUE;
	g->blocks_dirty++;
	temp = divide (bmb->blk.seqnr, andata.indexperblock);
	indexblock = GetBitmapIndex (temp /* & 0xffff */, g);
	indexblock->blk.index[temp >> 16] = newblocknr;
	MakeBlockDirty ((struct cachedblock *)indexblock, g);   /* recursie !! */
}

/* validness of seqnr is checked when block is loaded */
static void UpdateBMIBLK (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
	blk->changeflag = TRUE;
	g->blocks_dirty++;
	
	blk->volume->rootblk->idx.large.bitmapindex[((struct cindexblock *)blk)->blk.seqnr] = newblocknr;
	blk->volume->rootblockchangeflag = TRUE;
}

#if VERSION23
static void UpdateRBlkExtension (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
	blk->changeflag = TRUE;
	g->blocks_dirty++;      /* correct? (extension not in LRU) */
	blk->volume->rootblk->extension = newblocknr;
	blk->volume->rootblockchangeflag = TRUE;
}
#endif

#if DELDIR
static void UpdateDELDIR (struct cachedblock *blk, ULONG newblocknr, globaldata *g)
{
	blk->changeflag = TRUE;
	g->blocks_dirty++;      /* correct? (deldir not in LRU) */
	blk->volume->rblkextension->blk.deldir[((struct cdeldirblock *)blk)->blk.seqnr] = newblocknr;
	MakeBlockDirty((struct cachedblock *)blk->volume->rblkextension, g);
}
#endif

/* Update datestamp (copy from rootblock
 * Call before writing block (lru.c)
 */
void UpdateDatestamp (struct cachedblock *blk, globaldata *g)
{
  struct cdirblock *dblk = (struct cdirblock *)blk;
  struct crootblockextension *rext = (struct crootblockextension *)blk;

	switch (((UWORD *)blk->data)[0])
	{
		case DBLKID:    /* dirblock */
		case ABLKID:    /* anodeblock */
		case IBLKID:    /* indexblock */
		case BMBLKID:   /* bitmapblock */
		case BMIBLKID:  /* bitmapindexblock */
		case DELDIRID:  /* deldir */
		case SBLKID:	/* superblock */
			dblk->blk.datestamp = g->currentvolume->rootblk->datestamp;
			break;

		case EXTENSIONID:   /* rootblockextension */
			rext->blk.datestamp = g->currentvolume->rootblk->datestamp;
			break;
	}
}
