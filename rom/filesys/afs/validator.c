/*
    Copyright © 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      created disk validation procedures
 * 03-jan-2008 [Tomasz Wiszkowski]      updated procedure to strip fake dircache blocks
 *                                      no directory cache handling present here.
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected procedure to *ignore* data block sizes 
 *                                      for OFS volumes since DOSTYPE does not differentiate them,
 *                                      corrected tabulation.
 * 07-jan-2008 [Tomasz Wiszkowski]      partitioned procedures to prepare non-recursive scan
 */

#include "validator.h"
#include "volumes.h"

#undef SDEBUG
#undef DEBUG
#define DEBUG 0

#ifdef __AROS__
#include <dos/dostags.h>
#include <utility/tagitem.h>
#include <exec/ports.h>
#include <exec/types.h>
#include "afsblocks.h"
#include "cache.h"
#include "error.h"
#include <exec/ports.h>
#include <exec/io.h>
#include <dos/dos.h>
#include <aros/debug.h>
#include "misc.h"
#include "extstrings.h"
#endif

/*******************************************
 Name  : checkValid
 Descr : verify whether disk is validated and writable.
         since we need at least a stub function, it has to be
         declared outside __AROS__ scope
 Input : afsbase, volume
 Output: 0 for unvalidated disc, 1 otherwise
 Author: Tomasz Wiszkowski
********************************************/
LONG checkValid(struct AFSBase *afs, struct Volume *vol)
{
#ifdef __AROS__
    struct BlockCache *blockbuffer;
    blockbuffer = getBlock(afs, vol, vol->rootblock);
    UBYTE  n[MAX_NAME_LENGTH];
    STRPTR name;
    name=(STRPTR)((char *)blockbuffer->buffer+(BLK_DISKNAME_START(vol)*4));
    StrCpyFromBstr(name, n);
	while (vol->state == ID_WRITE_PROTECTED
              && showError(afs, ERR_WRITEPROTECT, n));

	if (vol->state == ID_VALIDATING)
	{
		if (showError(afs, ERR_DISKNOTVALID))
		{
			if (vr_OK == launchValidator(afs, vol))
				return 1;
		}
		return 0;
	}

	return vol->state == ID_VALIDATED ? 1 : 0;
#else
	return 1;
#endif
}

/*******************************************
 Name  : launchValidator
 Descr : launch validation process for specified medium
         since we need at least a stub function, it has to be
         declared outside __AROS__ scope
 Input : volume
 Output: none
 Author: Tomasz Wiszkowski
********************************************/
LONG launchValidator(struct AFSBase *afsbase, struct Volume *volume)
{
#ifdef __AROS__
	D(bug("[afs]: flushing cache...\n"));
	flushCache(afsbase, volume);

	/*
	 * initially this was meant to be a synchronous validation
	 * but due to obvious problems with IO commands idea was eventually given up
	 */
	return validate(afsbase, volume);
#else
	return 0;
#endif
}




/*****************************************
 * this set is good only for AROS NATIVE
 ****************************************/
#ifdef __AROS__

LONG validate(struct AFSBase *afs, struct Volume *vol)
{
	DiskStructure     ds;
	ValidationResult  res = vr_OK;

	/*
	 * fill in diskstructure. we will need it for the sake of validation.
	 */
	ds.vol = vol;
	ds.afs = afs;
	ds.flags = 0;

	/*
	 * we could use some inhibiting here
	 */
	if (0 == bm_allocate_bitmap(&ds))
	{
		inhibit(afs, vol, 1);
		res = start_superblock(&ds);
		inhibit(afs, vol, 0);
	}
	bm_free_bitmap(&ds);

	switch (res)
	{
		case vr_OK:
			D(bug("[afs validate]: Validation complete. \n"));
			break;
		case vr_NoAccess:
			D(bug("[afs validate]: Could not create access point. \n"));
			break;
		case vr_ReadError:
			D(bug("[afs validate]: Could not read disk. \n"));
			break;
		case vr_UnknownDiskType:
			D(bug("[afs validate]: Unhandled disk type. \n"));
			break;
		case vr_InvalidChksum:
			D(bug("[afs validate]: Invalid block checksum. \n"));
			break;
		case vr_StructureDamaged:
			D(bug("[afs validate]: Structure damaged. \n"));
			break;
		case vr_OutOfMemory:
			D(bug("[afs validate]: Could not allocate memory to complete operation. \n"));
			break;
		case vr_BlockUsedTwice:
			D(bug("[afs validate]: Physical block used more than once. \n"));
			break;
		case vr_Aborted:
			D(bug("[afs validate]: Aborted by user. \n"));
			break;
		case vr_BlockOutsideDisk:
			D(bug("[afs validate]: Block outside volume boundaries. \n"));
			break;
	}

	{
		struct BlockCache *bc = getBlock(afs, vol, vol->rootblock);
		ULONG* mem = bc->buffer;

		if (res != vr_OK)
		{
			mem[BLK_BITMAP_VALID_FLAG(vol)] = 0;
			vol->state = ID_VALIDATING;
		}
		else
			vol->state = ID_VALIDATED;

		if (verify_checksum(&ds, mem) != 0)
		{
			D(bug("[afs validate]: block checksum does not match.\n"));
			bc->flags |= BCF_WRITE;
		}
	}
	/*
	 * it's not neccessary here, but res is holding validation result
	 * one may wish to open some requester at this point in the future
	 */
	return res;
}

