/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#include <aros/macros.h>
#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>

#include "cache.h"

#include "ntfs_fs.h"
#include "ntfs_protos.h"

#include "debug.h"

LONG InitDirHandle(struct FSData *fs_data, struct DirHandle *dh, BOOL reuse)
{
    struct MFTAttr *curattr;
    UBYTE *bmp;
    UQUAD bitmap_len;
    LONG ret = 0;

    D(bug("[NTFS]: %s(%u)\n", __PRETTY_FUNCTION__, dh->ioh.mft.mftrec_no));

    /* dh may or may not be initialised when this is called. if it is, then it
     * probably has a valid cache block that we need to free, but we wouldn't
     * know. we test the superblock pointer to figure out if it's valid or
     * not */
    if (reuse && (dh->ioh.data == fs_data)) {
	D(bug("[NTFS] %s: re-using directory handle\n", __PRETTY_FUNCTION__));
        if (dh->ioh.mft.cblock != NULL) {
            Cache_FreeBlock(fs_data->cache, dh->ioh.mft.cblock);
            dh->ioh.mft.cblock = NULL;
        }
    }
    else {
        dh->ioh.data = fs_data;
        dh->ioh.mft.cblock = NULL;
    }

    RESET_DIRHANDLE(dh);

    dh->ioh.mft.data = fs_data;
    dh->ioh.first_cluster = dh->ioh.mft.mftrec_no * fs_data->mft_size;
    
    InitMFTEntry(&dh->ioh.mft, dh->ioh.mft.mftrec_no);

    INIT_MFTATTRIB(&dh->ioh.mft.attr, &dh->ioh.mft);

    while(1)
    {
	if ((curattr = FindMFTAttrib(&dh->ioh.mft.attr, AT_INDEX_ROOT)) == NULL)
	{
	    D(bug("[NTFS] %s: no $INDEX_ROOT found!\n", __PRETTY_FUNCTION__));
	    goto done;
	}

	if ((curattr->residentflag != ATTR_RESIDENT_FORM) ||
	    (curattr->attrname_length != 4) ||
	    (AROS_LE2WORD(curattr->attrname_offset) != 0x18) ||
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + 0x18))) != 0x00490024) ||
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + 0x1c))) != 0x00300033))
	{
	    continue;
	}
	curattr = (struct MFTAttr *)((IPTR)curattr + AROS_LE2WORD(curattr->data.resident.value_offset));
	if (*(UBYTE *)curattr != 0x30)	/* Not filename index */
	{
	    continue;
	}
	break;
    }

    dh->idx_root = (UBYTE *)curattr + 0x10;
    dh->idx_root += AROS_LE2WORD(*(UWORD *)(dh->idx_root));

    D(bug("[NTFS] %s: idx_root @ 0x%p\n", __PRETTY_FUNCTION__, dh->idx_root));

    dh->ioh.bitmap = NULL;
    dh->ioh.bitmap_len = 0;

    FreeMFTAttrib(&dh->ioh.mft.attr);
    INIT_MFTATTRIB(&dh->ioh.mft.attr, &dh->ioh.mft);
    while ((curattr = FindMFTAttrib(&dh->ioh.mft.attr, AT_BITMAP)) != NULL)
    {
	int ofs;

	ofs = ((UBYTE *)curattr)[0xA];
	if ((curattr->attrname_length == 4) &&
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + ofs))) == 0x00490024) &&
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + ofs + 4))) == 0x00300033)) /* "$I30" */
	{
	    int is_resident = (curattr->residentflag == ATTR_RESIDENT_FORM);

	    bitmap_len = (is_resident) ? AROS_LE2LONG(curattr->data.resident.value_length) : AROS_LE2QUAD(curattr->data.non_resident.data_size);

	    D(bug("[NTFS] %s: bitmap_len = %d\n", __PRETTY_FUNCTION__, bitmap_len));

	    if ((bmp = AllocVec(bitmap_len, MEMF_ANY)) == NULL)
		goto done;

	    if (is_resident)
	    {
		CopyMem((UBYTE *) ((IPTR)curattr + AROS_LE2WORD(curattr->data.resident.value_offset)), bmp, bitmap_len);
		dh->ioh.bitmap_len = bitmap_len;
	    }
	    else
            {
		if (ReadMFTAttribData(&dh->ioh.mft.attr, curattr, bmp, 0, bitmap_len, 0))
                {
		    D(bug("[NTFS] %s: failed to read $BITMAP\n", __PRETTY_FUNCTION__));
		    goto done;
                }
		dh->ioh.bitmap_len = AROS_LE2LONG(*((ULONG *)(curattr + 0x30)));
            }

	    dh->ioh.bitmap = (UBYTE *) bmp;
	    break;
	}
    }

    FreeMFTAttrib(&dh->ioh.mft.attr);
    INIT_MFTATTRIB(&dh->idx_attr, &dh->ioh.mft);
    curattr = MapMFTAttrib (&dh->idx_attr, &dh->ioh.mft, AT_INDEX_ALLOCATION);
    while (curattr != NULL)
    {
	if ((curattr->residentflag == ATTR_NONRESIDENT_FORM) &&
	    (curattr->attrname_length == 4) &&
	    (AROS_LE2WORD(curattr->attrname_offset) == 0x40) &&
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + 0x40))) == 0x00490024) &&
	    (AROS_LE2LONG(*((ULONG *)((IPTR)curattr + 0x44))) == 0x00300033)) /* "$I30" */
	    break;
	curattr = FindMFTAttrib(&dh->idx_attr, AT_INDEX_ALLOCATION);
    }

    if ((!curattr) && (dh->ioh.bitmap))
    {
	D(bug("[NTFS] %s: $BITMAP without $INDEX_ALLOCATION\n", __PRETTY_FUNCTION__));
	goto done;
    }

    D(bug("[NTFS] %s: initialised dir handle\n", __PRETTY_FUNCTION__));

