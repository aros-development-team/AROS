/*
 * File:
 *
 *  allocation.c
 *
 * Description:
 *
 *  Description
 *
 * Revision History:
 *
 *  $Id$
 *
 *  $Log: allocation.c $
 * Revision 14.8  1999/09/11  17:05:14  Michiel
 * bugfix version 18.4
 *
 * Revision 14.7  1999/05/14  11:31:34  Michiel
 * Long filename support implemented; bugfixes
 *
 * Revision 14.6  1999/03/09  10:35:13  Michiel
 * 00114: reserved roving op bit resolutie
 *
 * Revision 14.5  1998/09/27  11:26:37  Michiel
 * bugfixes
 * ErrorMsg param
 *
 * Revision 14.4  1998/09/03  07:12:14  Michiel
 * versie 17.4
 * bugfixes 118, 121, 123 and superindexblocks and td64 support
 *
 * Revision 14.3  1997/03/03  22:04:04  Michiel
 * Release 16.21
 *
 * Revision 14.2  1995/12/21  12:02:28  Michiel
 * Using reserved roving from rootblock extension
 * FreeAllAC() killed
 *
 * Revision 14.1  1995/11/15  15:10:44  Michiel
 * Postponed operation implemented and bugfixed
 * (FreeBlocksAC, RestoreAnodeChain)
 *
 * Revision 13.4  1995/11/07  14:48:30  Michiel
 * atomic update stuff
 *
 * Revision 13.3  1995/10/20  10:11:52  Michiel
 * Anode reserved area adaptions (16.3)
 *
 * Revision 13.2  1995/10/04  17:15:52  Michiel
 * preallocation for extended files implemented (in
 * AllocateBlocksAC)
 *
 * Revision 13.1  1995/10/03  10:47:40  Michiel
 * merged develop tree: anodecache
 *
 * Revision 12.11  1995/10/03  09:56:57  Michiel
 * AllocReservedBlock --> g->dirty
 *
 * Revision 12.10  1995/08/21  18:31:24  Michiel
 * fixed deldir FreeBlocks problem: disk was not dirty after
 * full tobefreed cache
 *
 * Revision 12.9  1995/08/21  04:17:36  Michiel
 * fixed deldir related bug in FreeBlocks
 *
 * Revision 12.8  1995/08/04  04:12:43  Michiel
 * extra CUTDOWN protection
 *
 * Revision 12.7  1995/07/21  06:51:11  Michiel
 * Adapted FreeBlocks() to allow for the anodes to be kept (needed for deldir)
 *
 * Revision 12.6  1995/07/11  17:29:31  Michiel
 * ErrorMsg () calls use messages.c variables now.
 *
 * Revision 12.5  1995/02/28  18:32:06  Michiel
 * UWORD optimize
 * AllocReservedBlockSave update
 *
 * Revision 12.4  1995/02/15  16:43:39  Michiel
 * Release version
 * Using new headers (struct.h & blocks.h)
 *
 * Revision 12.3  1995/01/29  07:34:57  Michiel
 * Small UpdateFreeList improvement
 * Raw res read/write and LOCK update
 *
 * Revision 12.2  1995/01/18  04:29:34  Michiel
 * Bugfixes. Now ready for beta release.
 *
 * Revision 12.1  1995/01/08  15:14:15  Michiel
 * Compiled
 *
 * Revision 11.2  1994/12/30  08:41:20  Michiel
 * Fixed and completed
 * Compilable
 *
 * Revision 11.1  1994/12/28  09:41:29  Michiel
 * Now using bitmaps for allocation
 * also using allocation.h now
 *
 */

/*
 * General includes
 */
#define __USE_SYSBASE

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/devices.h>
#include <exec/io.h>
#include <dos/filehandler.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <string.h>
#include "debug.h"
#include <limits.h>
#include <math.h>
#include "ass_protos.h"

/*
 * Own includes
 */
