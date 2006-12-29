/*
    Copyright © 1995-2005, The AROS Development Team. All rights reserved.
    $Id$
*/

#ifndef DEBUG
#define DEBUG 0
#endif

#include "os.h"
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
                       prior to the entry we are using
 Output: cache block of last object
 See   : locateObject, setDate, setComment, deleteObject
************************************************/
struct BlockCache *getHeaderBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		STRPTR name,
		struct BlockCache *blockbuffer,
		ULONG *block
	)
{
ULONG key;

	D(bug("[afs]    getHeaderBlock: searching for block of '%s'\n",name));
	key = getHashKey(name,volume->SizeBlock-56,volume->dosflags)+BLK_TABLE_START;
	*block = blockbuffer->blocknum;
	if (blockbuffer->buffer[key] == 0)
	{
		error = ERROR_OBJECT_NOT_FOUND;
		return NULL;
	}
	blockbuffer=getBlock(afsbase, volume,OS_BE2LONG(blockbuffer->buffer[key]));
	if (blockbuffer == NULL)
	{
		error = ERROR_UNKNOWN;
		return NULL;
	}
D
(
	{
		char *name;
		name = (char *)blockbuffer->buffer+(BLK_DIRECTORYNAME_START(volume)*4);
		kprintf("[afs] %.*s\n", name[0], name+1);
	}
);
	if (calcChkSum(volume->SizeBlock, blockbuffer->buffer) != 0)
	{
		showError(afsbase, ERR_CHECKSUM, blockbuffer->blocknum);
		error = ERROR_UNKNOWN;
		return NULL;
	}
	if (OS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE]) != T_SHORT)
	{
		showError(afsbase, ERR_BLOCKTYPE, blockbuffer->blocknum);
		error = ERROR_OBJECT_WRONG_TYPE;
		return NULL;
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
						volume->dosflags,
						MAX_NAME_LENGTH
					)
			)
	{
		*block = blockbuffer->blocknum;
		if (blockbuffer->buffer[BLK_HASHCHAIN(volume)] == 0)
		{
			error=ERROR_OBJECT_NOT_FOUND;
			return NULL;
		}
		blockbuffer=getBlock
			(
				afsbase,
				volume,
				OS_BE2LONG(blockbuffer->buffer[BLK_HASHCHAIN(volume)])
			);
		if (blockbuffer == NULL)
		{
			error = ERROR_UNKNOWN;
			return NULL;
		}
		if (calcChkSum(volume->SizeBlock, blockbuffer->buffer) != 0)
		{
			showError(afsbase, ERR_CHECKSUM,blockbuffer->blocknum);
			error=ERROR_UNKNOWN;
			return NULL;
		}
		if (OS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE]) != T_SHORT)
		{
			showError(afsbase, ERR_BLOCKTYPE, blockbuffer->blocknum);
			error = ERROR_OBJECT_WRONG_TYPE;
			return NULL;
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
		struct AFSBase *afsbase,
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
	*block = dirah->header_block;

	D(bug("[afs]    findBlock: startblock=%ld\n",*block));
	/* get first entry (root or filelock refers to) */
	blockbuffer = getBlock(afsbase, dirah->volume, *block);
	if (blockbuffer == NULL)
	{
		error = ERROR_UNKNOWN;
		D(bug("[afs]    error blockbuffer\n"));
		return NULL;
	}
	if (calcChkSum(dirah->volume->SizeBlock, blockbuffer->buffer) != 0)
	{
		showError(afsbase, ERR_CHECKSUM, *block);
		error = ERROR_UNKNOWN;
		D(bug("[afs]    error checksum\n"));
		return NULL;
	}
	if (OS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE]) != T_SHORT)
	{
		showError(afsbase, ERR_BLOCKTYPE, *block);
		error = ERROR_OBJECT_WRONG_TYPE;
		D(bug("[afs]    error wrong type\n"));
		return NULL;
	}
	while (*name)
	{
		if (*name == '/')	/* get parent entry ? */
		{
			if (blockbuffer->buffer[BLK_PARENT(dirah->volume)] == 0)
			{
				error = ERROR_OBJECT_NOT_FOUND;
				D(bug("[afs]    object not found\n"));
				return NULL;
			}
			blockbuffer = getBlock
				(
					afsbase,
					dirah->volume,
					OS_BE2LONG(blockbuffer->buffer[BLK_PARENT(dirah->volume)])
				);
			if (blockbuffer == NULL)
			{
				error = ERROR_UNKNOWN;
				D(bug("[afs]    error no blockbuffer\n"));
				return NULL;
			}
			name++;
		}
		else
		{
			if (
					(OS_BE2LONG
						(
							blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)]
						) != ST_ROOT) &&
					(OS_BE2LONG
						(
							blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)]
						) != ST_USERDIR) &&
					(OS_BE2LONG
						(
							blockbuffer->buffer[BLK_SECONDARY_TYPE(dirah->volume)]
						) != ST_LINKDIR))
			{
				error = ERROR_OBJECT_WRONG_TYPE;
				D(bug("[afs]    error wrong type\n"));
				return NULL;
			}
			pos = buffer;
			while ((*name != 0) && (*name != '/'))
			{
				*pos++ = *name++;
			}
			if (*name == '/')
				name++;
			*pos=0;
			D(bug
				(
					"[afs]   findBlock: searching for header block of %s\n",
					buffer
				));
			blockbuffer =
				getHeaderBlock(afsbase, dirah->volume, buffer, blockbuffer, block);
			if (blockbuffer == NULL)
				break;		/* object not found or other error */
		}
	}
	D(
		if (blockbuffer != NULL)
			bug("[afs]   findBlock: block=%ld\n",blockbuffer->blocknum);
		else
			bug("[afs]   findBlock: error\n");
	);
	return blockbuffer;
}