/*
 * validate range
 */
LONG check_block_range(DiskStructure *ds, ULONG num)
{
	if ((num < ds->vol->bstartblock) || (num >= ds->vol->countblocks))
	{
		D(bug("[afs validate]: Block located outside volume range. Condition %lu <= %lu < %lu not met.\n", 
			ds->vol->bstartblock,
			num,
			ds->vol->countblocks));
		showError(ds->afs, ERR_BLOCK_OUTSIDE_RANGE, num);
		return 0;
	}
	return 1;
} 

/*
 * begin
 */
ValidationResult start_superblock(DiskStructure *ds)
{
	ValidationResult res = vr_OK;

	res = collect_directory_blocks(ds, ds->vol->rootblock);
	D(bug("[afs validate]: validation complete. Result: %ld\n", res));

	/*
	 * record bitmap back to disk, set bitmap valid flag, and update checksum of the root sector
	 * please note: it is hell important to have this checksum valid ;) so you better keep an eye
	 * on all your changes
	 */
	if (res == vr_OK)
	{
		record_bitmap(ds);
	}

	return res;
}

/*
 * verify_checksum
 */
ULONG verify_checksum(DiskStructure *ds, ULONG *block)
{
	ULONG sum = 0;
	LONG i;

	D(bug("[afs validate]: verifying block checksum... "));

	for (i=0; i<ds->vol->SizeBlock; i++)
	{
		sum += OS_BE2LONG(block[i]);
	}

	if (sum != 0)
	{
		D(bug("checksum invalid!\n"));
		sum -= OS_BE2LONG(block[BLK_CHECKSUM]); 
		D(bug("[afs validate]: current sum: %08lx, valid sum: %08lx. correcting\n", block[BLK_CHECKSUM], sum));
		block[BLK_CHECKSUM] = OS_LONG2BE(-sum);
		return -sum;
	}

	D(bug("checksum valid!\n"));
	return 0;
} 

/*
 * verify_bm_checksum
 */
ULONG verify_bm_checksum(DiskStructure *ds, ULONG *block)
{
	ULONG sum = 0;
	ULONG i;

	D(bug("[afs validate]: verifying bitmap checksum... "));

	for (i=0; i<ds->vol->SizeBlock; i++)
	{
		sum += OS_BE2LONG(block[i]);
	}

	if (sum != 0)
	{
		D(bug("checksum invalid!\n"));
		sum -= OS_BE2LONG(block[0]); 
		D(bug("[afs validate]: current sum: %08lx, valid sum: %08lx. correcting\n", block[0], sum));
		block[0] = OS_LONG2BE(-sum);
		return -sum;
	}

	D(bug("checksum valid!\n"));
	return 0;
} 

/*
 * check single block - this procedure should do as much as possible
 */
