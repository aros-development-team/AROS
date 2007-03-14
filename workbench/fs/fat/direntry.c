/*
 * fat.handler - FAT12/16/32 filesystem handler
 *
 * Copyright © 2006 Marek Szyprowski
 * Copyright © 2007 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id$
 */

#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>

#include <proto/exec.h>
#include <proto/dos.h>

#include <string.h>   

#include "fat_fs.h"
#include "fat_protos.h"

LONG SetupDirCache(struct FSSuper *sb, struct DirCache *dc, struct Extent *ext, ULONG cluster)
{
	kprintf("\tSetupDirCache: cluster %ld\n", cluster);
        /* XXX bug? can be NULL at this point:
         *   handler
         *   ProcessDiskChange
         *   DoDiskInsert
         *   ReadFATSuper
         *   ReadVolumeName
         *   SetupDirCache
        if ((dc->buffer = FS_AllocBlock()) == NULL) */
        if ((dc->buffer = FS_AllocMem(sb->sectorsize)) == NULL)
		return ERROR_NO_FREE_STORE;
	dc->e = ext;
	if (cluster)
		InitExtent(sb, dc->e, cluster);
	else
		memcpy(dc->e, sb->first_rootdir_extent, sizeof(struct Extent));

	dc->cur_sector = 0;
	return FS_GetBlock(dc->e->sector, dc->buffer);
}

LONG GetDirCacheEntry(struct FSSuper *sb, struct DirCache *dc, LONG entry, struct DirEntry **de)
{
	LONG err = 0;
	ULONG entry_sector = (entry * sizeof(struct DirEntry)) >> sb->sectorsize_bits;
	ULONG entry_offset = (entry * sizeof(struct DirEntry)) & (sb->sectorsize - 1);

#ifdef __DEBUG_ENTRIES__  
	kprintf("\tGetDirCacheEntry: entry %ld sector %ld offset %ld\n", entry, entry_sector, entry_offset);
#endif

	if (entry_sector < dc->e->offset || entry_sector >= dc->e->offset + dc->e->count)
		err = SeekExtent(sb, dc->e, entry_sector);

	if (err == 0 && dc->cur_sector != entry_sector)
	{
		kprintf("\t\tchanging sector in directory cache from %ld to %ld\n", dc->cur_sector, entry_sector);
		err = FS_GetBlock(dc->e->sector + (entry_sector - dc->e->offset), dc->buffer);
		dc->cur_sector = entry_sector;
	}
	if (err == 0)
		*de = dc->buffer + entry_offset;

	return err;
}

LONG FreeDirCache(struct FSSuper *sb, struct DirCache *dc)
{
	kprintf("\tFreeDirCache\n");
	FS_FreeBlock(dc->buffer);
	return 0;
}
						
static const UWORD mdays[] = { 0, 31, 59, 90, 120, 151, 181, 212, 143, 273, 304, 334 };

void ConvertDate(UWORD date, UWORD time, struct DateStamp *ds) {
    UBYTE year, month, day, hours, mins, secs;
    UBYTE nleap;

    /* date bits: yyyy yyym mmmd dddd */
    year = (date & 0xfe00) >> 9;    /* bits 15-9 */
    month = (date & 0x01e0) >> 5;   /* bits 8-5 */
    day = date & 0x001f;            /* bits 4-0 */

    /* time bits: hhhh hmmm mmms ssss */
    hours = (time & 0xf800) >> 11;  /* bits 15-11 */
    mins = (time & 0x07e0) >> 5;    /* bits 8-5 */
    secs = time & 0x001f;           /* bits 4-0 */

    D(bug("[fat] convert date: year %d month %d day %d hours %d mins %d secs %d\n", year, month, day, hours, mins, secs));

    /* number of leap years in before this year. note this is only dividing by
     * four, which is fine because FAT dates range 1980-2107. The only year in
     * that range that is divisible by four but not a leap year is 2100. If
     * this code is still being used then, feel free to fix it :) */
    nleap = year >> 2;
    
    /* if this year is a leap year and its March or later, adjust for this
     * year too */
    if (year & 0x03 && month >= 3)
        nleap++;

    /* calculate days since 1978-01-01 (DOS epoch):
     *   730 days in 1978+1979, getting us to the FAT epoch 1980-01-01
     *   years * 365 days
     *   leap days
     *   days in all the months before this one
     *   day of this month */
    ds->ds_Days = 730 + year * 365 + nleap + mdays[month-1] + day-1;

    /* minutes since midnight */
    ds->ds_Minute = hours * 60 + mins;

    /* 1/50 sec ticks. FAT dates are 0-29, so we have to multiply them by two
     * as well */
    ds->ds_Tick = (secs << 1) * 50;

    D(bug("[fat] convert date: days %ld minutes %ld ticks %ld\n", ds->ds_Days, ds->ds_Minute, ds->ds_Tick));
}