#include "blocks.h"
#include "struct.h"
#include "allocation_protos.h"
#include "update_protos.h"
#include "anodes_protos.h"
#include "disk_protos.h"
#include "lru_protos.h"
#include "volume_protos.h"

/*
 * Contents
 */
static struct cindexblock *NewBitmapIndexBlock(UWORD , globaldata * );

/*
 * Get bitmapblock seqnr
 */

/*
 * InitAllocation
 *
 * currentvolume has to be ok.
 */
VOID InitAllocation (struct volumedata *volume, globaldata *g)
{
  ULONG t;
  rootblock_t *rootblock = volume->rootblk;

	if (g->harddiskmode)
	{
		alloc_data.clean_blocksfree = rootblock->blocksfree;
		alloc_data.alloc_available = rootblock->blocksfree - rootblock->alwaysfree;
		alloc_data.longsperbmb = LONGS_PER_BMB;

		t = (volume->numblocks - (rootblock->lastreserved + 1) + 31)/32;
		t = (t+LONGS_PER_BMB-1)/LONGS_PER_BMB;
		alloc_data.no_bmb = t;
		alloc_data.bitmapstart = rootblock->lastreserved + 1;
		memset (alloc_data.tobefreed, 0, TBF_CACHE_SIZE*2*sizeof(ULONG));
		alloc_data.tobefreed_index = 0;
		alloc_data.tbf_resneed = 0;
		alloc_data.res_bitmap = (bitmapblock_t *)(rootblock+1);     /* bitmap directly behind rootblock */

		if (volume->rblkextension)
		{
			if (!(rootblock->options & MODE_EXTROVING))
			{
				rootblock->options |= MODE_EXTROVING;
				g->dirty = TRUE;
				volume->rblkextension->blk.reserved_roving *= 32;
			}
			alloc_data.res_roving = volume->rblkextension->blk.reserved_roving;
			alloc_data.rovingbit = volume->rblkextension->blk.rovingbit;
		} 
		else
		{
			alloc_data.res_roving = 0;
			alloc_data.rovingbit = 0;
		}

		alloc_data.numreserved = (rootblock->lastreserved - rootblock->firstreserved + 1)/(volume->rescluster);
		alloc_data.reservedtobefreed = NULL;
		alloc_data.rtbf_size = 0;
		alloc_data.rtbf_index = 0;
		alloc_data.res_alert = 0;
	}
	else
	{
		ErrorMsg (AFS_ERROR_ANODE_INIT, NULL, g);
	}
}


/*
 * AllocateBlocks
 */
BOOL AllocateBlocks (ULONG anodenr, ULONG size, globaldata *g)
{
  struct anodechain *ac;
  BOOL success;

	if (!(ac = GetAnodeChain (anodenr, g)))
		return DOSFALSE;

	success = AllocateBlocksAC (ac, size, NULL, g);
	DetachAnodeChain (ac, g);
	return success;
}

/*
 * AllocateBlocksAC
 * Allocate blocks to end of (cached) anodechain.
 * If ref != NULL then online directory update enabled.
 * Make sure the state is valid before you call this function!
 * Returns success
 * if FAIL, then a part of the needed blocks could already have been allocated
 */
BOOL AllocateBlocksAC (struct anodechain *achain, ULONG size,
	struct fileinfo *ref, globaldata *g)
{
  ULONG nr, field, i, j, blocknr, blocksdone = 0;
  ULONG extra, oldfilesize = 0;
  UWORD bmseqnr, bmoffset, oldlocknr;
  cbitmapblock_t *bitmap;
  struct anodechainnode *chnode;
  struct volumedata *vol = g->currentvolume;
  BOOL extend = FALSE, updateroving = TRUE;


	ENTER("AllocateBlocksAC");

	/* Check if allocation possible */
	if(alloc_data.alloc_available < size)
		return DOSFALSE;
	
