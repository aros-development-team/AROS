#define DEBUG 1

#include <aros/debug.h>
#include <aros/macros.h>

#include "bitmap.h"
#include "blockaccess.h"
#include "checksums.h"
#include "error.h"
#include "afsblocks.h"

struct BlockCache *bitmapblock;	// last bitmap block used for marking
ULONG startblock;						// first block marked in bitmapblock
// the following vars are not really used yet
ULONG lastextensionblock;			// last used extensionblock (0=volume->bitmapblocks)
ULONG lastposition;					// last position in extensionblock
ULONG lastaccess=0;					// last marked block

ULONG countUsedBlocksInBitmap(struct Volume *volume,ULONG block, ULONG maxcount) {
UWORD i=1;
ULONG count=0,lg,bits;
struct BlockCache *blockbuffer;

	if ((blockbuffer=getBlock(volume,block))==0) {
		showText("Couldnt read bitmap block %d\nCount used blocks failed!",block);
		return count;
	}
	if (!calcChkSum(volume, blockbuffer->buffer)) {
		while (maxcount>=32) {
			lg=AROS_BE2LONG(blockbuffer->buffer[i]);
			if (!lg) {
				count += 32;
			}
			else if (lg!=0xFFFFFFFF) {
				bits=1;
				do {
					if (!(bits & lg))
						count++;
					bits=bits<<1;
				} while (bits);
			}
			maxcount -=32;
			i++;
		}
		// are there some bits left ?
		if (maxcount) {
			lg=AROS_BE2LONG(blockbuffer->buffer[i]);
			if (!lg) {
				count += maxcount;
			}
			else if (lg!=0xFFFFFFFF) {
				bits=1;
				for (;maxcount;maxcount--) {
					if (!(bits & lg)) count++;
					bits=bits<<1;
				}
			}
		}
	}
	else
		showError(ERR_CHECKSUM,block);
	return count;
}

ULONG countUsedBlocks(struct Volume *volume) {
UWORD i;
ULONG blocks;
ULONG maxinbitmap;
ULONG curblock;
ULONG count=0;
struct BlockCache *blockbuffer;

	//check bitmapblocks stored in rootblock
	blocks=volume->rootblock*2-volume->bootblocks;					//blocks to count
	maxinbitmap=(volume->SizeBlock-1)*32;			//max blocks marked in a bitmapblock
	for (i=0;i<=24;i++) {
		if (maxinbitmap>blocks) maxinbitmap=blocks;
		if (volume->bitmapblockpointers[i]) {
			count=count+countUsedBlocksInBitmap(volume,volume->bitmapblockpointers[i],maxinbitmap);
			blocks -= maxinbitmap;
		}
		if (blocks==0) break;
	}
	//check extension blocks if neccessary
	if (blocks) {
		curblock=volume->bitmapextensionblock;
		while (curblock) {
			if (!(blockbuffer=getBlock(volume,curblock))) {
				showText("Couldnt read bitmap extension block %d\nCount used blocks failed!",curblock);
				return count;
			}
			blockbuffer->flags |= BCF_USED;
			for (i=0;i<volume->SizeBlock-1;i++) {
				if (maxinbitmap>blocks) maxinbitmap=blocks;
				if (blockbuffer->buffer[i]) {
					count +=countUsedBlocksInBitmap(volume,AROS_BE2LONG(blockbuffer->buffer[i]),maxinbitmap);
					blocks -= maxinbitmap;
				}
				if (blocks==0) break;
			}
			if (blocks==0) break;
			curblock=AROS_BE2LONG(blockbuffer->buffer[volume->SizeBlock-1]);
			blockbuffer->flags &= ~BCF_USED;
		}
		if (blocks) showError(ERR_MISSING_BITMAP_BLOCKS);
	}
	return count;
}

