/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
#include "filehandles2.h"
#include "afsblocks.h"
#include "bitmap.h"
#include "checksums.h"
#include "error.h"
#include "extstrings.h"
#include "filehandles1.h"
#include "hashing.h"
#include "misc.h"
#include "baseredef.h"

extern ULONG error;

/********************************************
 Name  : setHeaderDate
 Descr.: set actual date for an object
 Input : volume      - 
         blockbuffer - header block of object
         ds          - datestamp to set
 Output: 0 for success; error code otherwise
*********************************************/
ULONG setHeaderDate
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *blockbuffer,
		struct DateStamp *ds
	)
{

	D(bug
		(
			"[afs] setHeaderDate: for headerblock %lu\n",
			blockbuffer->blocknum)
		);
	blockbuffer->buffer[BLK_DAYS(volume)] = OS_LONG2BE(ds->ds_Days);
	blockbuffer->buffer[BLK_MINS(volume)] = OS_LONG2BE(ds->ds_Minute);
	blockbuffer->buffer[BLK_TICKS(volume)] = OS_LONG2BE(ds->ds_Tick);
	writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
	blockbuffer = getBlock
		(afsbase, volume, OS_BE2LONG(blockbuffer->buffer[BLK_PARENT(volume)]));
	if (blockbuffer == NULL)
		return ERROR_UNKNOWN;
	return writeHeader(afsbase, volume, blockbuffer);
}


/********************************************
 Name  : setDate
 Descr.: set actual date for an object
 Input : ah      - filehandle name is relative to
         name    - name of object 
         ds      - datestamp to set
 Output: 0 for success; error code otherwise
*********************************************/
ULONG setDate
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		STRPTR name,
		struct DateStamp *ds
	)
{
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("[afs] setData()\n"));
	blockbuffer = findBlock(afsbase, ah, name, &block);
	if (blockbuffer == NULL)
		return error;
	return setHeaderDate(afsbase, ah->volume, blockbuffer, ds);
}

/********************************************
 Name  : setProtect
 Descr.: set protection bits for an object
 Input : ah      - filehandle name is relative to
         name    - name of object
         mask    - protection bit mask
 Output: 0 for success; error code otherwise
*********************************************/
ULONG setProtect
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		STRPTR name,
		ULONG mask
	)
{
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("[afs] setProtect(ah,%s,%ld)\n", name, mask));
	blockbuffer = findBlock(afsbase, ah, name, &block);
	if (blockbuffer == NULL)
		return error;
	blockbuffer->buffer[BLK_PROTECT(ah->volume)] = OS_LONG2BE(mask);
	return writeHeader(afsbase, ah->volume, blockbuffer);
}

/********************************************
 Name  : setComment
 Descr.: set comment for an object
 Input : ah      - filehandle name is relative to
         name    - name of object
         comment - comment to set
 Output: 0 for success; error code otherwise
*********************************************/
ULONG setComment
	(struct AFSBase *afsbase, struct AfsHandle *ah, STRPTR name, STRPTR comment)
{
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("[afs] setComment(ah,%s,%s)\n", name, comment));
	if (StrLen(comment) >= MAXCOMMENTLENGTH)
		return ERROR_COMMENT_TOO_BIG;
	blockbuffer = findBlock(afsbase, ah, name, &block);
	if (blockbuffer == NULL)
		return error;
	StrCpyToBstr
		(
			comment,
			(APTR)((char *)blockbuffer->buffer+(BLK_COMMENT_START(ah->volume)*4)),
			MAX_COMMENT_LENGTH
		);
	return writeHeader(afsbase, ah->volume, blockbuffer);
}

/************************************************
 Name  : unLinkBlock
 Descr.: unlinks an entry from the directorylist
 Input : lastentry - node before entry to unlink
                     (directory itself (head) or last block
                     which HASHCHAIN points to the entry to unlink
         entry     - entry to unlink
 Output: - 
 Note  : unlink is only done in buffers
         nothing is written to disk!
************************************************/
void unLinkBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *lastentry,
		struct BlockCache *entry
	)
{
ULONG key;

	D(bug("[afs] unlinkBlock: unlinking %lu\n", entry->blocknum));
	/* find the "member" where entry is linked
		->linked into hashchain or hashtable */
	key = BLK_HASHCHAIN(volume);
	if (OS_BE2LONG(lastentry->buffer[key])!=entry->blocknum)
	{
		for (key = BLK_TABLE_START; key<=BLK_TABLE_END(volume); key++)
		{
			if (OS_BE2LONG(lastentry->buffer[key]) == entry->blocknum)
				break;
		}
	}
	/* unlink block */
	lastentry->buffer[key] = entry->buffer[BLK_HASHCHAIN(volume)];
	lastentry->buffer[BLK_CHECKSUM] = 0;
	lastentry->buffer[BLK_CHECKSUM] =
		OS_LONG2BE(0-calcChkSum(volume->SizeBlock, lastentry->buffer));
}