	/* check for sufficient clean freespace (freespace that doesn't overlap
	 * with the current state on disk)
	 */
	if (alloc_data.clean_blocksfree < size)
		UpdateDisk (g);

#if VERSION23
	/* remember filesize in order to be able to cancel */
	if (ref)
		oldfilesize = ref->direntry->size;
#endif

	/* count number of fragments and decide on fileextend preallocation
	 * get anode to expand
	 */
	chnode = &achain->head;
	for (i=0; chnode->next; i++)
		chnode = chnode->next;

	extra = min (256, i*8);
	if (chnode->an.blocknr && (chnode->an.blocknr != -1))
	{
		i = chnode->an.blocknr + chnode->an.clustersize - alloc_data.bitmapstart;
		nr = i/32;
		i %= 32;
		j = 1<<(31-i);
		bmseqnr = nr/alloc_data.longsperbmb;
		bmoffset = nr%alloc_data.longsperbmb;
		bitmap = GetBitmapBlock (bmseqnr, g);
		field = bitmap->blk.bitmap[bmoffset];

		/* block directly behind file free ? */
		if (field & j)
		{
			extend = true;

			/* if the position we want to allocate does not corresponds to the
			 * rovingpointer, the rovingpointer should not be updated
			 */
			if (nr != g->rootblock->roving_ptr)
				updateroving = false;
		}
	}
	

	/* Get bitmap to allocate from */
	if (!extend)
	{
		nr = g->rootblock->roving_ptr;
		bmseqnr = nr/alloc_data.longsperbmb;
		bmoffset = nr%alloc_data.longsperbmb;
		i = alloc_data.rovingbit;
		j = 1<<(31-i);
	}

	/* Allocate */
	while (size)
	{
		/* scan all bitmapblocks */
		bitmap = GetBitmapBlock(bmseqnr, g);
		oldlocknr = bitmap->used;

		/* find all empty fields */
		while (bmoffset < alloc_data.longsperbmb)
		{
			field = bitmap->blk.bitmap[bmoffset];
			if (field)
			{
				/* take all empty bits */
				for ( ; i<32; j>>=1, i++)
				{
					if (field & j)
					{
						/* block is available, calc blocknr */
						blocknr = (bmseqnr*alloc_data.longsperbmb + bmoffset)*32 + i + 
								alloc_data.bitmapstart;

						/* check in range */
						if (blocknr >= vol->numblocks)
						{
							bmoffset = alloc_data.longsperbmb;
							continue;
						}

						/* take block */
						else
						{
							/* uninitialized anode */
							if (chnode->an.blocknr == -1)
							{
								chnode->an.blocknr = blocknr;
								chnode->an.clustersize = 0;
								chnode->an.next = 0;
							}
								
							/* check blockconnect */
							else if (!(chnode->an.blocknr + chnode->an.clustersize == blocknr))
							{
							  ULONG anodenr;

								LOCK(bitmap);

								if (!(chnode->next = AllocMemP (sizeof(struct anodechainnode), g)))
								{
#if VERSION23
									if (ref)
									{
										ref->direntry->size = oldfilesize;
										MakeBlockDirty ((struct cachedblock *)ref->dirblock, g);
									}
#endif
									/* undo allocation so far */
									FreeBlocksAC (achain, blocksdone, freeanodes, g);
									return DOSFALSE;
								}

								anodenr = AllocAnode (chnode->an.nr, g);    /* should not go wrong! */
								chnode->an.next = anodenr; 
								SaveAnode (&chnode->an, chnode->an.nr, g);
								chnode = chnode->next;
								chnode->an.nr = anodenr;
								chnode->an.blocknr = blocknr;
								chnode->an.clustersize = 0;
								chnode->an.next = 0;
							}

							bitmap->blk.bitmap[bmoffset] &= ~j;     /* remove block from freelist */
							chnode->an.clustersize++;           /* to file                  */
							MakeBlockDirty ((struct cachedblock *)bitmap, g);

							/* update counters */
							alloc_data.clean_blocksfree--;
							alloc_data.alloc_available--;
							blocksdone++;

							/* update reference */
							if (ref)
							{
								ref->direntry->size += BLOCKSIZE;
								if (IsUpdateNeeded(RTBF_POSTPONED_TH))
								{
									/* make state valid and update disk */
									MakeBlockDirty ((struct cachedblock *)ref->dirblock, g);
									SaveAnode (&chnode->an, chnode->an.nr, g);
									UpdateDisk (g);

									/* abort if running out of reserved blocks */
									if (g->rootblock->reserved_free <= RESFREE_THRESHOLD)
									{
#if VERSION23
										ref->direntry->size = oldfilesize;
										MakeBlockDirty ((struct cachedblock *)ref->dirblock, g);
#endif
										FreeBlocksAC (achain, blocksdone, freeanodes, g);
										return DOSFALSE;
									}
								}
							}


							if (!--size)
								goto alloc_end;
						}
					}
				}

				i = 0;
				j = 1<<31;
			}
			bmoffset++;
		}

		bitmap->used = oldlocknr;

		/* get ready for next block */
		bmseqnr = (bmseqnr+1)%(alloc_data.no_bmb);
		bmoffset = 0;
	}
	
alloc_end:

