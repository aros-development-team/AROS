/*
    Copyright (C) 1995-2026, The AROS Development Team. All rights reserved.
*/
/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      added disk validation
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected tabulation
 */

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 0

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
#include "afs_security.h"
#include "baseredef.h"
#include "validator.h"

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
ULONG *tail;

        D(bug
                (
                        "[afs] setHeaderDate: for headerblock %u\n",
                        blockbuffer->blocknum)
                );
        tail = BLK_TAILPTR(blockbuffer->buffer, volume);
        putStamp(&tail[BLKO_DAYS], ds);
        writeBlockDeferred(afsbase, volume, blockbuffer, BLK_CHECKSUM);
        blockbuffer = getBlock
                (afsbase, volume, OS_BE2LONG(tail[BLKO_PARENT]));
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
                CONST_STRPTR name,
                struct DateStamp *ds
        )
{
ULONG block;
struct BlockCache *blockbuffer;
SIPTR error;

        D(bug("[afs] setData()\n"));
        if (0 == checkValid(afsbase, ah->volume))
                return ERROR_DISK_WRITE_PROTECTED;

        blockbuffer = findBlock(afsbase, ah, name, &block, &error);
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
                CONST_STRPTR name,
                ULONG mask
        )
{
struct Volume *volume = ah->volume;
ULONG block;
struct BlockCache *blockbuffer;
SIPTR error;

        D(bug("[afs] setProtect(ah,%s,%d)\n", name, mask));
        if (0 == checkValid(afsbase, volume))
                return ERROR_DISK_WRITE_PROTECTED;
        blockbuffer = findBlock(afsbase, ah, name, &block, &error);
        if (blockbuffer == NULL)
                return error;
        blockbuffer->buffer[BLK_PROTECT(volume)] = OS_LONG2BE(mask);
        return writeHeader(afsbase, volume, blockbuffer);
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
        (struct AFSBase *afsbase, struct AfsHandle *ah, CONST_STRPTR name,
        CONST_STRPTR comment)
{
struct Volume *volume = ah->volume;
ULONG block;
struct BlockCache *blockbuffer;
SIPTR error;

        D(bug("[afs] setComment(ah,%s,%s)\n", name, comment));
        if (0 == checkValid(afsbase, volume))
                return ERROR_DISK_WRITE_PROTECTED;
        if (strlen(comment) >= MAXCOMMENTLENGTH)
                return ERROR_COMMENT_TOO_BIG;
        blockbuffer = findBlock(afsbase, ah, name, &block, &error);
        if (blockbuffer == NULL)
                return error;
        StrCpyToBstr
                (
                        comment,
                        (APTR)((char *)blockbuffer->buffer+(BLK_COMMENT_START(volume)*4)),
                        MAX_COMMENT_LENGTH
                );
        return writeHeader(afsbase, volume, blockbuffer);
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

        D(bug("[afs] unlinkBlock: unlinking %u\n", entry->blocknum));
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

/*
 * Free the data blocks listed in a header/extension block from "key"
 * downwards, then every following extension block and its data blocks.
 * *block is the pinned block to start with and, on success, the pinned
 * last block of the chain. On failure everything is released (including
 * "prior", if given, and the bitmap) and ERROR_UNKNOWN is returned.
 */
static ULONG freeFileBlocks
        (
                struct AFSBase *afsbase,
                struct Volume *volume,
                struct BlockCache **block,
                ULONG key,
                struct BlockCache *prior
        )
{
struct BlockCache *blockbuffer = *block;

        for (;;)
        {
                D(bug("[afs]   extensionblock=%u\n", blockbuffer->blocknum));
                while (key >= BLK_TABLE_START && blockbuffer->buffer[key] != 0)
                {
                        if (!markBlock(afsbase, volume,
                                OS_BE2LONG(blockbuffer->buffer[key]), -1))
                                goto mark_error;
                        key--;
                }
                if (blockbuffer->buffer[BLK_EXTENSION(volume)] == 0)
                        break;
                /* get next extensionblock */
                blockbuffer->flags &= ~BCF_USED;
                blockbuffer = getBlock
                        (
                                afsbase,
                                volume,
                                OS_BE2LONG(blockbuffer->buffer[BLK_EXTENSION(volume)])
                        );
                if (blockbuffer == NULL || calcChkSum(volume->SizeBlock, blockbuffer->buffer))
                {
                        if (prior != NULL)
                                prior->flags &= ~BCF_USED;
                        releaseBitmap(volume);
                        if (blockbuffer != NULL && showError(afsbase, ERR_CHECKSUM))
                                launchValidator(afsbase, volume);
                        return ERROR_UNKNOWN;
                }
                blockbuffer->flags |= BCF_USED;
                if (!markBlock(afsbase, volume, blockbuffer->blocknum, -1))
                        goto mark_error;
                key = BLK_TABLE_END(volume);
        }
        *block = blockbuffer;
        return 0;

mark_error:
        releaseBitmap(volume);
        blockbuffer->flags &= ~BCF_USED;
        if (prior != NULL)
                prior->flags &= ~BCF_USED;
        return ERROR_UNKNOWN;
}

/********************************************
 Name  : deleteObject
 Descr.: delete an object
 Input : ah      - filehandle name is relative to
         name    - name of object to delete
 Output: 0 for success; error code otherwise
*********************************************/
ULONG deleteObject(struct AFSBase *afsbase, struct AfsHandle *ah,
        CONST_STRPTR name)
{
struct Volume *volume = ah->volume;
ULONG lastblock,key;
struct BlockCache *blockbuffer, *priorbuffer;
SIPTR error;

        D(bug("[afs] delete(ah,%s)\n", name));
        /*
         * check disk validity *first*
         * it may turn out, that during validation, invalid entry gets deleted either way.
         */
        if (0 == checkValid(afsbase, volume))
                return ERROR_DISK_WRITE_PROTECTED;
        blockbuffer = findBlock(afsbase, ah, name, &lastblock, &error);
        if (blockbuffer == NULL)
                return error;
        if (findHandle(volume, blockbuffer->blocknum) != NULL)
                return ERROR_OBJECT_IN_USE;
        if (OS_BE2LONG(blockbuffer->buffer[BLK_PROTECT(volume)]) & FIBF_DELETE)
                return ERROR_DELETE_PROTECTED;
        /* if we try to delete a directory, check if it is empty */
        if (
                        OS_BE2LONG
                                (blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]) == ST_USERDIR
                )
        {
                for (key=BLK_TABLE_START; key<=BLK_TABLE_END(volume); key++)
                {
                        if (blockbuffer->buffer[key] != 0)
                                return ERROR_DIRECTORY_NOT_EMPTY;
                }
        }
        blockbuffer->flags |= BCF_USED;
        priorbuffer = getBlock(afsbase, volume, lastblock);
        if (priorbuffer == NULL)
        {
                blockbuffer->flags &= ~BCF_USED;
                return ERROR_UNKNOWN;
        }
        if (calcChkSum(volume->SizeBlock, priorbuffer->buffer) != 0)
        {
                blockbuffer->flags &= ~BCF_USED;
                if (showError(afsbase, ERR_CHECKSUM, priorbuffer->blocknum))
                        launchValidator(afsbase, volume);
                return ERROR_UNKNOWN;
        }
        priorbuffer->flags |= BCF_USED;
        if (!invalidBitmap(afsbase, volume))
        {
                priorbuffer->flags &= ~BCF_USED;
                blockbuffer->flags &= ~BCF_USED;
                return ERROR_UNKNOWN;
        }
        unLinkBlock(afsbase, volume, priorbuffer, blockbuffer);
        if (!writeBlock(afsbase, volume, priorbuffer, -1))
        {
                releaseBitmap(volume);
                priorbuffer->flags &= ~BCF_USED;
                blockbuffer->flags &= ~BCF_USED;
                return ERROR_UNKNOWN;
        }
        if (!markBlock(afsbase, volume, blockbuffer->blocknum, -1))
                goto delete_error;
        if (
                        OS_BE2LONG
                                (blockbuffer->buffer[BLK_SECONDARY_TYPE(volume)]) == ST_FILE
                        && freeFileBlocks(afsbase, volume, &blockbuffer,
                                BLK_TABLE_END(volume), priorbuffer) != 0
                )
                return ERROR_UNKNOWN;
        if (!validBitmap(afsbase, volume))
        {
                blockbuffer->flags &= ~BCF_USED;
                priorbuffer->flags &= ~BCF_USED;
                return ERROR_UNKNOWN;
        }
        blockbuffer->flags &= ~BCF_USED;
        priorbuffer->flags &= ~BCF_USED;
        return 0;
delete_error:
        releaseBitmap(volume);
        blockbuffer->flags &= ~BCF_USED;
        priorbuffer->flags &= ~BCF_USED;
        return ERROR_UNKNOWN;
}

/********************************************
 Name  : deleteFileRemainder
 Descr.: delete all data blocks in a file after the current position
 Input : ah      - filehandle to delete from
 Output: 0 for success; error code otherwise
*********************************************/
ULONG deleteFileRemainder(struct AFSBase *afsbase, struct AfsHandle *ah)
{
struct Volume *volume = ah->volume;
ULONG key;
struct BlockCache *blockbuffer;

        if (!invalidBitmap(afsbase, volume))
                return ERROR_UNKNOWN;

        blockbuffer = getBlock(afsbase, volume, ah->current.block);
        if (blockbuffer == NULL)
        {
                releaseBitmap(volume);
                return ERROR_UNKNOWN;
        }
        blockbuffer->flags |= BCF_USED;

        /* Start with the first block that will be wholly unused afterwards */
        key = ah->current.filekey;
        if (ah->current.byte != 0)
                key--;

        if (freeFileBlocks(afsbase, volume, &blockbuffer, key, NULL) != 0)
                return ERROR_UNKNOWN;

        if (!validBitmap(afsbase, volume))
        {
                blockbuffer->flags &= ~BCF_USED;
                return ERROR_UNKNOWN;
        }
        blockbuffer->flags &= ~BCF_USED;
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
UBYTE buffer[volume->FNameMax + 1];
CONST_FSBSTR name;
ULONG *filetail;

        SetMem(buffer, 0, volume->FNameMax + 1);
        filetail = BLK_TAILPTR(file->buffer, volume);
        filetail[BLKO_PARENT] = OS_LONG2BE(dir->blocknum);
        D(bug("[afs] linkNewBlock: linking block %d\n", file->blocknum));
        name = (CONST_FSBSTR)&filetail[BLKO_FILENAME_START];
        StrCpyFromBstr(name, buffer, sizeof(buffer) - 1);

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
        filetail[BLKO_HASHCHAIN] = dir->buffer[key];
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
                CONST_STRPTR name,
                STRPTR entryname,
                SIPTR *error
        )
{
ULONG block,len;
CONST_STRPTR end;
UBYTE buffer[256];

        end = PathPart(name);
        if (end[0] == '/')
                end++;
        len = strlen(name)+name-end;
        /*
         * Every caller passes a 34-byte entryname, and FFS names are at most
         * MAX_NAME_LENGTH - 1 characters: a longer component, or a path
         * longer than buffer, used to overrun the handler's stack.
         */
        if ((ULONG)(end-name) >= sizeof(buffer) || len >= MAX_NAME_LENGTH)
        {
                *error = ERROR_INVALID_COMPONENT_NAME;
                return NULL;
        }
        CopyMem(name, buffer, end-name);
        buffer[end-name] = 0;
        CopyMem(end, entryname, len);   /* skip slash or colon */
        entryname[len] = 0;
        return findBlock(afsbase, ah, buffer, &block, error);
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
                CONST_STRPTR oname,
                CONST_STRPTR newname
        )
{
struct Volume *volume = dirah->volume;
struct BlockCache *lastlink = NULL,*oldfile,*existingfile,*dirblock;
ULONG block,dirblocknum,lastblock;
UBYTE newentryname[34];
SIPTR error;

        D(bug("[afs] rename(%d,%s,%s)\n", dirah->header_block, oname, newname));
        if (0 == checkValid(afsbase, volume))
                return ERROR_DISK_WRITE_PROTECTED;
        dirblock = getDirBlockBuffer(afsbase, dirah, newname, newentryname, &error);
        if (dirblock == NULL)
                return error;
        dirblocknum = dirblock->blocknum;
        D(bug("[afs]    dir is on block %d\n", dirblocknum));
        oldfile = findBlock(afsbase, dirah, oname, &lastblock, &error);
        if (oldfile == NULL)
                return error;
        oldfile->flags |= BCF_USED;
        existingfile = getHeaderBlock(afsbase, volume, newentryname,
                dirblock, &block, &error);
        if (existingfile == NULL && error != ERROR_OBJECT_NOT_FOUND)
                goto rename_exit;
        if (existingfile != NULL && existingfile->blocknum != oldfile->blocknum)
        {
                /* Release the source header too, as every other exit does. */
                dirblock->flags &= ~BCF_USED;
                error = ERROR_OBJECT_EXISTS;
                goto rename_exit;
        }
        /* every failure from here on is ERROR_UNKNOWN unless said otherwise */
        error = ERROR_UNKNOWN;
        /* do we move a directory? */
        if (OS_BE2LONG(oldfile->buffer[BLK_SECONDARY_TYPE(volume)])==ST_USERDIR)
        {
                /* is newdirblock olock/oname itself, or a child of it */
                if (dirblocknum == oldfile->blocknum)
                {
                        error = ERROR_OBJECT_IN_USE;
                        goto rename_exit;
                }
                dirblock = getBlock(afsbase, volume, dirblocknum);
                if (dirblock == NULL)
                        goto rename_exit;
                while ((block = OS_BE2LONG(dirblock->buffer[BLK_PARENT(volume)])))
                {
                        if (block == oldfile->blocknum)
                        {
                                error = ERROR_OBJECT_IN_USE;
                                goto rename_exit;
                        }
                        dirblock = getBlock(afsbase, volume, block);
                        if (dirblock == NULL)
                                goto rename_exit;
                }
        }
        lastlink = getBlock(afsbase, volume, lastblock);
        if (lastlink == NULL)
                goto rename_exit;
        lastlink->flags |= BCF_USED;
        unLinkBlock(afsbase, volume, lastlink, oldfile);
        /* rename in same dir ? */
        if (lastlink->blocknum == dirblocknum)
        {
                /* use same buffers! */
                dirblock = lastlink;
        }
        /* otherwise we use different blocks */
        else
        {
                dirblock = getBlock(afsbase, volume, dirblocknum);
                if (dirblock == NULL)
                        goto rename_exit;
        }
        if (
                        (
                                OS_BE2LONG
                                        (
                                                dirblock->buffer[BLK_SECONDARY_TYPE(volume)]
                                        ) != ST_USERDIR
                        ) &&
                        (
                                OS_BE2LONG
                                        (
                                                dirblock->buffer[BLK_SECONDARY_TYPE(volume)]
                                        ) != ST_ROOT
                        )
                )
        {
                error = ERROR_OBJECT_WRONG_TYPE;
                goto rename_exit;
        }
        StrCpyToBstr
                (
                        newentryname,
                        (APTR)((char *)oldfile->buffer+(BLK_FILENAME_START(volume)*4)),
                        MAX_NAME_LENGTH
                );
        dirblock = linkNewBlock(afsbase, volume, dirblock, oldfile);
        if (dirblock == NULL)
                goto rename_exit;
        /* concurrent access if newdir=rootblock! */
        dirblock->flags |= BCF_USED;
        /*
                mark it as used, so that this buffer isn't
                used in invalidating volume!
        */
        if (!setBitmapFlag(afsbase, volume, 0))
        {
                dirblock->flags &= ~BCF_USED;
                goto rename_exit;
        }
        /* now we can release that buffer */
        dirblock->flags &= ~BCF_USED;
        /*
                syscrash after this: 2 dirs pointing to the same
                dir and one wrong linked entry->recoverable
        */
        if (!writeBlock(afsbase, volume, dirblock, -1))
                goto rename_error;
        /*
                syscrash after this: directory pointing is now
                correct but one wrong linked entry->recoverable
        */
        if (!writeBlock(afsbase, volume, lastlink, -1)
                || !writeBlock(afsbase, volume, oldfile, -1))
                goto rename_error;
        oldfile->flags &= ~BCF_USED;
        lastlink->flags &= ~BCF_USED;
        /*
                if newdir=rootblock we now write the correct
                (changed) buffer back
        */
        if (!setBitmapFlag(afsbase, volume, -1))
                return ERROR_UNKNOWN;
        return 0;

rename_error:
        /* Preserve all modified headers for an explicit retry. */
        writeBlockDeferred(afsbase, volume, dirblock, BLK_CHECKSUM);
        writeBlockDeferred(afsbase, volume, lastlink, BLK_CHECKSUM);
        writeBlockDeferred(afsbase, volume, oldfile, BLK_CHECKSUM);
rename_exit:
        oldfile->flags &= ~BCF_USED;
        if (lastlink != NULL)
                lastlink->flags &= ~BCF_USED;
        return error;
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
                CONST_STRPTR entryname,
                struct BlockCache *dirblock,
                ULONG protection,
                SIPTR *error
        )
{
struct BlockCache *newblock;
struct DateStamp ds;
ULONG *tail;
ULONG i;
LONG j;

        D(bug("[afs] createNewEntry(%d, %s)\n", dirblock->blocknum, entryname));
        dirblock->flags |= BCF_USED;
        if (getHeaderBlock(afsbase, volume, entryname, dirblock, &i, error) != NULL)
        {
                dirblock->flags &= ~BCF_USED;
                *error = ERROR_OBJECT_EXISTS;
                return NULL;
        }
        if (*error != ERROR_OBJECT_NOT_FOUND)
        {
                dirblock->flags &= ~BCF_USED;
                return NULL;
        }
        *error = 0;
        if (!invalidBitmap(afsbase, volume))
        {
                dirblock->flags &= ~BCF_USED;
                *error = ERROR_UNKNOWN;
                return NULL;
        }
        i = allocBlock(afsbase, volume);
        if (i == 0)
        {
                dirblock->flags &= ~BCF_USED;
                validBitmap(afsbase, volume);
                *error = ERROR_DISK_FULL;
                return NULL;
        }
        newblock = getFreeCacheBlock(afsbase, volume, i);
        if (newblock == NULL)
        {
                dirblock->flags &= ~BCF_USED;
                validBitmap(afsbase, volume);
                *error = ERROR_UNKNOWN;
                return NULL;
        }
        newblock->flags |= BCF_USED;
        tail = BLK_TAILPTR(newblock->buffer, volume);
        newblock->buffer[BLK_PRIMARY_TYPE] = OS_LONG2BE(T_SHORT);
        for (i=BLK_BLOCK_COUNT; i<=BLK_COMMENT_END(volume); i++)
                newblock->buffer[i] = 0;
        tail[BLKO_PROTECT] = OS_LONG2BE(protection);
        /* multi-user: new objects belong to the creator */
        tail[BLKO_OWNER] = OS_LONG2BE(afsSecNewOwner(afsbase));
        DateStamp(&ds);
        putStamp(&tail[BLKO_DAYS], &ds);
        StrCpyToBstr
                (
                        entryname,
                        (APTR)&tail[BLKO_FILENAME_START],
                        MAX_NAME_LENGTH
                );
        for (j=BLKO_FILENAME_END+1; j<BLKO_HASHCHAIN; j++)
                tail[j] = 0;
        tail[BLKO_PARENT] = OS_LONG2BE(dirblock->blocknum);
        tail[BLKO_EXTENSION] = 0;
        tail[BLKO_SECONDARY_TYPE] = OS_LONG2BE(entrytype);
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
        if (!writeBlock(afsbase, volume, newblock, -1)
                || !writeBlock(afsbase, volume, dirblock, -1))
        {
                writeBlockDeferred(afsbase, volume, dirblock, BLK_CHECKSUM);
                newblock->flags &= ~BCF_USED;
                validBitmap(afsbase, volume); /* Release bitmap while retaining invalid flag. */
                *error = ERROR_UNKNOWN;
                return NULL;
        }
        if (!validBitmap(afsbase, volume))
        {
                newblock->flags &= ~BCF_USED;
                *error = ERROR_UNKNOWN;
                return NULL;
        }
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
         NULL otherwise (global error set)
*********************************************/
struct AfsHandle *createDir
        (
                struct AFSBase *afsbase,
                struct AfsHandle *dirah,
                CONST_STRPTR filename,
                ULONG protection,
                SIPTR *error
        )
{
struct AfsHandle *ah = NULL;
struct BlockCache *dirblock;
char dirname[34];

        D(bug("[afs] createDir(ah,%s,%d)\n", filename, protection));
        if (0 == checkValid(afsbase, dirah->volume))
        {
                *error = ERROR_DISK_WRITE_PROTECTED;
                return NULL;
        }
        dirblock = getDirBlockBuffer(afsbase, dirah, filename, dirname, error);
        if (dirblock != NULL)
        {
                D(bug("[afs]    dir is on block %d\n", dirblock->blocknum));
                dirblock = createNewEntry
                        (afsbase, dirah->volume, ST_USERDIR, dirname, dirblock, protection, error);
                if (dirblock != NULL)
                        ah = getHandle(afsbase, dirah->volume, dirblock, MODE_OLDFILE, error);
        }
        return ah;
}

