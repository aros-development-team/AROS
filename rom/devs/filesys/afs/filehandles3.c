/*
    Copyright © 1995-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

#include <stddef.h>
#include "os.h"
#include "filehandles3.h"
#include "afsblocks.h"
#include "cache.h"
#include "hashing.h"
#include "baseredef.h"

LONG sameLock(struct AfsHandle *ah1, struct AfsHandle *ah2) {

	return (ah1->header_block == ah2->header_block) ? LOCK_SAME : LOCK_DIFFERENT;
}

static const ULONG sizes[]=
{
    0,
    offsetof(struct ExAllData,ed_Type),
    offsetof(struct ExAllData,ed_Size),
    offsetof(struct ExAllData,ed_Prot),
    offsetof(struct ExAllData,ed_Days),
    offsetof(struct ExAllData,ed_Comment),
    offsetof(struct ExAllData,ed_OwnerUID),
    sizeof(struct ExAllData)
};

ULONG examineEAD
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct ExAllData *ead,
		struct BlockCache *entryblock,
		ULONG size,
		ULONG mode
	)
{
STRPTR next,end,name;
ULONG owner;

	next = (STRPTR)ead+sizes[mode];
	end = (STRPTR)ead+size;

	if (next > end) /* > is correct. Not >= */
	    return ERROR_BUFFER_OVERFLOW;
	    
	switch (mode)
	{
	case ED_OWNER :
		owner = OS_BE2LONG(entryblock->buffer[BLK_OWNER(volume)]);
		ead->ed_OwnerUID = owner>>16;
		ead->ed_OwnerGID = owner & 0xFFFF;
	case ED_COMMENT :
		if (OS_BE2LONG(entryblock->buffer[BLK_SECONDARY_TYPE(volume)]) != ST_ROOT)
		{
			name = (STRPTR)((char *)entryblock->buffer+(BLK_COMMENT_START(volume)*4));
			if ((next+name[0]+1) > end)
				return ERROR_BUFFER_OVERFLOW;
			ead->ed_Comment = next;
			CopyMem(name+1, ead->ed_Comment, name[0]);
			ead->ed_Comment[(ULONG)name[0]] = 0;
			next += name[0]+1; /* NULL-Byte */
		}
		else
			ead->ed_Comment = 0;
	case ED_DATE :
		ead->ed_Days = OS_BE2LONG(entryblock->buffer[BLK_DAYS(volume)]);
		ead->ed_Mins = OS_BE2LONG(entryblock->buffer[BLK_MINS(volume)]);
		ead->ed_Ticks = OS_BE2LONG(entryblock->buffer[BLK_TICKS(volume)]);
	case ED_PROTECTION :
		ead->ed_Prot = OS_BE2LONG(entryblock->buffer[BLK_PROTECT(volume)]);
	case ED_SIZE :
		ead->ed_Size = OS_BE2LONG(entryblock->buffer[BLK_BYTE_SIZE(volume)]);
	case ED_TYPE :
		ead->ed_Type = OS_BE2LONG(entryblock->buffer[BLK_SECONDARY_TYPE(volume)]);
	case ED_NAME :
		name = (STRPTR)((char *)entryblock->buffer+(BLK_FILENAME_START(volume)*4));
		if ((next+name[0]+1) > end)
			return ERROR_BUFFER_OVERFLOW;
		ead->ed_Name = next;
		CopyMem(name+1, ead->ed_Name, name[0]);
		ead->ed_Name[(ULONG)name[0]] = 0;
		next += name[0]+1; /* NULL-Byte */
	case 0 :
		ead->ed_Next = (struct ExAllData *)(((ULONG)next + OS_PTRALIGN - 1) & ~(OS_PTRALIGN - 1));
	}
	return 0;
}

