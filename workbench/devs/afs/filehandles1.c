#define DEBUG 1

#include <proto/exec.h>
#include <proto/dos.h>
#include <exec/memory.h>
#include <dos/dos.h>

#include <aros/debug.h>
#include <aros/macros.h>

#include "filehandles1.h"
#include "filehandles2.h"
#include "hashing.h"
#include "extstrings.h"
#include "checksums.h"
#include "bitmap.h"
#include "error.h"
#include "afsblocks.h"

extern ULONG error;

/***********************************************
 Name  : getHeaderBlock
 Descr.: search through blocks until header block found
 Input : name        - object we are searching for
         blockbuffer - dirblock we are searching in
         block       - will be filled with the block number
                       prior the entry we are using
 Output: cache block of last object
 See   : locateObject, setDate, setComment, deleteObject
************************************************/
struct BlockCache *getHeaderBlock(struct Volume *volume,STRPTR name, struct BlockCache *blockbuffer, ULONG *block) {
ULONG key;

	D(bug("afs.handler:    getHeaderBlock: searching for block of %s\n",name));
	key=getHashKey(name,volume->SizeBlock-56,volume->flags)+BLK_TABLE_START;
	*block=blockbuffer->blocknum;
	if (!blockbuffer->buffer[key]) {
		error=ERROR_OBJECT_NOT_FOUND;
		return 0;
	}
	blockbuffer=getBlock(volume,AROS_BE2LONG(blockbuffer->buffer[key]));
	if (!blockbuffer) {
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (calcChkSum(volume, blockbuffer->buffer)) {
		showError(ERR_CHECKSUM,blockbuffer->blocknum);
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
		showError(ERR_BLOCKTYPE,blockbuffer->blocknum);
		error=ERROR_OBJECT_WRONG_TYPE;
		return 0;
	}
	while (!noCaseStrCmp(name,(char *)((ULONG)blockbuffer->buffer+(BLK_DIRECTORYNAME_START(volume)*4)),volume->flags)) {
		*block=blockbuffer->blocknum;
		if (!blockbuffer->buffer[BLK_HASHCHAIN(volume)]) {
			error=ERROR_OBJECT_NOT_FOUND;
			return 0;
		}
		blockbuffer=getBlock(volume,AROS_BE2LONG(blockbuffer->buffer[BLK_HASHCHAIN(volume)]));
		if (!blockbuffer) {
			error=ERROR_UNKNOWN;
			return 0;
		}
		if (calcChkSum(volume, blockbuffer->buffer)) {
			showError(ERR_CHECKSUM,blockbuffer->blocknum);
			error=ERROR_UNKNOWN;
			return 0;
		}
		if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
			showError(ERR_BLOCKTYPE,blockbuffer->blocknum);
			error=ERROR_OBJECT_WRONG_TYPE;
			return 0;
		}
	}
	return blockbuffer;
}

/*******************************************
 Name  : findBlock
 Descr.: find the header block of a file/dir
 Input : dirah    - directory lock as starting point
                   if NULL, start in root dir
         name    - path of file/dir
         block   - will be filled with the block number
                   prior the entry we are using; rootblock
                   if we are searching for the root
 Output: NULL=error (evtl. error=ERROR_...)
         blockcache structure of found block otherwise
********************************************/
struct BlockCache *findBlock(struct AfsHandle *dirah, STRPTR name, ULONG *block) {
STRPTR pos;
struct BlockCache *blockbuffer;
UBYTE buffer[32];

	*block=dirah->header_block;
	/* Skip ":" if there is one */
	pos=name;
	while ((*pos) && (*pos!=':'))
		pos++;
	if (*pos==':')
		name=pos+1;