/* add handle to locklist */
void addHandle(struct AfsHandle *ah) {

	ah->next=ah->volume->locklist;
	ah->volume->locklist=ah;
}

/* remove handle from locklist */
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

/* find handle in locklist */
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
 Input : volume    -
         fileblock - block of the entry
         mode      - type of lock
         hashtable - ptr to the (hash)table
 Output: AfsHandle for success; NULL otherwise
****************************************/
struct AfsHandle *allocHandle
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *fileblock,
		ULONG mode,
		ULONG *hashtable
	)
{
struct AfsHandle *ah;

	ah=(struct AfsHandle *)AllocMem
		(sizeof(struct AfsHandle), MEMF_PUBLIC | MEMF_CLEAR);
	if (ah != NULL)
	{
		ah->header_block = fileblock->blocknum;
		ah->dirpos = fileblock->blocknum;
		ah->mode = mode;
		ah->current.block = fileblock->blocknum;
		ah->current.filekey = BLK_TABLE_END(volume);
		ah->current.byte = 0;
		ah->current.offset = 0;
		ah->filesize = OS_BE2LONG(fileblock->buffer[BLK_BYTE_SIZE(volume)]);
		ah->volume = volume;
		addHandle(ah);
	}
	else
		error = ERROR_NO_FREE_STORE;
	return ah;
}

/****************************************
 Name  : getHandle
 Descr.: check if a new handle can be
         allocated and allocate one if
         possible
 Input : volume    -
         fileblock - block of the entry
         mode      - type of lock
 Output: AfsHandle for success; NULL otherwise
****************************************/
struct AfsHandle *getHandle
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *fileblock,
		ULONG mode
	)
{
struct AfsHandle *ah;

	D(bug
		(
			"[afs]    getHandle: trying to get handle for block %lu\n",
			fileblock->blocknum)
		);
	ah = findHandle(volume, fileblock->blocknum);
	if (ah != NULL)
	{
		if (ah->mode & FMF_LOCK)
		{
			error = ERROR_OBJECT_IN_USE;
			ah = 0;
		}
		else
		{
			ah = allocHandle
				(
					afsbase,
					volume,
					fileblock,
					mode,
					(ULONG *)((char *)fileblock->buffer+(BLK_TABLE_START*4))
				);
		}
	}
	else
	{
		ah = allocHandle
			(
				afsbase,
				volume,
				fileblock,
				mode,
				(ULONG *)((char *)fileblock->buffer+(BLK_TABLE_START*4))
			);
	}
	return ah;
}