ULONG examine
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		struct ExAllData *ead,
		ULONG size,
		ULONG mode,
		ULONG *dirpos
	)
{
struct BlockCache *entryblock;

	D(bug("[afs] examine(%ld,ead,%ld,%ld)\n",ah->header_block,size,mode));
	if (mode > ED_OWNER)
		return ERROR_BAD_NUMBER;
	entryblock = getBlock(afsbase, ah->volume, ah->header_block);
	if (entryblock == NULL)
		return ERROR_UNKNOWN;
	examineEAD(afsbase, ah->volume, ead, entryblock, size, mode);
	*dirpos = ah->header_block;
	return 0;
}

ULONG getNextExamineBlock
	(struct AFSBase *afsbase, struct AfsHandle *ah, ULONG *key, ULONG *pos)
{
struct BlockCache *entryblock;
UBYTE cstr[34];
STRPTR string;

	entryblock = getBlock(afsbase, ah->volume, *key);
	if (entryblock == NULL)
		return ERROR_UNKNOWN;
	if (*key == ah->header_block) /* start examining entries in ah */
	{
		*pos = BLK_TABLE_START;
	}
	else
	{
		if	(entryblock->buffer[BLK_HASHCHAIN(ah->volume)] != 0)
		{
			/* chained entry */
			*pos = BLK_HASHCHAIN(ah->volume);
		}
		else
		{
			string = (char *)entryblock->buffer+(BLK_FILENAME_START(ah->volume)*4);
			CopyMem(string+1, cstr, string[0]);
			cstr[(ULONG)string[0]] = 0;
			*pos = BLK_TABLE_START+getHashKey(cstr, ah->volume->SizeBlock-56, ah->volume->dosflags)+1;
			if (*pos > BLK_TABLE_END(ah->volume))
				return ERROR_NO_MORE_ENTRIES;
			entryblock = getBlock(afsbase, ah->volume, ah->header_block);
			if (entryblock == NULL)
				return ERROR_UNKNOWN;
		}
	}
	/* in one case entryblock->buffer[i] points to a block
		which should be examined next
		in the other cases it may point to NULL */
	while (entryblock->buffer[*pos] == 0)
	{
		if (*pos == BLK_TABLE_END(ah->volume))
			return ERROR_NO_MORE_ENTRIES;
		*pos += 1;
	}
	/* now i is on a valid position */
	*key = OS_BE2LONG(entryblock->buffer[*pos]);
	return 0;
}


ULONG examineAll
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		struct ExAllData *ead,
		struct ExAllControl *eac,
		ULONG size,
		ULONG mode
	)
{
struct BlockCache *headerblock;
struct BlockCache *entryblock;
struct ExAllData *last;
ULONG error,i,block;

	D(bug("[afs] examineAll(%ld,ead,%ld,%ld)\n",ah->header_block,size,mode));
	eac->eac_Entries = 0;
	if (mode > ED_OWNER)
		return ERROR_BAD_NUMBER;
	headerblock = getBlock(afsbase, ah->volume, ah->header_block);
	if (headerblock == NULL)
		return ERROR_UNKNOWN;
	/* is it a file? */
	if (OS_BE2LONG(headerblock->buffer[BLK_SECONDARY_TYPE(ah->volume)]) < 0)
	{
		error = examineEAD(afsbase, ah->volume, ead, headerblock, size, mode);
		if (error == 0)
			eac->eac_Entries = 1;
		return error;
	}
	error = getNextExamineBlock(afsbase, ah, &ah->dirpos, &i);
#warning "if ah->dirpos is an entry stored in a hashchain we return entries twice"
	if (error != 0)
		return error;
	last = ead;
	/*
		the contents of headerblock may have changed
		by getNextExamineBlock
	*/
	if (headerblock->blocknum != ah->header_block)
	{
		headerblock = getBlock(afsbase, ah->volume, ah->header_block);
		if (headerblock == NULL)
			return ERROR_UNKNOWN;
	}
	headerblock->flags |= BCF_USED;
	for (; i<=BLK_TABLE_END(ah->volume); i++)
	{
		if (headerblock->buffer[i] != 0)
		{
			block = OS_BE2LONG(headerblock->buffer[i]);
			do
			{
				entryblock = getBlock(afsbase, ah->volume, block);
				if (entryblock == NULL)
				{
					headerblock->flags &= ~BCF_USED;
					return ERROR_UNKNOWN;
				}
				error = examineEAD(afsbase, ah->volume, ead, entryblock, size, mode);
				if (error != 0)
				{
				    /* stegerg: CHECK CHECK CHECK CHECK CHECK */
					if (error == ERROR_BUFFER_OVERFLOW)
					{
						ah->dirpos = OS_BE2LONG(headerblock->buffer[i]);
						error = 0;
					}
				   /* stegerg: END CHECK CHECK CHECK CHECK CHECK */

					ead->ed_Next = NULL;
					headerblock->flags &= ~BCF_USED;
					return error;
				}
				size -= (ULONG)((char *)ead->ed_Next-(char *)ead);
				last = ead;
				ead = ead->ed_Next;
				ah->dirpos = OS_BE2LONG(headerblock->buffer[i]);
				block = OS_BE2LONG(entryblock->buffer[BLK_HASHCHAIN(ah->volume)]);
				eac->eac_Entries++;
			} while (block != 0);
		}
	}
	last->ed_Next = NULL;
	headerblock->flags &= ~BCF_USED;
	return ERROR_NO_MORE_ENTRIES;
}

