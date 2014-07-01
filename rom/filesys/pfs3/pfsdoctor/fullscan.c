/* $Id$ */
/* $Log: fullscan.c $
 * Revision 2.5  1999/09/10  22:14:49  Michiel
 * Bugfixes etc (1.4)
 *
 * Revision 2.4  1999/05/07  16:49:00  Michiel
 * bugfixes etc
 *
 * Revision 2.3  1999/05/04  17:59:09  Michiel
 * check mode, logfile, search rootblock implemented
 * bugfixes
 *
 * Revision 2.2  1999/05/04  04:27:13  Michiel
 * debugged upto buildrext
 *
 * Revision 2.1  1999/04/30  12:17:58  Michiel
 * Accepts OK disks, bitmapfix and hardlink fix works
 *
 * Revision 1.1  1999/04/22  15:25:10  Michiel
 * Initial revision
 * */

#define __USE_SYSBASE
#include <string.h>
#include <math.h>

#include "pfs3.h"
#include "doctor.h"
#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <stdlib.h>

uint32 maxdatestamp = 0;

void InitFullScan(void)
{
	NewList((struct List *)&volume.buildblocks);
}

/* free mem etc */
void ExitFullScan(void)
{	
	buildblock_t *bbl, *next;

	for (bbl = HeadOf(&volume.buildblocks); (next=bbl->next); bbl=next)
	{
		if (bbl->b.data)
			FreeBufMem(bbl->b.data);
		free (bbl);
	}
}

/* Allocate create blocks. Write them to the cache and free
 * memory.
 * pre: resbitmap ready and valid
 */
