/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

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
#include "validator.h"

/*
 * Get a block that must be a checksummed T_SHORT header. A bad block is
 * reported (and validation offered) exactly once here.
 * The caller does not keep the previous buffer, so it need not be preserved.
 */
AFS_SHARED struct BlockCache *getShortBlock
        (
                struct AFSBase *afsbase,
                struct Volume *volume,
                ULONG blocknum,
                SIPTR *error
        )
{
struct BlockCache *blockbuffer;
ULONG errtext;

        blockbuffer = getBlock(afsbase, volume, blocknum);
        if (blockbuffer == NULL)
        {
                *error = ERROR_UNKNOWN;
                return NULL;
        }
        if (calcChkSum(volume->SizeBlock, blockbuffer->buffer) != 0)
        {
                errtext = ERR_CHECKSUM;
                *error = ERROR_UNKNOWN;
        }
        else if (OS_BE2LONG(blockbuffer->buffer[BLK_PRIMARY_TYPE]) != T_SHORT)
        {
                errtext = ERR_BLOCKTYPE;
                *error = ERROR_OBJECT_WRONG_TYPE;
        }
        else
                return blockbuffer;
        if (showError(afsbase, errtext, blocknum))
                launchValidator(afsbase, volume);
        return NULL;
}

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
                CONST_STRPTR name,
                struct BlockCache *blockbuffer,
                ULONG *block,
                SIPTR *error
        )
{
ULONG key;

        D(bug("[afs]    getHeaderBlock: searching for block of '%s'\n",name));
        /* first the hash table slot, then along the hash chain */
        key = getHashKey(name,volume->SizeBlock-56,volume->dosflags)+BLK_TABLE_START;
        for (;;)
        {
                *block = blockbuffer->blocknum;
                if (blockbuffer->buffer[key] == 0)
                {
                        *error = ERROR_OBJECT_NOT_FOUND;
                        return NULL;
                }
                blockbuffer = getShortBlock
                        (afsbase, volume, OS_BE2LONG(blockbuffer->buffer[key]), error);
                if (blockbuffer == NULL)
                        return NULL;
                if (noCaseStrCmp(name, (char *)blockbuffer->buffer + (BLK_DIRECTORYNAME_START(volume)*4),
                             volume->dosflags, MAX_NAME_LENGTH))
                        return blockbuffer;
                key = BLK_HASHCHAIN(volume);
        }
}