	/* finish by saving anode and updating roving ptr */
	SaveAnode (&chnode->an, chnode->an.nr, g);

	/* add fileextension preallocation */
	if (updateroving)
	{
		if (extend)
		{
			i += extra;
			alloc_data.rovingbit = i%32;
			bmoffset += i/32;
			if (bmoffset > alloc_data.longsperbmb)
			{
				bmoffset -= alloc_data.longsperbmb;
				bmseqnr = (bmseqnr+1)%(alloc_data.no_bmb);
			}
		}
		else
		{
			alloc_data.rovingbit = i;
		}

		g->rootblock->roving_ptr = bmseqnr*alloc_data.longsperbmb + bmoffset;
	}

	EXIT ("AllocateBlocksAC");
	return DOSTRUE;
}


/*
 * Free all blocks in an anodechain? Use FreeBlocksAC(achain,ULONG_MAX,freetype,g)
 */

/*
 * Frees blocks allocated with AllocateBlocks. Freed blocks are added
 * to tobefreed list, and are not actually freed until UpdateFreeList
 * is called. Frees from END of anodelist.
 *
 * 'freetype' specifies if the anodes involved should be freed (freeanodes) or
 * not (keepanodes). In keepanodes mode the whole file (size >= filesize) should
 * be deleted, since otherwise the anodes cannot consistently be kept (dual
 * definition). In freeanodes mode the leading anode is not freed.
 *
 * VERSION23: uses tobedone fields. Because freeing blocks is idem-potent, fully
 * repeating an interrupted operation after reboot is ok. The blocks should not
 * be added to the blocksfree counter twice, however (see DoPostoned())
 * 
 * -> all references that indicate the freed blocks must have been
 *    done (atomically).
 */