done:
    
    return ret;
}

LONG ReleaseDirHandle(struct DirHandle *dh)
{
    D(bug("[NTFS] %s: releasing dir handle (cluster %ld)\n", __PRETTY_FUNCTION__, dh->ioh.first_cluster));

    RESET_DIRHANDLE(dh);

    if (dh->ioh.mft.buf != NULL)
    {
	FreeMem(dh->ioh.mft.buf, dh->ioh.data->mft_size << SECTORSIZE_SHIFT);
	dh->ioh.mft.buf = NULL;
    }
    dh->ioh.bitmap = NULL;
    dh->idx_root = NULL;

    return 0;
}

LONG GetDirEntry(struct DirHandle *dh, ULONG no, struct DirEntry *de)
{
    ULONG *count = &dh->cur_no;
    LONG ret = ~0;

    D(bug("[NTFS]: %s(dh @ 0x%p, de @ 0x%p, no #%ld)\n", __PRETTY_FUNCTION__, dh, de, no));

    /* setup the return object */
    de->data = dh->ioh.data;
    de->no = no;
    de->cluster = dh->ioh.first_cluster;

    if (de->key == NULL)
    {
	de->key = AllocMem(sizeof(struct Index_Key), MEMF_ANY|MEMF_CLEAR);
	dh->cur_no = -1;
	de->key->indx = dh->ioh.mft.buf;
	de->key->pos = dh->idx_root;
    }

    if (de->key->indx == dh->ioh.mft.buf)
    {
	while (de->key->pos != NULL)
	{
	    ret = ProcessFSEntry(&dh->ioh.mft, de, &count);
	    if (de->key->pos != NULL) de->key->pos += AROS_LE2WORD(*((UWORD *)(de->key->pos + 8)));
	    if (ret)
		goto done;
	}
	de->key->indx = NULL;
    }

    if (de->key->bitmap == NULL)
	de->key->bitmap = dh->ioh.bitmap;

    if (de->key->bitmap != NULL)
    {
	UQUAD i;

	if (de->key->indx == NULL)
	    de->key->indx = AllocMem(dh->ioh.data->idx_size << SECTORSIZE_SHIFT, MEMF_ANY);

	if (de->key->indx == NULL)
	    goto done;

	if (de->key->i == 0)
	{
	    de->key->v = 1;
	}

	i = de->key->i;

	for (; i < (dh->ioh.bitmap_len * 8); i++)
	{
	    de->key->i = i;

	    if (*de->key->bitmap & de->key->v)
	    {
		if ((de->key->pos) && (de->key->pos < de->key->indx + (dh->ioh.data->idx_size << SECTORSIZE_SHIFT)))
		    de->key->pos += AROS_LE2WORD(*((UWORD *)(de->key->pos + 8)));
		else
		{
		    D(bug("[NTFS] %s: key @ 0x%p [index #%u @ 0x%p - 0x%p]\n", __PRETTY_FUNCTION__, de->key, (IPTR)i, de->key->indx, de->key->indx + (dh->ioh.data->idx_size << SECTORSIZE_SHIFT)));

		    if ((ReadMFTAttrib
		      (&dh->idx_attr, de->key->indx, i * (dh->ioh.data->idx_size << SECTORSIZE_SHIFT),
		      (dh->ioh.data->idx_size << SECTORSIZE_SHIFT), 0))
		      || (PostProcessMFTRecord(dh->ioh.data, (struct MFTRecordEntry *)de->key->indx, dh->ioh.data->idx_size, "INDX")))
			goto done;
		    de->key->pos = &de->key->indx[0x18 + AROS_LE2WORD(*((UWORD *)(de->key->indx + 0x18)))];
		}
		ret = ProcessFSEntry(&dh->ioh.mft, de, &count);
		if (ret)
		    goto done;
	    }
	    de->key->v <<= 1;
	    if (de->key->v >= 0x100)
	    {
		de->key->v = 1;
		de->key->bitmap++;
	    }
	}
	D(bug("[NTFS] %s: Reached End\n", __PRETTY_FUNCTION__));
    }
    ret = ~0;
    de->key->i = 0;
    de->key->bitmap = dh->ioh.bitmap;

done:
    if (ret == 1)
    {
	ret = 0;
	if (de->entry->buf != NULL)
	{
	    FreeMem(de->entry->buf, dh->ioh.data->mft_size << SECTORSIZE_SHIFT);
	    de->entry->buf = NULL;
	}
	InitMFTEntry(de->entry, de->entry->mftrec_no);

	D(bug("[NTFS] %s: entry initialised\n", __PRETTY_FUNCTION__));
    }

    return ret ? ERROR_OBJECT_NOT_FOUND : 0;
}

