#define DEBUG 1

#include <proto/exec.h>
#include <proto/dos.h>

#include <aros/debug.h>
#include <aros/macros.h>

#include "filehandles2.h"
#include "afsblocks.h"
#include "bitmap.h"
#include "checksums.h"
#include "error.h"
#include "extstrings.h"
#include "filehandles1.h"
#include "hashing.h"
#include "misc.h"

extern ULONG error;

ULONG setHeaderDate(struct Volume *volume, struct BlockCache *blockbuffer, struct DateStamp *ds) {

	D(bug("afs.handler: setHeaderDate: for headerblock %ld\n", blockbuffer->blocknum));
	blockbuffer->buffer[BLK_DAYS(volume)]=AROS_LONG2BE(ds->ds_Days);
	blockbuffer->buffer[BLK_MINS(volume)]=AROS_LONG2BE(ds->ds_Minute);
	blockbuffer->buffer[BLK_TICKS(volume)]=AROS_LONG2BE(ds->ds_Tick);
	blockbuffer->buffer[BLK_CHECKSUM]=0;
	blockbuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume, blockbuffer->buffer));
	writeBlock(volume,blockbuffer);
	blockbuffer=getBlock(volume, AROS_LONG2BE(blockbuffer->buffer[BLK_PARENT(volume)]));
	if (!blockbuffer)
		return ERROR_UNKNOWN;
	return writeHeader(volume, blockbuffer);
}


ULONG setDate(struct AfsHandle *ah, STRPTR name, struct DateStamp *ds) {
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("afs.handler: setData()\n"));
	blockbuffer=findBlock(ah, name, &block);
	if (!blockbuffer)
		return error;
	return setHeaderDate(ah->volume,blockbuffer,ds);
}

ULONG setProtect(struct AfsHandle *ah, STRPTR name, ULONG mask) {
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("afs.handler: setProtect(ah,%s,%ld)\n",name,mask));
	blockbuffer=findBlock(ah, name, &block);
	if (!blockbuffer)
		return error;
	blockbuffer->buffer[BLK_PROTECT(ah->volume)]=AROS_LONG2BE(mask);
	return writeHeader(ah->volume, blockbuffer);
}

ULONG setComment(struct AfsHandle *ah, STRPTR name, STRPTR comment) {
ULONG block;
struct BlockCache *blockbuffer;

	D(bug("afs.handler: setComment(ah,%s,%s)\n",name,comment));
	blockbuffer=findBlock(ah, name, &block);
	if (!blockbuffer)
		return error;
	CopyMem
		(
			comment,
			(APTR)((ULONG)blockbuffer->buffer+(BLK_COMMENT_START(ah->volume)*4)),
			(comment[0]>79) ? 80 : (comment[0]+1)
		);
	return writeHeader(ah->volume, blockbuffer);
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
void unLinkBlock(struct Volume *volume,struct BlockCache *lastentry, struct BlockCache *entry) {
ULONG key;

	D(bug("afs.handler: unlinkBlock: unlinking %ld\n",entry->blocknum));
	// find the "member" where entry is linked
	// ->linked into hashchain or hashtable
	key=BLK_HASHCHAIN(volume);
	if (AROS_BE2LONG(lastentry->buffer[key])!=entry->blocknum)
		for (key=BLK_TABLE_START;key<=BLK_TABLE_END(volume);key++)
			if (AROS_BE2LONG(lastentry->buffer[key])==entry->blocknum)
				break;
	lastentry->buffer[key]=entry->buffer[BLK_HASHCHAIN(volume)];	//unlink block
	lastentry->buffer[BLK_CHECKSUM]=0;
	lastentry->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume, lastentry->buffer));
}