ValidationResult check_block(DiskStructure* ds, struct BlockCache* block)
{
	ULONG* mem;
	ULONG id;

	/*
	 * first check cache validity
	 */
	if (0 == block)
	{
		D(bug("[afs validate] Error: no block passed.\n"));
		return vr_ReadError;
	}

	if (0 == block->blocknum)
	{
		D(bug("[afs validate] Block could not be read.\n"));
		return vr_ReadError;
	}

	mem = block->buffer;

	/*
	 * check types first: we expect block of T_SHORT (2) or T_LIST (16) type
	 */
	id = OS_BE2LONG(mem[BLK_PRIMARY_TYPE]);
	if ((id != T_SHORT) && 
		 (id != T_LIST))
	{
		D(bug("[afs validate]: block is not of known-type (%08lx). asking whether to correct\n", id));

		if (block->blocknum == ds->vol->rootblock)
		{
			if ((0 == (ds->flags & ValFlg_DisableReq_MaybeNotAFS)) && (0 == showError(ds->afs, ERR_POSSIBLY_NOT_AFS)))
				return vr_Aborted;
			ds->flags |= ValFlg_DisableReq_MaybeNotAFS;
			D(bug("[afs validate]: will continue per user decision. faulty block will be removed from structure.\n"));
		}
		return vr_StructureDamaged;
	}

	/*
	 * set bitmap block (mark as used)
	 */
	if (bm_mark_block(ds, block->blocknum))
	{
		D(bug("[afs validate]: block %lu used twice! aborting!\n", block->blocknum));
		return vr_BlockUsedTwice;
	}

	return vr_OK;
}

ValidationResult collect_bitmap(DiskStructure* ds, struct BlockCache* block)
{
	ULONG strt=BLK_BITMAP_POINTERS_START(ds->vol),
			stop=BLK_BITMAP_POINTERS_END(ds->vol),
			ext =BLK_BITMAP_EXTENSION(ds->vol),
			i, blk;

	ULONG* mem = block->buffer;

	do
	{
		for (i=strt; i<=stop; ++i)
		{
			blk = OS_BE2LONG(mem[i]);
			if (blk == 0)
				continue;

			/*
			 * verify whether bitmap block resides on disk
			 * if the range validation fails, simply do not add the block to the list
			 *
			 * unfortunately bitmap block reallocation has not been implemented
			 * since this is the most unlikely happening issue
			 */
			if (0 == check_block_range(ds, blk))
				return vr_BlockOutsideDisk;

			/*
			 * mark block as used.
			 * the bitmap blocks *could be reallocated* (and in fact should be)
			 * if the bm_mark_block returns something else than st_OK (that is,
			 * in case, when block is out of range, or simply used twice).
			 *
			 * i'm sorry, but this has not been introduced yet.
			 */
			if (bm_mark_block(ds, blk) != st_OK)
			{
				showError(ds->afs, ERR_BLOCK_USED_TWICE, blk);
				return vr_BlockUsedTwice;
			}

			/*
			 * add bitmap block to bitmap block set
			 */
			bm_add_bitmap_block(ds, blk);
		}

		/*
		 * collect bitmap extension blocks
		 */
		if ((blk = OS_BE2LONG(mem[ext])) != 0) 
		{
			D(bug("[afs validate] Following to next bitmap extension block at %08lx\n", blk));
			if (0 == check_block_range(ds, blk)) 
				return vr_BlockOutsideDisk;

			if (bm_mark_block(ds, blk) != st_OK)
				return vr_BlockUsedTwice;

			block = getBlock(ds->afs, ds->vol, blk);
			mem = block->buffer;
			strt = 0;
			stop = ds->vol->SizeBlock-2;
			ext  = ds->vol->SizeBlock-1;
		}
	}
	while (blk != 0);

	return vr_OK;
}