/*****************************************
 Name  : openf
 Descr.: open (lock) a file
 Input : dirah    - a handle filename is
                    relative to
         filename - filename to lock
         mode     - lock type
 Output: AfsHandle for success; NULL otherwise
******************************************/
struct AfsHandle *openf
	(
		struct AFSBase *afsbase,
		struct AfsHandle *dirah,
		STRPTR filename,
		ULONG mode
	)
{
struct AfsHandle *ah = NULL;
struct BlockCache *fileblock;
ULONG block;

	D(bug("[afs] openf(%ld,%s,%ld)\n",dirah->header_block,filename,mode));
	fileblock = findBlock(afsbase, dirah, filename, &block);
	if (fileblock != NULL)
		ah = getHandle(afsbase, dirah->volume, fileblock, mode);
	return ah;
}

/*****************************************
 Name  : openfile
 Descr.: open (lock) a file
 Input : dirah      - a handle filename is
                      relative to
         filename   - filename to lock
         mode       - FMF_...
         protection - bits for new files
 Output: AfsHandle for success; NULL otherwise
******************************************/
struct AfsHandle *openfile
	(
		struct AFSBase *afsbase,
		struct AfsHandle *dirah,
		STRPTR name,
		ULONG mode,
		ULONG protection
	)
{
struct AfsHandle *ah = NULL;
struct BlockCache *fileblock, *dirblock;
UBYTE filename[34];
ULONG block;
ULONG dirblocknum;
ULONG fileblocknum = -1;

	D(bug
		(
			"[afs] openfile(%lu,%s,0x%lx,%lu)\n",
			dirah->header_block,name,mode,protection)
		);
	error = 0;
	/* get the directory the last component of "name" is in */
	dirblock = getDirBlockBuffer(afsbase, dirah, name, filename);
	if (dirblock != NULL)
	{
		if (
				(OS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)]) == ST_USERDIR) ||
				(OS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)]) == ST_ROOT)
			)
		{
			D(bug("[afs]    parent of %s is on block %lu\n", name, dirblock->blocknum));
			dirblocknum = dirblock->blocknum;
			/* get the header block of the file to open */
			fileblock = getHeaderBlock(afsbase, dirah->volume, filename, dirblock, &block);
			if (
					(fileblock != NULL) &&
					(OS_BE2LONG(fileblock->buffer[BLK_SECONDARY_TYPE(dirah->volume)])!=ST_FILE)
				)
			{
				error = ERROR_OBJECT_WRONG_TYPE;
			}
			else
			{
				if (fileblock != NULL)
					fileblocknum = fileblock->blocknum;
				if (mode & FMF_CLEAR)
					error = deleteObject(afsbase, dirah, name);
				if ((error == 0) || (error == ERROR_OBJECT_NOT_FOUND))
				{
					if (
							(mode & FMF_CREATE) &&
							((fileblock == NULL) || (mode & FMF_CLEAR))
						)
					{
						dirblock = getBlock(afsbase, dirah->volume, dirblocknum);
						if (dirblock != NULL)
						{
							fileblock = createNewEntry(afsbase, dirah->volume, ST_FILE, filename, dirblock, protection);
						}
						else
							error = ERROR_UNKNOWN;
					}
					else
					{
						/* we read it before so fileblocknum is valid */
						if (fileblock != NULL)
							fileblock = getBlock(afsbase, dirah->volume, fileblocknum);
					}
					if (fileblock != NULL)
					{
						error = 0;
						ah = getHandle(afsbase, dirah->volume, fileblock, mode);
					}
				}
			}
		}
		else
			error = ERROR_OBJECT_WRONG_TYPE;
	}
	return ah;
}

