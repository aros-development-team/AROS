/*
	 Copyright © 1995-2007, The AROS Development Team. All rights reserved.
	 $Id: validator.c 25132 2007-01-03 01:43:09Z neil $
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      created disk validation procedures
 * 03-jan-2008 [Tomasz Wiszkowski]      updated procedure to strip fake dircache blocks
 *                                      no directory cache handling present here.
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected procedure to *ignore* data block sizes 
 *                                      for OFS volumes since DOSTYPE does not differentiate them,
 *                                      corrected tabulation.
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
#endif

/*******************************************
 Name  : checkValid
 Descr : verify whether disk is validated and therefore writable
         since we need at least a stub function, it has to be
         declared outside __AROS__ scope
 Input : afsbase, volume
 Output: 0 for unvalidated disc, 1 otherwise
 Author: Tomasz Wiszkowski
********************************************/
LONG checkValid(struct AFSBase *afs, struct Volume *vol)
{
#ifdef __AROS__
	if (vol->state == ID_VALIDATED)
		return 1;

	if (showError(afs, ERR_DISKNOTVALID))
	{
		if (vr_OK == launchValidator(afs, vol))
			return 1;
	}		  
	return 0;
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
 * as the name says: collect all bitmap blocks
 */
ValidationResult collect_bitmap_blocks(DiskStructure* ds, ULONG ext)
{
	struct BlockCache *bc;
	ULONG *mem;
	ULONG i;
	ULONG blk;

	/*
	 * check range first
	 */
	if (0 == check_block_range(ds, ext))
		return vr_BlockOutsideDisk;

	/*
	 * try to mark current block as used
	 */
	if (st_OK != bm_mark_block(ds, ext))
		return vr_BlockUsedTwice;

	bc = getBlock(ds->afs, ds->vol, ext);
	mem = bc->buffer;

	for (i=0; i<ds->vol->SizeBlock-1; i++)
	{
		blk = OS_BE2LONG(mem[i]);
		if (blk == 0)
			continue;

		if (0 == check_block_range(ds, blk)) 
		{
			return vr_BlockOutsideDisk;
		}

		if (bm_mark_block(ds, blk) != st_OK)
		{
			return vr_BlockUsedTwice;
		}

		bm_add_bitmap_block(ds, blk);
	}

	blk = OS_BE2LONG(mem[i]);

	if (blk != 0)
		return collect_bitmap_blocks(ds, blk);

	return vr_OK;
}

/*
 * collect directories
 */
ValidationResult collect_directory_blocks(DiskStructure *ds, ULONG blk)
{
	struct BlockCache *bc;
	ULONG *mem;
	LONG primary_type;
	LONG entry_type;
	ULONG id;

	D(bug("[afs validate]: analyzing block %lu\n", blk));

	/*
	 * check range first. Note that the whole process here is not 'reversible':
	 * if you mark bitmap blocks as used, and then just return in the middle,
	 * it won't get freed automagically, so mark it only when you know it should be there.
	 */
	if (0 == check_block_range(ds, blk))
		return vr_BlockOutsideDisk;

	/*
	 * read block from disk
	 */
	bc = getBlock(ds->afs, ds->vol, blk);
	if (bc == 0)
	{
		D(bug("[afs validate]: NO BLOCK RETURNED!\n"));
	}

	bc->flags |= BCF_USED;
	mem = bc->buffer;

	/*
	 * check types first: we expect block of T_SHORT (2) or T_LIST (16) type
	 */
	entry_type = OS_BE2LONG(mem[BLK_SECONDARY_TYPE(ds->vol)]);
	primary_type = OS_BE2LONG(mem[BLK_PRIMARY_TYPE]);
	if ((primary_type != T_SHORT) && 
		 (primary_type != T_LIST))
	{
		D(bug("[afs validate]: block is not of known-type (%08lx). asking whether to correct\n", primary_type));

		bc->flags &= ~BCF_USED;
		bc = 0;
		mem = 0;
		if (blk == ds->vol->rootblock)
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
	if (bm_mark_block(ds, blk))
	{
		D(bug("[afs validate]: block %lu used twice! aborting!\n", blk));
		bc->flags &= ~BCF_USED;
		bc = 0;
		mem = 0;
		return vr_BlockUsedTwice;
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
		ULONG i;
		ULONG blk;
		ValidationResult res = vr_OK;

		for (i=BLK_BITMAP_POINTERS_START(ds->vol); 
			  i<=BLK_BITMAP_POINTERS_END(ds->vol);
			  ++i)
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
			{
				bc->flags &= ~BCF_USED;
				bc = 0;
				mem = 0;
				return vr_BlockOutsideDisk;
			}

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
				bc->flags &= ~BCF_USED;
				bc = 0;
				mem = 0;
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
		if ((blk = OS_BE2LONG(mem[BLK_BITMAP_EXTENSION(ds->vol)])) != 0) 
			res = collect_bitmap_blocks(ds, blk);
	  
		if (res != vr_OK)
		{
			bc->flags &= ~BCF_USED;
			bc = 0;
			mem = 0;
			return res;
		}

		/*
		 * initially -- mark bitmap status as valid
		 */
		mem[BLK_BITMAP_VALID_FLAG(ds->vol)] = ~0;
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
		ValidationResult res = vr_OK;

		/*
		 * clear directory cache block.
		 * fancy thing about directory cache: it is the best way to run into inconsistencies between file trees.
		 * two trees, one kept for compatibility (which is not kept btw as dostype is different), and the other
		 * for 'faster directory listing', but not always in sync
		 */
		if (mem[BLK_EXTENSION(ds->vol)] != 0)
		{
			D(bug("[afs validate]: clearing dircache pointer\n"));
			mem[BLK_EXTENSION(ds->vol)] = 0;  
			bc->flags |= BCF_WRITE;
		}

		for (i=BLK_TABLE_START; i<=BLK_TABLE_END(ds->vol); i++)
		{
			id = OS_BE2LONG(mem[i]);
			if (id != 0)
			{
				/*
				 * release current block
				 */
				bc->flags &= ~BCF_USED;
				bc = 0;
				mem = 0;

				/* 
				 * proceed with block collection. unless user decides to stop validation
				 * move on and bug them about whatever is really bad
				 */
				res = collect_directory_blocks(ds, id);
				if (res == vr_Aborted)
					return res;

				/*
				 * restore current block
				 * this will work well if the modified block gets marked as BCF_WRITE
				 */
				bc = getBlock(ds->afs, ds->vol, blk);
				bc->flags |= BCF_USED;
				mem = bc->buffer;

				if (vr_OK != res)
				{
					/* it's a good idea to flag this requester, so it shows only once */
					if ((0 == (ds->flags & ValFlg_DisableReq_DataLossImminent)) && (0 == showError(ds->afs, ERR_DATA_LOSS_POSSIBLE)))
					{
						bc->flags &= ~BCF_USED;
						return vr_Aborted;
					}
					ds->flags |= ValFlg_DisableReq_DataLossImminent;
					mem[i] = 0;
					bc->flags |= BCF_WRITE;
				}
			}
		}
	}

	if ((primary_type == T_SHORT) && (entry_type == -3))
	{
		ds->max_file_len = 0;
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
		ValidationResult res = vr_OK;
		LONG clear = 0;
		LONG i;

		/*
		 * collect all file data blocks
		 */
		for (i=BLK_TABLE_START; i<=BLK_TABLE_END(ds->vol); i++)
		{
			/*
			 * if file is corrupted, then we will simply attempt to trim it.
			 */
			if (clear != 0)
			{
				mem[i] = 0;
				bc->flags |= BCF_WRITE;
				continue;
			}
			
			/*
			 * pick up next data block. see if it is within range and available for that file
			 * if not - remove it and following blocks from the list (=truncate file), otherwise
			 * increment maximum allowed file length
			 */
			id = OS_BE2LONG(mem[i]);
			if (id != 0)
			{
				if ((0 == check_block_range(ds, id)) ||
					 (st_OK != bm_mark_block(ds, id)))
				{
					clear = 1;
					mem[i] = 0;
					bc->flags |= BCF_WRITE;
					continue;
				}
				/*
				 * actually, the OFS uses BLOCK_SIZE-24
				 * but the case where file is short is rare
				 */
				/*
				 * the DOSTYPE ID DOES NOT HOLD INFORMATION WHETHER WE DEAL WITH FFS OR OFS DISK
				 * THIS LEADS TO FILE TRUNCATION WHICH IS NOT DESIRED
				 * DO NOT UNCOMMENT THIS UNLESS YOU KNOW WHAT YOU ARE DEALING WITH
				 */
				/*
				if ((ds->vol->dostype == ID_DOS_DISK) || (ds->vol->dostype == ID_INTER_DOS_DISK))
					ds->max_file_len += BLOCK_SIZE(ds->vol) - 24;
				else
					ds->max_file_len += BLOCK_SIZE(ds->vol);
				*/
				ds->max_file_len += BLOCK_SIZE(ds->vol);
			}
		}
	  
		/*
		 * having collected all data blocks from this block, see if there is more available.
		 * also, keep in mind, that truncating file means also removing redundant file extension blocks
		 * so remove them upon truncate
		 */
		if (clear != 0)
		{
			mem[BLK_EXTENSION(ds->vol)] = 0;
			bc->flags |= BCF_WRITE;
		}
		else
		{ 
			/*
			 * check file extension blocks
			 */
			D(bug("[afs validate]: checking file extension blocks\n"));

			id = OS_BE2LONG(mem[BLK_EXTENSION(ds->vol)]);
			if (id != 0)
			{
				/*
				 * release current block
				 */
				bc->flags &= ~BCF_USED;
				bc = 0;
				mem = 0;

				/* 
				 * proceed with block collection. unless user decides to stop validation
				 * move on and bug them about whatever is really bad
				 */
				res = collect_directory_blocks(ds, id);
				if (res == vr_Aborted)
					return res;

				/*
				 * restore current block
				 * this will work well if the modified block gets marked as BCF_WRITE
				 */
				bc = getBlock(ds->afs, ds->vol, blk);
				bc->flags |= BCF_USED;
				mem = bc->buffer;

				/*
				 * if checking failed, it could only mean our block was outside range or already occupied
				 * in this case - we truncate file.
				 */

				if (res != vr_OK)
				{
					D(bug("[afs validate]: file extension block is faulty. truncating file\n"));
					mem[BLK_EXTENSION(ds->vol)] = 0;
					bc->flags |= BCF_WRITE;
					res = vr_OK;
				}
			}
			else
			{
				D(bug("[afs validate]: no more file extension blocks found.\n"));
			}
		}
	}

	if ((primary_type == T_SHORT) && (entry_type == -3))
	{
		D(bug("[afs validate]: file length: %lu. max allowed file length: %lu\n", OS_BE2LONG(mem[BLK_BYTE_SIZE(ds->vol)]), ds->max_file_len));
		if (OS_BE2LONG(mem[BLK_BYTE_SIZE(ds->vol)]) > ds->max_file_len)
		{
			mem[BLK_BYTE_SIZE(ds->vol)] = OS_LONG2BE(ds->max_file_len);
			bc->flags |= BCF_WRITE;
		}  
		ds->max_file_len = 0;
	}

	/*
	 * finally, move on to next file in this hash chain. IF next file suffers anything, remove it and following files from hash chain.
	 */
	id = OS_BE2LONG(mem[BLK_HASHCHAIN(ds->vol)]);
	if (id != 0)
	{
		ValidationResult res;
		
		D(bug("[afs validate]: collecting other items in this chain\n"));

		/*
		 * release current block
		 */
		bc->flags &= ~BCF_USED;
		bc = 0;
		mem = 0;

		/*
		 * collect other elements
		 */
		res = collect_directory_blocks(ds, id);

		/*
		 * if aborted, simply quit
		 */
		if (res == vr_Aborted)
		{
			return res;
		}

		/*
		 * otherwise alter structure
		 */
		if (res != 0)
		{
			bc = getBlock(ds->afs, ds->vol, blk);
			mem = bc->buffer;
			D(bug("[afs validate]: removing faulty chain\n"));
			mem[BLK_HASHCHAIN(ds->vol)] = 0;
			bc->flags |= BCF_WRITE;
		}
	}
	else
	{
		bc->flags &= ~BCF_USED;
		bc = 0;
		mem = 0;
	}

	/*
	 * check (and update) block checksum.
	 */
	bc = getBlock(ds->afs, ds->vol, blk);
	mem = bc->buffer;

	if (verify_checksum(ds, mem) != 0)
	{
		D(bug("[afs validate]: block checksum does not match.\n"));
		bc->flags |= BCF_WRITE;
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
