/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
#include "bitmap.h"
#include "cache.h"
#include "checksums.h"
#include "error.h"
#include "afsblocks.h"
#include "baseredef.h"

/**********************************************
 Name  : countUsedBlocksInBitmap
 Descr.: count used blocks in a bitmap block
 Input : volume  -
         block   - bitmap block to count in
         maxcount- max blocks marked in a bitmap block
 Output: nr of used blocks
***********************************************/
ULONG countUsedBlocksInBitmap
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG block,
		ULONG maxcount
	)
{
UWORD i=1;
ULONG count=0,lg,bits;
struct BlockCache *blockbuffer;

	if ((blockbuffer=getBlock(afsbase, volume,block))==0)
	{
		showText(afsbase, "Couldn't read bitmap block %lu\nCount used blocks failed!",block);
		return count;
	}
	if (!calcChkSum(volume->SizeBlock, blockbuffer->buffer))
	{
		while (maxcount>=32)
		{
			lg=OS_BE2LONG(blockbuffer->buffer[i]);
			if (!lg)
			{
				count += 32;
			}
			else if (lg!=0xFFFFFFFF)
			{
				bits=1;
				do
				{
					if (!(bits & lg))
						count++;
					bits=bits<<1;
				} while (bits);
			}
			maxcount -=32;
			i++;
		}
		/* are there some bits left ? */
		if (maxcount)
		{
			lg=OS_BE2LONG(blockbuffer->buffer[i]);
			if (!lg)
			{
				count += maxcount;
			}
			else if (lg!=0xFFFFFFFF)
			{
				bits=1;
				for (;maxcount;maxcount--)
				{
					if (!(bits & lg))
						count++;
					bits=bits<<1;
				}
			}
		}
	}
	else
		showError(afsbase, ERR_CHECKSUM,block);
	return count;
}

/**************************************
 Name  : countUsedBlocks
 Descr.: count used blocks of a volume
 Input : volume  -
 Output: nr of used blocks of the volume
***************************************/
ULONG countUsedBlocks(struct AFSBase *afsbase, struct Volume *volume) {
UWORD i;
ULONG blocks;
ULONG maxinbitmap;
ULONG curblock;
ULONG count=0;
struct BlockCache *blockbuffer;

	blocks=volume->countblocks-volume->bootblocks; /* blocks to count */
	maxinbitmap = (volume->SizeBlock-1)*32;        /* max blocks marked in a bitmapblock */
	/* check bitmap blocks stored in rootblock */
	for (i=0;i<=24;i++)
	{
		if (maxinbitmap>blocks)
			maxinbitmap = blocks;
		if (volume->bitmapblockpointers[i])
		{
			count +=
				countUsedBlocksInBitmap
					(afsbase, volume, volume->bitmapblockpointers[i], maxinbitmap);
			blocks -= maxinbitmap;
		}
		if (blocks == 0)
			break;
	}
	/* check extension blocks if neccessary */
	if (blocks != 0)
	{
		curblock = volume->bitmapextensionblock;
		while (curblock != 0)
		{
			
			blockbuffer = getBlock(afsbase, volume, curblock);
			if (blockbuffer == NULL)
			{
				showText
					(
						afsbase,
						"Couldn't read bitmap extension block %lu\nCount used blocks failed!",
						curblock
					);
				return count;
			}
			blockbuffer->flags |= BCF_USED;
			for (i=0;i<volume->SizeBlock-1;i++)
			{
				if (maxinbitmap > blocks)
					maxinbitmap = blocks;
				if (blockbuffer->buffer[i] != 0)
				{
					count +=
						countUsedBlocksInBitmap
							(
								afsbase,
								volume,
								OS_BE2LONG(blockbuffer->buffer[i]),
								maxinbitmap
							);
					blocks -= maxinbitmap;
				}
				if (blocks == 0)
					break;
			}
			blockbuffer->flags &= ~BCF_USED;
			if (blocks == 0)
				break;
			curblock = OS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
		}
		if (blocks != 0)
			showError(afsbase, ERR_MISSING_BITMAP_BLOCKS);
	}
	return count;
}