/********************************************
 Name  : deleteObject
 Descr.: delete an object
 Input : ah      - filehandle name is relative to
         name    - name of object to delete
 Output: 0 for success; error code otherwise
*********************************************/
ULONG deleteObject(struct AFSBase *afsbase, struct AfsHandle *ah, STRPTR name) {
ULONG lastblock,key;
struct BlockCache *blockbuffer, *priorbuffer;

	D(bug("[afs] delete(ah,%s)\n", name));
	blockbuffer = findBlock(afsbase, ah, name, &lastblock);
	if (blockbuffer == NULL)
		return error;
	if (findHandle(ah->volume, blockbuffer->blocknum) != NULL)
		return ERROR_OBJECT_IN_USE;
	if (OS_BE2LONG(blockbuffer->buffer[BLK_PROTECT(ah->volume)]) & FIBF_DELETE)
		return ERROR_DELETE_PROTECTED;
	/* if we try to delete a directory, check if it is empty */
	if (
			OS_BE2LONG
				(blockbuffer->buffer[BLK_SECONDARY_TYPE(ah->volume)]) == ST_USERDIR
		)
	{
		for (key=BLK_TABLE_START; key<=BLK_TABLE_END(ah->volume); key++)
		{
			if (blockbuffer->buffer[key] != 0)
				return ERROR_DIRECTORY_NOT_EMPTY;
		}
	}
	blockbuffer->flags |= BCF_USED;
	priorbuffer = getBlock(afsbase, ah->volume, lastblock);
	if (priorbuffer == NULL)
	{
		blockbuffer->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	if (calcChkSum(ah->volume->SizeBlock, priorbuffer->buffer) != 0)
	{
		blockbuffer->flags &= ~BCF_USED;
		showError(afsbase, ERR_CHECKSUM, priorbuffer->blocknum);
		return ERROR_UNKNOWN;
	}
	priorbuffer->flags |= BCF_USED;
	unLinkBlock(afsbase, ah->volume, priorbuffer, blockbuffer);
	invalidBitmap(afsbase, ah->volume);
	writeBlock(afsbase, ah->volume, priorbuffer, -1);
	markBlock(afsbase, ah->volume, blockbuffer->blocknum, -1);
	if (
			OS_BE2LONG
				(blockbuffer->buffer[BLK_SECONDARY_TYPE(ah->volume)]) == ST_FILE
		)
	{
		for (;;)
		{
			D(bug("[afs]   extensionblock=%lu\n", blockbuffer->blocknum));
			for
				(
					key = BLK_TABLE_END(ah->volume);
					(key >= BLK_TABLE_START) && (blockbuffer->buffer[key]!=0);
					key--
				)
			{
				markBlock
					(afsbase, ah->volume, OS_BE2LONG(blockbuffer->buffer[key]), -1);
			}
			if (blockbuffer->buffer[BLK_EXTENSION(ah->volume)] == 0)
				break;
			/* get next extensionblock */
			blockbuffer->flags &= ~BCF_USED;
			blockbuffer = getBlock
				(
					afsbase,
					ah->volume,
					OS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(ah->volume)])
				);
			if (blockbuffer == NULL)
			{
				priorbuffer->flags &= ~BCF_USED;
				return ERROR_UNKNOWN;
			}
			if (calcChkSum(ah->volume->SizeBlock, blockbuffer->buffer))
			{
				priorbuffer->flags &= ~BCF_USED;
				showError(afsbase, ERR_CHECKSUM);
				return ERROR_UNKNOWN;
			}
			blockbuffer->flags |= BCF_USED;
			markBlock(afsbase, ah->volume, blockbuffer->blocknum, -1);
		}
	}
	validBitmap(afsbase, ah->volume);
	blockbuffer->flags &= ~BCF_USED;
	priorbuffer->flags &= ~BCF_USED;
	return 0;
}