static void RestoreAnodeChain (struct anodechain *achain, BOOL empty, struct anodechainnode *tail, globaldata *g);
VOID FreeBlocksAC (struct anodechain *achain, ULONG size, enum freeblocktype freetype, globaldata *g)
{
  struct anodechainnode *chnode, *tail;
  ULONG freeing;
  UWORD i, t = 0;
  BOOL empty = FALSE;
  struct crootblockextension *rext;
  ULONG blocksdone = 0;

	ENTER("FreeBlocksAC");

	i = alloc_data.tobefreed_index;
	tail = NULL;

	/* store operation tobedone */
	rext = g->currentvolume->rblkextension;
	if (rext)
	{
		if (freetype == keepanodes)
			rext->blk.tobedone.operation_id = PP_FREEBLOCKS_KEEP;
		else
			rext->blk.tobedone.operation_id = PP_FREEBLOCKS_FREE;

		rext->blk.tobedone.argument1 = achain->head.an.nr;
		rext->blk.tobedone.argument2 = size;
		rext->blk.tobedone.argument3 = 0;       /* blocks done (FREEBLOCKS_KEEP) */
		MakeBlockDirty ((struct cachedblock *)rext, g);
	}

	/* check if tobefreedcache is sufficiently large,
	 * otherwise updatedisk
	 */
	for (chnode = &achain->head; chnode->next; t++)
		chnode = chnode->next;

	if ((i>0) && (t+i >= TBF_CACHE_SIZE-1))
	{
		UpdateDisk (g);
		i = alloc_data.tobefreed_index;
	}

	if (size)
		goto l1;

	/* reverse order freeloop */
	while (size && !empty)
	{
		/* Get chainnode to free from */
		chnode = &achain->head;
		while (chnode->next != tail)
			chnode = chnode->next;

l1:     /* get blocks to free */
		if (chnode->an.clustersize <= size)
		{
			freeing = chnode->an.clustersize;
			chnode->an.clustersize = 0;

			tail = chnode;
			empty = tail == &achain->head;
		}
		else
		{
			/* anode is partially freed;
			 * should only be possible in freeanodes mode 
			 */
			freeing = size;
			chnode->an.clustersize -= size;
			chnode->an.next = 0;
			if (freetype == freeanodes)
				SaveAnode (&chnode->an, chnode->an.nr, g);
		}

		/* and put them in the tobefreed list */
		if (freeing)
		{
			alloc_data.tobefreed[i][TBF_BLOCKNR] = chnode->an.blocknr + chnode->an.clustersize;
			alloc_data.tobefreed[i++][TBF_SIZE] = freeing;
			alloc_data.tbf_resneed += 3 + freeing/(32*alloc_data.longsperbmb);
			alloc_data.alloc_available += freeing;
			size -= freeing;
			blocksdone += freeing;

			/* free anode if it is empty, we're supposed to and it is not the head */
			if (!empty && freetype == freeanodes && !chnode->an.clustersize)
			{
				FreeAnode (chnode->an.nr, g);
				FreeMemP (chnode, g);
			}
		}

		/* check if intermediate update is needed 
		 * (tobefreed cache full, low on reserved blocks etc)
		 */
		if (i>=TBF_CACHE_SIZE || IsUpdateNeeded(RTBF_POSTPONED_TH))
		{
			alloc_data.tobefreed_index = i;
			g->dirty = DOSTRUE;
			if (rext && freetype == freeanodes)
			{
				/* make anodechain consistent */
				RestoreAnodeChain (achain, empty, tail, g);
				tail = NULL;

				/* postponed op: finish operation later */
				rext->blk.tobedone.argument2 = size;
			}
			else
				/* postponed op: repeat operation later, but don't increase blocks free twice */
				rext->blk.tobedone.argument3 = blocksdone;

			MakeBlockDirty ((struct cachedblock *)rext, g);
			UpdateDisk (g);
			i = alloc_data.tobefreed_index;
		}
	}

	/* restore anode chain (both cached and on disk) */
	if (freetype == freeanodes)
		RestoreAnodeChain (achain, empty, tail, g);

	/* cancel posponed operation */
	if (rext)
	{
		rext->blk.tobedone.operation_id = 0;
		rext->blk.tobedone.argument1 = 0;
		rext->blk.tobedone.argument2 = 0;
		rext->blk.tobedone.argument3 = 0;
		MakeBlockDirty ((struct cachedblock *)rext, g);
	}

	/* update tobefreed index */
	g->dirty = DOSTRUE;
	alloc_data.tobefreed_index = i;

	EXIT ("FreeBlocksAC");
}


/* local function of FreeBlocksAC
 * restore anodechain (freeanode mode only)
 */
