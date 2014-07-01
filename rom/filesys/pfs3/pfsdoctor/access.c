/* $Id$ */
/* $Log: access.c $
 * Revision 2.6  1999/09/11  16:45:50  Michiel
 * Bug (1024 byte blok supprort) in AccessTest fixed
 *
 * Revision 2.5  1999/07/28  09:24:56  Michiel
 * Unneeded anodeblock searches removed
 *
 * Revision 2.4  1999/05/07  16:49:00  Michiel
 * bugfixes etc
 *
 * Revision 2.3  1999/05/07  09:31:31  Michiel
 * bugfix
 *
 * Revision 2.2  1999/05/04  04:27:13  Michiel
 * debugged upto buildrext
 *
 * Revision 2.1  1999/04/30  12:17:58  Michiel
 * Accepts OK disks, bitmapfix and hardlink fix works
 *
 * Revision 1.2  1999/04/22  15:26:49  Michiel
 * compiled
 * */

#include <exec/ports.h>
#include <stdlib.h>
#include "pfs3.h"
#include "doctor.h"
#include <string.h>

/**************************************
 * Non-checking functions for access to structures on PFS3 disks
 **************************************/

/* get a buildblock from the buildblock cache
 */
cachedblock_t *GetBuildBlock(uint16 bloktype, uint32 seqnr)
{
	struct buildblock *bbl;
	
	for (bbl = HeadOf(&volume.buildblocks); bbl->next; bbl = bbl->next)
	{
		if (bbl->b.data->id == bloktype &&
			bbl->b.data->indexblock.seqnr == seqnr)
		{
			return &bbl->b;
		}
	}

	return NULL;
}

/* get reserved block
 * anything, except deldir, root, boot, dir and rext
 */
error_t GetResBlock(cachedblock_t *blok, uint16 bloktype, uint32 seqnr, bool fix)
{
	cachedblock_t blk, *t;
	uint32 blknr, *bp = NULL, index, offset;
	error_t error = e_none;

	// controleer build block lijst
	t = GetBuildBlock(bloktype, seqnr);
	if (t)
	{
		blok->blocknr = t->blocknr;
		blok->mode = t->mode;
		*blok->data = *t->data;
		return e_none;
	}

	blk.data = calloc(1, SIZEOF_RESBLOCK);
	index = seqnr/INDEX_PER_BLOCK;
	offset = seqnr%INDEX_PER_BLOCK;
	switch(bloktype)
	{
		case SBLKID:

			if (seqnr > MAXSUPER)
			{
				free (blk.data);
				return e_number_error;
			}
			
			bp = &rext.data->superindex[seqnr];
			if (!*bp)
			{
				if (fix)
				{
					adderror("superindex block not found");
					error = RepairSuperIndex(bp, seqnr);
					if (error)
						*bp = 0;
					else
					{
						volume.writeblock((cachedblock_t *)&rext);
						KillAnodeBitmap();
					}
				}
			}
			break;

		case BMIBLKID:

			bp = &rbl->idx.large.bitmapindex[seqnr];
			if (!*bp)
			{
				if (fix)
				{
					adderror("bitmapindex block not found");
					error = RepairBitmapIndex(bp, seqnr);
					if (error)
						*bp = 0;
					else
						c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
				}
			}
			break;

		case IBLKID:

			if (rbl->options & MODE_SUPERINDEX)
			{
				error = GetResBlock(&blk, SBLKID, index, fix);
				if (error)
				{
					free (blk.data);
					return error;
				}

				bp = &blk.data->indexblock.index[offset];
			}
			else
			{
				bp = &rbl->idx.small.indexblocks[seqnr];
			}
			if (!*bp)
			{
				if (fix)
				{
					adderror("anodeindex block not found");
					error = RepairAnodeIndex(bp, seqnr);
					if (error)
						*bp = 0;
					else
					{
						/* the anodebitmap, which is made per aib, 
						 * could be too small
						 */
						KillAnodeBitmap();
						if (rbl->options & MODE_SUPERINDEX)
							volume.writeblock((cachedblock_t *)&blk);
						else
							c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
					}
				}
			}
			break;
			
		case ABLKID:
		
			error = GetResBlock(&blk, IBLKID, index, fix);
			if (error)
			{
				free (blk.data);
				return error;
			}
			
			bp = &blk.data->indexblock.index[offset];
			if (!*bp)
			{
				if (fix)
				{
					adderror("anode block not found");
					// RepairAnodeBlock already called from RepairAnodeTree
					// Pointless to call it again here.
					//if (error = RepairAnodeBlock(bp, seqnr))
					//	*bp = 0;
					//else
					//	volume.writeblock((cachedblock_t *)&blk);
				}
			}
			break;

		case BMBLKID:

			error = GetResBlock(&blk, BMIBLKID, index, fix);
			if (error)
			{
				free (blk.data);
				return error;
			}

			bp = &blk.data->indexblock.index[offset];
			if (!*bp)
			{
				if (fix)
				{
					adderror("bitmap block not found");
					error = RepairBitmapBlock(bp, seqnr);
					if (error)
						*bp = 0;
					else
						volume.writeblock((cachedblock_t *)&blk);
				}
			}
			break;
	}

	blknr = *bp;

	free (blk.data);
	if (!blknr)
		return e_not_found;

	error = volume.getblock(blok, blknr);
	if (error)
		return error;

	return e_none;
}