ValidationResult collect_file_extensions(DiskStructure* ds, struct BlockCache* block)
{
	ULONG i, blk, clr=0;
	ULONG* mem = block->buffer;

	ds->file_blocks = 0;

	do
	{
		for (i=BLK_TABLE_END(ds->vol); i>=BLK_TABLE_START; --i)
		{
			if (clr != 0)
			{
				mem[i] = 0;
				block->flags |= BCF_WRITE;
				continue;
			}

			blk = OS_BE2LONG(mem[i]);

			/*
			 * if this was the last data block, purge rest
			 */
			if (blk == 0)
			{
				clr = 1;
				continue;
			}


			/*
			 * verify if block still belongs to this disk, purge if it doesn't
			 */
			if (0 == check_block_range(ds, blk))
			{
				D(bug("[afs validate] file data block outside range. truncating\n"));
				block->flags |= BCF_WRITE;
				clr = 1;
				mem[i] = 0;
				continue;
			}

			/*
			 * mark block as used.
			 */
			if (bm_mark_block(ds, blk) != st_OK)
			{
				D(bug("[afs validate] file data block used twice. truncating\n"));
				block->flags |= BCF_WRITE;
				clr = 1;
				mem[i] = 0;
				continue;
			}

			++ds->file_blocks;
		}

		/*
		 * if block is marked as 'write', make sure it holds correct sum.
		 */
		if (block->flags & BCF_WRITE)
			verify_checksum(ds, block->buffer);
		/*
		 * collect bitmap extension blocks
		 */
		if ((blk = OS_BE2LONG(mem[BLK_EXTENSION(ds->vol)])) != 0) 
		{
			D(bug("[afs validate] Following to next file extension block at %08lx\n", blk));
			if (0 == check_block_range(ds, blk)) 
			{
				D(bug("[afs validate] Extension block outside range. truncating file\n"));
				mem[BLK_EXTENSION(ds->vol)] = 0;
				block->flags |= BCF_WRITE;
				blk = 0;
			}
			else if (bm_mark_block(ds, blk) != st_OK)
			{
				D(bug("[afs validate] Bitmap block already marked as used. truncating file\n"));
				mem[BLK_EXTENSION(ds->vol)] = 0;
				block->flags |= BCF_WRITE;
				blk = 0;
			}
			else
			{
				block = getBlock(ds->afs, ds->vol, blk);
				mem = block->buffer;
			}
		}

		/*
		 * update sum; if changed, mark block for writing
		 */
		if (0 != verify_checksum(ds, block->buffer))
			block->flags |= BCF_WRITE;
	}
	while (blk != 0);

	return vr_OK;
}

/*
 * collect directories
 */
