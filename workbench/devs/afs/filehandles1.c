/*
   $Id$
*/

#ifndef DEBUG
#define DEBUG 1
#endif

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
#include "baseredef.h"

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
struct BlockCache *getHeaderBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		STRPTR name,
		struct BlockCache *blockbuffer,
		ULONG *block
	)
{
ULONG key;

	D(bug("afs.handler:    getHeaderBlock: searching for block of %s\n",name));
	key=getHashKey(name,volume->SizeBlock-56,volume->flags)+BLK_TABLE_START;
	*block=blockbuffer->blocknum;
	if (!blockbuffer->buffer[key]) {
		error=ERROR_OBJECT_NOT_FOUND;
		return 0;
	}
	blockbuffer=getBlock(afsbase, volume,AROS_BE2LONG(blockbuffer->buffer[key]));
	if (!blockbuffer) {
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (calcChkSum(volume->SizeBlock, blockbuffer->buffer)) {
		showError(afsbase, ERR_CHECKSUM,blockbuffer->blocknum);
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
		showError(afsbase, ERR_BLOCKTYPE,blockbuffer->blocknum);
		error=ERROR_OBJECT_WRONG_TYPE;
		return 0;
	}
	while (
				!noCaseStrCmp
					(
						name,
						(char *)
							(
								(ULONG)blockbuffer->buffer+
								(BLK_DIRECTORYNAME_START(volume)*4)
							),
						volume->flags
					)
			)
	{
		*block=blockbuffer->blocknum;
		if (!blockbuffer->buffer[BLK_HASHCHAIN(volume)]) {
			error=ERROR_OBJECT_NOT_FOUND;
			return 0;
		}
		blockbuffer=getBlock
			(
				afsbase, volume,AROS_BE2LONG(blockbuffer->buffer[BLK_HASHCHAIN(volume)])
			);
		if (!blockbuffer) {
			error=ERROR_UNKNOWN;
			return 0;
		}
		if (calcChkSum(volume->SizeBlock, blockbuffer->buffer)) {
			showError(afsbase, ERR_CHECKSUM,blockbuffer->blocknum);
			error=ERROR_UNKNOWN;
			return 0;
		}
		if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
			showError(afsbase, ERR_BLOCKTYPE,blockbuffer->blocknum);
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
struct BlockCache *findBlock
	(
		struct afsbase *afsbase,
		struct AfsHandle *dirah,
		STRPTR name,
		ULONG *block
	)
{
STRPTR pos;
struct BlockCache *blockbuffer;
UBYTE buffer[32];

	if (dirah->volume->dostype != 0x444F5300)
	{
		error = ERROR_NOT_A_DOS_DISK;
		return 0;
	}
	*block=dirah->header_block;
	/* Skip ":" if there is one */
	pos=name;
	while ((*pos) && (*pos!=':'))
		pos++;
	if (*pos==':')
		name=pos+1;

	D(bug("afs.handler:    findBlock: startblock=%ld\n",*block));
	// get first entry (root or filelock refers to)
	if (!(blockbuffer=getBlock(afsbase, dirah->volume,*block))) {
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (calcChkSum(dirah->volume->SizeBlock, blockbuffer->buffer)) {
		showError(afsbase, ERR_CHECKSUM,*block);
		error=ERROR_UNKNOWN;
		return 0;
	}
	if (AROS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE])!=T_SHORT) {
		showError(afsbase, ERR_BLOCKTYPE,*block);
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
			blockbuffer=getBlock
				(
					afsbase,
					dirah->volume,
					AROS_BE2LONG(blockbuffer->buffer[BLK_PARENT(dirah->volume)])
				);
			if (!blockbuffer)
			{
				error=ERROR_UNKNOWN;
				return 0;
			}
			name++;
		}
		else
		{
			if (
					(AROS_BE2LONG
						(blockbuffer->buffer
							[
								BLK_SECONDARY_TYPE(dirah->volume)
							]
						)!=ST_ROOT) &&
					(AROS_BE2LONG
						(blockbuffer->buffer
							[
								BLK_SECONDARY_TYPE(dirah->volume)
							]
						)!=ST_USERDIR) &&
					(AROS_BE2LONG
						(blockbuffer->buffer
							[
								BLK_SECONDARY_TYPE(dirah->volume)
							]
						)!=ST_LINKDIR))
			{
				error=ERROR_OBJECT_WRONG_TYPE;
				return 0;
			}
			pos=buffer;
			while ((*name) && (*name!='/'))
			{
				*pos++=*name++;
			}
			if (*name=='/')
				name++;
			*pos=0;
			D(bug("afs.handler:   findBlock: searching for header block of %s\n",buffer));
			blockbuffer=getHeaderBlock(afsbase, dirah->volume,buffer,blockbuffer, block);
			if (blockbuffer==0)
				break;		//object not found or other error
		}
	}
	D(
		if (blockbuffer)
			bug("afs.handler:   findBlock: block=%ld\n",blockbuffer->blocknum);
		else
			bug("afs.handler:   findBlock: error\n");
	);
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

/****************************************
 Name  : allocHandle
 Descr.: allocate a new handle
 Input : afsbase   - 
         volume    -
         fileblock - block of the entry
         mode      - type of lock
         hashtable - ptr to the (hash)table
 Output: AfsHandle for success; NULL otherwise
****************************************/
struct AfsHandle *allocHandle
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		struct BlockCache *fileblock,
		ULONG mode,
		ULONG *hashtable
	)
{
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

/****************************************
 Name  : getHandle
 Descr.: check if if a new handle can be
         allocated and allocate one if
         possible
 Input : afsbase   - 
         volume    -
         fileblock - block of the entry
         mode      - type of lock
 Output: AfsHandle for success; NULL otherwise
****************************************/
struct AfsHandle *getHandle
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		struct BlockCache *fileblock,
		ULONG mode
	)
{
struct AfsHandle *ah;

	D(bug
		(
			"afs.handler:    getHandle: trying to get handle for block %ld\n",
			fileblock->blocknum)
		);
	ah=findHandle(volume, fileblock->blocknum);
	if (ah)
	{
		if (ah->mode & FMF_LOCK) {
			error=ERROR_OBJECT_IN_USE;
			ah=0;
		}
		else
		{
			ah=allocHandle
				(
					afsbase,
					volume,
					fileblock,
					mode,
					(ULONG *)((ULONG)fileblock->buffer+(BLK_TABLE_START*4))
				);
		}
	}
	else
	{
		ah=allocHandle
			(
				afsbase,
				volume,
				fileblock,
				mode,
				(ULONG *)((ULONG)fileblock->buffer+(BLK_TABLE_START*4))
			);
	}
	return ah;
}

/*****************************************
 Name  : openf
 Descr.: open (lock) a file
 Input : afsbase  -
         dirah    - a handle filename is
                    relative to
         filename - filename to lock
         mode     - lock type
 Output: AfsHandle for success; NULL otherwise
******************************************/
struct AfsHandle *openf
	(
		struct afsbase *afsbase,
		struct AfsHandle *dirah,
		STRPTR filename,
		ULONG mode
	)
{
struct AfsHandle *ah=0;
struct BlockCache *fileblock;
ULONG block;

	D(bug("afs.handler: openf(%ld,%s,%ld)\n",dirah->header_block,filename,mode));
	fileblock=findBlock(afsbase, dirah,filename,&block);
	if (fileblock)
		ah=getHandle(afsbase, dirah->volume,fileblock,mode);
	return ah;
}

/*****************************************
 Name  : openfile
 Descr.: open (lock) a file
 Input : afsbase    -
         dirah      - a handle filename is
                      relative to
         filename   - filename to lock
         mode       - FMF_...
         protection - bits for new files
 Output: AfsHandle for success; NULL otherwise
******************************************/
struct AfsHandle *openfile
	(
		struct afsbase *afsbase,
		struct AfsHandle *dirah,
		STRPTR name,
		ULONG mode,
		ULONG protection
	)
{
struct AfsHandle *ah=0;
struct BlockCache *fileblock, *dirblock;
UBYTE filename[34];
ULONG block;

	D(bug
		(
			"afs.handler: openfile(%ld,%s,%ld,%d)\n",
			dirah->header_block,name,mode,protection)
		);
	dirblock=getDirBlockBuffer(afsbase, dirah, name, filename);
	if (dirblock)
	{
		D(bug
			(
				"afs.handler:    parent of %s is on block %ld\n",
				name,dirblock->blocknum)
			);
		dirblock->flags |= BCF_USED;
		fileblock=getHeaderBlock
			(
				afsbase,
				dirah->volume,
				filename,
				dirblock,
				&block
			);
		dirblock->flags &= ~BCF_USED;
		if (
				(fileblock) &&
				(AROS_BE2LONG
					(
						fileblock->buffer
							[
								BLK_SECONDARY_TYPE(dirah->volume)
							]
					)!=ST_FILE)
				)
		{
			error=ERROR_OBJECT_WRONG_TYPE;
		}
		else
		{
			if (mode & FMF_CLEAR)
				deleteObject(afsbase, dirah, name);
			if (mode & FMF_CREATE)
			{
				fileblock=createNewEntry
					(
						afsbase,
						dirah->volume,
						ST_FILE,
						filename,
						dirblock,
						protection
					);
			}
			if (fileblock)
			{
				error=0;	//reset error
				ah=getHandle(afsbase, dirah->volume,fileblock, mode);
			}
		}
	}
	return ah;
}

/***********************************
 Name  : closef
 Descr.: close a file/free a lock
 Input : afsbase -
         ah      - the handle to close
 Output -
************************************/
void closef(struct afsbase *afsbase, struct AfsHandle *ah) {

	D(bug("afs.handler: closef(%lx)\n",ah->header_block));
	remHandle(ah);
	FreeMem(ah,sizeof(struct AfsHandle));
}

/******************************************
 Name  : readData
 Descr.: read data from file
 Input : afsbase -
         ah      - handle (file) to read from
         buffer  - buffer to store data into
         length  - size of data to read
 Output: read bytes
*******************************************/
LONG readData
	(
		struct afsbase *afsbase,
		struct AfsHandle *ah,
		void *buffer,
		ULONG length
	)
{
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
	if (!(extensionbuffer=getBlock(afsbase, ah->volume,ah->current.block)))
		return 0;
	extensionbuffer->flags |=BCF_USED;	// dont overwrite that cache block!
	while (length)
	{
		D(bug("afs.handler:   readData: bytes left=%ld\n",length));
		/*
			block, filekey always point to the next block
			so update them if we have read a whole block
		*/
		/* do we have to read next extension block ? */
		if (ah->current.filekey<BLK_TABLE_START)
		{
			ah->current.block=AROS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(ah->volume)]);
			ah->current.filekey=BLK_TABLE_END(ah->volume);
			extensionbuffer->flags &= ~BCF_USED;		//we can now overwrite that cache block
			D(bug("afs.handler:   readData: reading extensionblock=%ld\n",ah->current.block));
			if (ah->current.block)
			{
				extensionbuffer=getBlock(afsbase, ah->volume,ah->current.block);
				if (!extensionbuffer)
					return readbytes;
				extensionbuffer->flags |= BCF_USED;	//dont overwrite this cache block
			}
D(
			else
				if (length)
					bug
						(
							"Shit, out of extensionblocks!\n"
							"Bytes left: %d\n"
							"Last extensionblock: %d\n",
							length,extensionbuffer->blocknum
						);
);
		}
		D(bug
			(
				"afs.handler:   readData: reading datablock %ld\n",
				AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
			);
		databuffer=getBlock
			(
				afsbase,
				ah->volume,
				AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])
			);
		if (!databuffer)
		{
			extensionbuffer->flags &=~BCF_USED;	//free that block
			return readbytes;
		}
		source=(ULONG)databuffer->buffer+ah->current.byte;
		if (ah->volume->flags==0)
		{
			size=AROS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]);
			source += (BLK_DATA_START*4);
		}
		else
		{
			size=BLOCK_SIZE(ah->volume);
		}
		size -= ah->current.byte;
		if (size>length)
		{
			size=length;
			ah->current.byte += size;
		}
		else
		{
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

LONG read(struct afsbase *afsbase, struct AfsHandle *ah,void *buffer,ULONG length) {
LONG readbytes;

	D(bug("afs.handler:   read(ah,buffer,%ld)\n",length));
	readbytes=readData(afsbase, ah,buffer,length);
	ah->current.offset=ah->current.offset+readbytes;
	return readbytes;
}

void newFileExtensionBlock
	(
		struct Volume *volume,
		struct BlockCache *extension,
		ULONG parent)
{
UWORD i;

	extension->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_LIST);
	extension->buffer[BLK_OWN_KEY]=AROS_LONG2BE(extension->blocknum);
	for (i=3;i<BLK_PARENT(volume);i++)
		extension->buffer[i]=0;
	extension->buffer[BLK_PARENT(volume)]=AROS_LONG2BE(parent);
	extension->buffer[BLK_EXTENSION(volume)]=0;
	extension->buffer[BLK_SECONDARY_TYPE(volume)]=AROS_LONG2BE(ST_FILE);
}