/***********************************
 Name  : closef
 Descr.: close a file/free a lock
 Input : ah - the handle to close
 Output -
************************************/
void closef(struct AFSBase *afsbase, struct AfsHandle *ah) {

	D(bug("[afs] closef(%lu)\n",ah->header_block));
	remHandle(ah);
	FreeMem(ah,sizeof(struct AfsHandle));
}

/******************************************
 Name  : readData
 Descr.: read data from file
 Input : ah      - handle (file) to read from
         buffer  - buffer to store data into
         length  - size of data to read
 Output: read bytes
*******************************************/
LONG readData
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		void *buffer,
		ULONG length
	)
{
struct BlockCache *extensionbuffer;
struct BlockCache *databuffer;
UWORD size;
LONG readbytes=0;
char *source;

	if (ah->current.block == 0)
		return 0;                     /* we can't read beyond EOF so return EOF */
	if (length > (ah->filesize-ah->current.offset))
	{
		length = ah->filesize-ah->current.offset; /* we can't read more bytes than left in file! */
	}
	D(bug("[afs]   readData: offset=%ld\n", ah->current.offset));
	extensionbuffer = getBlock(afsbase, ah->volume, ah->current.block);
	if (extensionbuffer == NULL)
	{
		error = ERROR_UNKNOWN;
		return ENDSTREAMCH;
	}
	extensionbuffer->flags |= BCF_USED; /* don't overwrite that cache block! */
	while (length != 0)
	{
		D(bug("[afs]   readData: bytes left=%ld\n",length));
		/*
			block, filekey always point to the next block
			so update them if we have read a whole block
		*/
		/* do we have to read next extension block? */
		if (ah->current.filekey<BLK_TABLE_START)
		{
			ah->current.block=
				OS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(ah->volume)]);
			ah->current.filekey = BLK_TABLE_END(ah->volume);
			extensionbuffer->flags &= ~BCF_USED;		//we can now overwrite that cache block
			D(bug("[afs]   readData: reading extensionblock=%ld\n",ah->current.block));
			if (ah->current.block != 0)
			{
				extensionbuffer = getBlock(afsbase, ah->volume,ah->current.block);
				if (extensionbuffer == 0)
				{
					error = ERROR_UNKNOWN;
					return ENDSTREAMCH; //was   readbytes;
				}
				extensionbuffer->flags |= BCF_USED;	//don't overwrite this cache block
			}
D(
			else
				if (length)
					bug
						(
							"Shit, out of extensionblocks!\n"
							"Bytes left: %ld\n"
							"Last extensionblock: %ld\n",
							length,extensionbuffer->blocknum
						);
);
		}
		D(bug
			(
				"[afs]   readData: reading datablock %ld\n",
				OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
			);
		databuffer = getBlock
			(
				afsbase,
				ah->volume,
				OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])
			);
		if (databuffer == 0)
		{
			extensionbuffer->flags &= ~BCF_USED;	//free that block
			error = ERROR_UNKNOWN;
			return ENDSTREAMCH; //was   readbytes;
		}
		source = (char *)databuffer->buffer+ah->current.byte;
		if (ah->volume->dosflags == 0)
		{
			size = OS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]);
			source += (BLK_DATA_START*4);
		}
		else
		{
			size = BLOCK_SIZE(ah->volume);
		}
		size -= ah->current.byte;
		if (size > length)
		{
			size = length;
			ah->current.byte += size;
		}
		else
		{
			ah->current.byte = 0;
			ah->current.filekey--;
		}
		CopyMem((APTR)source, (APTR)((char *)buffer+readbytes),size);
		length -= size;
		readbytes += size;
	}
	extensionbuffer->flags &= ~BCF_USED;
	return readbytes;
}

LONG readf
	(struct AFSBase *afsbase, struct AfsHandle *ah, void *buffer, ULONG length)
{
LONG readbytes;

	D(bug("[afs]   read(%ld,buffer,%ld)\n", ah->header_block, length));
	readbytes = readData(afsbase, ah,buffer,length);
	if (readbytes != ENDSTREAMCH)
		ah->current.offset = ah->current.offset+readbytes;
	return readbytes;
}