static void RestoreAnodeChain (struct anodechain *achain, BOOL empty,
	struct anodechainnode *tail, globaldata *g)
{
  struct anodechainnode *chnode;

	if (empty)
	{
		achain->head.next = NULL;
		achain->head.an.clustersize = 0;
		achain->head.an.blocknr = ~0L;
		achain->head.an.next = 0;
		SaveAnode (&achain->head.an, achain->head.an.nr, g);
	}
	else
	{
		chnode = &achain->head;
		while (chnode->next != tail)
			chnode = chnode->next;

		chnode->next = NULL;
		chnode->an.next = 0;
		SaveAnode (&chnode->an, chnode->an.nr, g);
	}
}

/*
 * Update bitmap
 */
void UpdateFreeList (globaldata *g)
{
  cbitmapblock_t *bitmap = 0;
  UWORD i;
  ULONG longnr, blocknr, bmseqnr, newbmseqnr, bmoffset, bitnr;

	/* sort the free list */
	// not done right now

	/* free all blocks in list */
	bmseqnr = ~0;
	for (i=0; i<alloc_data.tobefreed_index; i++)
	{
		for ( blocknr = alloc_data.tobefreed[i][TBF_BLOCKNR];
			  blocknr < alloc_data.tobefreed[i][TBF_SIZE] + alloc_data.tobefreed[i][TBF_BLOCKNR];
			  blocknr++ )
		{
			/* now free block blocknr */
			bitnr = blocknr - alloc_data.bitmapstart;
			longnr = bitnr/32;
			newbmseqnr = longnr/alloc_data.longsperbmb;
			bmoffset = longnr%alloc_data.longsperbmb;
			if(newbmseqnr != bmseqnr)
			{
				bmseqnr = newbmseqnr;
				bitmap = GetBitmapBlock (bmseqnr, g);
			}
			bitmap->blk.bitmap[bmoffset] |= (1<<(31-(bitnr%32)));
			MakeBlockDirty ((struct cachedblock *)bitmap, g);
		}

		alloc_data.clean_blocksfree += alloc_data.tobefreed[i][TBF_SIZE];
	}

	/* update global data */
	/* alloc_data.alloc_available should already be equal blocksfree - alwaysfree */
	alloc_data.tobefreed_index = 0;
	alloc_data.tbf_resneed = 0;
	g->rootblock->blocksfree = alloc_data.clean_blocksfree;
	g->currentvolume->rootblockchangeflag = TRUE;
}

/*
 * AllocReservedBlock
 */

ULONG AllocReservedBlock (globaldata *g)
{
  struct volumedata *vol = g->currentvolume;
  ULONG *bitmap = alloc_data.res_bitmap->bitmap;
  ULONG *free = &g->rootblock->reserved_free;
  ULONG blocknr;
  LONG i, j;

  ENTER("AllocReservedBlock");

  /* Check if allocation possible 
   * (really necessary?)
   */
  if (*free == 0)
	return 0;

  j = 31 - alloc_data.res_roving % 32;
  for (i = alloc_data.res_roving / 32; i <= (alloc_data.numreserved/32); i++, j=31)
  {
	if (bitmap[i] != 0)
	{
	  ULONG field = bitmap[i];
	  for ( ;j >= 0; j--)
	  {
		if (field & (1 << j))
		{
		  blocknr = g->rootblock->firstreserved + (i*32+(31-j))*vol->rescluster;
		  if (blocknr <= g->rootblock->lastreserved) 
		  {
			bitmap[i] &= ~(1 << j);
			g->currentvolume->rootblockchangeflag = TRUE;
			g->dirty = TRUE;
			(*free)--;
			alloc_data.res_roving = 32*i + (31 - j);
			DB(Trace(10,"AllocReservedBlock","allocated %ld\n", blocknr));
			return blocknr;
		  }
		}
	  }
	}
  }

  /* end of bitmap reached. Reset roving pointer and try again 
  */
  if (alloc_data.res_roving)
  {
	alloc_data.res_roving = 0;
	return AllocReservedBlock (g);
  }
  else
	return 0;

  EXIT("AllocReservedBlock");
}