void writeExtensionBlock
	(
		struct afsbase *afsbase,
		struct Volume *volume,
		struct BlockCache *extension,
		ULONG filekey,
		ULONG next)
{

	extension->buffer[BLK_BLOCK_COUNT]=AROS_LONG2BE(BLK_TABLE_END(volume)-(filekey-1));
	if (next)
		extension->buffer[BLK_EXTENSION(volume)]=AROS_LONG2BE(next);
	extension->buffer[BLK_CHECKSUM]=0;
	extension->buffer[BLK_CHECKSUM]=
		AROS_LONG2BE
			(
				0-calcChkSum(volume->SizeBlock,extension->buffer)
			);
	writeBlock(afsbase, volume,extension);
}

LONG writeData(struct afsbase *afsbase, struct AfsHandle *ah,void *buffer,ULONG length) {
ULONG block,lastblock=0;	//lastblock=0 means: dont update BLK_NEXT_DATA
struct BlockCache *extensionbuffer=0;
struct BlockCache *databuffer=0;
UWORD size;
LONG writtenbytes=0;
ULONG destination;

	D(bug("afs.handler:   writeData: offset=%ld\n",ah->current.offset));
	if (!(extensionbuffer=getBlock(afsbase, ah->volume,ah->current.block)))
		return 0;
	extensionbuffer->flags |=BCF_USED;	// dont overwrite that cache block!
	while (length)
	{
		/* save last data block for OFS data */
		if ((ah->current.byte==0) &&						// last block fully written
			(ah->current.filekey!=BLK_TABLE_END(ah->volume)))	// this is not the first block of the file
		{
			lastblock=AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey+1]);
			D(bug("afs.handler:   writeData: for OFS last datablock was %ld\n",lastblock));
		}
		/*
			block, filekey always point to the last block
			so update them if we have read a whole block
		*/
		/* read next extension block ?*/
		if (ah->current.filekey<BLK_TABLE_START)
		{
			extensionbuffer->flags &= ~BCF_USED;		//we can now overwrite that cache block
			if (extensionbuffer->buffer[BLK_EXTENSION(ah->volume)])
			{
				extensionbuffer=getBlock
					(
						afsbase,
						ah->volume,
						AROS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(ah->volume)])
					);
				if (!extensionbuffer)
					return writtenbytes;
			}
			else
			{
				D(bug("afs.handler:   writeData: need new extensionblock\n"));
				block=allocBlock(afsbase, ah->volume);
				writeExtensionBlock
					(
						afsbase,
						ah->volume,
						extensionbuffer,
						ah->current.filekey+1,
						block
					);
				if (!block)
					return writtenbytes;
				extensionbuffer=getFreeCacheBlock(afsbase, ah->volume,block);
				if (!extensionbuffer)
					return writtenbytes;
				newFileExtensionBlock(ah->volume,extensionbuffer, ah->header_block);
			}
			ah->current.filekey=BLK_TABLE_END(ah->volume);
			extensionbuffer->flags |= BCF_USED;	//dont overwrite this cache block
			ah->current.block=block;
		}
		/* find a block to write data into */
		if (extensionbuffer->buffer[ah->current.filekey])	/* do we already have that block ? */
		{
			D(bug
				(
					"afs.handler:   writeData: using old datablock %ld\n",
					AROS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
				);
			databuffer=getBlock
				(
					afsbase,
					ah->volume,
					AROS_LONG2BE(extensionbuffer->buffer[ah->current.filekey])
				);
			if (!databuffer)
			{
				writeExtensionBlock
					(
						afsbase,
						ah->volume,
						extensionbuffer,
						ah->current.filekey,
						0
					);
				extensionbuffer->flags &= ~BCF_USED;	//free that block
				return writtenbytes;
			}
		}
		else
		{
			D(bug("afs.handler:   writeData: need a new datablock\n"));
			if (!(block=allocBlock(afsbase, ah->volume)))
			{
				writeExtensionBlock
					(
						afsbase,
						ah->volume,
						extensionbuffer,
						ah->current.filekey,
						0
					);
				extensionbuffer->flags &= ~BCF_USED;
				return writtenbytes;
			}
			extensionbuffer->buffer[ah->current.filekey]=AROS_LONG2BE(block);
			if ((ah->volume->flags==0) && (lastblock))
			{
				D(bug("afs.handler:   writeData: OFS->fill in %ld BLK_NEXT_DATA\n",lastblock));
				/*
					 we allocated a new block
					so there MUST be an initialized lastblock
				*/
				databuffer=getBlock(afsbase, ah->volume,lastblock);
				if (!databuffer)
				{
					writeExtensionBlock
						(
							afsbase,
							ah->volume,
							extensionbuffer,
							ah->current.filekey,
							0
						);
					extensionbuffer->flags &= ~BCF_USED;	//free that block
					return writtenbytes;
				}
				databuffer->buffer[BLK_NEXT_DATA]=AROS_LONG2BE(block);
				databuffer->buffer[BLK_CHECKSUM]=0;
				databuffer->buffer[BLK_CHECKSUM]=
					AROS_LONG2BE
						(
							0-calcChkSum(ah->volume->SizeBlock,databuffer->buffer)
						);
				writeBlock(afsbase, ah->volume,databuffer);
			}
			databuffer=getFreeCacheBlock(afsbase, ah->volume,block);
			if (!databuffer)
			{
				writeExtensionBlock
					(
						afsbase,
						ah->volume,
						extensionbuffer,
						ah->current.filekey,
						0
					);
				extensionbuffer->flags &= ~BCF_USED;	//free that block
				return writtenbytes;
			}
			if (ah->volume->flags==0) {
				databuffer->buffer[BLK_PRIMARY_TYPE]=AROS_LONG2BE(T_DATA);
				databuffer->buffer[BLK_HEADER_KEY]=AROS_LONG2BE(ah->header_block);
				databuffer->buffer[BLK_SEQUENCE_NUMBER]=
					AROS_LONG2BE
						(
							((ah->current.offset+writtenbytes)/488)+1
						);
				databuffer->buffer[BLK_DATA_SIZE]=0;
				databuffer->buffer[BLK_NEXT_DATA]=0;
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
			if (ah->current.byte==0)
			{
				databuffer->buffer[BLK_DATA_SIZE]=AROS_LONG2BE(BLOCK_SIZE(ah->volume)-(6*4));
			}
			else if (AROS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE])<ah->current.byte)
			{
				databuffer->buffer[BLK_DATA_SIZE]=AROS_LONG2BE(ah->current.byte);
			}
			databuffer->buffer[BLK_CHECKSUM]=0;
			databuffer->buffer[BLK_CHECKSUM]=
				AROS_LONG2BE
					(
						0-calcChkSum(ah->volume->SizeBlock,databuffer->buffer)
					);
		}
		writeBlock(afsbase, ah->volume,databuffer);
		length -= size;
		writtenbytes += size;
	}
	writeExtensionBlock
		(
			afsbase,
			ah->volume,
			extensionbuffer,
			ah->current.byte==0 ?
				ah->current.filekey+1 :
				ah->current.filekey,0);
	extensionbuffer->flags &= ~BCF_USED;
	return writtenbytes;
}