/*******************************************
 Name  : findBlock
 Descr.: find the header block of a file/dir
 Input : dirah    - directory lock as starting point
                   if NULL, start in root dir
         name    - path of file/dir
         block   - will be filled with the block number
                   prior to the entry we are using; rootblock
                   if we are searching for the root
 Output: NULL=error (evtl. error=ERROR_...)
         blockcache structure of found block otherwise
********************************************/
struct BlockCache *findBlock
        (
                struct AFSBase *afsbase,
                struct AfsHandle *dirah,
                CONST_STRPTR name,
                ULONG *block,
                SIPTR *error
        )
{
struct Volume *volume = dirah->volume;
STRPTR pos;
struct BlockCache *blockbuffer;
UBYTE buffer[32];

        if ((volume->dostype != ID_DOS_DISK) && (volume->dostype != ID_DOS_muFS_DISK))
        {
                D(bug("[afs] Unknown dostype 0x%08x\n", volume->dostype));
                *error = ERROR_NOT_A_DOS_DISK;
                return 0;
        }
        *block = dirah->header_block;

        D(bug("[afs]    findBlock: startblock=%d\n",*block));
        /* get first entry (root or filelock refers to) */
        blockbuffer = getShortBlock(afsbase, volume, *block, error);
        if (blockbuffer == NULL)
                return NULL;
        while (*name)
        {
                if (*name == '/')       /* get parent entry ? */
                {
                        if (blockbuffer->buffer[BLK_PARENT(volume)] == 0)
                        {
                                *error = ERROR_OBJECT_NOT_FOUND;
                                D(bug("[afs]    object not found\n"));
                                return NULL;
                        }
                        D(bug("[afs]   findBlock: getting parent\n"));
                        blockbuffer = getBlock
                                (
                                        afsbase,
                                        volume,
                                        OS_BE2LONG(blockbuffer->buffer[BLK_PARENT(volume)])
                                );
                        if (blockbuffer == NULL)
                        {
                                *error = ERROR_UNKNOWN;
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
                                                        blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]
                                                ) != ST_ROOT) &&
                                        (OS_BE2LONG
                                                (
                                                        blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]
                                                ) != ST_USERDIR) &&
                                        (OS_BE2LONG
                                                (
                                                        blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]
                                                ) != ST_LINKDIR))
                        {
                                *error = ERROR_OBJECT_WRONG_TYPE;
                                D(bug("[afs]    error wrong type\n"));
                                return NULL;
                        }
                        pos = buffer;
                        while ((*name != 0) && (*name != '/') && ((UBYTE *)pos < buffer + sizeof(buffer) - 1))
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
                                getHeaderBlock(afsbase, volume, buffer, blockbuffer, block, error);
                        if (blockbuffer == NULL)
                                break;          /* object not found or other error */
                }
        }
        D(
                if (blockbuffer != NULL)
                        bug("[afs]   findBlock: block=%d\n",blockbuffer->blocknum);
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
void remHandle(struct AFSBase *afsbase, struct AfsHandle *ah) {
struct AfsHandle *old = NULL;

        D(bug("[afs 0x%p] Removing handle 0x%p\n", ah->volume, ah));
        if (ah->volumenode == NULL || ah->volume->volumenode == ah->volumenode) {
            D(bug("[afs 0x%p] Lock's volume is online\n", ah->volume));
            if (ah->volume->locklist==ah)
                ah->volume->locklist=ah->next;
            else
                old=ah->volume->locklist;
        }
#ifdef __AROS__
        else {
            D(bug("[afs 0x%08lX] Lock's volume is offline\n", ah->volume));
            if (BADDR(ah->volumenode->dol_misc.dol_volume.dol_LockList) == ah)
                if (ah->next)
                    ah->volumenode->dol_misc.dol_volume.dol_LockList = MKBADDR(ah->next);
                else {
                    D(bug("[afs 0x%08lX] Last lock removed, removing VolumeNode\n", ah->volume));
                    remDosNode(afsbase, ah->volumenode);
                }
            else
                old = BADDR(ah->volumenode->dol_misc.dol_volume.dol_LockList);
        }
#endif
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
        return NULL;
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
                ULONG *hashtable,
                SIPTR *error
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
                ah->volumenode = volume->volumenode;
                addHandle(ah);
        }
        else
                *error = ERROR_NO_FREE_STORE;
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
                ULONG mode,
                SIPTR *error
        )
{
struct AfsHandle *ah;

        D(bug
                (
                        "[afs]    getHandle: trying to get handle for block %u\n",
                        fileblock->blocknum)
                );

        *error = 0;

        ah = findHandle(volume, fileblock->blocknum);
        if (ah != NULL)
        {
                if (ah->mode == MODE_READWRITE ||
                    ah->mode == MODE_NEWFILE)
                {
                        *error = ERROR_OBJECT_IN_USE;
                        ah = NULL;
                }
        }

        if (*error == 0)
        {
                ah = allocHandle
                        (
                                afsbase,
                                volume,
                                fileblock,
                                mode,
                                (ULONG *)((char *)fileblock->buffer+(BLK_TABLE_START*4)),
                                error
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
                CONST_STRPTR filename,
                ULONG mode,
                SIPTR *error
        )
{
struct AfsHandle *ah = NULL;
struct BlockCache *fileblock;
ULONG block;

        D(bug("[afs] openf(%d,%s,0x%8x)\n",dirah->header_block,filename,mode));
        fileblock = findBlock(afsbase, dirah, filename, &block, error);
        if (fileblock != NULL)
                ah = getHandle(afsbase, dirah->volume, fileblock, mode, error);
        return ah;
}

/*****************************************
 Name  : openfile
 Descr.: open (lock) a file
 Input : dirah      - a handle filename is
                      relative to
         name   - filename to lock
         mode       - MODE_...
         protection - bits for new files
 Output: AfsHandle for success; NULL otherwise
******************************************/
struct AfsHandle *openfile
        (
                struct AFSBase *afsbase,
                struct AfsHandle *dirah,
                CONST_STRPTR name,
                ULONG mode,
                ULONG protection,
                SIPTR *error
        )
{
struct Volume *volume = dirah->volume;
struct AfsHandle *ah = NULL;
struct BlockCache *fileblock, *dirblock;
UBYTE filename[34];
ULONG block;
ULONG fileblocknum = -1;
ULONG dirblocknum = 0;

        /*
         * nicely say what's going on
         */
        D(bug("[afs] openfile(%u,%s,0x%x,%u)\n", dirah->header_block,name,mode,protection));
        *error = 0;
        
        /*
         * if user wants to delete or create a new file - make sure we can do that.
         */
        if ((mode != MODE_OLDFILE) && (0 == checkValid(afsbase, volume)))
                *error = ERROR_DISK_WRITE_PROTECTED;

        /*
         * get the directory the last component of "name" is in
         */
        dirblock = getDirBlockBuffer(afsbase, dirah, name, filename, error);

        /*
         * if no error so far and directory block is found, move on.
         */
        if (*error == 0 && dirblock != NULL)
        {
                /*
                 * only if the directory is of DIR or ROOT type
                 */
                if ((OS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(volume)]) == ST_USERDIR) ||
                        (OS_BE2LONG(dirblock->buffer[BLK_SECONDARY_TYPE(volume)]) == ST_ROOT))
                {
                        D(bug("[afs]    parent of %s is on block %u\n", name, dirblock->blocknum));

                        /*
                         * get the header block of the file to open
                         */
                        fileblock = getHeaderBlock(afsbase, volume, filename, dirblock, &block, error);

                        /*
                         * check if 'file' is really a FILE
                         */
                        if ((fileblock != NULL) && (OS_BE2LONG(fileblock->buffer[BLK_SECONDARY_TYPE(volume)])!=ST_FILE))
                        {
                                *error = ERROR_OBJECT_WRONG_TYPE;
                        }
                        else
                        {
                                /*
                                 * get file block, if there was (is) a file
                                 */
                                if (fileblock != NULL)
                                        fileblocknum = fileblock->blocknum;

                                /*
                                 * remove existing file if we are asked to clear its contents
                                 */
                                if (mode == MODE_NEWFILE)
                                {
                                        /*
                                         * deleteObject() walks the directory again and frees every
                                         * block of the old file through the bitmap; each of those
                                         * getBlock() calls may recycle the cache entry dirblock points
                                         * at. Re-fetch the directory by block number afterwards - using
                                         * the stale pointer made createNewEntry() treat whatever block
                                         * now occupied the entry (typically the deleted file's own
                                         * header) as the directory and link the new file into it.
                                         */
                                        dirblocknum = dirblock->blocknum;
                                        *error = deleteObject(afsbase, dirah, name);
                                        if ((*error == 0) || (*error == ERROR_OBJECT_NOT_FOUND))
                                        {
                                                dirblock = getBlock(afsbase, volume, dirblocknum);
                                                if (dirblock == NULL)
                                                        *error = ERROR_UNKNOWN;
                                        }
                                }

                                /*
                                 * if we cleared the file or there was no file at all, move on
                                 */
                                if ((*error == 0) || (*error == ERROR_OBJECT_NOT_FOUND))
                                {
                                        /*
                                         * in case we could not find existing file, or if we deleted this file previously,
                                         * create a new one.
                                         */
                                        if (mode == MODE_NEWFILE)
                                        {
                                                /*
                                                 * please note that dirblock may become invalid here
                                                 */
                                                fileblock = createNewEntry(afsbase, volume, ST_FILE, filename, dirblock, protection, error);
                                        }
                                        else
                                        {
                                                /*
                                                 * we should already have fileblock here
                                                 * again, dirblock may become invalid here
                                                 */
                                                if (fileblock != NULL)
                                                        fileblock = getBlock(afsbase, volume, fileblocknum);
                                        }

                                        /*
                                         * create a handle for the file
                                         */
                                        if (fileblock != NULL)
                                        {
                                                *error = 0;
                                                ah = getHandle(afsbase, volume, fileblock, mode, error);
                                        }
                                }
                        }
                }
                else
                        *error = ERROR_OBJECT_WRONG_TYPE;
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

        D(bug("[afs] closef(%u)\n",ah->header_block));
        remHandle(afsbase, ah);
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
/*
 * Read consecutive whole data blocks, starting at the current one, in one
 * request. Returns the number of blocks read, 0 if none qualified, -1 on error.
 */
static ULONG bulkRead
        (
                struct AFSBase *afsbase,
                struct AfsHandle *ah,
                struct BlockCache *extensionbuffer,
                char *dest,
                ULONG maxblocks
        )
{
struct Volume *volume = ah->volume;
ULONG key = ah->current.filekey;
ULONG start = OS_BE2LONG(extensionbuffer->buffer[key]);
ULONG count = 1;
        if (maxblocks > volume->bulkblocks)
                maxblocks = volume->bulkblocks;
        while (count < maxblocks && key >= BLK_TABLE_START + count
                && OS_BE2LONG(extensionbuffer->buffer[key - count]) == start + count)
                count++;
        count = cleanRun(volume, start, count);
        if (count == 0)
                return 0;
        if (readDisk(afsbase, volume, start, count, volume->bulkbuffer) != 0)
                return (ULONG)-1;
        CopyMem(volume->bulkbuffer, dest, count * BLOCK_SIZE(volume));
        return count;
}

LONG readData
        (
                struct AFSBase *afsbase,
                struct AfsHandle *ah,
                void *buffer,
                ULONG length,
                SIPTR *error
        )
{
struct Volume *volume = ah->volume;
struct BlockCache *extensionbuffer;
struct BlockCache *databuffer;
ULONG size;
LONG readbytes=0;
char *source;

        if (ah->current.block == 0)
                return 0;                     /* we can't read beyond EOF so return EOF */
        if (length > (ah->filesize-ah->current.offset))
        {
                length = ah->filesize-ah->current.offset; /* we can't read more bytes than left in file! */
        }
        D(bug("[afs]   readData: offset=%d\n", ah->current.offset));
        extensionbuffer = getBlock(afsbase, volume, ah->current.block);
        if (extensionbuffer == NULL)
        {
                *error = ERROR_UNKNOWN;
                return ENDSTREAMCH;
        }
        extensionbuffer->flags |= BCF_USED; /* don't overwrite that cache block! */
        while (length != 0)
        {
                D(bug("[afs]   readData: bytes left=%d\n",length));
                /*
                        block, filekey always point to the next block
                        so update them if we have read a whole block
                */
                /* do we have to read next extension block? */
                if (ah->current.filekey<BLK_TABLE_START)
                {
                        ah->current.block=
                                OS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(volume)]);
                        ah->current.filekey = BLK_TABLE_END(volume);
                        extensionbuffer->flags &= ~BCF_USED;            //we can now overwrite that cache block
                        D(bug("[afs]   readData: reading extensionblock=%d\n",ah->current.block));
                        if (ah->current.block != 0)
                        {
                                extensionbuffer = getBlock(afsbase, volume,ah->current.block);
                                if (extensionbuffer == 0)
                                {
                                        *error = ERROR_UNKNOWN;
                                        return ENDSTREAMCH; //was   readbytes;
                                }
                                extensionbuffer->flags |= BCF_USED;     //don't overwrite this cache block
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
                                "[afs]   readData: reading datablock %d\n",
                                OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
                        );
                if (volume->dosflags != 0 && ah->current.byte == 0
                        && length >= BLOCK_SIZE(volume) && volume->bulkblocks != 0)
                {
                        ULONG count = bulkRead(afsbase, ah, extensionbuffer,
                                (char *)buffer + readbytes, length / BLOCK_SIZE(volume));
                        if (count == (ULONG)-1)
                        {
                                extensionbuffer->flags &= ~BCF_USED;
                                *error = ERROR_UNKNOWN;
                                return ENDSTREAMCH;
                        }
                        if (count != 0)
                        {
                                ah->current.filekey -= count;
                                count *= BLOCK_SIZE(volume);
                                length -= count;
                                readbytes += count;
                                continue;
                        }
                }
                databuffer = getBlock
                        (
                                afsbase,
                                volume,
                                OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey])
                        );
                if (databuffer == 0)
                {
                        extensionbuffer->flags &= ~BCF_USED;    //free that block
                        *error = ERROR_UNKNOWN;
                        return ENDSTREAMCH; //was   readbytes;
                }
                source = (char *)databuffer->buffer+ah->current.byte;
                if (volume->dosflags == 0)
                {
                        size = OS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]);
                        if (size > BLOCK_SIZE(volume) - BLK_DATA_START * 4
                                || size <= ah->current.byte)
                        {
                                extensionbuffer->flags &= ~BCF_USED;
                                *error = ERROR_DISK_NOT_VALIDATED;
                                return ENDSTREAMCH;
                        }
                        source += (BLK_DATA_START*4);
                }
                else
                {
                        size = BLOCK_SIZE(volume);
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
        (struct AFSBase *afsbase, struct AfsHandle *ah, void *buffer, ULONG length, SIPTR *error)
{
LONG readbytes;

        D(bug("[afs]   read(%d,buffer,%d)\n", ah->header_block, length));
        readbytes = readData(afsbase, ah,buffer,length, error);
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
                ULONG length,
                SIPTR *error
        )
{
struct Volume *volume = ah->volume;
ULONG block = 0;
ULONG savedBitmap = 0;
ULONG lastblock = 0;    /* 0 means: don't update BLK_NEXT_DATA */
struct BlockCache *extensionbuffer = NULL;
struct BlockCache *databuffer = NULL;
struct BlockCache *previous = NULL;
UWORD size, blockCapacity;
LONG writtenbytes = 0, sumoffset;
char *destination;
BOOL extensionModified = FALSE;

        D(bug("[afs]   writeData: offset=%d\n", ah->current.offset));
        extensionbuffer = getBlock(afsbase, volume, ah->current.block);
        if (extensionbuffer == NULL)
        {
                *error = ERROR_UNKNOWN;
                return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH;
        }
        extensionbuffer->flags |=BCF_USED;      /* don't overwrite that cache block! */
        while (length != 0)
        {
                savedBitmap = 0;
                /* save last data block for OFS data */
                if (
                                (ah->current.byte==0) &&                         /* last block fully written */
                                (ah->current.filekey!=BLK_TABLE_END(volume)) /* this is not the first block of the file */
                        )
                {
                        lastblock = OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey+1]);
                        D(bug
                                ("[afs]   writeData: for OFS last datablock was %u\n", lastblock));
                }
                /*
                        block, filekey always point to the last block
                        so update them if we have read a whole block
                */
                /* read next extension block? */
                if (ah->current.filekey < BLK_TABLE_START)
                {
                        extensionModified = FALSE;
                        if (extensionbuffer->buffer[BLK_EXTENSION(volume)] != 0)
                        {
                                block = OS_BE2LONG(extensionbuffer->buffer[BLK_EXTENSION(volume)]);
                                extensionbuffer->flags &= ~BCF_USED;
                                extensionbuffer = getBlock(afsbase, volume, block);
                                if (extensionbuffer == NULL)
                                {
                                        *error = ERROR_UNKNOWN;
                                        return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; // was   writtenbytes;
                                }
                        }
                        else
                        {
                                D(bug("[afs]   writeData: need new extensionblock\n"));
                                block = allocBlock(afsbase, volume);
                                writeExtensionBlock
                                        (
                                                afsbase,
                                                volume,
                                                extensionbuffer,
                                                ah->current.filekey+1,
                                                block
                                        );
                                extensionbuffer->flags &= ~BCF_USED;
                                if (block == 0)
                                {
                                        *error = ERROR_NO_FREE_STORE;
                                        return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; /* was   writtenbytes; */
                                }
                                extensionbuffer = getFreeCacheBlock(afsbase, volume,block);
                                if (extensionbuffer == NULL)
                                {
                                        *error = ERROR_UNKNOWN;
                                        return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; /* was   writtenbytes; */
                                }
                                newFileExtensionBlock(volume,extensionbuffer, ah->header_block);
                        }
                        ah->current.filekey = BLK_TABLE_END(volume);
                        extensionbuffer->flags |= BCF_USED;     /* don't overwrite this cache block */
                        ah->current.block = block;
                }
                /* find a block to write data into */
                if (extensionbuffer->buffer[ah->current.filekey] != 0) /* do we already have that block? */
                {
                        D(bug
                                (
                                        "[afs]   writeData: using old datablock %u\n",
                                        OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]))
                                );
                        /* Only get the block's old contents if some of it won't be overwritten
                           (except for OFS or a final, partially-used block) */
                        block = OS_BE2LONG(extensionbuffer->buffer[ah->current.filekey]);
                        if ((ah->current.byte == 0) && (length >= BLOCK_SIZE(volume))
                                && (volume->dosflags != 0))
                                databuffer = getFreeCacheBlock(afsbase, volume, block);
                        else
                                databuffer = getBlock(afsbase, volume, block);
                        if (databuffer == NULL)
                        {
                                writeExtensionBlock
                                        (
                                                afsbase,
                                                volume,
                                                extensionbuffer,
                                                ah->current.filekey,
                                                0
                                        );
                                extensionbuffer->flags &= ~BCF_USED;    //free that block
                                *error = ERROR_UNKNOWN;
                                return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; //was   writtenbytes;
                        }
                }
                else
                {
                        extensionModified = TRUE;
                        D(bug("[afs]   writeData: need a new datablock\n"));
                        block=allocBlock(afsbase, volume);
                        if (block == 0)
                        {
                                writeExtensionBlock
                                        (
                                                afsbase,
                                                volume,
                                                extensionbuffer,
                                                ah->current.filekey,
                                                0
                                        );
                                extensionbuffer->flags &= ~BCF_USED;
                                *error = ERROR_NO_FREE_STORE;
                                return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; //was   writtenbytes;
                        }
                        previous = NULL;
                        if (volume->dosflags == 0 && lastblock != 0)
                        {
                                /* Keep the previous block resident while allocating its successor. */
                                previous = getBlock(afsbase, volume, lastblock);
                                if (previous == NULL)
                                {
                                        writeExtensionBlock(afsbase, volume, extensionbuffer,
                                                ah->current.filekey + 1, 0);
                                        extensionbuffer->flags &= ~BCF_USED;
                                        *error = ERROR_UNKNOWN;
                                        return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH;
                                }
                                previous->flags |= BCF_USED;
                        }
                        databuffer = getFreeCacheBlock(afsbase, volume,block);
                        if (databuffer == NULL && previous != NULL
                                && !volume->writefailed && volume->bitmapblock != NULL)
                        {
                                /* Four-buffer OFS caches have no fifth slot. The root is
                                 * already invalid: checksum and release the bitmap so
                                 * eviction can persist it and make room for the new data. */
                                savedBitmap = volume->bitmapblock->blocknum;
                                writeBlockDeferred(afsbase, volume, volume->bitmapblock, 0);
                                releaseBitmap(volume);
                                volume->bitmapblock = NULL;
                                databuffer = getFreeCacheBlock(afsbase, volume, block);
                        }
                        if (databuffer == NULL)
                        {
                                if (previous != NULL)
                                        previous->flags &= ~BCF_USED;
                                writeExtensionBlock
                                        (afsbase, volume, extensionbuffer, ah->current.filekey + 1, 0);
                                extensionbuffer->flags &= ~BCF_USED;    //free that block
                                *error = ERROR_UNKNOWN;
                                return writtenbytes != 0 ? writtenbytes : ENDSTREAMCH; //was   writtenbytes;
                        }
                        if (volume->dosflags == 0)
                        {
                                databuffer->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_DATA);
                                databuffer->buffer[BLK_HEADER_KEY] = OS_LONG2BE(ah->header_block);
                                blockCapacity = (volume->SizeBlock-BLK_DATA_START)*sizeof(ULONG);
                                databuffer->buffer[BLK_SEQUENCE_NUMBER] =
                                        OS_LONG2BE
                                                (((ah->current.offset+writtenbytes)/blockCapacity)+1);
                                databuffer->buffer[BLK_DATA_SIZE] = 0;
                                databuffer->buffer[BLK_NEXT_DATA] = 0;
                        }
                        extensionbuffer->buffer[ah->current.filekey] = OS_LONG2BE(block);
                        if (previous != NULL)
                        {
                                previous->buffer[BLK_NEXT_DATA] = OS_LONG2BE(block);
                                writeBlock(afsbase, volume,previous, BLK_CHECKSUM);
                                previous->flags &= ~BCF_USED;
                        }
                }
                destination = (char *)databuffer->buffer+ah->current.byte;
                size = BLOCK_SIZE(volume);
                if (volume->dosflags == 0)
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
                if (buffer != NULL)
                        CopyMem((APTR)((char *)buffer+writtenbytes),(APTR)destination,size);
                if (volume->dosflags == 0)
                {
                        if (ah->current.byte == 0)
                        {
                                databuffer->buffer[BLK_DATA_SIZE] =
                                        OS_LONG2BE(BLOCK_SIZE(volume)-(6*4));
                        }
                        else if (OS_BE2LONG(databuffer->buffer[BLK_DATA_SIZE]) < ah->current.byte)
                        {
                                databuffer->buffer[BLK_DATA_SIZE] = OS_LONG2BE(ah->current.byte);
                        }
                        sumoffset = BLK_CHECKSUM;
                }
                else
                        sumoffset = -1;
                if (buffer != NULL || volume->dosflags == 0)
                        writeBlock(afsbase, volume, databuffer, sumoffset);
                length -= size;
                writtenbytes += size;
                if (savedBitmap != 0 && !volume->writefailed)
                {
                        volume->bitmapblock = getBlock(afsbase, volume, savedBitmap);
                        if (volume->bitmapblock != NULL)
                                volume->bitmapblock->flags |= BCF_USED;
                        else
                        {
                                volume->writefailed = TRUE;
                                volume->state = ID_VALIDATING;
                        }
                }
                /* A failed buffer remains queued. Finish its metadata/accounting,
                 * then stop before allocating or advancing to another block. */
                if (volume->writefailed)
                        break;
        }
        if (extensionModified)
        {
                writeExtensionBlock
                        (
                                afsbase,
                                volume,
                                extensionbuffer,
                                ah->current.byte==0 ? ah->current.filekey+1 : ah->current.filekey,
                                0
                        );
        }
        extensionbuffer->flags &= ~BCF_USED;
        D(bug("[afs]   writeData=%d\n", writtenbytes));
        return writtenbytes;
}