/********************************************
 Name  : linkNewBlock
 Descr.: links a new entry into a directorylist
 Input : dir  - directory to link in
         file - file to link
 Output: the block which must be written to disk
         -> if hashtable of directory changed it's the same
            pointer as arg1; otherwise a HASHCHAIN pointer
            changed so we don't have to write this block but
            another one
         NULL if error
*********************************************/
struct BlockCache *linkNewBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *dir,
		struct BlockCache *file
	)
{
ULONG key; /* parent; */
UBYTE buffer[32];
STRPTR name;

	file->buffer[BLK_PARENT(volume)] = OS_LONG2BE(dir->blocknum);
	D(bug("[afs] linkNewBlock: linking block %ld\n", file->blocknum));
	name = (STRPTR)((char *)file->buffer+(BLK_FILENAME_START(volume)*4));
	StrCpyFromBstr(name, buffer);
	key = getHashKey
		(buffer, volume->SizeBlock-56, volume->dosflags)+BLK_TABLE_START;
	/* sort in ascending order */
	if (
			(dir->buffer[key] != 0) &&
			(OS_BE2LONG(dir->buffer[key]) < file->blocknum))
	{
		dir = getBlock(afsbase, volume, OS_BE2LONG(dir->buffer[key]));
		if (dir == NULL)
			return NULL;
		key = BLK_HASHCHAIN(volume);
		while (
					(dir->buffer[key] != 0) &&
					(OS_BE2LONG(dir->buffer[key]) < file->blocknum)
				)
		{
			dir = getBlock(afsbase, volume, OS_BE2LONG(dir->buffer[key]));
			if (dir == NULL)
				return NULL;
		}
	}
	file->buffer[BLK_HASHCHAIN(volume)] = dir->buffer[key];
	dir->buffer[key] = OS_LONG2BE(file->blocknum);
	file->buffer[BLK_CHECKSUM] = 0;
	file->buffer[BLK_CHECKSUM] =
		OS_LONG2BE(0-calcChkSum(volume->SizeBlock,file->buffer));
	dir->buffer[BLK_CHECKSUM] = 0;
	dir->buffer[BLK_CHECKSUM] =
		OS_LONG2BE(0-calcChkSum(volume->SizeBlock,dir->buffer));
	return dir;
}

/********************************************
 Name  : getDirBlockBuffer
 Descr.: returns cacheblock of the block the
         last component before "/" or ":" of
         name refers to or cacheblock of ah
         if there is no such component
 Input : ah        - filehandle name is relative to
         name      - name of object
         entryname - will be filled with a copy
                     of the last component of
                     name
 Output: NULL for error (global error will be set);
         pointer to a struct BlockCache otherwise
*********************************************/
struct BlockCache *getDirBlockBuffer
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		STRPTR name,
		STRPTR entryname
	)
{
ULONG block,len;
STRPTR end;
UBYTE buffer[256];

	end = PathPart(name);
	CopyMem(name, buffer, end-name);
	buffer[end-name] = 0;
	if (end[0] == '/')
		end++;
	len = StrLen(name)+name-end;
	CopyMem(end, entryname, len);	/* skip backslash or colon */
	entryname[len] = 0;
	return findBlock(afsbase, ah, buffer, &block);
}