ValidationResult collect_directory_blocks(DiskStructure *ds, ULONG blk)
{
	struct BlockCache *bc;
	LONG entry_type;
	LONG primary_type;
	ULONG id;

	D(bug("[afs validate]: analyzing block %lu\n", blk));

	/*
	 * check range. Note that the whole process here is not 'reversible':
	 * if you mark bitmap blocks as used, and then just return in the middle,
	 * it won't get freed automagically, so mark it only when you know it should be there.
	 */
	if (0 == check_block_range(ds, blk))
		return vr_BlockOutsideDisk;
	
	/*
	 * we don't set block usage flags: we will re-read the block anyways.
	 */
	bc = getBlock(ds->afs, ds->vol, blk);

	/*
	 * initial block analysis
	 */
	id = check_block(ds, bc);
	if (id != vr_OK)
		return id;

	/*
	 * collect types
	 */
	entry_type = OS_BE2LONG(bc->buffer[BLK_SECONDARY_TYPE(ds->vol)]);
	primary_type = OS_BE2LONG(bc->buffer[BLK_PRIMARY_TYPE]);

	switch (primary_type) {
	    case T_LIST:
	    	break;
	    case T_SHORT:
	    	break;
	    case T_DATA:
	    	break;
	    default:
	    	/* TODO: handle unknown primary type */
	    	break;
	}

	/*
	 * for root block: collect all bitmap blocks now
	 */

	/*
	 * i won't hide it: there's plenty of things to check here, but
	 * since it's just a validation task
	 * i have decided to remove the redundant code.
	 */

	/*
	 * ok, if anyone wants to remove the bitmap allocations and re-allocate them by himself
	 * here's the good place to start doing it. myself, i think the best way is to rely on
	 * operating system in this matter: unless someone damaged the disk structure, it's 
	 * perfectly safe to assume that number of allocated bitmap blocks is just enough for us
	 * to store the bitmap. Again, it's not a salvaging, repairing NOR reorganizing task.
	 */

	/*
	 * oh, btw; we also want to collect the bitmap blocks here so we know where to save new bitmap.
	 */
	if (entry_type == 1)
	{
		D(bug("[afs validate]: Checking and collecting bitmap blocks\n"));

		id = collect_bitmap(ds, bc);
		if (id != vr_OK)
			return id;

		/*
		 * initially -- mark bitmap status as valid
		 */
		bc = getBlock(ds->afs, ds->vol, blk);
		bc->buffer[BLK_BITMAP_VALID_FLAG(ds->vol)] = ~0;
		verify_checksum(ds, bc->buffer);
		bc->flags |= BCF_WRITE;
	}
	
	/*
	 * this is the collection procedure 
	 * we check for directories/files here only. if an entry is not a directory,
	 * check if it is a file and accumulate data blocks for it.
	 * here we are interested only in FOUR of SEVEN types: ROOT, DIRECTORY, FILE and FILE EXTENSION
	 */
	if ((entry_type == 1) || (entry_type == 2))	 // 1 = root, which is handled already, 3 = symlink, which we don't need, 4 = hardlink, which we don't want
	{
		LONG i;

		/*
		 * clear directory cache block.
		 * fancy thing about directory cache: it is the best way to run into inconsistencies between file trees.
		 * two trees, one kept for compatibility (which is not kept btw as dostype is different), and the other
		 * for 'faster directory listing', but not always in sync
		 */
		if (bc->buffer[BLK_EXTENSION(ds->vol)] != 0)
		{
			D(bug("[afs validate]: clearing dircache pointer\n"));
			bc->buffer[BLK_EXTENSION(ds->vol)] = 0;  
			verify_checksum(ds, bc->buffer);
			bc->flags |= BCF_WRITE;
		}

		for (i=BLK_TABLE_START; i<=BLK_TABLE_END(ds->vol); i++)
		{
			id = OS_BE2LONG(bc->buffer[i]);
			if (id != 0)
			{
				/* 
				 * proceed with block collection. unless user decides to stop validation
				 * move on and bug them about whatever is really bad
				 */
				id = collect_directory_blocks(ds, id);
				if (id == vr_Aborted)
					return id;

				/*
				 * restore current block
				 * this will work well if the modified block gets marked as BCF_WRITE
				 */
				bc = getBlock(ds->afs, ds->vol, blk);

				if (vr_OK != id)
				{
					/* it's a good idea to flag this requester, so it shows only once */
					if ((0 == (ds->flags & ValFlg_DisableReq_DataLossImminent)) && (0 == showError(ds->afs, ERR_DATA_LOSS_POSSIBLE)))
						return vr_Aborted;

					ds->flags |= ValFlg_DisableReq_DataLossImminent;
					bc->buffer[i] = 0;
					verify_checksum(ds, bc->buffer);
					bc->flags |= BCF_WRITE;
				}
			}
		}
	}

	/*
	 * collect file data blocks.
	 * some files are too large to fit in a single list of data blocks
	 * this covers scanning more blocks
	 *
	 * crap, i don't know if i should use entry_type=0 here, too :|
	 */
	if (entry_type == -3) 
	{
		collect_file_extensions(ds, bc);
		bc = getBlock(ds->afs, ds->vol, blk);
		id = OS_BE2LONG(bc->buffer[BLK_BYTE_SIZE(ds->vol)]);
		if (id > (ds->file_blocks * ds->vol->SizeBlock << 2))
		{
			bc->buffer[BLK_BYTE_SIZE(ds->vol)] = OS_LONG2BE(ds->file_blocks * ds->vol->SizeBlock << 2);
			verify_checksum(ds, bc->buffer);
			bc->flags |= BCF_WRITE;
		}
	}

	/*
	 * finally, move on to next file in this hash chain. IF next file suffers anything, remove it and following files from hash chain.
	 */
	id = OS_BE2LONG(bc->buffer[BLK_HASHCHAIN(ds->vol)]);
	if (id != 0)
	{
		D(bug("[afs validate]: collecting other items in this chain\n"));

		/*
		 * collect other elements
		 */
		id = collect_directory_blocks(ds, id);

		/*
		 * if aborted, simply quit
		 */
		if (id == vr_Aborted)
		{
			return id;
		}

		/*
		 * otherwise alter structure
		 */
		if (id != 0)
		{
			D(bug("[afs validate]: removing faulty chain\n"));
			bc = getBlock(ds->afs, ds->vol, blk);
			bc->buffer[BLK_HASHCHAIN(ds->vol)] = 0;
			verify_checksum(ds, bc->buffer);
			bc->flags |= BCF_WRITE;
		}
	}

	return vr_OK;
}

/*
 * record bitmap: stores bitmap in RAM (if you redefine a little) and saves it to disk.
 */