ULONG deleteObject(struct AfsHandle *ah, STRPTR name) {
ULONG lastblock,key;
struct BlockCache *blockbuffer, *priorbuffer;

	D(bug("afs.handler: delete(ah,%s)\n",name));
	blockbuffer=findBlock(ah,name,&lastblock);
	if (!blockbuffer)
		return error;
	if (findHandle(ah->volume, blockbuffer->blocknum))
		return ERROR_OBJECT_IN_USE;
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_PROTECT(ah->volume)]) & FIBF_DELETE)
		return ERROR_DELETE_PROTECTED;
	/* if we try to delete a directory
      check if it is empty
	*/
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(ah->volume)])==ST_USERDIR) {
		for (key=BLK_TABLE_START;key<=BLK_TABLE_END(ah->volume);key++) {
			if (blockbuffer->buffer[key])
				return ERROR_DIRECTORY_NOT_EMPTY;
		}
	}
	blockbuffer->flags |= BCF_USED;
	priorbuffer=getBlock(ah->volume, lastblock);
	if (!priorbuffer) {
		blockbuffer->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	if (calcChkSum(ah->volume, priorbuffer->buffer)) {
		blockbuffer->flags &= ~BCF_USED;
		showError(ERR_CHECKSUM,priorbuffer->blocknum);
		return ERROR_UNKNOWN;
	}
	priorbuffer->flags |= BCF_USED;
	unLinkBlock(ah->volume,priorbuffer, blockbuffer);
	invalidBitmap(ah->volume);
	writeBlock(ah->volume,priorbuffer);
	markBlock(ah->volume, blockbuffer->blocknum, -1);
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(ah->volume)])==ST_FILE) {
		for (;;) {
			D(bug("afs.handler:   extensionblock=%ld\n",blockbuffer->blocknum));
			for (key=BLK_TABLE_END(ah->volume);(key>=BLK_TABLE_START) && (blockbuffer->buffer[key]);key--)
				markBlock(ah->volume, AROS_BE2LONG(blockbuffer->buffer[key]), -1);
			if (!blockbuffer->buffer[BLK_EXTENSION(ah->volume)]) break;
			// get next extensionblock
			blockbuffer->flags &= ~BCF_USED;
			if (!(blockbuffer=getBlock(ah->volume, AROS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(ah->volume)])))) {
				priorbuffer->flags &= ~BCF_USED;
				return ERROR_UNKNOWN;
			}
			if (calcChkSum(ah->volume, blockbuffer->buffer)) {
				priorbuffer->flags &= ~BCF_USED;
				showError(ERR_CHECKSUM);
				return ERROR_UNKNOWN;
			}
			blockbuffer->flags |= BCF_USED;
			markBlock(ah->volume, blockbuffer->blocknum, -1);
		}
	}
	validBitmap(ah->volume);
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
         -> if hashtable of directory changed its the same
            pointer as arg1; otherwise a HASHCHAIN pointer
            changed so we dont have to write this block but
            another one
         0 if error
*********************************************/
struct BlockCache *linkNewBlock(struct Volume *volume, struct BlockCache *dir, struct BlockCache *file) {
ULONG key;
char buffer[32];
char *name;

	D(bug("afs.handler: linkNewBlock: linking block %ld\n",file->blocknum));
	name=(char *)((ULONG)file->buffer+(BLK_FILENAME_START(volume)*4));
	CopyMem(name+1,buffer,name[0]);
	buffer[(LONG)name[0]]=0;
	key=getHashKey(buffer,volume->SizeBlock-56,volume->flags)+BLK_TABLE_START;
	// sort in ascending order
	if ((dir->buffer[key]) && (AROS_BE2LONG(dir->buffer[key])<file->blocknum)) {
		if (!(dir=getBlock(volume, AROS_BE2LONG(dir->buffer[key]))))
			return 0;
		key=BLK_HASHCHAIN(volume);
		while ((dir->buffer[key]) && (AROS_BE2LONG(dir->buffer[key])<file->blocknum)) {
			if (!(dir=getBlock(volume, AROS_BE2LONG(dir->buffer[key]))))
				return 0;
		}
	}
	file->buffer[BLK_HASHCHAIN(volume)]=dir->buffer[key];
	dir->buffer[key]=AROS_LONG2BE(file->blocknum);
	file->buffer[BLK_PARENT(volume)]=AROS_LONG2BE(dir->blocknum);
	file->buffer[BLK_CHECKSUM]=0;
	file->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume,file->buffer));
	dir->buffer[BLK_CHECKSUM]=0;
	dir->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume,dir->buffer));
	return dir;
}

struct BlockCache *getDirBlockBuffer(struct AfsHandle *ah, STRPTR name, STRPTR entryname) {
ULONG block;
STRPTR end;
UBYTE buffer[256];