LONG write(struct afsbase *afsbase, struct AfsHandle *ah, void *buffer, ULONG length) {
struct BlockCache *headerblock;
LONG writtenbytes;
struct DateStamp ds;

	D(bug("afs.handler: write(ah,buffer,%ld)\n",length));
	invalidBitmap(afsbase, ah->volume);
	writtenbytes=writeData(afsbase, ah, buffer, length);
	ah->current.offset += writtenbytes;
	headerblock=getBlock(afsbase, ah->volume,ah->header_block);
	if (headerblock)
	{
		headerblock->buffer[BLK_FIRST_DATA]=headerblock->buffer[BLK_TABLE_END(ah->volume)];
		if (ah->current.offset>ah->filesize)
		{
			ah->filesize=ah->current.offset;
			headerblock->buffer[BLK_BYTE_SIZE(ah->volume)]=AROS_LONG2BE(ah->filesize);
		}
		DateStamp(&ds);
		setHeaderDate(afsbase, ah->volume,headerblock,&ds);
	}
	validBitmap(afsbase, ah->volume);
	return writtenbytes;
}

LONG seek(struct afsbase* afsbase, struct AfsHandle *ah,LONG offset, LONG mode) {
LONG old=-1;
UWORD filekey,byte;
ULONG block;
UWORD size,togo;
ULONG newoffset;
struct BlockCache *blockbuffer;

	D(bug("afs.handler: seek(ah,%ld,%ld)\n",offset,mode));
	error=ERROR_SEEK_ERROR;
	if (mode==OFFSET_BEGINNING)
	{
		block=ah->header_block;
		newoffset=(ULONG)offset;
	}
	else if (mode==OFFSET_CURRENT)
	{
	 	if (offset == 0)
		{
		    error = 0;
		    return ah->current.offset;
		}

		newoffset=ah->current.offset+offset;
		block=ah->header_block;
	}
	else if (mode==OFFSET_END)
	{
		newoffset=ah->filesize+offset;
		block=ah->header_block;
	}
	if (newoffset>=0)
	{
		size=BLOCK_SIZE(ah->volume);
		if (ah->volume->flags==0)
			size -= (BLK_DATA_START*4);
		togo = newoffset / size;
		byte = BLK_TABLE_END(ah->volume)-BLK_TABLE_START+1; /* hashtable size */
		filekey = BLK_TABLE_END(ah->volume)-(togo % byte);
		togo /= byte; /* # of extensionblock we need */
		byte = newoffset % size;
		while ((togo) && (block))
		{
			blockbuffer=getBlock(afsbase, ah->volume,block);
			if (!blockbuffer)
				return -1;
			block=AROS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(ah->volume)]);
			togo--;
		}
		if (togo == 0) {
			error = 0;
			old = ah->current.offset;
			ah->current.block = block;
			ah->current.filekey = filekey;
			ah->current.byte = byte;
			ah->current.offset = newoffset;
		}
	}
	return old;
}