	D(bug("afs.handler:    findBlock: startblock=%ld\n",*block));
	// get first entry (root or filelock refers to)
	if (!(blockbuffer=getBlock(dirah->volume,*block))) {
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (calcChkSum(dirah->volume, blockbuffer->buffer)) {
		showError(ERR_CHECKSUM,*block);
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
		showError(ERR_BLOCKTYPE,*block);
		error=ERROR_OBJECT_WRONG_TYPE;
		return 0;
	}
	if ((AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_ROOT) &&
		(AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_USERDIR) &&
		(AROS_BE2LONG(blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_LINKDIR))
	{
		error=ERROR_OBJECT_WRONG_TYPE;
		return 0;
	}
	while (*name)
	{
		if (*name=='/')	// get parent entry ?
		{
			if (!blockbuffer->buffer[BLK_PARENT(dirah->volume)])
			{
				error=ERROR_OBJECT_NOT_FOUND;
				return 0;
			}
			blockbuffer=getBlock(dirah->volume, AROS_BE2LONG(blockbuffer->buffer[BLK_PARENT(dirah->volume)]));
			if (!blockbuffer)
			{
				error=ERROR_UNKNOWN;
				return 0;
			}
			name++;
		}
		else
		{
			pos=buffer;
			while ((*name) && (*name!='/'))
			{
				*pos++=*name++;
			}
			if (*name=='/')
				name++;
			*pos=0;
			D(bug("afs.handler:   findBlock: searching for header block of %s\n",buffer));
			blockbuffer=getHeaderBlock(dirah->volume,buffer,blockbuffer, block);
			if (blockbuffer==0)
				break;		//object not found or other error
		}
	}
	return blockbuffer;
}

void addHandle(struct AfsHandle *ah) {

	ah->next=ah->volume->locklist;
	ah->volume->locklist=ah;
}

void remHandle(struct AfsHandle *ah) {
struct AfsHandle *old;

	if (ah->volume->locklist==ah)
		ah->volume->locklist=ah->next;
	else
	{
		old=ah->volume->locklist;
		while (old)
		{
			if (old->next==ah)
			{
				old->next=ah->next;
				return;
			}
			old=old->next;
		}
	}
}

struct AfsHandle *findHandle(struct Volume *volume, ULONG block) {
struct AfsHandle *ah;

	ah=volume->locklist;
	while (ah)
	{
		if (ah->header_block==block)
			return ah;
		ah=ah->next;
	}
	return 0;
}

struct AfsHandle *allocHandle(struct Volume *volume, struct BlockCache *fileblock, ULONG mode, ULONG *hashtable) {
struct AfsHandle *ah;

	ah=(struct AfsHandle *)AllocMem(sizeof(struct AfsHandle), MEMF_PUBLIC | MEMF_CLEAR);
	if (ah)
	{
		ah->header_block=fileblock->blocknum;
		ah->dirpos=fileblock->blocknum;
		ah->mode=mode;
		ah->current.block=fileblock->blocknum;
		ah->current.filekey=BLK_TABLE_END(volume);
		ah->current.byte=0;
		ah->current.offset=0;
		ah->filesize=AROS_BE2LONG(fileblock->buffer[BLK_BYTE_SIZE(volume)]);
		ah->volume=volume;
		addHandle(ah);
	}
	else
		error=ERROR_NO_FREE_STORE;
	return ah;
}

struct AfsHandle *getHandle(struct Volume *volume, struct BlockCache *fileblock, ULONG mode) {
struct AfsHandle *ah;

	D(bug("afs.handler:    getHandle: trying to get handle for block %ld\n",fileblock->blocknum));
	ah=findHandle(volume, fileblock->blocknum);
	if (ah)
	{
		if (ah->mode & FMF_LOCK) {
			error=ERROR_OBJECT_IN_USE;
			ah=0;
		}
		else
			ah=allocHandle(volume, fileblock,mode,(ULONG *)((ULONG)fileblock->buffer+(BLK_TABLE_START*4)));
	}
	else
		ah=allocHandle(volume, fileblock,mode,(ULONG *)((ULONG)fileblock->buffer+(BLK_TABLE_START*4)));
	return ah;
}

struct AfsHandle *openf(struct AfsHandle *dirah, STRPTR filename, ULONG mode) {
struct AfsHandle *ah=0;
struct BlockCache *fileblock;
ULONG block;

	D(bug("afs.handler: openf(%ld,%s,%ld)\n",dirah->header_block,filename,mode,filename));
	fileblock=findBlock(dirah,filename,&block);
	if (fileblock)
		ah=getHandle(dirah->volume,fileblock,mode);
	return ah;
}

struct AfsHandle *openfile(struct AfsHandle *dirah, STRPTR name, ULONG mode, ULONG protection) {
struct AfsHandle *ah=0;
struct BlockCache *fileblock, *dirblock;
UBYTE filename[34];
ULONG block;

	D(bug("afs.handler: openfile(%ld,%s,%ld,%d)\n",dirah->header_block,name,mode,protection));
	if ((dirblock=getDirBlockBuffer(dirah, name, filename))) {
		D(bug("afs.handler:    parent of %s is on block %ld\n",name,dirblock->blocknum));
		dirblock->flags |= BCF_USED;
		fileblock=getHeaderBlock(dirah->volume,filename+1,dirblock,&block);
		dirblock->flags &= ~BCF_USED;
		if ((fileblock) && (AROS_BE2LONG(fileblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_FILE)) {
			error=ERROR_OBJECT_WRONG_TYPE;
		}
		else {
			if (mode & FMF_CLEAR)
				deleteObject(dirah, name);
			if (mode & FMF_CREATE)
				fileblock=createNewEntry(dirah->volume, ST_FILE, filename, dirblock, protection);
			if (fileblock) {
				error=0;	//reset error
				ah=getHandle(dirah->volume,fileblock, mode);
			}
		}
	}
	return ah;
}

void closef(struct AfsHandle *ah) {

	D(bug("afs.handler: closef(%lx)\n",ah->header_block));
	remHandle(ah);
	FreeMem(ah,sizeof(struct AfsHandle));
}

LONG readData(struct AfsHandle *ah,void *buffer,ULONG length) {
struct BlockCache *extensionbuffer;
struct BlockCache *databuffer;
UWORD size;
LONG readbytes=0;
ULONG source;

	if (ah->current.block==0)
		return 0;								//we can't read beyond EOF so return EOF
	if (length>(ah->filesize-ah->current.offset))
		length=ah->filesize-ah->current.offset;	//we can't read more bytes than left in file!
	D(bug("afs.handler:   readData: offset=%ld\n",ah->current.offset));
	if (!(extensionbuffer=getBlock(ah->volume,ah->current.block)))
		return 0;
	extensionbuffer->flags |=BCF_USED;	// dont overwrite that cache block!
	while (length) {
		D(bug("afs.handler:   readData: bytes left=%ld\n",length));
		//block, filekey always point to the next block
		//so update them if we have read a whole block
		if (ah->current.filekey<BLK_TABLE_START) {	//next extension block?
			ah->current.block=AROS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(ah->volume)]);
			ah->current.filekey=BLK_TABLE_END(ah->volume);
			extensionbuffer->flags &= ~BCF_USED;		//we can now overwrite that cache block
			D(bug("afs.handler:   readData: reading extensionblock=%ld\n",ah->current.block));
			if (ah->current.block) {
				if (!(extensionbuffer=getBlock(ah->volume,ah->current.block)))
					return readbytes;
				extensionbuffer->flags |= BCF_USED;	//dont overwrite this cache block
			}
#ifdef DEBUG
			else
				if (length) D(bug("Shit, out of extensionblocks!\nBytes left: %d\nLast extensionblock: %d",length,extensionbuffer->blocknum));
#endif
		}
#ifdef DEBUG
if (!extensionbuffer->buffer[ah->current.filekey]) D(bug("Oh, oh! Damaged fileblock pointers!\nBytes left: %d\nExtension block: %d (pos in table is %d)",length,ah->current.block,ah->current.filekey));
#endif
		D(bug("afs.handler:   readData: reading datablock %ld\n",AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])));
		if (!(databuffer=getBlock(ah->volume,AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])))) {
			extensionbuffer->flags &=~BCF_USED;	//free that block
			return readbytes;
		}
		source=(ULONG)databuffer->buffer+ah->current.byte;
		if (ah->volume->flags==0) {
			size=AROS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]);
			source += (BLK_DATA_START*4);
		}
		else
			size=BLOCK_SIZE(ah->volume);
		size -= ah->current.byte;
		if (size>length) {
			size=length;
			ah->current.byte += size;
		}
		else {
			ah->current.byte=0;
			ah->current.filekey--;
		}
		CopyMem((APTR)source,(APTR)((ULONG)buffer+readbytes),size);
		length -= size;
		readbytes += size;
	}
	extensionbuffer->flags &= ~BCF_USED;
	return readbytes;
}

