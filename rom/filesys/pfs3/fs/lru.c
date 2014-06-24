/* $Id$ */
/* $Log: lru.c $
 * Revision 10.21  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 10.20  1998/09/27  11:26:37  Michiel
 * ErrorMsg param
 *
 * Revision 10.19  1998/06/12  21:30:29  Michiel
 * Fixed bug 116: FreeResBlock
 *
 * Revision 10.18  1998/05/29  19:31:18  Michiel
 * fixed bug 108
 *
 * Revision 10.17  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 10.16  1995/11/15  15:52:10  Michiel
 * UpdateLE and UpdateLE_exa now call MakeLRU
 *
 * Revision 10.15  1995/11/07  15:00:58  Michiel
 * ResToBeFreed added
 * AllocLRU() changed for atomic update
 *
 * Revision 10.14  1995/10/04  14:05:41  Michiel
 * checking buffermemory against memorymask
 *
 * Revision 10.13  1995/09/01  11:22:53  Michiel
 * ErrorMsg adaption (see disk.c and volume.c)
 *
 * Revision 10.12  1995/07/28  07:58:58  Michiel
 * using SPECIAL_FLUSHED, needed for UpdateLE to recognize flushed
 * entries against deldirentries
 *
 * Revision 10.11  1995/07/21  06:53:54  Michiel
 * DELDIR adaptions
 *
 * Revision 10.10  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 10.9  1995/06/23  17:27:53  Michiel
 * MIN_BUFFERS <= number of LRU buffers <= MAX_BUFFERS
 *
 * Revision 10.8  1995/02/28  18:35:32  Michiel
 * '12.6' bugfix in AllocLRU (direct write if dirty while updating)
 *
 * Revision 10.7  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 10.6  1995/02/01  16:04:00  Michiel
 * UpdateLE_exa enforcer hit fixed
 *
 * Revision 10.5  1995/01/29  07:34:57  Michiel
 * Minbuffers now is a minimum, no longer an offset
 * ChechCache routine using hash table added.
 *
 * Revision 10.4  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 10.3  1994/11/15  17:48:34  Michiel
 * Flush block / UpdateReference bug fixed
 *
 * Revision 10.2  1994/10/27  11:32:46  Michiel
 * *** empty log message ***
 *
 * Revision 10.1  1994/10/24  11:16:28  Michiel
 * first RCS revision
 * */

#define __USE_SYSBASE

#include <exec/memory.h>
#include <exec/lists.h>
#include <dos/filehandler.h>
#include "debug.h"
#include <math.h>
#include <clib/alib_protos.h>

#include "blocks.h"
#include "struct.h"
#include "volume_protos.h"
#include "lru_protos.h"
#include "directory_protos.h"
#include "update_protos.h"
#include "disk_protos.h"
#include "allocation_protos.h"


/*
 * prototypes
 */



/* Allocate LRU queue
*/
BOOL InitLRU (globaldata *g)
{
  int i;
  UBYTE *array;

	ENTER("InitLRU");

	NewList((struct List *)&g->glob_lrudata.LRUqueue);
	NewList((struct List *)&g->glob_lrudata.LRUpool);

	i = g->dosenvec->de_NumBuffers;

	/* sanity checks. If HDToolbox default of 30, then 150,
	 * otherwise round in range 70 -- 600
	 */
	if (i==30) i=150;
	if (i<MIN_BUFFERS) i = MIN_BUFFERS;
	if (i>MAX_BUFFERS) i = MAX_BUFFERS;
	g->dosenvec->de_NumBuffers = g->glob_lrudata.poolsize = i;
	g->uip = FALSE;
	g->locknr = 1;

	if (!(g->glob_lrudata.LRUarray = AllocVec(SIZEOF_LRUBLOCK * g->glob_lrudata.poolsize,
		g->dosenvec->de_BufMemType | MEMF_CLEAR)))
		return FALSE;

	/* check memory against mask */
	if (((SIPTR)g->glob_lrudata.LRUarray) & ~g->dosenvec->de_Mask)
		ErrorMsg (AFS_WARNING_MEMORY_MASK, NULL, g);

	array = (UBYTE *)g->glob_lrudata.LRUarray;
	for(i=0;i<g->glob_lrudata.poolsize;i++)
		MinAddHead(&g->glob_lrudata.LRUpool, array + i*SIZEOF_LRUBLOCK);

	return TRUE;
}