/********************************************
 Name  : renameObject
 Descr.: rename an object
 Input : dirah   - filehandle names are relative to
         oname   - object to rename
         newname - new name of the object
 Output: 0 for success; error code otherwise
*********************************************/
ULONG renameObject
	(
		struct AFSBase *afsbase,
		struct AfsHandle *dirah,
		STRPTR oname,
		STRPTR newname
	)
{
struct BlockCache *lastlink,*oldfile, *dirblock;
ULONG block,dirblocknum,lastblock;
UBYTE newentryname[34];

	D(bug("[afs] rename(%ld,%s,%s)\n", dirah->header_block, oname, newname));
	dirblock = getDirBlockBuffer(afsbase, dirah, newname, newentryname);
	if (dirblock == NULL)
		return error;
	dirblocknum = dirblock->blocknum;
	D(bug("[afs]    dir is on block %ld\n", dirblocknum));
	if (getHeaderBlock(afsbase, dirah->volume, newentryname, dirblock, &block) != NULL)
	{
		dirblock->flags &= ~BCF_USED;
		return ERROR_OBJECT_EXISTS;
	}
	oldfile = findBlock(afsbase, dirah, oname, &lastblock);
	if (oldfile == NULL)
		return error;
	oldfile->flags |= BCF_USED;
	/* do we move a directory? */
	if (OS_BE2LONG(oldfile->buffer[BLK_SECONDARY_TYPE(dirah->volume)])==ST_USERDIR)
	{
		/* is newdirblock child of olock/oname */
		dirblock = getBlock(afsbase, dirah->volume, dirblocknum);
		if (dirblock == NULL)
		{
			oldfile->flags &= ~BCF_USED;
			return ERROR_UNKNOWN;
		}
		while ((block = OS_BE2LONG(dirblock->buffer[BLK_PARENT(dirah->volume)])))
		{
			if (block == oldfile->blocknum)
			{
				oldfile->flags &= ~BCF_USED;
				return ERROR_OBJECT_IN_USE;
			}
			dirblock = getBlock(afsbase, dirah->volume, block);
			if (dirblock == NULL)
			{
				oldfile->flags &= ~BCF_USED;
				return ERROR_UNKNOWN;
			}
		}
	}
	lastlink = getBlock(afsbase, dirah->volume, lastblock);
	if (lastlink == NULL)
	{
		oldfile->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	lastlink->flags |= BCF_USED;
	unLinkBlock(afsbase, dirah->volume, lastlink, oldfile);
	/* rename in same dir ? */
	if (lastlink->blocknum == dirblocknum)
	{
		/* use same buffers! */
		dirblock = lastlink;
	}
 	/* otherwise we use different blocks */
	else
	{
		dirblock = getBlock(afsbase, dirah->volume, dirblocknum);
		if (dirblock == NULL)
		{
			oldfile->flags &= ~BCF_USED;
			lastlink->flags &= ~BCF_USED;
			return ERROR_UNKNOWN;
		}
	}
	if (
			(
				OS_BE2LONG
					(
						dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)]
					) != ST_USERDIR
			) &&
			(
				OS_BE2LONG
					(
						dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)]
					) != ST_ROOT
			)
		)
	{
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		return ERROR_OBJECT_WRONG_TYPE;
	}
	StrCpyToBstr
		(
			newentryname,
			(APTR)((char *)oldfile->buffer+(BLK_FILENAME_START(dirah->volume)*4)),
			MAX_NAME_LENGTH
		);
	dirblock = linkNewBlock(afsbase, dirah->volume, dirblock, oldfile);
	if (dirblock == NULL)
	{
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	/* concurrent access if newdir=rootblock! */
	dirblock->flags |= BCF_USED;
	/*
		mark it as used, so that this buffer isn't
		used in invalidating volume!
	*/
	if (!setBitmapFlag(afsbase, dirah->volume, 0))
	{
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		dirblock->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	/* now we can release that buffer */
	dirblock->flags &= ~BCF_USED;
	/*
		syscrash after this: 2 dirs pointing to the same
		dir and one wrong linked entry->recoverable
	*/
	writeBlock(afsbase, dirah->volume, dirblock, -1);
	/*
		syscrash after this: directory pointing is now
		correct but one wrong linked entry->recoverable
	*/
	writeBlock(afsbase, dirah->volume, lastlink, -1);
	writeBlock(afsbase, dirah->volume, oldfile, -1);
	oldfile->flags &= ~BCF_USED;
	lastlink->flags &= ~BCF_USED;
	/*
		if newdir=rootblock we now write the correct
		(changed) buffer back
	*/
	setBitmapFlag(afsbase, dirah->volume, -1);
	return 0;
}

/********************************************
 Name  : createNewEntry
 Descr.: create a new object on disk
 Input : volume     -
         entrytype  - ST_USERDIR/ST_FILE/...
         entryname  - name of object
         dirblock   - pointer to struct BlockCache
                      containing a directory in which
                      name shall be created in
         protection - protection bit mask
 Output: NULL for error (global error set);
         pointer to struct BlockCache of the newly
         created object otherwise
*********************************************/
struct BlockCache *createNewEntry
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		ULONG entrytype,
		STRPTR entryname,
		struct BlockCache *dirblock,
		ULONG protection
	)
{
struct BlockCache *newblock;
struct DateStamp ds;
ULONG i;

	D(bug("[afs] createNewEntry(%ld, %s)\n", dirblock->blocknum, entryname));
	dirblock->flags |= BCF_USED;
	if (getHeaderBlock(afsbase, volume, entryname, dirblock, &i) != NULL)
	{
		dirblock->flags &= ~BCF_USED;
		error = ERROR_OBJECT_EXISTS;
		return NULL;
	}
	error = 0;
	if (!invalidBitmap(afsbase, volume))
	{
		dirblock->flags &= ~BCF_USED;
		return NULL;
	}
	i = allocBlock(afsbase, volume);
	if (i == 0)
	{
		dirblock->flags &= ~BCF_USED;
		validBitmap(afsbase, volume);
		error = ERROR_DISK_FULL;
		return NULL;
	}
	newblock = getFreeCacheBlock(afsbase, volume, i);
	if (newblock == NULL)
	{
		dirblock->flags &= ~BCF_USED;
		validBitmap(afsbase, volume);
		error = ERROR_UNKNOWN;
		return NULL;
	}
	newblock->flags |= BCF_USED;
	newblock->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_SHORT);
	for (i=BLK_BLOCK_COUNT; i<=BLK_COMMENT_END(volume); i++)
		newblock->buffer[i] = 0;
	newblock->buffer[BLK_PROTECT(volume)] = OS_LONG2BE(protection);
	DateStamp(&ds);
	newblock->buffer[BLK_DAYS(volume)] = OS_LONG2BE(ds.ds_Days);
	newblock->buffer[BLK_MINS(volume)] = OS_LONG2BE(ds.ds_Minute);
	newblock->buffer[BLK_TICKS(volume)] = OS_LONG2BE(ds.ds_Tick);
	StrCpyToBstr
		(
			entryname,
			(APTR)((char *)newblock->buffer+(BLK_FILENAME_START(volume)*4)),
			MAX_NAME_LENGTH
		);
	for (i=BLK_FILENAME_END(volume)+1; i<BLK_HASHCHAIN(volume); i++)
		newblock->buffer[i] = 0;
	newblock->buffer[BLK_PARENT(volume)] = OS_LONG2BE(dirblock->blocknum);
	newblock->buffer[BLK_EXTENSION(volume)] = 0;
	newblock->buffer[BLK_SECONDARY_TYPE(volume)] = OS_LONG2BE(entrytype);
	newblock->buffer[BLK_OWN_KEY] = OS_LONG2BE(newblock->blocknum);
	dirblock->flags &= ~BCF_USED;
	dirblock = linkNewBlock(afsbase, volume, dirblock, newblock);
	if (dirblock == NULL)
	{
		markBlock(afsbase, volume, newblock->blocknum, -1);
		newblock->flags &= ~BCF_USED;
		newblock->newness = 0;
		validBitmap(afsbase, volume);
		return NULL;
	}
	/*
		if crash after this block not yet linked->block
		not written to disk, bitmap corrected
	*/
	writeBlock(afsbase, volume, newblock, -1);
	/* consistent after this */
	writeBlock(afsbase, volume, dirblock, -1);
	validBitmap(afsbase, volume);
	newblock->flags &= ~BCF_USED;
	return newblock;
}


/********************************************
 Name  : createDir
 Descr.: create a directory object
 Input : dirah      - filehandle filename is relative to
         filename   - path to the new directory
         protection - protection bit mask
 Output: pointer to struct AfsHandle;
         0 otherwise (global error set)
*********************************************/
struct AfsHandle *createDir
	(
		struct AFSBase *afsbase,
		struct AfsHandle *dirah,
		STRPTR filename,
		ULONG protection
	)
{
struct AfsHandle *ah = NULL;
struct BlockCache *dirblock;
char dirname[34];

	D(bug("[afs] createDir(ah,%s,%ld)\n", filename, protection));
	dirblock = getDirBlockBuffer(afsbase, dirah, filename, dirname);
	if (dirblock != NULL)
	{
		D(bug("[afs]    dir is on block %ld\n", dirblock->blocknum));
		dirblock = createNewEntry
			(afsbase, dirah->volume, ST_USERDIR, dirname, dirblock, protection);
		if (dirblock != NULL)
			ah = getHandle(afsbase, dirah->volume, dirblock, FMF_READ);
	}
	return ah;
}