LONG writef
        (struct AFSBase *afsbase, struct AfsHandle *ah, void *buffer, ULONG length, SIPTR *error)
{
struct Volume *volume = ah->volume;
struct BlockCache *headerblock;
LONG writtenbytes;
struct DateStamp ds;

        D(bug("[afs] write(ah,buffer,%d)\n", length));
        if (0 == checkValid(afsbase, volume))
        {
                *error = ERROR_DISK_WRITE_PROTECTED;
                return 0;
        }

        if (!invalidBitmap(afsbase, volume))
        {
                *error = ERROR_UNKNOWN;
                return 0;
        }
        writtenbytes = writeData(afsbase, ah, buffer, length, error);
        if (writtenbytes != ENDSTREAMCH)
        {
                ah->current.offset += writtenbytes;
                headerblock = getBlock(afsbase, volume,ah->header_block);
                if (headerblock != NULL)
                {
                        headerblock->buffer[BLK_FIRST_DATA] =
                                headerblock->buffer[BLK_TABLE_END(volume)];
                        if (ah->current.offset > ah->filesize)
                        {
                                ah->filesize = ah->current.offset;
                                headerblock->buffer[BLK_BYTE_SIZE(volume)] =
                                        OS_LONG2BE(ah->filesize);
                        }
                        DateStamp(&ds);
                        setHeaderDate(afsbase, volume, headerblock, &ds);
                }
                else
                {
                        *error = ERROR_UNKNOWN;
                        writtenbytes = ENDSTREAMCH;
                }
        }
        if (!validBitmap(afsbase, volume))
        {
                *error = ERROR_UNKNOWN;
                return ENDSTREAMCH;
        }
        return writtenbytes;
}