ULONG createNewBitmapBlocks(struct AFSBase *afsbase, struct Volume *volume) {
struct BlockCache *bitmapblock,*extensionblock;
ULONG i, blocks, maxinbitmap;

	/* initialize a block as a bitmap block */
	extensionblock = getFreeCacheBlock(afsbase, volume, -1);
	extensionblock->flags |= BCF_USED;
	bitmapblock = getFreeCacheBlock(afsbase, volume, volume->rootblock+1);
	/* all blocks are free */
	for (i=1;i<volume->SizeBlock;i++)
		bitmapblock->buffer[i] = 0xFFFFFFFF;
	bitmapblock->buffer[0] = 0;
	bitmapblock->buffer[0] = OS_LONG2BE(0-calcChkSum(volume->SizeBlock,bitmapblock->buffer));
	/* get nr of blocks to mark in bitmap blocks */
	blocks = volume->countblocks - volume->bootblocks;
	/* calc max blocks that can be marked in a single bitmap block */
	maxinbitmap = (volume->SizeBlock-1)*32;
	/* first create bitmapblocks stored in rootblock */
	for (i=0;i<=24;i++)
	{
		if (maxinbitmap > blocks)
			maxinbitmap = blocks;
		volume->bitmapblockpointers[i] = bitmapblock->blocknum;
		writeBlock(afsbase, volume, bitmapblock, -1);
		bitmapblock->blocknum += 1;
		blocks = blocks - maxinbitmap;
		if (blocks == 0)
		{
			i++;
			while (i<=24)
				volume->bitmapblockpointers[i++] = 0;
		}
	}
	/* check extension blocks if neccessary */
	if (blocks != 0)
	{
		volume->bitmapextensionblock=bitmapblock->blocknum;
		do
		{
			/* initialize extensionblock with zeros */
			extensionblock->blocknum = bitmapblock->blocknum;
			for (i=0;i<volume->SizeBlock;i++)
				extensionblock->buffer[i] = 0;
			/* fill extensionblock and write bitmapblocks */
			for (i=0;i<volume->SizeBlock-1;i++)
			{
				if (maxinbitmap > blocks)
					maxinbitmap = blocks;
				bitmapblock->blocknum += 1;
				extensionblock->buffer[i] = OS_LONG2BE(bitmapblock->blocknum);
				writeBlock(afsbase, volume, bitmapblock, -1);
				blocks = blocks-maxinbitmap;
				if (blocks == 0)
					break;
			}
			if (blocks != 0)	/*write another extensionblock ? */
			{
				/* fill next extension */
				extensionblock->buffer[volume->SizeBlock-1]=OS_LONG2BE(bitmapblock->blocknum+1);
			}
			writeBlock(afsbase, volume, extensionblock, -1);
			bitmapblock->blocknum += 1;
		} while (blocks != 0);
	}
	else
		volume->bitmapextensionblock = 0;
	extensionblock->flags &= ~BCF_USED;
	return DOSTRUE;
}

LONG setBitmapFlag(struct AFSBase *afsbase, struct Volume *volume, LONG flag) {
struct BlockCache *blockbuffer;

	D(bug("[afs] setBitmapFlag()\n"));
	blockbuffer = getBlock(afsbase, volume, volume->rootblock);
	if (blockbuffer == NULL)
		return DOSFALSE;
	blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)] = flag;
	if ((blockbuffer->flags & BCF_WRITE) == 0)
	{
		writeBlock(afsbase, volume, blockbuffer, BLK_CHECKSUM);
		blockbuffer->flags |= BCF_WRITE;
	}
	else
		writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
	return DOSTRUE;
}

LONG invalidBitmap(struct AFSBase *afsbase, struct Volume *volume) {

	volume->lastextensionblock = 0;
	volume->lastposition = 0;
	volume->bstartblock = volume->bootblocks;	/* reserved */
	volume->bitmapblock = getBlock(afsbase, volume, volume->bitmapblockpointers[volume->lastposition]);
	if (volume->bitmapblock != NULL)
	{
		volume->bitmapblock->flags |= BCF_USED;
		if (setBitmapFlag(afsbase, volume, 0))
		{
			return DOSTRUE;
		}
		volume->bitmapblock->flags &= ~BCF_USED;
	}
	return DOSFALSE;
}

LONG validBitmap(struct AFSBase *afsbase, struct Volume *volume) {

	if (volume->bitmapblock->flags & BCF_WRITE)
	{
		writeBlockDeferred(afsbase, volume, volume->bitmapblock, 0);
	}
	volume->bitmapblock->flags &= ~BCF_USED;
	setBitmapFlag(afsbase, volume, -1);
	return DOSTRUE;
}