void newFileExtensionBlock
	(
		struct Volume *volume,
		struct BlockCache *extension,
		ULONG parent)
{
UWORD i;

	extension->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_LIST);
	extension->buffer[BLK_OWN_KEY ] = OS_LONG2BE(extension->blocknum);
	for (i=2; i<BLK_PARENT(volume); i++)
		extension->buffer[i] = 0;
	extension->buffer[BLK_PARENT(volume)] = OS_LONG2BE(parent);
	extension->buffer[BLK_EXTENSION(volume)] = 0;
	extension->buffer[BLK_SECONDARY_TYPE(volume)] = OS_LONG2BE(ST_FILE);
}

void writeExtensionBlock
	(
		struct AFSBase *afsbase,
		struct Volume *volume,
		struct BlockCache *extension,
		ULONG filekey,
		ULONG next
	)
{
ULONG newcount = BLK_TABLE_END(volume)-(filekey-1);

	if (OS_BE2LONG(extension->buffer[BLK_BLOCK_COUNT]) < newcount)
		extension->buffer[BLK_BLOCK_COUNT] = OS_LONG2BE(newcount);
	if (next != 0)
		extension->buffer[BLK_EXTENSION(volume)] = OS_LONG2BE(next);
	writeBlockDeferred(afsbase, volume, extension, BLK_CHECKSUM);
}