#define sb glob->sb

LONG FillFIB (struct ExtFileLock *fl, struct FileInfoBlock *fib)
{
        struct DirEntry *de;
	LONG result = 0;

	kprintf("\tFilling FIB data.\n");

	if ((fl->attr & (ATTR_DIRECTORY | ATTR_REALENTRY)) == (ATTR_DIRECTORY | ATTR_REALENTRY))
	{
		kprintf("\t\ttype: directory\n");
		fib->fib_DirEntryType = ST_USERDIR;
	}
    else if (fl->attr & ATTR_ROOTDIR)
	{
		kprintf("\t\ttype: root directory\n");
		fib->fib_DirEntryType = ST_ROOT;
	}
    else
	{
		kprintf("\t\ttype: file\n");
		fib->fib_DirEntryType = ST_FILE;
	}
	kprintf("\t\tsize: %ld\n", fl->size);
	fib->fib_Size = fl->size;
	fib->fib_NumBlocks = ((fl->size + (sb->clustersize - 1)) >> sb->clustersize_bits) << sb->cluster_sectors_bits;
	fib->fib_EntryType = fib->fib_DirEntryType;
	fib->fib_DiskKey = 0xfffffffflu; //fl->entry;

        if (fib->fib_DirEntryType == ST_FILE) {
            GetDirCacheEntry(sb, fl->dircache, fl->entry, &de);
            ConvertDate(de->write_date, de->write_time, &fib->fib_Date);
        }
        else {
            memset(&fib->fib_Date, 0, sizeof(struct DateStamp));
        }

	memcpy(fib->fib_FileName, fl->name, 108);

	kprintf("\t\tname (len %ld) ", fib->fib_FileName[0]); knprints(&fib->fib_FileName[1], fib->fib_FileName[0]);
	fib->fib_Protection = 0;
	fib->fib_Comment[0] = '\0';

	return result;
}

LONG ReadNextDirEntry(struct ExtFileLock *fl, struct FileInfoBlock *fib)
{
	LONG err = 0;
	struct DirEntry *de;
	ULONG entry = fib->fib_DiskKey + 1;

	if (! fl->dircache_active)
	{
		SetupDirCache(sb, fl->dircache, fl->data_ext, fl->first_cluster);
		fl->dircache_active = TRUE;
	}

	if (entry == 0 && fl->first_cluster != 0)
		entry = 2;	/* skip "." and ".." entries */

	while (entry < 65536)
    {
		if ((err = GetDirCacheEntry(sb, fl->dircache, entry, &de)) != 0)
		{
			if (err == ERROR_OBJECT_NOT_FOUND)
				err = ERROR_NO_MORE_ENTRIES;
			break;
		}
		if (de->name[0] == 0xE5 || de->attr & ATTR_VOLUME_ID)
		{
			entry++;
			continue;
		}
		if (de->name[0] == 0)
		{
			kprintf("\tDirEntry %ld - EOD Marker found\n", entry);
			err = ERROR_NO_MORE_ENTRIES;
			break;
		}

		kprintf("\tFound valid dir entry %ld. Filling FIB data.\n", entry);

		if (de->attr & ATTR_DIRECTORY)
			fib->fib_DirEntryType = ST_USERDIR;
		else
			fib->fib_DirEntryType = ST_FILE;

		fib->fib_Size = AROS_LE2LONG(de->file_size);
		fib->fib_NumBlocks = ((AROS_LE2LONG(de->file_size) + (sb->clustersize - 1)) >> sb->clustersize_bits) << sb->cluster_sectors_bits;
		fib->fib_EntryType = fib->fib_DirEntryType;
		fib->fib_DiskKey = entry;

                ConvertDate(de->write_date, de->write_time, &fib->fib_Date);

		GetShortName(de, &fib->fib_FileName[1], &fib->fib_FileName[0]);
		GetLongName(sb, fl->dircache, de, entry, &fib->fib_FileName[1], &fib->fib_FileName[0]); /* replaces short name only if long name has been found */

		fib->fib_Protection = 0;
		fib->fib_Comment[0] = '\0';
		break;
	}
	return err;
}