/*************************************************
 Name  : gotoBitmapBlock
 Descr.: go to the bitmap block where a block is marked
 Input : volume - which volume
         block  - the block we want to get infos about
         longnr - returns the n-th long block is associated to
         bitnr  - returns the bitnr block is associated to
 Note  : bitmapblock is changed (should be initialized first!)
**************************************************/
LONG gotoBitmapBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG block,
		ULONG *longnr,
		ULONG *bitnr
	)
{
struct BlockCache *extensionblock;
ULONG bblock,togo,maxinbitmap;

	block -= volume->bootblocks;  /* reserved blocks are not marked */
	maxinbitmap = (volume->SizeBlock-1)*32; /* max blocks marked in a bitmapblock */
	*bitnr = block % maxinbitmap; /* in the bblock-th block we have to mark the bit-th bit */
	*longnr = *bitnr/32+1;        /* int the longnr-th LONG is "block" marked (+1 because [0]=BLK_CHECKSUM) */
	*bitnr = *bitnr % 32;         /* in the bit-th bit of LONG "longnr" "block" is marked */
	/* load new block ? */
	if (
			(block<volume->bstartblock) ||
			(block>=(volume->bstartblock+maxinbitmap))
		)
	{
		bblock = block/maxinbitmap; /* in the bblock-th bitmap block is "block" marked */
		if (volume->bitmapblock->flags & BCF_WRITE)
		{
			writeBlockDeferred(afsbase, volume, volume->bitmapblock, 0);
		}
		volume->bitmapblock->flags &= ~BCF_USED;
		/* load new block */
		if (bblock<=24)
		{
			volume->bitmapblock = getBlock(afsbase, volume, volume->bitmapblockpointers[bblock]);
		}
		else
		{
			volume->lastextensionblock = volume->bitmapextensionblock;
//kprintf("get block from extension %lu\n", volume->lastextensionblock);
			/* 25 entries in rootblock already processed */
			volume->lastposition = bblock-25;
			/* do we have to go to another extensionblock ? */
			togo = volume->lastposition / (volume->SizeBlock-1);
//kprintf("togo=%lu\n", togo);
			volume->lastposition %= volume->SizeBlock-1;
//kprintf("lp=%lu\n", volume->lastposition);
			extensionblock=getBlock(afsbase, volume, volume->lastextensionblock);
			if (extensionblock == NULL)
			{
				showText(afsbase, "Could not read bitmap extionsion block %lu!", volume->lastextensionblock);
				return FALSE;
			}
//if (volume->lastposition == 5)
//{
//	kprintf("00: %08lx %08lx %08lx %08lx\n", extensionblock->buffer[0], extensionblock->buffer[1], extensionblock->buffer[2], extensionblock->buffer[3]);
//	kprintf("10: %08lx %08lx %08lx %08lx\n", extensionblock->buffer[4], extensionblock->buffer[5], extensionblock->buffer[6], extensionblock->buffer[7]);
//}
			while (togo != 0)
			{
				extensionblock =
					getBlock
						(
							afsbase,
							volume,
							OS_BE2LONG(extensionblock->buffer[volume->SizeBlock-1])
						);
				if (extensionblock == NULL)
				{
					showText(afsbase, "Could not read bitmap extension block %lu!", OS_BE2LONG(extensionblock->buffer[volume->SizeBlock-1]));
					return FALSE;
				}
				volume->lastextensionblock = extensionblock->blocknum;
				togo--;
			}
			volume->bitmapblock =
				getBlock
					(
						afsbase,
						volume,
						OS_BE2LONG(extensionblock->buffer[volume->lastposition])
					);
		}
		volume->bstartblock = bblock*maxinbitmap;
		if (volume->bitmapblock == NULL)
		{
			showText(afsbase, "Could not read bitmap block!");
			return FALSE;
		}
		volume->bitmapblock->flags |= BCF_USED;
	}
	return TRUE;
}