void record_bitmap(DiskStructure *ds)
{
	struct BlockCache *bc;
	ULONG *mem;
	ULONG i;

	for (i=0; i<ds->bm_lastblk; i++)
	{
		bc = getBlock(ds->afs, ds->vol, ds->bm_blocks[i]);
		mem = bc->buffer;

		CopyMemQuick(&((char*)ds->bitmap)[i*((ds->vol->SizeBlock-1)<<2)], &mem[1], (ds->vol->SizeBlock-1)<<2);
		verify_bm_checksum(ds, mem);
		bc->flags |= BCF_WRITE;
	}
}

/*
 * allocates enough store for the complete bitmap
 */
LONG bm_allocate_bitmap(DiskStructure *ds)
{
	ULONG i;
	ULONG bmap_blk_size;

	/*
	 * bitmap block size: blk_size - 4
	 */
	bmap_blk_size = ((ds->vol->SizeBlock-1)<<2);

	/*
	 * total bitmap size is calculated as follows:
	 * - total number of bits (rounded up to nearest multiple of 8) converted to bytes
	 * - the above rounded up to nearest size of a single block - 4 bytes
	 */ 
	i = (((ds->vol->countblocks + 7) >> 3) + (bmap_blk_size - 1)) / bmap_blk_size;

	D(bug("[afs validate]: allocating bitmaps (%ld bytes)\n", i * bmap_blk_size));
	ds->bitmap = AllocVec(i * bmap_blk_size, MEMF_ANY);

	D(bug("[afs validate]: allocating storage for bitmap blocks (%ld bytes)\n", i * sizeof(ULONG)));
	ds->bm_blocks	= (ULONG*)AllocVec(i * sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);

	D(bug("[afs validate]: allocating storage for bitmap extension blocks (%ld bytes) - way more than we really need\n", i * sizeof(ULONG)));
	ds->bme_blocks  = (ULONG*)AllocVec(i * sizeof(ULONG), MEMF_ANY | MEMF_CLEAR);

	if ((ds->bitmap == 0) || (ds->bm_blocks == 0) || (ds->bme_blocks == 0))
	{
		D(bug("[afs validate]: Unable to allocate memory for bitmap!\n"));
		return -1;
	}

	i *= bmap_blk_size;

	while (i--)
	{
		((UBYTE*)ds->bitmap)[i] = ~0;
	}

	ds->bm_lastblk  = 0;
	ds->bme_lastblk = 0;

	return 0;
}

void bm_free_bitmap(DiskStructure* ds)
{
	D(bug("[afs validate]: freeing previously allocated bitmaps\n"));
	if (ds->bitmap != 0)
		FreeVec(ds->bitmap);
	if (ds->bm_blocks != 0)
		FreeVec(ds->bm_blocks);
	if (ds->bme_blocks != 0)
		FreeVec(ds->bme_blocks);
}

BitmapResult bm_mark_block(DiskStructure *ds, ULONG block)
{
	if ((block < ds->vol->bstartblock) || (block >= ds->vol->countblocks))
	{
		D(bug("block %ld is out of disk area range\n", block));
		return st_OutOfRange;
	}

	block -= ds->vol->bootblocks;

#if AROS_BIG_ENDIAN
	if ((((ULONG*)ds->bitmap)[block >> 5] & (1 << (block & 31))) == 0)
#else
	if ((((ULONG*)ds->bitmap)[block >> 5] & (1 << ((block & 31)^24))) == 0)
#endif
	{
		D(bug("Duplicate block allocation for block %ld! Overlapping files?\n", (block+ds->vol->bstartblock)));
		return st_AlreadyInUse;
	}
	
#if AROS_BIG_ENDIAN
	((ULONG*)ds->bitmap)[block >> 5] &= ~(1 << (block & 31));
#else
	((ULONG*)ds->bitmap)[block >> 5] &= ~(1 << ((block & 31)^24));
#endif

	return st_OK;
}

void bm_add_bitmap_block(DiskStructure *ds, ULONG blk)
{
	ds->bm_blocks[ds->bm_lastblk] = blk;
	ds->bm_lastblk++;
}

void bm_add_bitmap_extension_block(DiskStructure *ds, ULONG blk)
{
	ds->bme_blocks[ds->bme_lastblk] = blk;
	ds->bme_lastblk++;
}


#endif
/* vim: set noet:fdm=marker:fmr={,} :*/