/* Allocate a block from the LRU chain and make
** it current LRU.
** Returns NULL if none available
*/
struct cachedblock *AllocLRU (globaldata *g)
{
  struct lru_cachedblock *lrunode;
  ULONG error;

	ENTER("AllocLRU");

	/* Use free block from pool or flush lru unused
	** block (there MUST be one!)
	*/
//  retry:
	if (IsMinListEmpty(&g->glob_lrudata.LRUpool))
	{
		for (lrunode = (struct lru_cachedblock *)g->glob_lrudata.LRUqueue.mlh_TailPred; lrunode->prev; lrunode = lrunode->prev)
		{
			/* skip locked blocks */
			if (ISLOCKED(&lrunode->cblk))
				continue;

			if (lrunode->cblk.changeflag)
			{
				DB(Trace(1,"AllocLRU","ResToBeFreed %lx\n",&lrunode->cblk));
				ResToBeFreed(lrunode->cblk.oldblocknr, g);
				UpdateDatestamp(&lrunode->cblk, g);
				error = RawWrite ((UBYTE *)&lrunode->cblk.data, RESCLUSTER, lrunode->cblk.blocknr, g);
				if (error) {
					ULONG args[2] = { lrunode->cblk.blocknr, error };
					ErrorMsg (AFS_ERROR_LRU_UPDATE_FAIL, args, g);
				}
			}

			FlushBlock(&lrunode->cblk, g);
			goto ready;
		}
	}
	else
	{
		lrunode = HeadOf(&g->glob_lrudata.LRUpool);
		goto ready;
	}

	/* No suitable block found -> we are in trouble */
	NormalErrorMsg (AFS_ERROR_OUT_OF_BUFFERS, NULL, 1);
	return NULL;

  ready:
	MinRemove(lrunode);
	MinAddHead(&g->glob_lrudata.LRUqueue, lrunode);

	DB(Trace(1,"AllocLRU","Allocated block %lx\n", &lrunode->cblk));

	//  LOCK(&lrunode->cblk);
	return &lrunode->cblk;
}


/* Adds a block to the ReservedToBeFreedCache
 */
void ResToBeFreed(ULONG blocknr, globaldata *g)
{
	/* bug 00116, 13 June 1998 */
	if (blocknr)
	{
		/* check if cache has space left */
		if (alloc_data.rtbf_index < alloc_data.rtbf_size)
		{
			alloc_data.reservedtobefreed[alloc_data.rtbf_index++] = blocknr;
		}
		else
		{
			/* reallocate cache */
			ULONG newsize = alloc_data.rtbf_size ? alloc_data.rtbf_size * 2 : RTBF_CACHE_SIZE;
			ULONG *newbuffer = AllocMem(sizeof(*newbuffer) * newsize, MEMF_ANY);
			if (newbuffer)
			{
				if (alloc_data.reservedtobefreed)
				{
					CopyMem(alloc_data.reservedtobefreed, newbuffer, sizeof(*newbuffer) * alloc_data.rtbf_index);
					FreeMem(alloc_data.reservedtobefreed, sizeof(*newbuffer) * alloc_data.rtbf_size);
				}
				alloc_data.reservedtobefreed = newbuffer;
				alloc_data.rtbf_size = newsize;
				alloc_data.reservedtobefreed[alloc_data.rtbf_index++] = blocknr;
				return;
			}

			/* this should never happen */
			DB(Trace(10,"ResToBeFreed","reserved to be freed cache full\n"));
#ifdef BETAVERSION
			ErrorMsg (AFS_BETA_WARNING_1, NULL, g);
#endif
			/* hope nobody allocates this block before the disk has been
			 * updated
			 */
			FreeReservedBlock (blocknr, g);
		}
	}
}