LONG GetNextDirEntry(struct DirHandle *dh, struct DirEntry *de, BOOL skipsys)
{
    LONG err;

    D(bug("[NTFS] %s: looking for next entry after #%ld\n", __PRETTY_FUNCTION__, dh->cur_no));

    while ((err = GetDirEntry(dh, dh->cur_no + 1, de)) == 0) {
	if (dh->ioh.first_cluster == (FILE_ROOT * dh->ioh.data->mft_size))
	{
	    D(bug("[NTFS] %s: in ROOT dir\n", __PRETTY_FUNCTION__));
	    if (skipsys)
	    {
		if ((de->entryname[0] == '.' && de->entryname[1] == '\0'))
		{
		    D(bug("[NTFS] %s: skipping special entry '.'\n", __PRETTY_FUNCTION__));
		    continue;
		}
		else if (de->entryname[0] == '$')
		{
		    if (
			(de->entryname[1] == 'B' && de->entryname[2] == 'a' && de->entryname[3] == 'd' && de->entryname[4] == 'C' && de->entryname[5] == 'l' && de->entryname[6] == 'u' && de->entryname[7] == 's' && de->entryname[8] == '\0') ||
			// $BadClus
			(de->entryname[1] == 'A' && de->entryname[2] == 't' && de->entryname[3] == 't' && de->entryname[4] == 'r' && de->entryname[5] == 'D' && de->entryname[6] == 'e' && de->entryname[7] == 'f' && de->entryname[8] == '\0') ||
			// $AttrDef
			(de->entryname[1] == 'B' && de->entryname[2] == 'i' && de->entryname[3] == 't' && de->entryname[4] == 'm' && de->entryname[5] == 'a' && de->entryname[6] == 'p' && de->entryname[7] == '\0') ||
			// $Bitmap
			(de->entryname[1] == 'B' && de->entryname[2] == 'o' && de->entryname[3] == 'o' && de->entryname[4] == 't' && de->entryname[5] == '\0') ||
			// $Boot
			(de->entryname[1] == 'E' && de->entryname[2] == 'x' && de->entryname[3] == 't' && de->entryname[4] == 'e' && de->entryname[5] == 'n' && de->entryname[6] == 'd' && de->entryname[7] == '\0') ||
			// $Extend
			(de->entryname[1] == 'L' && de->entryname[2] == 'o' && de->entryname[3] == 'g' && de->entryname[4] == 'F' && de->entryname[5] == 'i' && de->entryname[6] == 'l' && de->entryname[7] == 'e' && de->entryname[8] == '\0') ||
			// $LogFile
			(de->entryname[1] == 'M' && de->entryname[2] == 'F' && de->entryname[3] == 'T' && de->entryname[4] == '\0') ||
			// $MFT
			(de->entryname[1] == 'M' && de->entryname[2] == 'F' && de->entryname[3] == 'T' && de->entryname[4] == 'M' && de->entryname[5] == 'i' && de->entryname[6] == 'r' && de->entryname[7] == 'r' && de->entryname[8] == '\0') ||
			// $MFTMirr
			(de->entryname[1] == 'S' && de->entryname[2] == 'e' && de->entryname[3] == 'c' && de->entryname[4] == 'u' && de->entryname[5] == 'r' && de->entryname[6] == 'e' && de->entryname[7] == '\0') ||
			// $Secure
			(de->entryname[1] == 'U' && de->entryname[2] == 'p' && de->entryname[3] == 'C' && de->entryname[4] == 'a' && de->entryname[5] == 's' && de->entryname[6] == 'e' && de->entryname[7] == '\0') ||
			// $UpCase
			(de->entryname[1] == 'V' && de->entryname[2] == 'o' && de->entryname[3] == 'l' && de->entryname[4] == 'u' && de->entryname[5] == 'm' && de->entryname[6] == 'e' && de->entryname[7] == '\0')
			// $Volume
		      )
		    {
			D(bug("[NTFS] %s: skipping special entry '%s'\n", __PRETTY_FUNCTION__, de->entryname));
			continue;
		    }
		}
	    }

	    if (de->entryname[0] == '$' && de->entryname[1] == 'R' && de->entryname[2] == 'e' && de->entryname[3] == 'c' && de->entryname[4] == 'y' && de->entryname[5] == 'c' && de->entryname[6] == 'l' && de->entryname[7] == 'e'  && de->entryname[8] == '.' && de->entryname[9] == 'B' && de->entryname[10] == 'i' && de->entryname[11] == 'n' && de->entryname[12] == '\0')
		// $Recycle.Bin
	    {
		char *fixedname = AllocVec(strlen(de->entryname), MEMF_ANY|MEMF_CLEAR);
		CopyMem(de->entryname + 1, fixedname, strlen(de->entryname) - 1);
		FreeVec(de->entryname);
		de->entryname = fixedname;
	    }
	}

        D(bug("[NTFS] %s: returning entry #%u\n", __PRETTY_FUNCTION__, dh->cur_no));

        return 0;
    }

    return err;
}