LONG seek
        (struct AFSBase* afsbase, struct AfsHandle *ah, LONG offset, LONG mode, SIPTR *error)
{
struct Volume *volume = ah->volume;
LONG old = -1;
UWORD filekey, byte;
ULONG block, extblockindex, newextblockindex;
UWORD blocksize, tablesize;
ULONG newoffset;
struct BlockCache *blockbuffer;

        D(bug("[afs] seek(%d,%d,%d)\n", ah->header_block, offset, mode));
        *error = ERROR_SEEK_ERROR;
        if (mode == OFFSET_BEGINNING)
        {
                newoffset = (ULONG)offset;
        }
        else if (mode == OFFSET_CURRENT)
        {
                if (offset == 0)
                {
                        *error = 0;
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
        if ((LONG)newoffset >= 0)
        {
                blocksize = BLOCK_SIZE(volume);
                if (volume->dosflags == 0)
                        blocksize -= (BLK_DATA_START*4);
                newextblockindex = newoffset / blocksize;
                tablesize = BLK_TABLE_END(volume)-BLK_TABLE_START+1;
                filekey = BLK_TABLE_END(volume)-(newextblockindex % tablesize);
                newextblockindex /= tablesize; /* # of extensionblock we need */
                byte = newoffset % blocksize;
                if (newoffset != 0 && byte == 0 && filekey == BLK_TABLE_END(volume))
                {
                        newextblockindex--;
                        filekey = BLK_TABLE_START - 1;
                }

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
                        blockbuffer = getBlock(afsbase, volume, block);
                        if (blockbuffer == NULL)
                                return -1;
                        block = OS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(volume)]);
                        extblockindex++;
                }
                if (block != 0)
                {
                        *error = 0;
                        old = ah->current.offset;
                        ah->current.block = block;
                        ah->current.filekey = filekey;
                        ah->current.byte = byte;
                        ah->current.offset = newoffset;
                }
        }
        return old;
}