	end=PathPart(name);
	CopyMem(name,buffer,end-name);
	buffer[end-name]=0;
	if (end[0]=='/')
		end++;
	entryname[0]=StrLen(name)+name-end;		// we want to use that as a BCPL string
	CopyMem(end, entryname+1, entryname[0]);	//skip backslash or colon
	entryname[entryname[0]+1]=0;
	return findBlock(ah,buffer,&block);
}

ULONG rename(struct AfsHandle *dirah, STRPTR oname, STRPTR newname) {
struct BlockCache *lastlink,*oldfile, *dirblock;
ULONG block,dirblocknum,lastblock;
UBYTE newentryname[34];

	D(bug("afs.handler: rename(%ld,%s,%s)\n",dirah->header_block,oname,newname));
	dirblock=getDirBlockBuffer(dirah, newname, newentryname);
	if (!dirblock)
		return error;
	dirblocknum=dirblock->blocknum;
	D(bug("afs.handler:    dir is on block %ld\n",dirblocknum));
	if (getHeaderBlock(dirah->volume,newentryname+1,dirblock,&block)) {
		dirblock->flags &= ~BCF_USED;
		return ERROR_OBJECT_EXISTS;
	}
	oldfile=findBlock(dirah, oname, &lastblock);
	if (!oldfile)
		return error;
	oldfile->flags |= BCF_USED;
	// do we move a directory?
	if (AROS_BE2LONG(oldfile->buffer[BLK_SECONDARY_TYPE(dirah->volume)])==ST_USERDIR) {
		// is newdirblock child of olock&oname
		dirblock=getBlock(dirah->volume,dirblocknum);
		if (!dirblock) {
			oldfile->flags &= ~BCF_USED;
			return ERROR_UNKNOWN;
		}
		while ((block=AROS_BE2LONG(dirblock->buffer[BLK_PARENT(dirah->volume)]))) {
			if (block==oldfile->blocknum) {
				oldfile->flags &= ~BCF_USED;
				return ERROR_OBJECT_IN_USE;
			}
			dirblock=getBlock(dirah->volume,block);
			if (!dirblock) {
				oldfile->flags &= ~BCF_USED;
				return ERROR_UNKNOWN;
			}
		}
	}
	lastlink=getBlock(dirah->volume, lastblock);
	if (!lastlink) {
		oldfile->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	lastlink->flags |= BCF_USED;
	unLinkBlock(dirah->volume,lastlink, oldfile);
	if (lastlink->blocknum==dirblocknum)	// rename in same dir ?
	{
		dirblock=lastlink;	// use same buffers!
	}
	else	// otherwise we use different blocks
	{
		dirblock=getBlock(dirah->volume, dirblocknum);
		if (!dirblock) {
			oldfile->flags &= ~BCF_USED;
			lastlink->flags &= ~BCF_USED;
			return ERROR_UNKNOWN;
		}
	}
	if ((AROS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_USERDIR) &&
		(AROS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_ROOT)) {
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		return ERROR_OBJECT_WRONG_TYPE;
	}
	CopyMem(newentryname,(APTR)((ULONG)oldfile->buffer+(BLK_FILENAME_START(dirah->volume)*4)),newentryname[0]+1);
	dirblock=linkNewBlock(dirah->volume,dirblock,oldfile);
	if (!dirblock) {
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	dirblock->flags |= BCF_USED;				// concurrent access if newdir=rootblock!
	if (!setBitmapFlag(dirah->volume, 0)) {	// mark it as used, so that this buffer isnt used in invalidating volume!
		oldfile->flags &= ~BCF_USED;
		lastlink->flags &= ~BCF_USED;
		dirblock->flags &= ~BCF_USED;
		return ERROR_UNKNOWN;
	}
	dirblock->flags &= ~BCF_USED;			// now we can release that buffer
//D(bug("dumping new linked: %d\n", dirblock->blocknum));
//D(umpBlock(dirblock));
	writeBlock(dirah->volume, dirblock);	// syscrash after this: 2 dirs pointing to the same dir and one wrong linked entry->recoverable
//D(bug("dumping last linked: %d\n", lastlink->blocknum));
//D(umpBlock(lastlink));
	writeBlock(dirah->volume, lastlink);	// syscrash after this: directory pointing is now correct but one wrong linked entry->recoverable
//D(bug("dumping file: %d\n", oldfile->blocknum));
//D(umpBlock(oldfile));
	writeBlock(dirah->volume, oldfile);
	oldfile->flags &= ~BCF_USED;
	lastlink->flags &= ~BCF_USED;
	setBitmapFlag(dirah->volume, -1);				// if newdir=rootblock we now write the correct (changed) buffer back
	return 0;
}

struct BlockCache *createNewEntry(struct Volume *volume, ULONG entrytype, STRPTR entryname, struct BlockCache *dirblock, ULONG protection) {
struct BlockCache *newblock;
struct DateStamp ds;
ULONG i;

	dirblock->flags |= BCF_USED;
	if (!(newblock=getFreeCacheBlock(volume,-1)))
		return DOSFALSE;
	newblock->flags |= BCF_USED;
	newblock->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_SHORT);
	for (i=BLK_BLOCK_COUNT;i<=BLK_COMMENT_END(volume);i++)
		newblock->buffer[i]=0;
	newblock->buffer[BLK_PROTECT(volume)]=AROS_LONG2BE(protection);
	DateStamp(&ds);
	newblock->buffer[BLK_DAYS(volume)]=AROS_LONG2BE(ds.ds_Days);
	newblock->buffer[BLK_MINS(volume)]=AROS_LONG2BE(ds.ds_Minute);
	newblock->buffer[BLK_TICKS(volume)]=AROS_LONG2BE(ds.ds_Tick);
	CopyMem(entryname, (APTR)((ULONG)newblock->buffer+(BLK_FILENAME_START(volume)*4)), entryname[0]+1);
	for (i=BLK_FILENAME_END(volume)+1;i<BLK_HASHCHAIN(volume);i++)
		newblock->buffer[i]=0;
	dirblock->flags &= ~BCF_USED;
	newblock->buffer[BLK_PARENT(volume)]=AROS_LONG2BE(dirblock->blocknum);
	newblock->buffer[BLK_EXTENSION(volume)]=0;
	newblock->buffer[BLK_SECONDARY_TYPE(volume)]=AROS_LONG2BE(entrytype);
	dirblock->flags |= BCF_USED;
	if (!invalidBitmap(volume)) {
		newblock->flags &= ~BCF_USED;
		dirblock->flags &= ~BCF_USED;
		return DOSFALSE;
	}
	newblock->blocknum=allocBlock(volume);
	if (newblock->blocknum==0) {
		newblock->flags &= ~BCF_USED;
		newblock->acc_count=0;
		newblock->volume=0;
		dirblock->flags &= ~BCF_USED;
		validBitmap(volume);
		error=ERROR_DISK_FULL;
		return DOSFALSE;
	}
	newblock->buffer[BLK_OWN_KEY]=AROS_LONG2BE(newblock->blocknum);
	if (!(dirblock=linkNewBlock(volume,dirblock,newblock))) {
		markBlock(volume,newblock->blocknum,-1);
		newblock->flags &= ~BCF_USED;
		newblock->acc_count=0;
		newblock->volume=0;
		dirblock->flags &= ~BCF_USED;
		validBitmap(volume);
		return DOSFALSE;
	}
	writeBlock(volume, newblock);		// if crash after this block not yet linked->block not written to disk, bitmap corrected
	writeBlock(volume, dirblock);		// consistent
	newblock->flags &= ~BCF_USED;
	dirblock->flags &= ~BCF_USED;
	validBitmap(volume);				// set bitmap valid
	return newblock;
}


struct AfsHandle *createDir(struct AfsHandle *dirah, STRPTR filename, ULONG protection) {
struct AfsHandle *ah=0;
struct BlockCache *dirblock;
char dirname[34];

	D(bug("afs.handler: createDir(ah,%s,%ld)\n",filename,protection));
	if ((dirblock=getDirBlockBuffer(dirah, filename, dirname))) {
		D(bug("afs.handler:    dir is on block %ld\n",dirblock->blocknum));
		dirblock=createNewEntry(dirah->volume,ST_USERDIR, dirname, dirblock, protection);
		if (dirblock)
			ah=getHandle(dirah->volume,dirblock, FMF_READ);
	}
	return ah;
}