LONG read(struct AfsHandle *ah,void *buffer,ULONG length) {
LONG readbytes;

	D(bug("afs.handler:   read(ah,buffer,%ld)\n",length));
	readbytes=readData(ah,buffer,length);
	ah->current.offset=ah->current.offset+readbytes;
	return readbytes;
}

void newFileExtensionBlock(struct Volume *volume,struct BlockCache *extension,ULONG parent) {
UWORD i;

	extension->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_LIST);
	extension->buffer[BLK_OWN_KEY]=AROS_LONG2BE(extension->blocknum);
	for (i=3;i<BLK_PARENT(volume);i++)
		extension->buffer[i]=0;
	extension->buffer[BLK_PARENT(volume)]=AROS_LONG2BE(parent);
	extension->buffer[BLK_EXTENSION(volume)]=0;
	extension->buffer[BLK_SECONDARY_TYPE(volume)]=AROS_LONG2BE(ST_FILE);
}

void writeExtensionBlock(struct Volume *volume, struct BlockCache *extension, ULONG filekey, ULONG next) {

	extension->buffer[BLK_BLOCK_COUNT]=AROS_LONG2BE(BLK_TABLE_END(volume)-(filekey-1));
	extension->buffer[BLK_EXTENSION(volume)]=AROS_LONG2BE(next);
	extension->buffer[BLK_CHECKSUM]=0;
	extension->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(volume,extension->buffer));
	writeBlock(volume,extension);
}