#if 0
/*
 * Allocates a reserved block while keeping at least 
 * RESERVED_BUFFER reserved blocks free
 */
ULONG AllocReservedBlockSave (globaldata *g)
{
  ULONG free = g->rootblock->reserved_free;
  ULONG blocknr;

	/* dirty blocks claim two blocks, so */
	if (free + g->blocks_dirty < RESERVED_BUFFER)
		return NULL;

	if (!(blocknr = AllocReservedBlock(g)))
	{
		UpdateDisk(g);
		blocknr = AllocReservedBlock(g);
	}

	return blocknr;
}
#endif

/*
 * frees reserved block, or does nothing if blocknr = 0
 */
VOID FreeReservedBlock (ULONG blocknr, globaldata *g)
{
  ULONG *bitmap, t;

	if (blocknr && blocknr <= g->rootblock->lastreserved)
	{
		bitmap = alloc_data.res_bitmap->bitmap;
		t = (blocknr - g->rootblock->firstreserved)/g->currentvolume->rescluster;
		bitmap[t/32] |= (0x80000000UL >> (t%32));
		g->rootblock->reserved_free++;
		g->currentvolume->rootblockchangeflag = TRUE;
	}
}

/* this routine is analogous GetAnodeBlock()
 * GetBitmapIndex is analogous GetIndexBlock()
 */
struct cbitmapblock *GetBitmapBlock(UWORD seqnr, globaldata *g)
{
  ULONG blocknr, temp;
  cbitmapblock_t *bmb;
  cindexblock_t *indexblock;
  struct volumedata *volume = g->currentvolume;

	/* check cache */
	for (bmb = HeadOf(&volume->bmblks); bmb->next; bmb=bmb->next)
	{
		if (bmb->blk.seqnr == seqnr)
		{
			MakeLRU (bmb);
			return bmb;
		}
	}

	/* not in cache, put it in */
	/* get the indexblock */
	temp = divide (seqnr, andata.indexperblock);
	if (!(indexblock = GetBitmapIndex(temp /* & 0xffff */, g)))
		return NULL;

	/* get blocknr */
	if (!(blocknr = indexblock->blk.index[temp>>16]) ||
		!(bmb = (cbitmapblock_t *)AllocLRU(g)))
		return NULL;

	DB(Trace(10,"GetBitmapBlock", "seqnr = %ld blocknr = %lx\n", seqnr, blocknr));

	/* read it */
	if (RawRead((UBYTE*)&bmb->blk, RESCLUSTER, blocknr, g) != 0)
	{
		FreeLRU((struct cachedblock *)bmb);
		return NULL;
	}

	/* check it */
	if (bmb->blk.id != BMBLKID)
	{
		FreeLRU ((struct cachedblock *)bmb);
		ErrorMsg (AFS_ERROR_DNV_WRONG_BMID, NULL, g);
		return NULL;
	}
	
	/* initialize it */
	bmb->volume = volume;
	bmb->blocknr = blocknr;
	bmb->used = FALSE;
	bmb->changeflag = FALSE;
	MinAddHead(&volume->bmblks, bmb);

	return bmb;
}