LONG GetParentDir(struct DirHandle *dh, struct DirEntry *de)
{
    LONG err = 0;
    struct NTFSMFTAttr dirattr;
    struct DirHandle parentdh;
    struct MFTAttr *attrentry;

    D(bug("[NTFS]: %s(dh @ 0x%p)\n", __PRETTY_FUNCTION__, dh));

    // if we're already at the root, then we can't go any further
    if (dh->ioh.first_cluster == (dh->ioh.data->mft_start  * dh->ioh.data->mft_size))
    {
        D(bug("[NTFS] %s: trying to go up past the root, so entry not found\n", __PRETTY_FUNCTION__));
        return ERROR_OBJECT_NOT_FOUND;
    }

    D(bug("[NTFS] %s: finding parent of directory mft #%u\n", __PRETTY_FUNCTION__, dh->ioh.mft.mftrec_no));
    
//    InitDirHandle(dh->ioh.data, dh, TRUE);
    if (dh->parent_mft == 0)
    {
	INIT_MFTATTRIB(&dirattr, &dh->ioh.mft);
	attrentry = FindMFTAttrib(&dirattr, AT_FILENAME);
	attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

	// take us up
	parentdh.ioh.mft.mftrec_no = *((UQUAD *)attrentry) & MFTREF_MASK;
    }
    else
	parentdh.ioh.mft.mftrec_no = dh->parent_mft;
    if (parentdh.ioh.mft.mftrec_no == 0x2)
	    parentdh.ioh.mft.mftrec_no = FILE_ROOT;

    de->cluster = parentdh.ioh.first_cluster = parentdh.ioh.mft.mftrec_no * glob->data->mft_size;

    D(bug("[NTFS] %s: parent_mft = %d [%d]\n", __PRETTY_FUNCTION__, (parentdh.ioh.first_cluster / glob->data->mft_size), parentdh.ioh.mft.mftrec_no));
    parentdh.ioh.mft.buf = NULL;
    InitDirHandle(dh->ioh.data, &parentdh, TRUE);
    
    if ((parentdh.ioh.mft.mftrec_no != 0x2) && (parentdh.ioh.mft.mftrec_no != FILE_ROOT))
    {
	INIT_MFTATTRIB(&dirattr, &parentdh.ioh.mft);
	attrentry = FindMFTAttrib(&dirattr, AT_FILENAME);
	attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

	// take us up
	parentdh.ioh.mft.mftrec_no = *((UQUAD *)attrentry) & MFTREF_MASK;
	if (parentdh.ioh.mft.mftrec_no == 0x2)
		parentdh.ioh.mft.mftrec_no = FILE_ROOT;
	parentdh.ioh.first_cluster = parentdh.ioh.mft.mftrec_no * glob->data->mft_size;

	D(bug("[NTFS] %s: grandparent_mft = %d [%d]\n", __PRETTY_FUNCTION__, (parentdh.ioh.first_cluster / glob->data->mft_size), parentdh.ioh.mft.mftrec_no));
	ReleaseDirHandle(&parentdh);
	InitDirHandle(dh->ioh.data, &parentdh, TRUE);
	
	err = GetDirEntryByCluster(&parentdh, de->cluster, de);
    }
    else
    {
	de->entry->mftrec_no = parentdh.ioh.mft.mftrec_no;
	de->entrytype = ATTR_DIRECTORY;
	de->no = -1;
    }
    return err;
}