LONG writeData(struct AfsHandle *ah,void *buffer,ULONG length) {
ULONG block,lastblock=0;	//lastblock=0 means: dont update BLK_NEXT_DATA
struct BlockCache *extensionbuffer=0;
struct BlockCache *databuffer=0;
UWORD size;
LONG writtenbytes=0;
ULONG destination;

	D(bug("afs.handler:   writeData: offset=%ld\n",ah->current.offset));
	if (!(extensionbuffer=getBlock(ah->volume,ah->current.block)))
		return 0;
	extensionbuffer->flags |=BCF_USED;	// dont overwrite that cache block!
	while (length) {
		if ((ah->current.byte==0) &&						// last block fully written
			(ah->current.filekey!=BLK_TABLE_END(ah->volume))) {	// this is not the first block of the file
			lastblock=AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey+1]);	// get the previously written block
			D(bug("afs.handler:   writeData: for OFS last datablock was %ld\n",lastblock));
		}
		//block, filekey always point to the last block
		//so update them if we have read a whole block
		if (ah->current.filekey<BLK_TABLE_START) {	//next extension block?
			D(bug("afs.handler:   writeData: need new extensionblock\n"));
			block=allocBlock(ah->volume);
			extensionbuffer->flags &= ~BCF_USED;		//we can now overwrite that cache block
			writeExtensionBlock(ah->volume,extensionbuffer,ah->current.filekey,block);
			if (!block)
				return writtenbytes;
			ah->current.filekey=BLK_TABLE_END(ah->volume);
			if (!(extensionbuffer=getFreeCacheBlock(ah->volume,block)))
				return writtenbytes;
			newFileExtensionBlock(ah->volume,extensionbuffer, ah->header_block);
			extensionbuffer->flags |= BCF_USED;	//dont overwrite this cache block
			ah->current.block=block;
		}
		if (extensionbuffer->buffer[ah->current.filekey]) {	// do we already have that block ?
			D(bug("afs.handler:   writeData: using old datablock %ld\n",AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])));
			if (!(databuffer=getBlock(ah->volume, AROS_LONG2BE(extensionbuffer->buffer[ah->current.filekey])))) {
				writeExtensionBlock(ah->volume,extensionbuffer,ah->current.filekey,0);
				extensionbuffer->flags &= ~BCF_USED;	//free that block
				return writtenbytes;
			}
		}
		else {
			D(bug("afs.handler:   writeData: need a new datablock\n"));
			if (!(block=allocBlock(ah->volume))) {
				writeExtensionBlock(ah->volume,extensionbuffer,ah->current.filekey,0);
				extensionbuffer->flags &= ~BCF_USED;
				return writtenbytes;
			}
			extensionbuffer->buffer[ah->current.filekey]=AROS_LONG2BE(block);
			if ((ah->volume->flags==0) && (lastblock)) {
				D(bug("afs.handler:   writeData: OFS->fill in %ld BLK_NEXT_DATA\n",lastblock));
				// we allocated a new block
				// so there MUST be an initialized lastblock
				if (!(databuffer=getBlock(ah->volume,lastblock))) {
					writeExtensionBlock(ah->volume,extensionbuffer,ah->current.filekey,0);
					extensionbuffer->flags &= ~BCF_USED;	//free that block
					return writtenbytes;
				}
				databuffer->buffer[BLK_NEXT_DATA]=AROS_LONG2BE(block);
				databuffer->buffer[BLK_CHECKSUM]=0;
				databuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(ah->volume,databuffer->buffer));
				writeBlock(ah->volume,databuffer);
			}
			if (!(databuffer=getFreeCacheBlock(ah->volume,block))) {
				writeExtensionBlock(ah->volume,extensionbuffer,ah->current.filekey,0);
				extensionbuffer->flags &= ~BCF_USED;	//free that block
				return writtenbytes;
			}
			if (ah->volume->flags==0) {
				databuffer->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_DATA);
				databuffer->buffer[BLK_HEADER_KEY]=AROS_LONG2BE(ah->header_block);
				databuffer->buffer[BLK_SEQUENCE_NUMBER]=AROS_LONG2BE(((ah->current.offset+writtenbytes)/488)+1);
				databuffer->buffer[BLK_DATA_SIZE]=0;
			}
		}
		destination=(ULONG)databuffer->buffer+ah->current.byte;
		size=BLOCK_SIZE(ah->volume);
		if (ah->volume->flags==0) {
			size -= (6*4);
			destination += (BLK_DATA_START*4);
		}
		size -= ah->current.byte;
		if (size>length) {
			size=length;
			ah->current.byte += size;
		}
		else {
			ah->current.byte=0;
			ah->current.filekey--;
		}
		CopyMem((APTR)((ULONG)buffer+writtenbytes),(APTR)destination,size);
		if (ah->volume->flags==0) {
			databuffer->buffer[BLK_DATA_SIZE]=AROS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE])+AROS_LONG2BE(size);
			databuffer->buffer[BLK_NEXT_DATA]=0;
			databuffer->buffer[BLK_CHECKSUM]=0;
			databuffer->buffer[BLK_CHECKSUM]=AROS_LONG2BE(0-calcChkSum(ah->volume,databuffer->buffer));
		}
		writeBlock(ah->volume,databuffer);
		length -= size;
		writtenbytes += size;
	}
	writeExtensionBlock(ah->volume,extensionbuffer,ah->current.byte==0 ? ah->current.filekey+1 : ah->current.filekey,0);
	extensionbuffer->flags &= ~BCF_USED;
	return writtenbytes;
}