ULONG createNewBitmapBlocks(struct Volume *volume) {
struct BlockCache *bitmapblock,*extensionblock;
ULONG i,blocks,maxinbitmap;

	//initialize a block as a bitmap block
	extensionblock=getFreeCacheBlock(volume,-1);
	extensionblock->flags |= BCF_USED;
	bitmapblock=getFreeCacheBlock(volume,volume->rootblock+1);
	for (i=1;i<volume->SizeBlock;i++)
		bitmapblock->buffer[i]=0xFFFFFFFF;								//all blocks are free
	bitmapblock->buffer[0]=AROS_LONG2BE(0-calcChkSum(volume,bitmapblock->buffer));
	blocks=volume->rootblock*2-volume->bootblocks;					//blocks to mark in bitmaps
	maxinbitmap=(volume->SizeBlock-1)*32;			//max blocks marked in a bitmapblock
	//first create bitmapblocks stored in rootblock
	for (i=0;i<=24;i++) {
		if (maxinbitmap>blocks)
			maxinbitmap=blocks;
		volume->bitmapblockpointers[i]=bitmapblock->blocknum;
		writeBlock(volume,bitmapblock);
		bitmapblock->blocknum += 1;
		blocks=blocks-maxinbitmap;
		if (blocks==0)
			for (;i<=24;i++)
				volume->bitmapblockpointers[i]=0;
	}
	//check extension blocks if neccessary
	if (blocks) {
		volume->bitmapextensionblock=bitmapblock->blocknum;
		do {
			// initialize extensionblock with zeros
			extensionblock->blocknum=bitmapblock->blocknum;
			for (i=0;i<volume->SizeBlock;i++)
				extensionblock->buffer[i]=0;
			// fill extensionblock and write bitmapblocks
			for (i=0;i<volume->SizeBlock-1;i++) {
				if (maxinbitmap>blocks)
					maxinbitmap=blocks;
				bitmapblock->blocknum += 1;
				extensionblock->buffer[i]=AROS_LONG2BE(bitmapblock->blocknum);
				writeBlock(volume, bitmapblock);
				blocks=blocks-maxinbitmap;
				if (blocks==0) break;
			}
			if (blocks)	//write another extensionblock ? fill next extension
				extensionblock->buffer[volume->SizeBlock-1]=AROS_LONG2BE(bitmapblock->blocknum+1);
			writeBlock(volume, extensionblock);
			bitmapblock->blocknum += 1;
		} while (blocks);
	}
	else
		volume->bitmapextensionblock=0;
	extensionblock->flags &= ~BCF_USED;
	return DOSTRUE;
}

LONG setBitmapFlag(struct Volume *volume, LONG flag) {
struct BlockCache *blockbuffer;

	blockbuffer=getBlock(volume,volume->rootblock);
	if (!blockbuffer)
		return DOSFALSE;
	blockbuffer->buffer[BLK_BITMAP_VALID_FLAG(volume)]=flag;
	blockbuffer->buffer[BLK_CHECKSUM]=0;
	blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume,blockbuffer->buffer));
	writeBlock(volume, blockbuffer);
	// in case of a concurrent access
	blockbuffer->blocknum=0;
	blockbuffer->volume=0;
	blockbuffer->acc_count=0;
	return DOSTRUE;
}

LONG invalidBitmap(struct Volume *volume) {

	lastextensionblock=0;
	lastposition=0;
	startblock=volume->bootblocks;	//reserved
	bitmapblock=getBlock(volume,volume->bitmapblockpointers[lastposition]);
	if (!bitmapblock)
		return DOSFALSE;
	bitmapblock->flags |= BCF_USED;
	if (!setBitmapFlag(volume,0)) {
		bitmapblock->flags &= ~BCF_USED;
		return DOSFALSE;
	}
	return DOSTRUE;
}