cindexblock_t *GetBitmapIndex (UWORD nr, globaldata *g)
{
  ULONG blocknr;
  cindexblock_t *indexblk;
  struct volumedata *volume = g->currentvolume;

	/* check cache */
	for (indexblk = HeadOf(&volume->bmindexblks); indexblk->next; indexblk=indexblk->next)
	{
		if (indexblk->blk.seqnr == nr)
		{
			MakeLRU(indexblk);
			return indexblk;
		}
	}

	/* not in cache, put it in */
	if ((nr > (g->supermode ? MAXBITMAPINDEX : MAXSMALLBITMAPINDEX)) ||
		!(blocknr = volume->rootblk->idx.large.bitmapindex[nr]) ||
		!(indexblk = (struct cindexblock *)AllocLRU(g)) )
	return NULL;

	DB(Trace(10,"GetBitmapIndex", "seqnr = %ld blocknr = %lx\n", nr, blocknr));

	if (RawRead((UBYTE*)&indexblk->blk, RESCLUSTER, blocknr, g) != 0) {
		FreeLRU((struct cachedblock *)indexblk);
		return NULL;
	}

	if (indexblk->blk.id == BMIBLKID)
	{
		indexblk->volume     = volume;
		indexblk->blocknr    = blocknr;
		indexblk->used       = FALSE;
		indexblk->changeflag = FALSE;
		MinAddHead (&volume->bmindexblks, indexblk);
	}
	else
	{
		FreeLRU ((struct cachedblock *)indexblk);
		ErrorMsg (AFS_ERROR_DNV_WRONG_INDID, NULL, g);
		return NULL;
	}

	LOCK(indexblk);
	return indexblk;
}       

/*
 * the following routines (NewBitmapBlock & NewBitmapIndexBlock are
 * primarily (read only) used by Format
 */
struct cbitmapblock *NewBitmapBlock (UWORD seqnr, globaldata *g)
{
  struct cbitmapblock *blok;
  struct cindexblock *indexblock;
  struct volumedata *volume = g->currentvolume;
  ULONG indexblnr, blocknr, indexoffset;
  ULONG *bitmap, i;
  UWORD oldlock;

	/* get indexblock */
	indexblnr = seqnr/andata.indexperblock;
	indexoffset = seqnr%andata.indexperblock;
	if (!(indexblock = GetBitmapIndex(indexblnr, g)))
		if (!(indexblock = NewBitmapIndexBlock(indexblnr, g)))
			return NULL;

	oldlock = indexblock->used;
	LOCK(indexblock);
	if (!(blok = (struct cbitmapblock *)AllocLRU(g)) ||
		!(blocknr = AllocReservedBlock(g)) )
		return NULL;

	indexblock->blk.index[indexoffset] = blocknr;

	blok->volume = volume;
	blok->blocknr = blocknr;
	blok->used = FALSE;
	blok->blk.id = BMBLKID;
	blok->blk.seqnr = seqnr;
	blok->changeflag = TRUE;

	/* fill bitmap */
	bitmap = blok->blk.bitmap;
	for (i = 0; i<alloc_data.longsperbmb; i++)
		*bitmap++ = ~0;

	MinAddHead(&volume->bmblks, blok);
	MakeBlockDirty((struct cachedblock *)indexblock, g);
	indexblock->used = oldlock;             // unlock;

	return blok;
}

static struct cindexblock *NewBitmapIndexBlock (UWORD seqnr, globaldata *g)
{
  struct cindexblock *blok;
  struct volumedata *volume = g->currentvolume;

	if (seqnr > (g->supermode ? MAXBITMAPINDEX : MAXSMALLBITMAPINDEX) ||
		!(blok = (struct cindexblock *)AllocLRU(g)))
		return NULL;

	if (!(g->rootblock->idx.large.bitmapindex[seqnr] = AllocReservedBlock (g)))
	{
		FreeLRU((struct cachedblock *)blok);
		return NULL;
	}

	volume->rootblockchangeflag = TRUE;

	blok->volume     = volume;
	blok->blocknr    = volume->rootblk->idx.large.bitmapindex[seqnr];
	blok->used       = FALSE;
	blok->blk.id     = BMIBLKID;
	blok->blk.seqnr  = seqnr;
	blok->changeflag = TRUE;
	MinAddHead(&volume->bmindexblks, blok);

	return blok;
}


/*
 * End allocation.c
 */