error_t AllocBuildBlocks(void)
{
	buildblock_t *bbl, *next;
	cachedblock_t tblk;
	error_t error = e_none;
	uint32 blocknr, offset, seqnr;
	uint16 type;

	for (bbl = HeadOf(&volume.buildblocks); bbl->next; bbl=next)
	{
		volume.progress(0, 1);
		next = bbl->next;
		blocknr = fs_AllocResBlock();
		if (blocknr)
		{
			bbl->b.blocknr = blocknr;
			bbl->b.mode = done;
			volume.writeblock(&bbl->b);
			switch(bbl->b.data->id)
			{
				case EXTENSIONID:
					rbl->extension = blocknr;
					c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
					rext.mode = check;
					rext.data = calloc(1, SIZEOF_RESBLOCK);
					rext.blocknr = blocknr;
					memcpy(rext.data, bbl->b.data, SIZEOF_RESBLOCK); 
					break;

				case SBLKID:
					rext.data->superindex[bbl->b.data->indexblock.seqnr] = blocknr;
					volume.writeblock((cachedblock_t *)&rext);
					break;

				case BMIBLKID:
					rbl->idx.large.bitmapindex[bbl->b.data->indexblock.seqnr] = blocknr;
					c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
					break;

				case IBLKID:
					if (!(rbl->options & MODE_SUPERDELDIR))
					{
						rbl->idx.small.indexblocks[bbl->b.data->indexblock.seqnr] = blocknr;
						c_WriteBlock((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
						break;
					}
					type = SBLKID;

				case BMBLKID:
					if (bbl->b.data->id == BMBLKID)
						type = BMIBLKID;

					seqnr = bbl->b.data->indexblock.seqnr/INDEX_PER_BLOCK;
					offset = bbl->b.data->indexblock.seqnr%INDEX_PER_BLOCK;
					tblk.data = calloc(1, SIZEOF_RESBLOCK);
					if (tblk.data)
					{
						GetResBlock(&tblk, type, seqnr, true);
						tblk.data->indexblock.index[offset] = blocknr;
						volume.writeblock(&tblk);
						free(tblk.data);
					}
					else
					{
						adderror("memory allocation error");
						return e_out_of_memory;
					}
					break;

			}
			MinRemove(bbl);
			FreeBufMem(bbl->b.data);
			free(bbl);
		}
		else
		{
			adderror("allocation error");
			error = e_alloc_fail;
		}
	}

	return error;
}

/* Allocate a block from the generated reserved bitmap.
 * This reserved bitmap must be complete and valid
 */
uint32 fs_AllocResBlock(void)
{
	uint32 i, field, blocknr;
	int32 j;

	for (i=0; i<volume.resbitmap->lwsize; i++)
	{
		field = volume.resbitmap->map[i];
		if (field)
		{
			for (j=31; j>=0; j--)
			{
				if (field & (1 << j))
				{
					blocknr = rbl->firstreserved + (i*32+(31-j))*volume.rescluster;
					if (blocknr < rbl->lastreserved)
					{
						volume.resbitmap->map[i] &= ~(1 << j);
						return blocknr;
					}
				}
			}
		}
	}

	/* reserved area full */
	return 0;
}

/* Redefine volume from rootblock located at 'bloknr'
 */
error_t Repartition(uint32 bloknr)
{
	rootblock_t *rbl;
	error_t error = e_none;

	if (!(rbl = (rootblock_t *)AllocBufMem (MAXRESBLOCKSIZE)))
		return e_out_of_memory;

	// read rootblock 
	error = c_GetBlock ((uint8 *)rbl, ROOTBLOCK + volume.firstblock, volume.blocksize);
	if (error)
		goto ret_error;

	if (!IsRootBlock(rbl) || !rbl->disksize)
	{
		error = e_syntax_error;
		goto ret_error;
	}

	volume.firstblock = bloknr - ROOTBLOCK;
	volume.lastblock = volume.firstblock + rbl->disksize - 1;
	volume.disksize = rbl->disksize;
	volume.lastreserved = rbl->lastreserved;
	volume.repartitioned = true;

 ret_error:

	FreeBufMem(rbl);
	return error;
}

/* special case: no alloc needed */
error_t BuildBootBlock(void)
{
	bootblock_t *bbl;
	error_t error;

	if (!(bbl = AllocBufMem(2*volume.blocksize)))
		return e_out_of_memory;
	
	memset (bbl, 0, 2*volume.blocksize);
	bbl->disktype = ID_PFS_DISK;
	error = c_WriteBlock ((UBYTE *)bbl, 1, BOOTBLOCK + volume.firstblock);
	FreeBufMem (bbl);
	return error;
}

/* special case: no alloc needed
 * Create rootblock. The datestamp is set to the highest value found by
 * SeachBlocks, increased by 0x299. All references are uptodate.
 */
error_t BuildRootBlock(rootblock_t *rbl)
{
	struct DateStamp time;
	uint32 i, j;
	scanelement_t ind[104];
	uint32 resblocksize = 1024;

	volume.status(0, "Building rootblock", 3);
	memset (rbl, 0, volume.blocksize);
	DateStamp (&time);

	rbl->disktype = ID_PFS_DISK;
	rbl->options = MODE_HARDDISK | MODE_SPLITTED_ANODES | MODE_DIR_EXTENSION |
			MODE_SIZEFIELD | MODE_DATESTAMP | MODE_EXTROVING;
#if LARGE_FILE_SIZE
	rbl->options |= MODE_LARGEFILE;
	rbl->disktype = ID_PFS2_DISK;
#endif

	rbl->disksize = volume.disksize;
	if (volume.disksize > MAXSMALLDISK) {
		rbl->options |= MODE_SUPERINDEX;
		if (volume.disksize > MAXDISKSIZE1K) {
			rbl->disktype = ID_PFS2_DISK;
			resblocksize = 2048;
			if (volume.disksize > MAXDISKSIZE2K)
				resblocksize = 4096;
		}
	}

	rbl->datestamp = ~0;	// don't be a break on search functions
	rbl->creationday = (UWORD)time.ds_Days;
	rbl->creationminute = (UWORD)time.ds_Minute;
	rbl->creationtick = (UWORD)time.ds_Tick;
	rbl->protection	= 0xf0;
	rbl->firstreserved = 2;
	rbl->lastreserved = SearchLastReserved(&volume);
	volume.progress(0,1);
	rbl->reserved_free = (rbl->lastreserved - rbl->firstreserved)/volume.rescluster;

	rbl->reserved_blksize = resblocksize;
	rbl->blocksfree = volume.disksize - rbl->lastreserved;
	rbl->alwaysfree = rbl->blocksfree/20;

	rbl->diskname[0] = strlen("FixedDisk");
	memcpy(rbl->diskname+1, "FixedDisk", strlen("FixedDisk"));
	
	/* extension is created by build extension */

	/* create bitmap index */
	memset (ind, 0, 104*sizeof(scanelement_t));
	volume.status(1, "Searching bitmap blocks", rbl->lastreserved - 2);
	SearchBlocks(ind, 0, 103, rbl->firstreserved, rbl->lastreserved, BMIBLKID);
	volume.progress(0,1);
	if (rbl->options & MODE_SUPERINDEX)
		j = 104;
	else
		j = 5;

	for (i=0; i<j; i++)
		rbl->idx.large.bitmapindex[i] = ind[i].blocknr;

	/* create anodeindex */
	if (!(rbl->options & MODE_SUPERINDEX))
	{
		memset (ind, 0, 104*sizeof(scanelement_t));
		volume.status(1,"Searching index blocks", rbl->lastreserved - 2);
		SearchBlocks(ind, 0, 98, rbl->firstreserved, rbl->lastreserved, IBLKID);
		for (i=0; i<99; i++)
			rbl->idx.small.indexblocks[i] = ind[i].blocknr;
	}
	rbl->datestamp = maxdatestamp + 0x200;
	volume.progress(0,1);
	volume.status(1, " ", 100);
	return e_none;
}

/* Search deldirblocks, indexblocks, anodeblocks, bitmapblocks
 * el = preallocated and cleared array of scanelement for storing result
 * seqlow = lowest element array
 * seqhigh = highest element array
 * start = first blocknr 
 * stop = last blocknr
 */
void SearchBlocks(scanelement_t el[], uint32 seqlow, uint32 seqhigh,
	uint32 start, uint32 stop, uint16 bloktype)
{
	cachedblock_t blk;
	uint32 i, seqnr, ds = 0;

	blk.data = calloc(1, SIZEOF_RESBLOCK);
	volume.status(1, NULL, (stop-start)/volume.rescluster);
	for (i = start; i < stop; i+=volume.rescluster) 
	{
		volume.progress(1,1);
		if (volume.getblock(&blk, i))
			continue;

		if (aborting)
		{
			volume.showmsg("Aborting block search\n");
			aborting = 0;
			break;
		}

		if (blk.data->id == bloktype)
		{
			seqnr = blk.data->indexblock.seqnr;
			ds = blk.data->indexblock.datestamp;
			if (seqnr >= seqlow && seqnr <= seqhigh &&
				ds >= el[seqnr-seqlow].datestamp)
			{
				el[seqnr-seqlow].blocknr = i;
				el[seqnr-seqlow].datestamp = ds;
				maxdatestamp = max(ds, maxdatestamp);
			}
		}
	}

	free (blk.data);
	volume.status(1," ",100);
}

/* search for a block on a volume.
 * bloktype - kind of block to look for
 * seqnr - longword on offset 8 (not for REXT)
 * last - blocknr. Max is last + 32
 * datestamp - maximum datestamp on offset 4, 0 = no max
 * anodenr, parent - for dirblocks only, 0 = ignore
 * returns bloknr
 *
 * bloktypes: REXT, BM, BMI, S, AI, A, D
 */
uint32 SearchBlock(uint16 bloktype, uint32 seqnr, uint32 last, uint32 datestamp, uint32 anodenr,
	uint32 parent)
{
	cachedblock_t blk;
	uint32 ds, tmp;
	int32 i, bloknr;	// signed !!
	int found = 0;

	blk.data = calloc(1, SIZEOF_RESBLOCK);
	bloknr = ds = 0;
	if (last)
	{
		last += 64;
		last = min(rbl->lastreserved, last);
	}
	else
		last = rbl->lastreserved;
	last -= (last % volume.rescluster);

	if (!datestamp)
		datestamp = rbl->datestamp;

	volume.status(1,NULL,1024);
	for (i = last; i != last+volume.rescluster; i -= volume.rescluster) 
	{
		/* rollover to end of reserved area */
		if (i < rbl->firstreserved)
		{
			if (last >= rbl->lastreserved - 4)
				break;
			i = rbl->lastreserved & ~(volume.rescluster-1);
		}

		/* break if nothing found for 128 blocks, and already found promising
		 * candidate. Uses roundrobin allocation
		 */
		if (found && !--found)
			break;

		volume.progress(1,1);
		if (volume.getblock(&blk, i))
			continue;

		if (aborting)
		{
			// break search, don't break repair
			volume.showmsg("Aborting block search\n");
			aborting = 0;
			break;
		}

		if (blk.data->id == bloktype)
		{
			if (blk.data->indexblock.seqnr != seqnr && bloktype != EXTENSIONID)
				continue;

			if (anodenr && (blk.data->dirblock.anodenr != anodenr ||
					blk.data->dirblock.parent != parent))
				continue;

			if (datestamp && blk.data->dirblock.datestamp > datestamp)
				continue;

			/* found block */
			tmp = (bloktype == EXTENSIONID) ?
				  (blk.data->extensionblock.datestamp) :
				  (blk.data->dirblock.datestamp);
			
			if (tmp >= ds)
			{
				if (datestamp)
				{
					if (tmp > datestamp)
						continue;

					/* found a promising candidate ? */
					if (datestamp - tmp < 100)
						found = 512;		// 1.3: was 128
				}
				ds = tmp;
				bloknr = i;
			}
		}
	}

	free (blk.data);
	volume.status(1," ",100);
	return (uint32)bloknr;
}

/* Get last reserved block */
uint32 SearchLastReserved(volume_t *vol)
{
	cachedblock_t blk;
	uint32 i, last, cdwn;

	blk.data = calloc(1, SIZEOF_RESBLOCK);
	cdwn = 4096;
	i = last = vol->disksize/256;
	i -= i % volume.rescluster;
	vol->status(1, "Scanning disk", vol->disksize/16 - last);
	while (i < vol->disksize/16) 
	{
		vol->progress(1,1);
		if (aborting)
		{
			volume.showmsg("Aborting filesystem search\n");
			aborting = 0;
			break;
		}

		if (volume.getblock(&blk, i))
			goto s_ret;

		switch (blk.data->id)
		{
			case DBLKID:
			case ABLKID:
			case IBLKID:
			case BMBLKID:
			case BMIBLKID:
			case DELDIRID:
			case EXTENSIONID:
			case SBLKID:

				cdwn = 1024;
				last = i;
				break;

			default:

				/* Exit if blocks not reserved */
				if (!cdwn--)
					goto s_ret;
		}
		
		i += volume.rescluster;
	}

 s_ret:
 	free (blk.data);
	vol->status(1, " ", 100);
	return last;
}

/* Search filesystem on specified execdevice/unit. Start at startblok,
 * end at endblok
 */
uint32 SearchFileSystem(int32 startblok, int32 endblok)
{
	rootblock_t *rbl;
	uint32 blnr = 0, b;

	if (!(rbl = (rootblock_t *)AllocBufMem (MAXRESBLOCKSIZE)))
		return e_out_of_memory;

	volume.status(0, "Searching filesystem", endblok-startblok);
	startblok = max(startblok, 0);
	startblok &= ~1;		/* even bloknr */
	for (b = startblok; b<endblok; b += volume.rescluster)
	{
		volume.progress(0,1);
		if (aborting)
		{
			volume.showmsg("Aborting filesystem search\n");
			aborting = 0;
			break;
		}

		// read block and check if it is a rootblock */
		if (c_GetBlock ((uint8 *)rbl, b, volume.blocksize))
			break;

		if (IsRootBlock(rbl))
		{
			blnr = b;
			break;
		}
	}

	FreeBufMem(rbl);
	return blnr;
}

/* cached block structure is managed by caller
 * block data is managed by this function
 * creates an extension block with uptodate reference to
 * superblock and no deldir.
 */
error_t BuildRext(c_extensionblock_t *rext)
{
	struct DateStamp time;
	struct buildblock *bbl;
	extensionblock_t *r;
	scanelement_t ind[16];
	int i;

	volume.status(1, "Building rext", (rbl->lastreserved-rbl->firstreserved)/volume.rescluster);

	if (!(r = rext->data = AllocBufMem(SIZEOF_RESBLOCK)))
		return e_out_of_memory;

	if (!(bbl = malloc(sizeof(struct buildblock))))
	{
		FreeBufMem(r);
		return e_out_of_memory;
	}

	memset (r, 0, volume.blocksize);
	memset (bbl, 0, sizeof(*bbl));
	DateStamp (&time);

	r->id = EXTENSIONID;
	r->datestamp = rbl->datestamp;
	r->pfs2version = (17<<16) + 99;
	r->root_date[0] = time.ds_Days;
	r->root_date[1] = time.ds_Minute;
	r->root_date[2] = time.ds_Tick;

	volume.progress(1, 1);
	if (rbl->options & MODE_SUPERINDEX)
	{
		memset (ind, 0, 16*sizeof(scanelement_t));
		SearchBlocks(ind, 0, 15, rbl->firstreserved, rbl->lastreserved, SBLKID);
		for (i=0; i<16; i++)
			r->superindex[i] = ind[i].blocknr;
	}

	/* add to list */
	bbl->b.blocknr = rext->blocknr = ~0;
	bbl->b.mode    = rext->mode    = build;
	bbl->b.data    = (reservedblock_t *)r;
	MinAddHead(&volume.buildblocks, bbl);

	volume.status(1, " ", 100);
	return e_none;
}


/* zet mode to build
 */
error_t BuildIndexBlock(c_indexblock_t *blk, uint16 bloktype, uint32 seqnr)
{
	struct buildblock *bbl;
	indexblock_t *ib;
	int i;
	uint16 childtype;
	scanelement_t *ind;

	volume.status(1, "Building indexblock", INDEX_PER_BLOCK);	
	if (!(ind = calloc(INDEX_PER_BLOCK, sizeof(scanelement_t))))
		return e_out_of_memory;

	if (!(ib = blk->data = AllocBufMem(SIZEOF_RESBLOCK))) {
		free(ind);
		return e_out_of_memory;
	}

	if (!(bbl = malloc(sizeof(struct buildblock))))
	{
		FreeBufMem(ib);
		free(ind);
		return e_out_of_memory;
	}

	memset (ib, 0, volume.blocksize);
	memset (bbl, 0, sizeof(*bbl));

	ib->id = bloktype;
	ib->datestamp = rbl->datestamp;
	ib->seqnr = seqnr;
	
	/* fill index */
	switch (bloktype)
	{
		case IBLKID:	childtype = ABLKID; break;
		case BMIBLKID:	childtype = BMBLKID; break;
		case SBLKID:	childtype = IBLKID; break;
		default:		return e_fatal_error;
	}

	SearchBlocks (ind, seqnr*INDEX_PER_BLOCK, seqnr*INDEX_PER_BLOCK + INDEX_PER_BLOCK - 1, rbl->firstreserved, rbl->lastreserved, childtype);
	for (i=0; i<INDEX_PER_BLOCK; i++)
		ib->index[i] = ind[i].blocknr;
	
	/* add to list */
	bbl->b.blocknr = blk->blocknr = ~0;
	bbl->b.mode    = blk->mode    = build;
	bbl->b.data    = (reservedblock_t *)ib;
	MinAddHead(&volume.buildblocks, bbl);

	free(ind);
	volume.status(1, " ", 100);
	return e_none;
}

error_t BuildBitmapBlock(c_bitmapblock_t *blk, uint32 seqnr)
{
	struct buildblock *bbl;
	bitmapblock_t *bmb;
	uint32 *bitmap;
	int i;
	
	volume.status(1, "Building bitmapblock", 100);
	if (!(bmb = blk->data = AllocBufMem(SIZEOF_RESBLOCK)))
		return e_out_of_memory;

	if (!(bbl = malloc(sizeof(struct buildblock))))
	{
		FreeBufMem(bmb);
		return e_out_of_memory;
	}

	memset (bmb, 0, volume.blocksize);
	memset (bbl, 0, sizeof(*bbl));

	bmb->id = BMBLKID;
	bmb->datestamp = rbl->datestamp;
	bmb->seqnr = seqnr;
	
	/* fill bitmap */
	bitmap = bmb->bitmap;
	for (i = 0; i<LONGS_PER_BMB; i++)
		*bitmap++ = ~0;

	/* add to list */
	bbl->b.blocknr = blk->blocknr = ~0;
	bbl->b.mode    = blk->mode    = build;
	bbl->b.data    = (reservedblock_t *)bmb;
	MinAddHead(&volume.buildblocks, bbl);

	volume.status(1, " ", 100);
	return e_none;
}