static c_anodeblock_t tablk = { 0 };
static ULONG tanodedata[MAXRESBLOCKSIZE / 4];
static anodeblock_t *tanodeblk = (anodeblock_t*)tanodedata;

bool GetAnode(canode_t *anode, uint32 anodenr, bool fix)
{
	anodenr_t *split = (anodenr_t *)&anodenr;

	if (!(tablk.data && tanodeblk->seqnr == split->seqnr))
	{
		tablk.data = tanodeblk;
		if (GetResBlock((cachedblock_t *)&tablk, ABLKID, split->seqnr, fix))
		{
			tablk.data = NULL;
			return false;
		}
	}

	if (split->offset > ANODES_PER_BLOCK)
		return false;

	anode->nr = anodenr;
	anode->clustersize = tablk.data->nodes[split->offset].clustersize;
	anode->blocknr     = tablk.data->nodes[split->offset].blocknr;
	anode->next        = tablk.data->nodes[split->offset].next;
	return true;
}

bool SaveAnode(canode_t *anode, uint32 nr)
{
	anodenr_t *split = (anodenr_t *)&nr;
	c_anodeblock_t ablk;
	uint32 buffer[MAXRESBLOCKSIZE/4];

	tablk.data = NULL;			/* kill anode read cache */
	ablk.data = (anodeblock_t *)buffer;
	if (GetResBlock((cachedblock_t *)&ablk, ABLKID, split->seqnr, false))
		return false;

	if (split->offset > ANODES_PER_BLOCK)
		return false;

	ablk.data->nodes[split->offset].clustersize = anode->clustersize;
	ablk.data->nodes[split->offset].blocknr     = anode->blocknr;
	ablk.data->nodes[split->offset].next        = anode->next;
	volume.writeblock((cachedblock_t *)&ablk);
	return true;
}

ULONG GetDEFileSize(struct direntry *direntry, struct extrafields *extra, ULONG *high)
{
	*high = 0;
	if (rbl->options & MODE_LARGEFILE) {
		*high = extra->fsizex;
		return direntry->fsize;
	}
	return direntry->fsize;
}

ULONG GetDDFileSize(struct deldirentry *dde, ULONG *high)
{
	*high = 0;
	if ((rbl->options & MODE_LARGEFILE) && dde->filename[0] <= DELENTRYFNSIZE) {
		*high = dde->fsizex;
		return dde->fsize;
	}
	return dde->fsize;
}