ULONG examineNext
	(struct AFSBase *afsbase, struct AfsHandle *ah, struct FileInfoBlock *fib)
{
struct BlockCache *entryblock;
STRPTR string;
ULONG filelistentries,datablocksize,datablocks;
ULONG owner;
ULONG error,filekey;

	D(bug("[afs] examineNext(%ld,fib)\n", ah->header_block));
	D(bug("[afs] examineNext: diskey=%ld\n", fib->fib_DiskKey));

	error = getNextExamineBlock(afsbase, ah, &fib->fib_DiskKey, &filekey);
	if (error != 0)
		return error;
	/* examine the block */
	entryblock = getBlock(afsbase, ah->volume, fib->fib_DiskKey);
	if (entryblock == NULL)
		return ERROR_UNKNOWN;
	fib->fib_DirEntryType = 
		OS_BE2LONG(entryblock->buffer[BLK_SECONDARY_TYPE(ah->volume)]);
	string = (char *)entryblock->buffer+(BLK_FILENAME_START(ah->volume)*4);
	CopyMem(string+1, fib->fib_FileName, string[0]);
	fib->fib_FileName[(ULONG)string[0]] = 0;
	fib->fib_Protection = OS_BE2LONG(entryblock->buffer[BLK_PROTECT(ah->volume)]);
	fib->fib_EntryType = fib->fib_DirEntryType;
	fib->fib_Size = OS_BE2LONG(entryblock->buffer[BLK_BYTE_SIZE(ah->volume)]);
	filelistentries = ah->volume->SizeBlock-56;
	datablocksize = BLOCK_SIZE(ah->volume);
	if (ah->volume->dosflags == 0)
		datablocksize = datablocksize-24;
	datablocks = ((fib->fib_Size+datablocksize-1)/datablocksize);
	fib->fib_NumBlocks = 
		datablocks+((datablocks+filelistentries-1)/filelistentries);
	fib->fib_Date.ds_Days = OS_BE2LONG(entryblock->buffer[BLK_DAYS(ah->volume)]);
	fib->fib_Date.ds_Minute = OS_BE2LONG(entryblock->buffer[BLK_MINS(ah->volume)]);
	fib->fib_Date.ds_Tick = OS_BE2LONG(entryblock->buffer[BLK_TICKS(ah->volume)]);
	if (fib->fib_DirEntryType != ST_ROOT)
	{
		string = (char *)entryblock->buffer+(BLK_COMMENT_START(ah->volume)*4);
		CopyMem(string+1, fib->fib_Comment, string[0]);
		fib->fib_Comment[(ULONG)string[0]] = 0;
	}
	owner = OS_BE2LONG(entryblock->buffer[BLK_OWNER(ah->volume)]);
	fib->fib_OwnerUID = owner>>16;
	fib->fib_OwnerGID = owner & 0xFFFF;
	return 0;
}