LONG setFileSize
        (struct AFSBase* afsbase, struct AfsHandle *ah, LONG size, LONG mode, SIPTR *error)
{
struct Volume *volume = ah->volume;
LONG pos = -1, extra, savederror, newsize;
struct BlockCache *headerblock;
struct DateStamp ds;
struct AfsHandle *ah2;

        if (0 == checkValid(afsbase, volume))
        {
                *error = ERROR_DISK_WRITE_PROTECTED;
                return -1;
        }

        /* Get absolute new length */
        D(bug("[afs] setfilesize(%d,%d,%d)\n", ah->header_block, size, mode));
        *error = ERROR_SEEK_ERROR;
        if (mode == OFFSET_BEGINNING)
        {
                newsize = 0;
        }
        else if (mode == OFFSET_CURRENT)
        {
                newsize = ah->current.offset;
        }
        else if (mode == OFFSET_END)
        {
                newsize = ah->filesize;
        }
        else
                return -1;

        /* Ensure new length isn't negative */
        if (-size > newsize)
        {
                return -1;
        }
        newsize += size;

        /* Ensure there aren't any other filehandles positioned after the new
           length */
        for (ah2 = volume->locklist; ah2 != NULL; ah2 = ah2->next)
        {
                        if (ah2 != ah && ah2->header_block == ah->header_block
                                && ah2->current.offset > newsize)
                                return -1;
        }

        /* Lengthen or shorten file */
        pos = ah->current.offset;
        if (newsize > ah->filesize)
        {
                seek(afsbase, ah, 0, OFFSET_END, error);
                if (!invalidBitmap(afsbase, volume))
                {
                        *error = ERROR_UNKNOWN;
                        return -1;
                }
                extra = writeData(afsbase, ah, NULL, newsize - ah->filesize, error);
                if (!validBitmap(afsbase, volume))
                {
                        *error = ERROR_UNKNOWN;
                        return -1;
                }

                /* Revert to original size if we couldn't fully lengthen the file */
                if (extra < newsize - ah->filesize)
                {
                        savederror = *error;
                        setFileSize(afsbase, ah, ah->filesize, OFFSET_BEGINNING, error);
                        *error = savederror;
                        return -1;
                }
        }
        else
        {
                seek(afsbase, ah, newsize, OFFSET_BEGINNING, error);
                if (deleteFileRemainder(afsbase, ah) != 0)
                {
                        *error = ERROR_UNKNOWN;
                        return -1;
                }
                if (pos < newsize)
                        pos = newsize;
        }
        *error = 0;

        /* Update metadata */
        headerblock = getBlock(afsbase, volume, ah->header_block);
        if (headerblock != NULL)
        {
                ah->filesize = newsize;
                headerblock->buffer[BLK_BYTE_SIZE(volume)] =
                        OS_LONG2BE(ah->filesize);
                DateStamp(&ds);
                setHeaderDate(afsbase, volume, headerblock, &ds);
        }
        seek(afsbase, ah, pos, OFFSET_BEGINNING, error);

        return newsize;
}