LONG GetDirEntryByCluster(struct DirHandle *dh, ULONG cluster, struct DirEntry *de)
{
    LONG err = 0;

    D(bug("[NTFS] %s: looking for dir entry with first cluster %lu\n", __PRETTY_FUNCTION__, cluster));

    // start at the start
    RESET_DIRHANDLE(dh);

    // loop through the entries until we find a match
    while ((err = GetNextDirEntry(dh, de, FALSE)) == 0)
    {
        if (cluster == (de->entry->mftrec_no * glob->data->mft_size))
	{
            D(bug("[NTFS] %s: matched starting cluster at entry %ld, returning\n", __PRETTY_FUNCTION__, de->no));
            break;
        }
    }

    D(bug("[NTFS] %s: dir entry with first cluster %lu not found\n", __PRETTY_FUNCTION__, cluster));
    return err;
}

LONG GetDirEntryByName(struct DirHandle *dh, STRPTR name, ULONG namelen, struct DirEntry *de)
{
    LONG err;

    D(bug("[NTFS]: %s('", __PRETTY_FUNCTION__); RawPutChars(name, namelen) ; bug("')\n"));

    // loop through the entries until we find a match
    while ((err = GetNextDirEntry(dh, de, FALSE)) == 0)
    {
        if ((strnicmp((char *) name, de->entryname, namelen) == 0) &&
	    (strlen(de->entryname) == namelen))
	{
            D(bug("[NTFS] %s: matched name '%s' at entry %ld, returning\n", __PRETTY_FUNCTION__, de->entryname, de->no));
            break;
        }
    }

    return err;
}