LONG FindEntryByName(struct DirCache *dc, STRPTR name, ULONG namelen, ULONG *result)
{
	static UBYTE namebuff[108];
	struct DirEntry *de;
	ULONG entry = 0;

	kprintf("\tFindEntryByName: namelen %ld name ", namelen); knprints(name, namelen);

	while (entry < 65536)
    {
		LONG res, err;
#ifdef __DEBUG_ENTRIES__
		kprintf("\n\tentry %ld\n", entry);
#endif
		if ((err = GetDirCacheEntry(sb, dc, entry, &de)) != 0)
		{
			kprintf("\tGetDirCacheEntry error\n");
            return err;
		}
		if (de->name[0] == 0xE5 || de->attr & ATTR_VOLUME_ID)
		{
			entry++;
			continue;
		}
		if (de->name[0] == 0)
		{
			kprintf("\tDirEntry %ld - EOD Marker found\n", entry);
			break;
		}

		GetShortName(de, &namebuff[1], &namebuff[0]);
		if (namelen == namebuff[0] && strnicmp(&namebuff[1], name, namelen) == 0)
		{
			*result = entry;
			kprintf("\tFound entry %ld\n\n", entry);
			return 0;
		}

		res = GetLongName(sb, dc, de, entry, &namebuff[1], &namebuff[0]); /* replaces short name only if long name has been found */
		if (res == 0 && namelen == namebuff[0] && strnicmp(&namebuff[1], name, namelen) == 0)
		{
			*result = entry;
			kprintf("\tFound entry %ld\n\n", entry);
			return 0;
		}
		entry++;
	} 

	return ERROR_OBJECT_NOT_FOUND;
}

LONG FindEntryByPath(ULONG start_cluster, UBYTE *path, LONG pathlen, ULONG *dst_cluster, ULONG *dst_entry)
{
	LONG err = 0;
	struct Extent ext;
	struct DirCache dc;
	struct DirEntry *de;

	while (err == 0)
	{
		ULONG len;
		kprintf("\tFindEntryByPath: start cluster %ld pathlen %ld path ", start_cluster, pathlen); knprints(path, pathlen);

		SetupDirCache(sb, &dc, &ext, start_cluster);

		for (len=0; len < pathlen && path[len] != '/'; len++)
			;

		*dst_cluster = start_cluster;

		if (pathlen && *path == '/')
		{
			kprintf("\tGetting parent directory.\n");
			if ((err = GetDirCacheEntry(sb, &dc, 1, &de)) == 0)
			{
				if (strncmp("..         ", de->name, 11) == 0)
				{
					ULONG parent_cluster = GetFirstCluster(de);

					if (parent_cluster == 0)
					{
						*dst_entry = FAT_ROOTDIR_MARK;
					}
					else
					{
						FreeDirCache(sb, &dc);
						SetupDirCache(sb, &dc, &ext, parent_cluster); /* find grandparent cluster */
						if ((err = GetDirCacheEntry(sb, &dc, 1, &de)) == 0)
						{
							if (strncmp("..         ", de->name, 11) == 0)
							{
								ULONG grandparent_cluster = GetFirstCluster(de);
								ULONG entry;
								FreeDirCache(sb, &dc);
								SetupDirCache(sb, &dc, &ext, grandparent_cluster);
								for (entry=0; entry<65535; entry++)  /* entries in rootdir starts from 0!! */
								{
									if ((err = GetDirCacheEntry(sb, &dc, entry, &de)) != 0)
										break;
									if (de->name[0] != 0xE5 && (de->attr & ATTR_VOLUME_ID) == 0 && de->attr & ATTR_DIRECTORY && GetFirstCluster(de) == parent_cluster)
										break;
								}
								*dst_entry = entry;
								*dst_cluster = grandparent_cluster;
								err = 0;
							}
							else
								err = ERROR_OBJECT_NOT_FOUND;
						}
					}
					start_cluster = parent_cluster;
				}
				else
					err = ERROR_OBJECT_NOT_FOUND;
			}
		}
		else if (pathlen && (err = FindEntryByName(&dc, path, len, dst_entry)) == 0)
		{
			if ((err = GetDirCacheEntry(sb, &dc, *dst_entry, &de)) == 0)
				start_cluster = GetFirstCluster(de);
		}

		FreeDirCache(sb, &dc);
		if (err == 0)
		{
			while (pathlen > 0)
			{
				pathlen--;
				if (*path++ == '/')
					break;
			}
			if (pathlen == 0)
				break;
		}
	}

	return err;
}