LONG validBitmap(struct Volume *volume) {

	if (bitmapblock->flags & BCF_WRITE) {
		bitmapblock->buffer[0]=0;
		bitmapblock->buffer[0]=AROS_LONG2BE(0-calcChkSum(volume,bitmapblock->buffer));
		writeBlock(volume, bitmapblock);
		bitmapblock->flags &= ~BCF_WRITE;
	}
	bitmapblock->flags &= ~BCF_USED;
	setBitmapFlag(volume, -1);
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
void gotoBitmapBlock(struct Volume *volume, ULONG block, ULONG *longnr, ULONG *bitnr) {
struct BlockCache *extensionblock;
ULONG bblock,togo,maxinbitmap;

	block -= volume->bootblocks;	//reserved blocks are not marked
	maxinbitmap = (volume->SizeBlock-1)*32;			//max blocks marked in a bitmapblock
	*bitnr = block % maxinbitmap;	// in the bblock-th block we have to mark the bit-th bit
	*longnr = *bitnr/32+1;			// int the longnr-th LONG is "block" marked (+1 because [0]=BLK_CHECKSUM)
	*bitnr = *bitnr % 32;			// in the bit-th bit of LONG "longnr" "block" is marked
	// load new block ?
	if ((block<startblock) || (block>=(startblock+maxinbitmap))) {
		bblock = block/maxinbitmap;	// in the bblock-th bitmap block is "block" marked
		if (bitmapblock->flags & BCF_WRITE) {
			bitmapblock->buffer[0]=0;
			bitmapblock->buffer[0]=AROS_LONG2BE(0-calcChkSum(volume,bitmapblock->buffer));
			writeBlock(volume, bitmapblock);
			bitmapblock->flags &= ~BCF_WRITE;
		}
		bitmapblock->flags &= ~BCF_USED;
		// load new block
		if (bblock<=24) {
			bitmapblock=getBlock(volume,volume->bitmapblockpointers[bblock]);
		}
		else {
			lastextensionblock=volume->bitmapextensionblock;
			lastposition = bblock-25;				// 25 entries in rootblock already processed
			togo = lastposition / (volume->SizeBlock-1);	// do we have to go to another extensionblock ?
			lastposition %= volume->SizeBlock-1;
			extensionblock=getBlock(volume, lastextensionblock);
			if (!extensionblock)
				return;
			while (togo) {
				extensionblock=getBlock(volume, AROS_BE2LONG(extensionblock->buffer[volume->SizeBlock-1]));
				if (!extensionblock)
					return;
				lastextensionblock=extensionblock->blocknum;
				togo--;
			}
			bitmapblock=getBlock(volume, AROS_BE2LONG(extensionblock->buffer[lastposition]));
		}
		startblock=bblock*maxinbitmap;
		if (!bitmapblock)
			return;
		bitmapblock->flags |= BCF_USED;
	}
}

LONG markBlock(struct Volume *volume, ULONG block, ULONG mode) {
ULONG bitnr, longnr;

	D(bug("afs.handler:    markBlock: block=%ld mode=%ld\n",block,mode));
	gotoBitmapBlock(volume, block, &longnr, &bitnr);
	if (mode) { //free a block
		bitmapblock->buffer[longnr] = AROS_LONG2BE(AROS_BE2LONG(bitmapblock->buffer[longnr]) | (1 << bitnr));
		volume->usedblockscount -= 1;
	}
	else {
		bitmapblock->buffer[longnr] = AROS_LONG2BE(AROS_BE2LONG(bitmapblock->buffer[longnr]) & ~(1 << bitnr));
		volume->usedblockscount += 1;
	}
	lastaccess=block;
	bitmapblock->flags |= BCF_WRITE;
	return DOSTRUE;
}

ULONG findFreeBlock(struct Volume *volume, ULONG togo, ULONG longnr, ULONG block) {
ULONG maxinbitmap,maxblocks;
ULONG bits,trash,lg;

	maxinbitmap = (volume->SizeBlock-1)*32;		//max blocks marked in a bitmapblock
	maxblocks = maxinbitmap-((longnr-1)*32);						// blocks left to check
	while (1) {
		togo -= maxblocks;
		while (maxblocks>=32) {
			// do we have a free block ? if yes search within this long which block
			lg=AROS_BE2LONG(bitmapblock->buffer[longnr]);
			if (lg) {
				bits=1;
				do {
					if (bits & lg)
						return block;
					bits=bits<<1;
					block++;
				} while (bits);
			}
			else
				block += 32;
			longnr++;
			maxblocks -= 32;
		}
		if (maxblocks) {
			lg=AROS_BE2LONG(bitmapblock->buffer[longnr]);
			bits=1;
			for (;maxblocks;maxblocks--) {
				if (bits & lg)
					return block;
				bits=bits<<1;
				block++;
			}
		}
		if (!togo)
			break;
		gotoBitmapBlock(volume,block,&longnr,&trash);
		if ((longnr!=1) || (trash)) showText("Wrong bitmapblockjump!");
		maxblocks = togo<maxinbitmap ? togo : maxinbitmap;
		longnr= 1;		// skip checksum
	}
	return 0;
}

ULONG getFreeBlock(struct Volume *volume) {
ULONG block,longnr,bitnr,lg;

	// check all blocks after rootblock
	block = lastaccess ? lastaccess : volume->rootblock;
	gotoBitmapBlock(volume,block,&longnr,&bitnr);
	if (bitnr) {
		lg=AROS_BE2LONG(bitmapblock->buffer[longnr]);
		for (;bitnr<32;bitnr++) {
			if ((1<<bitnr) & lg)
				return block;
			block++;
		}
		longnr++;
	}
	if (block>=volume->rootblock) {
		block = findFreeBlock(volume,volume->rootblock-(block-volume->rootblock), longnr, block);
		if (block)
			return block;
		block=volume->bootblocks;
	}
	gotoBitmapBlock(volume,block,&longnr,&bitnr);
	block = findFreeBlock(volume, volume->rootblock-block,longnr, block);
	return block;
}

ULONG allocBlock(struct Volume *volume) {
ULONG block;

	if ((block=getFreeBlock(volume))) {
		D(bug("afs.handler:    allocBlock: found a free block on %ld\n",block));
		if (!markBlock(volume,block,0))
			block=0;
	}
	return block;
}