/* Makes a cached block ready for reuse:
** - Remove from queue
** - (dirblock) Decouple all references to the block
** - wipe memory
** NOTE: NOT REMOVED FROM LRU!
*/
void FlushBlock (struct cachedblock *block, globaldata *g)
{
  lockentry_t *le;

	DB(Trace(10,"FlushBlock","Flushing block %lx\n", block->blocknr));

	/* remove block from blockqueue */
	MinRemove(block);

	/* decouple references */
	if (IsDirBlock(block))
	{
		/* check fileinfo references */
		for (le = (lockentry_t *)HeadOf(&block->volume->fileentries); le->le.next; le = (lockentry_t *)le->le.next)
		{
			/* only dirs and files have fileinfos that need to be updated,
			** but the volume * pointer of volumeinfos never points to
			** a cached block, so the type != ETF_VOLUME check is not
			** necessary. Just check the dirblockpointer
			*/
			if (le->le.info.file.dirblock == (struct cdirblock *)block)
			{
				le->le.dirblocknr = block->blocknr;
				le->le.dirblockoffset = (UBYTE *)le->le.info.file.direntry - (UBYTE *)block;
#if DELDIR
				le->le.info.deldir.special = SPECIAL_FLUSHED;  /* flushed reference */
#else
				le->le.info.direntry = NULL;
#endif
				le->le.info.file.dirblock = NULL;
			}

			/* exnext references */
			if (le->le.type.flags.dir && le->nextentry.dirblock == (struct cdirblock *)block)
			{
				le->nextdirblocknr = block->blocknr;
				le->nextdirblockoffset = (UBYTE *)le->nextentry.direntry - (UBYTE *)block;
#if DELDIR
				le->nextentry.direntry = (struct direntry *)SPECIAL_FLUSHED;
#else
				le->nextentry.direntry = NULL;
#endif
				le->nextentry.dirblock = NULL;
			}
		}
	}

	/* wipe memory */
	memset(block, 0, SIZEOF_CACHEDBLOCK);
}

/* updates references of listentries to dirblock
*/
void UpdateReference (ULONG blocknr, struct cdirblock *blk, globaldata *g)
{
  lockentry_t *le;

	DB(Trace(1,"UpdateReference","block %lx\n", blocknr));

	for (le = (lockentry_t *)HeadOf(&blk->volume->fileentries); le->le.next; le = (lockentry_t *)le->le.next)
	{
		/* ignoring the fact that not all objectinfos are fileinfos, but the
		** 'volumeinfo.volume' and 'deldirinfo.deldir' fields never are NULL anyway, so ...
		** maybe better to check for SPECIAL_FLUSHED
		*/
		if (le->le.info.file.dirblock == NULL && le->le.dirblocknr == blocknr)
		{
			le->le.info.file.dirblock = blk;
			le->le.info.file.direntry = (struct direntry *)((UBYTE *)blk + le->le.dirblockoffset);
			le->le.dirblocknr =
			le->le.dirblockoffset = 0;
		}

		/* exnext references */
		if (le->le.type.flags.dir && le->nextdirblocknr == blocknr)
		{
			le->nextentry.dirblock = blk;
			le->nextentry.direntry = (struct direntry *)((UBYTE *)blk + le->nextdirblockoffset);
			le->nextdirblocknr =
			le->nextdirblockoffset = 0;
		}
	}
}

/* Updates objectinfo of a listentry (if necessary)
 * This function only reloads the flushed directory block referred to. The
 * load directory block routine will actually restore the reference.
 */
void UpdateLE (listentry_t *le, globaldata *g)
{
	//DB(Trace(1,"UpdateLE","Listentry %lx\n", le));

	/* don't update volumeentries or deldirs!! */
#if DELDIR
	if (!le || le->info.deldir.special <= SPECIAL_DELFILE)
#else
	if (!le || IsVolumeEntry(le))
#endif
		return;

	if (le->dirblocknr)
		LoadDirBlock (le->dirblocknr, g);

	MakeLRU (le->info.file.dirblock);
	LOCK(le->info.file.dirblock);
}

void UpdateLE_exa (lockentry_t *le, globaldata *g)
{
	//DB(Trace(1,"UpdateLE_exa","LE %lx\n", le));

	if (!le) return;

	if (le->le.type.flags.dir)
	{
#if DELDIR
		if (IsDelDir(le->le.info))
			return;
#endif

		if (le->nextdirblocknr)
			LoadDirBlock (le->nextdirblocknr, g);

		if (le->nextentry.dirblock)
		{
			MakeLRU (le->nextentry.dirblock);
			LOCK(le->nextentry.dirblock);
		}
	}
}

/*
 * Cache check ..
 * The 'mask' is used as a fast modulo operator for the hash table size.
 */

struct cachedblock *CheckCache (struct MinList *list, UWORD mask, ULONG blocknr, globaldata *g)
{
  struct cachedblock *block;

	for (block = HeadOf(&list[(blocknr/2)&mask]); block->next; block=block->next)
	{
		if (block->blocknr == blocknr)
		{
			MakeLRU(block);
			return block;
		}
	}

	return NULL;
}