LONG writeData
	(
		struct AFSBase *afsbase,
		struct AfsHandle *ah,
		void *buffer,
		ULONG length
	)
{
ULONG block = 0;
ULONG lastblock = 0;	/* 0 means: don't update BLK_NEXT_DATA */
struct BlockCache *extensionbuffer = NULL;
struct BlockCache *databuffer = NULL;
UWORD size, blockCapacity;
LONG writtenbytes = 0, sumoffset;
char *destination;
BOOL extensionModified = FALSE;

	D(bug("[afs]   writeData: offset=%ld\n", ah->current.offset));
	extensionbuffer = getBlock(afsbase, ah->volume, ah->current.block);
	if (extensionbuffer == NULL)
	{
		error = ERROR_UNKNOWN;
		return ENDSTREAMCH;
	}
	extensionbuffer->flags |=BCF_USED;	/* don't overwrite that cache block! */
	while (length != 0)
	{
		/* save last data block for OFS data */
		if (
				(ah->current.byte==0) &&                         /* last block fully written */
				(ah->current.filekey!=BLK_TABLE_END(ah->volume)) /* this is not the first block of the file */
			)
		{
			lastblock = OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey+1]);
			D(bug
				("[afs]   writeData: for OFS last datablock was %lu\n", lastblock));
		}
		/*
			block, filekey always point to the last block
			so update them if we have read a whole block
		*/
		/* read next extension block? */
		if (ah->current.filekey < BLK_TABLE_START)
		{
			extensionModified = FALSE;
			if (extensionbuffer->buffer[BLK_EXTENSION(ah->volume)] != 0)
			{
				block = OS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(ah->volume)]);
				extensionbuffer->flags &= ~BCF_USED;
				extensionbuffer = getBlock(afsbase, ah->volume, block);
				if (extensionbuffer == NULL)
				{
					error = ERROR_UNKNOWN;
					return ENDSTREAMCH; // was   writtenbytes;
				}
			}
			else
			{
				D(bug("[afs]   writeData: need new extensionblock\n"));
				block = allocBlock(afsbase, ah->volume);
				writeExtensionBlock
					(
						afsbase,
						ah->volume,
						extensionbuffer,
						ah->current.filekey+1,
						block
					);
				extensionbuffer->flags &= ~BCF_USED;
				if (block == 0)
				{
					error = ERROR_NO_FREE_STORE;
					return ENDSTREAMCH; /* was   writtenbytes; */
				}
				extensionbuffer = getFreeCacheBlock(afsbase, ah->volume,block);
				if (extensionbuffer == NULL)
				{
					error = ERROR_UNKNOWN;
					return ENDSTREAMCH; /* was   writtenbytes; */
				}
				newFileExtensionBlock(ah->volume,extensionbuffer, ah->header_block);
			}
			ah->current.filekey = BLK_TABLE_END(ah->volume);
			extensionbuffer->flags |= BCF_USED;	/* don't overwrite this cache block */
			ah->current.block = block;
		}
		/* find a block to write data into */
		if (extensionbuffer->buffer[ah->current.filekey] != 0) /* do we already have that block? */
		{
			D(bug
				(
					"[afs]   writeData: using old datablock %lu\n",
					OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
				);
			/* Only get the block's old contents if some of it won't be overwritten
			   (except for OFS or a final, partially-used block) */
			block = OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]);
			if ((length >= BLOCK_SIZE(ah->volume)) && (ah->volume->dosflags != 0))
				databuffer = getFreeCacheBlock(afsbase, ah->volume, block);
			else
				databuffer = getBlock(afsbase, ah->volume, block);
			if (databuffer == NULL)
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
				error = ERROR_UNKNOWN;
				return ENDSTREAMCH; //was   writtenbytes;
			}
		}
		else
		{
			extensionModified = TRUE;
			D(bug("[afs]   writeData: need a new datablock\n"));
			block=allocBlock(afsbase, ah->volume);
			if (block == 0)
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
				error = ERROR_NO_FREE_STORE;
				return ENDSTREAMCH; //was   writtenbytes;
			}
			extensionbuffer->buffer[ah->current.filekey] = OS_LONG2BE(block);
			if ((ah->volume->dosflags==0) && (lastblock != 0))
			{
				D(bug("[afs]   writeData: OFS->fill in %ld BLK_NEXT_DATA\n",lastblock));
				/*
					 we allocated a new block
					so there MUST be an initialized lastblock
				*/
				databuffer = getBlock(afsbase, ah->volume,lastblock);
				if (databuffer == NULL)
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
					error = ERROR_UNKNOWN;
					return ENDSTREAMCH; //was   writtenbytes;
				}
				databuffer->buffer[BLK_NEXT_DATA] = OS_LONG2BE(block);
				writeBlock(afsbase, ah->volume,databuffer, BLK_CHECKSUM);
			}
			databuffer = getFreeCacheBlock(afsbase, ah->volume,block);
			if (databuffer == NULL)
			{
				writeExtensionBlock
					(afsbase, ah->volume, extensionbuffer, ah->current.filekey, 0);
				extensionbuffer->flags &= ~BCF_USED;	//free that block
				error = ERROR_UNKNOWN;
				return ENDSTREAMCH; //was   writtenbytes;
			}
			if (ah->volume->dosflags == 0)
			{
				databuffer->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_DATA);
				databuffer->buffer[BLK_HEADER_KEY] = OS_LONG2BE(ah->header_block);
				blockCapacity = (ah->volume->SizeBlock-BLK_DATA_START)*sizeof(ULONG);
				databuffer->buffer[BLK_SEQUENCE_NUMBER] =
					OS_LONG2BE
						(((ah->current.offset+writtenbytes)/blockCapacity)+1);
				databuffer->buffer[BLK_DATA_SIZE] = 0;
				databuffer->buffer[BLK_NEXT_DATA] = 0;
			}
		}
		destination = (char *)databuffer->buffer+ah->current.byte;
		size = BLOCK_SIZE(ah->volume);
		if (ah->volume->dosflags == 0)
		{
			size -= (6*4);
			destination += (BLK_DATA_START*4);
		}
		size -= ah->current.byte;
		if (size > length)
		{
			size = length;
			ah->current.byte += size;
		}
		else
		{
			ah->current.byte = 0;
			ah->current.filekey--;
		}
		CopyMem((APTR)((char *)buffer+writtenbytes),(APTR)destination,size);
		if (ah->volume->dosflags == 0)
		{
			if (ah->current.byte == 0)
			{
				databuffer->buffer[BLK_DATA_SIZE] =
					OS_LONG2BE(BLOCK_SIZE(ah->volume)-(6*4));
			}
			else if (OS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]) < ah->current.byte)
			{
				databuffer->buffer[BLK_DATA_SIZE] = OS_LONG2BE(ah->current.byte);
			}
			sumoffset = BLK_CHECKSUM;
		}
		else
			sumoffset = -1;
		writeBlock(afsbase, ah->volume, databuffer, sumoffset);
		length -= size;
		writtenbytes += size;
	}
	if (extensionModified)
	{
		writeExtensionBlock
			(
				afsbase,
				ah->volume,
				extensionbuffer,
				ah->current.byte==0 ? ah->current.filekey+1 : ah->current.filekey,
				0
			);
	}
	extensionbuffer->flags &= ~BCF_USED;
	D(bug("[afs]   writeData=%ld\n", writtenbytes));
	return writtenbytes;
}