LONG GetDirEntryByPath(struct DirHandle *dh, STRPTR path, ULONG pathlen, struct DirEntry *de)
{
    LONG err;
    ULONG len, i;

    D(bug("[NTFS]: %s('", __PRETTY_FUNCTION__); RawPutChars(path, pathlen); bug("')\n"));

    // if it starts with a volume specifier (or just a :), remove it and get
    // us back to the root dir
    for (i = 0; i < pathlen; i++)
    {
        if (path[i] == ':')
	{
            D(bug("[NTFS] %s: path has volume specifier, moving to the root dir\n", __PRETTY_FUNCTION__));

            pathlen -= (i+1);
            path = &path[i+1];

	    dh->ioh.mft.mftrec_no = FILE_ROOT;
	    if (dh->ioh.mft.buf)
		ReleaseDirHandle(dh);
	    InitDirHandle(dh->ioh.data, dh, TRUE);

	    /* If we were called with simply ":" as the name we will return
	       immediately after this, so we prepare a fictional direntry for
	       such a case.
	       Note that we fill only fields which are actually used in our handler */
	    de->no = -1;			/* WARNING! Dummy index */
	    de->entrytype = ATTR_DIRECTORY;

            break;
        }
    }

    D(bug("[NTFS] %s: looking for entry '", __PRETTY_FUNCTION__); RawPutChars(path, pathlen); bug("'\n"));

    // get back to the start of the dir
    RESET_DIRHANDLE(dh);

    /* each time around the loop we find one dir/file in the full path */
    while (pathlen > 0)
    {
        /* zoom forward and find the first dir separator */
        for (len = 0; (len < pathlen) && (path[len] != '/') && (path[len] != '\0'); len++);

	D(bug("[NTFS] %s: remaining path is '", __PRETTY_FUNCTION__); RawPutChars(path, pathlen);
	  bug("' (%d bytes), current chunk is '", pathlen); RawPutChars(path, (len == 0) ? 1 : len);
	  bug("' (%d bytes)\n", (len == 0) ? 1 : len));

        /* if the first character is a /, then we have to go up a level */
        if (path[0] == '/')
	{
            /* get the parent dir, and bale if we've gone past it (i.e. we are
             * the root) */
            if ((err = GetParentDir(dh, de)) != 0)
                return err;
        }
        /* otherwise, we want to search the current directory for this name */
        else {
            if ((err = GetDirEntryByName(dh, path, len, de)) != 0)
                return ERROR_OBJECT_NOT_FOUND;
	    D(bug("[NTFS] %s: #%d\n", __PRETTY_FUNCTION__, de->no));
        }

        /* move up the buffer */
        path += len;
        pathlen -= len;

        /* a / here is either the path separator or the directory we just went
         * up. either way, we have to ignore it */
        if (pathlen > 0 && path[0] == '/')
	{
            path++;
            pathlen--;
        }

        if (pathlen > 0)
	{
	    D(bug("[NTFS] %s: next part.. (%u bytes)\n", __PRETTY_FUNCTION__, pathlen));
            /* more to do, so this entry had better be a directory */
            if (!(de->entrytype & ATTR_DIRECTORY))
	    {
                D(bug("[NTFS] %s: '%.*s' is not a directory, so can't go any further\n", __PRETTY_FUNCTION__, len, path));
                return ERROR_OBJECT_WRONG_TYPE;
            }

	    if (de->key)
	    {
		if ((de->key->indx) && (de->key->indx != dh->ioh.mft.buf))
		{
		    D(bug("[NTFS] %s: freeing old key indx buffer @ 0x%p\n", __PRETTY_FUNCTION__, de->key->indx));
		    FreeMem(de->key->indx, dh->ioh.data->idx_size << SECTORSIZE_SHIFT);
		    de->key->indx = NULL;
		}
		D(bug("[NTFS] %s: freeing old key @ 0x%p\n", __PRETTY_FUNCTION__, de->key));
		FreeMem(de->key, sizeof(struct Index_Key));
		de->key = NULL;
	    }
	    dh->ioh.mft.mftrec_no = de->entry->mftrec_no;
	    ReleaseDirHandle(dh);
	    InitDirHandle(dh->ioh.data, dh, TRUE);
        }
    }

    /* nothing left, so we've found it */
    D(bug("[NTFS] %s: found the entry, returning it\n", __PRETTY_FUNCTION__));
    return 0;
}

LONG UpdateDirEntry(struct DirEntry *de)
{
//    struct DirHandle dh;
    LONG err = 0;
//    ULONG nwritten;

    D(bug("[NTFS] %s: writing dir entry %ld in dir starting at cluster %ld\n", __PRETTY_FUNCTION__, de->no, de->cluster));

#if defined(NTFS_READONLY)
    err = ERROR_DISK_WRITE_PROTECTED;
#else
    InitDirHandle(glob->data, &dh, FALSE);

//    err = WriteFileChunk(&(dh.ioh), de->pos, sizeof(struct FATDirEntry), (UBYTE *) &(de->e.entry), &nwritten);
    if (err != 0)
    {
        D(bug("[NTFS] %s: dir entry update failed\n", __PRETTY_FUNCTION__));
        ReleaseDirHandle(&dh);
        return err;
    }

    ReleaseDirHandle(&dh);

#endif    
    return err;
}

#define fs_data glob->data