LONG write(struct AfsHandle *ah, void *buffer, ULONG length) {
struct BlockCache *headerblock;
LONG writtenbytes;
struct DateStamp ds;

	D(bug("afs.handler: write(ah,buffer,%ld)\n",length));
	invalidBitmap(ah->volume);
	writtenbytes=writeData(ah, buffer, length);
	ah->current.offset += writtenbytes;
	if ((headerblock=getBlock(ah->volume,ah->header_block))) {
		headerblock->buffer[BLK_FIRST_DATA]=headerblock->buffer[BLK_TABLE_END(ah->volume)];
		if (ah->current.offset>ah->filesize) {
			ah->filesize=ah->current.offset;
			headerblock->buffer[BLK_BYTE_SIZE(ah->volume)]=AROS_LONG2BE(ah->filesize);
		}
		DateStamp(&ds);
		setHeaderDate(ah->volume,headerblock,&ds);
	}
	validBitmap(ah->volume);
	return writtenbytes;
}

LONG seek(struct AfsHandle *ah,LONG offset, LONG mode) {
LONG old=-1;
UWORD filekey,byte=0;
ULONG block;
UWORD size;
ULONG newoffset;
struct BlockCache *blockbuffer;

	D(bug("afs.handler: seek(ah,%ld,%ld)\n",offset,mode));
	error=ERROR_SEEK_ERROR;
	if (mode==OFFSET_BEGINNING) {
		block=ah->header_block;
		filekey=BLK_TABLE_END(ah->volume);
		newoffset=offset;
	}
	else if (mode==OFFSET_CURRENT) {
		newoffset=ah->current.offset+offset;
		if (offset>=0) {
			offset=offset+ah->current.byte;		// we dont use extra code for ah.current.byte! so add it with offset
		}
		else {
			block=ah->header_block;
			filekey=BLK_TABLE_END(ah->volume);
			offset=ah->current.offset+offset+ah->current.byte;	// we dont use extra code for ah.current.byte! so add it with offset
		}
	}
	else if (mode==OFFSET_END) {
		offset=ah->filesize+offset;
		newoffset=offset;
		block=ah->header_block;
		filekey=BLK_TABLE_END(ah->volume);
	}
	if (offset>=0) {
		size=BLOCK_SIZE(ah->volume);
		if (ah->volume->flags==0)
			size -= (BLK_DATA_START*4);
		if (!(blockbuffer=getBlock(ah->volume,block)))
			return -1;
		while ((block>0) && (offset>0)) {
			if (filekey<BLK_TABLE_START) {
				block=AROS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(ah->volume)]);
				D(bug("afs.handler: seek: reading new extensionblock %ld\n",block));
				filekey=BLK_TABLE_END(ah->volume);
				if (!(blockbuffer=getBlock(ah->volume,block)))
					return -1;
			}
			if (blockbuffer->buffer[filekey]) {
				if (size>offset) {
					size=offset;
					byte=size;
				}
				else
					filekey--;
				offset -= size;
			}
			else
				block=0;
		}
		if (offset==0) {
			old=ah->current.offset;
			ah->current.block=block;
			ah->current.filekey=filekey;
			ah->current.byte=byte;
			ah->current.offset=newoffset;
		}
	}
	return old;
}