LONG writef
	(struct AFSBase *afsbase, struct AfsHandle *ah, void *buffer, ULONG length)
{
struct BlockCache *headerblock;
LONG writtenbytes;
struct DateStamp ds;

	D(bug("[afs] write(ah,buffer,%ld)\n", length));
	invalidBitmap(afsbase, ah->volume);
	writtenbytes = writeData(afsbase, ah, buffer, length);
	if (writtenbytes != ENDSTREAMCH)
	{
		ah->current.offset += writtenbytes;
		headerblock = getBlock(afsbase, ah->volume,ah->header_block);
		if (headerblock != NULL)
		{
			headerblock->buffer[BLK_FIRST_DATA] =
				headerblock->buffer[BLK_TABLE_END(ah->volume)];
			if (ah->current.offset > ah->filesize)
			{
				ah->filesize = ah->current.offset;
				headerblock->buffer[BLK_BYTE_SIZE(ah->volume)] =
					OS_LONG2BE(ah->filesize);
			}
			DateStamp(&ds);
			setHeaderDate(afsbase, ah->volume, headerblock, &ds);
		}
	}
	validBitmap(afsbase, ah->volume);
	return writtenbytes;
}

LONG seek
	(struct AFSBase* afsbase, struct AfsHandle *ah, LONG offset, LONG mode)
{
LONG old = -1;
UWORD filekey, byte;
ULONG block, extblockindex, newextblockindex;
UWORD blocksize, tablesize;
ULONG newoffset;
struct BlockCache *blockbuffer;

	D(bug("[afs] seek(%ld,%ld,%ld)\n", ah->header_block, offset, mode));
	error = ERROR_SEEK_ERROR;
	if (mode == OFFSET_BEGINNING)
	{
		newoffset = (ULONG)offset;
	}
	else if (mode == OFFSET_CURRENT)
	{
		if (offset == 0)
		{
		    error = 0;
		    return ah->current.offset;
		}
		newoffset = ah->current.offset+offset;
	}
	else if (mode == OFFSET_END)
	{
		newoffset = ah->filesize+offset;
	}
	else
		return -1;
	if (newoffset >= 0)
	{
		blocksize = BLOCK_SIZE(ah->volume);
		if (ah->volume->dosflags == 0)
			blocksize -= (BLK_DATA_START*4);
		newextblockindex = newoffset / blocksize;
		tablesize = BLK_TABLE_END(ah->volume)-BLK_TABLE_START+1; /* hashtable size */
		filekey = BLK_TABLE_END(ah->volume)-(newextblockindex % tablesize);
		newextblockindex /= tablesize; /* # of extensionblock we need */
		byte = newoffset % blocksize;

		/* Get index of current extension block */
		extblockindex = (ah->current.offset / blocksize) / tablesize;
		if (ah->current.filekey<BLK_TABLE_START)
			extblockindex--;

		/* Start at current extension block, unless we have to go back to
		   a previous extension block */
		if (newextblockindex >= extblockindex)
			block = ah->current.block;
		else
		{
			block = ah->header_block;
			extblockindex = 0;
		}

		while ((extblockindex != newextblockindex) && (block != 0))
		{
			blockbuffer = getBlock(afsbase, ah->volume, block);
			if (blockbuffer == NULL)
				return -1;
			block = OS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(ah->volume)]);
			extblockindex++;
		}
		if (block != 0)
		{
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