LONG FillFIB (struct ExtFileLock *fl, struct FileInfoBlock *fib) {
    struct GlobalLock *gl = (fl != NULL ? fl->gl : &fs_data->info->root_lock);
    LONG result = 0;
    int len;

    D(bug("[NTFS]: %s()\n", __PRETTY_FUNCTION__));

    if (gl == &fs_data->info->root_lock) {
        D(bug("[NTFS] %s:\t\ttype: root directory\n", __PRETTY_FUNCTION__));
        fib->fib_DirEntryType = ST_ROOT;
    }
    else if ((fl->entry == NULL) || (fl->entry && fl->entry->entrytype & ATTR_DIRECTORY)) {
        D(bug("[NTFS] %s:\t\ttype: directory\n", __PRETTY_FUNCTION__));
        fib->fib_DirEntryType = ST_USERDIR;
    }
    else {
        D(bug("[NTFS] %s:\t\ttype: file\n", __PRETTY_FUNCTION__));
        fib->fib_DirEntryType = ST_FILE;
    }

    D(bug("[NTFS] %s:\t\tsize: %llu\n", __PRETTY_FUNCTION__, gl->size));

    /* Mark >32bit sizes so that software knows to use the 64bit packets to get the file size */
    if (fl->entry && fl->entry->entry)
    {
	if (fl->entry->entry->size >= 0x7FFFFFFF)
	    fib->fib_Size = 0x7FFFFFFF;
	else
	    fib->fib_Size = fl->entry->entry->size;
    }
    else
    {
	if (fl->entry)
	{
	    
	}
	else
	{
	    
	}
    }

    /* Warning: TODO - technically if the $data is resident it doesnt take any space (since its embeded in the mft record) */
    fib->fib_NumBlocks = gl->size / (fs_data->mft_size << SECTORSIZE_SHIFT);
    if (fib->fib_NumBlocks << SECTORSIZE_SHIFT != gl->size)
	fib->fib_NumBlocks += 1;

    fib->fib_EntryType = fib->fib_DirEntryType;

    if (fib->fib_DirEntryType == ST_ROOT)
        CopyMem(&fs_data->volume.create_time, &fib->fib_Date, sizeof(struct DateStamp));
    else {
	struct MFTAttr *attrentry;
	struct NTFSMFTAttr  *mftattr, dirattr;
	struct NTFSMFTEntry *mft;
	if (fl->entry)
	{
	    if (!(fl->entry->entry))
	    {
		D(bug("[NTFS] %s:\t\tNO MFT ENTRY\n", __PRETTY_FUNCTION__));
		return result;
	    }
	    mftattr = &fl->entry->entry->attr;
	    mft = fl->entry->entry;
	}
	else
	{
	    mftattr = &dirattr;
	    mft = &fl->dir->ioh.mft;
	}
	D(bug("[NTFS] %s:\t\tNTFSMFTEntry @ 0x%p, NTFSMFTAttr @ 0x%p\n", __PRETTY_FUNCTION__, mft, mftattr));

	INIT_MFTATTRIB(mftattr, mft);
	attrentry = FindMFTAttrib(mftattr, AT_STANDARD_INFORMATION);
	attrentry = (struct MFTAttr *)((IPTR)attrentry + AROS_LE2WORD(attrentry->data.resident.value_offset));

	D(bug("[NTFS] %s: nfstime     = %d\n", __PRETTY_FUNCTION__, *((UQUAD *)(attrentry + 8))));

	NTFS2DateStamp((UQUAD *)((IPTR)attrentry + 8), &fib->fib_Date);

	D(bug("[NTFS] %s:\t Date: days %ld minutes %ld ticks %ld\n", __PRETTY_FUNCTION__, fib->fib_Date.ds_Days, fib->fib_Date.ds_Minute, fib->fib_Date.ds_Tick));
    }

    len = gl->name[0] <= 106 ? gl->name[0] : 106;
    CopyMem(gl->name, fib->fib_FileName, len + 1);
    fib->fib_FileName[len + 1] = '\0';
    D(bug("[NTFS] %s:\t\tname (len %ld) %s\n", __PRETTY_FUNCTION__, len, fib->fib_FileName + 1));

    fib->fib_Protection = 0;
    if (gl->attr & ATTR_READ_ONLY) fib->fib_Protection |= (FIBF_DELETE | FIBF_WRITE);
    if (gl->attr & ATTR_ARCHIVE)   fib->fib_Protection |= FIBF_ARCHIVE;

    fib->fib_Comment[0] = 0;

    return result;
}