LONG markBlock(struct AFSBase *afsbase, struct Volume *volume, ULONG block, ULONG mode) {
ULONG bitnr, longnr,null=0;

	D(bug("[afs]    markBlock: block=%lu mode=%lu\n",block,mode));
	if (block>=volume->countblocks)
		null = null/null;
	if (!gotoBitmapBlock(afsbase, volume, block, &longnr, &bitnr))
		return 0;
	if (mode)
	{
		/* free a block */
		volume->bitmapblock->buffer[longnr] =
			OS_LONG2BE(OS_BE2LONG(volume->bitmapblock->buffer[longnr]) | (1 << bitnr));
		volume->usedblockscount -= 1;
		if (
				(
					(volume->lastaccess<volume->rootblock) && (block>=volume->rootblock)
				) || // 1. case
				(
					(
						((volume->lastaccess>=volume->rootblock) && (block>=volume->rootblock)) ||	// 2.case and ...
						((volume->lastaccess<volume->rootblock) && (block<volume->rootblock)) 		// 3.case with ...
					) &&
					(block<volume->lastaccess)
				)
			)
		{
				volume->lastaccess = block;
		}
	}
	else
	{
		volume->bitmapblock->buffer[longnr] =
			OS_LONG2BE(OS_BE2LONG(volume->bitmapblock->buffer[longnr]) & ~(1 << bitnr));
		volume->usedblockscount += 1;
		volume->lastaccess = block; /* all blocks before "block" are used! */
	}
	volume->bitmapblock->flags |= BCF_WRITE;
	return DOSTRUE;
}

ULONG findFreeBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG togo,
		ULONG longnr,
		ULONG block
	)
{
ULONG maxinbitmap,maxblocks;
ULONG bits,trash,lg;

	maxinbitmap = (volume->SizeBlock-1)*32;  /* max blocks marked in a bitmapblock */
	maxblocks = maxinbitmap-((longnr-1)*32); /* blocks left to check */
	if (maxblocks > togo)
		maxblocks = togo;
	for (;;)
	{
		togo -= maxblocks;
		while (maxblocks>=32)
		{
			/* do we have a free block ?
				if yes search within this long which block */
			lg = OS_BE2LONG(volume->bitmapblock->buffer[longnr]);
			if (lg != 0)
			{
				bits = 1;
				do
				{
					if (bits & lg)
						return block;
					bits = bits<<1;
					block++;
				} while (bits != 0);
			}
			else
				block += 32;
			longnr++;
			maxblocks -= 32;
		}
		if (maxblocks != 0)
		{
			lg = OS_BE2LONG(volume->bitmapblock->buffer[longnr]);
			bits = 1;
			for (;maxblocks;maxblocks--)
			{
				if (bits & lg)
					return block;
				bits = bits<<1;
				block++;
			}
		}
		if (togo == 0)
			break;
		if (!gotoBitmapBlock(afsbase, volume,block,&longnr,&trash))
			return 0;
		if ((longnr!=1) || (trash!=0))
			showText(afsbase, "Wrong bitmapblockjump!");
		maxblocks = togo<maxinbitmap ? togo : maxinbitmap;
		longnr = 1;		// skip checksum
	}
	return 0;
}

ULONG getFreeBlock(struct AFSBase *afsbase, struct Volume *volume) {
ULONG block,longnr,bitnr,lg,maxbitcount;

	D(bug("[afshandler] %s: enter\n", __FUNCTION__));
	/* check all blocks after rootblock */
	block = volume->lastaccess;
	/*
		got exact position of last accessed block
		this will update volume->bitmapblock and return
		the long number and bit number that block is
	*/
	if (!gotoBitmapBlock(afsbase, volume, block, &longnr, &bitnr))
		return 0;
	if (bitnr != 0)
	{
		maxbitcount = volume->countblocks-block;
		if (maxbitcount != 0)
		{
			maxbitcount = ((32-bitnr)<maxbitcount) ? 32 : bitnr+maxbitcount;
			lg = OS_BE2LONG(volume->bitmapblock->buffer[longnr]);
			for (;bitnr<maxbitcount;bitnr++)
			{
				if ((1<<bitnr) & lg)
					return block;
				block++;
			}
			longnr++;
		}
		if (block>=volume->countblocks)
			block = volume->bootblocks;
	}
	if (block>=volume->rootblock)
	{
		block =
			findFreeBlock
				(
					afsbase,
					volume,
					volume->countblocks-block-1,
					longnr,
					block
				);
		if (block != 0)
			return block;
		block = volume->bootblocks;
	}
	if (!gotoBitmapBlock(afsbase, volume, block, &longnr, &bitnr))
		return 0;
	block = findFreeBlock(afsbase, volume, volume->rootblock-block,longnr, block);
	return block;
}

ULONG allocBlock(struct AFSBase *afsbase, struct Volume *volume) {
ULONG block;

	block = getFreeBlock(afsbase, volume);
	if (block != 0)
	{
		D(bug("[afs]    allocBlock: found a free block on %lu\n", block));
		if (!markBlock(afsbase, volume,block,0))
			block = 0;
	}
	return block;
}

