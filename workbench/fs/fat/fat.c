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
#include <ctype.h>

#include "fat_fs.h"
#include "fat_protos.h"

static ULONG GetFat12Entry(struct FSSuper *sb, ULONG n)
{
	ULONG offset = n + n/2;
	UWORD val = LE16(*((UWORD*)(sb->fat + offset)));

	if (n & 1)
		val >>= 4;
	else
		val &= 0x0FFF;

	return (val);
}

static ULONG GetFat16Entry(struct FSSuper *sb, ULONG n)
{
	ULONG offset = n << 1;
	return LE16(*((UWORD*)(sb->fat + offset)));
}

static ULONG GetFat32Entry(struct FSSuper *sb, ULONG n)
{
	ULONG offset = n << 2;
	ULONG entry_cache_block = offset >> sb->fat32_cachesize_bits;
	ULONG entry_cache_offset = offset & (sb->fat32_cachesize - 1);

	if (sb->fat32_cache_block != entry_cache_block)
	{
		sb->fat32_cache_block = entry_cache_block;
		FS_GetBlocks(sb->first_fat_sector + (entry_cache_block << (sb->fat32_cachesize_bits - sb->sectorsize_bits)), sb->fat, sb->fat32_cachesize >> sb->sectorsize_bits);
	}
	return LE32(*((ULONG*)(sb->fat + entry_cache_offset)));
}
 

LONG ReadFATSuper (struct FSSuper *sb )
{
	LONG err;
	UBYTE *bh;
	struct BootSector *boot;
	BOOL invalid = FALSE;

	kprintf("\tReading FAT boot block.\n");

	if ((bh = FS_AllocMem(512)) == NULL)
		return ERROR_NO_FREE_STORE;

	if ((err = DoRawRead(0, bh)) != 0)
	{
		kprintf("Can't read boot sector!\n");
		ErrorReq("Can't read boot sector on specified device! Error code %ld", &err);
		FS_FreeMem(bh);
		return err;
	}
	boot = (struct BootSector *)bh;

	kprintf("\tBoot sector:\n");

	sb->sectorsize = LE16(boot->bpb_bytes_per_sect);
	sb->sectorsize_bits = log2(sb->sectorsize);
	kprintf("\tSectorSize = %ld\n", sb->sectorsize);
	kprintf("\tSectorSize Bits = %ld\n", sb->sectorsize_bits);

	sb->clustersize = sb->sectorsize * boot->bpb_sect_per_clust;
	sb->clustersize_bits = log2(sb->clustersize);
	sb->cluster_sectors_bits = sb->clustersize_bits - sb->sectorsize_bits;

	kprintf("\tSectorsPerCluster = %ld\n", (ULONG)boot->bpb_sect_per_clust);
	kprintf("\tClusterSize = %ld\n", sb->clustersize);
	kprintf("\tClusterSize Bits = %ld\n", sb->clustersize_bits);
	kprintf("\tCluster Sectors Bits = %ld\n", sb->cluster_sectors_bits);

	sb->first_fat_sector = LE16(boot->bpb_rsvd_sect_count);
	kprintf("\tFirst FAT Sector = %ld\n", sb->first_fat_sector);

	if (boot->bpb_fat_size_16 != 0)
		sb->fat_size = LE16(boot->bpb_fat_size_16);
	else
		sb->fat_size = LE32(boot->type.fat32.bpb_fat_size_32);
	kprintf("\tFAT Size = %ld\n", sb->fat_size);

	if (boot->bpb_total_sectors_16 != 0)
		sb->total_sectors = LE16(boot->bpb_total_sectors_16);
	else
		sb->total_sectors = LE32(boot->bpb_total_sectors_32);
	kprintf("\tTotal Sectors = %ld\n", sb->total_sectors);

	sb->rootdir_sectors = ((LE16(boot->bpb_root_entries_count) * sizeof(struct DirEntry)) + (sb->sectorsize - 1)) >> sb->sectorsize_bits;
	kprintf("\tRootDir Sectors = %ld\n", sb->rootdir_sectors);

	sb->data_sectors = sb->total_sectors - (sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors);
	kprintf("\tData Sectors = %ld\n", sb->data_sectors);

	sb->clusters_count = sb->data_sectors >> sb->cluster_sectors_bits;
	kprintf("\tClusters Count = %ld\n", sb->clusters_count);

	sb->first_rootdir_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size);
	kprintf("\tFirst RootDir Sector = %ld\n", sb->first_rootdir_sector);

	sb->first_data_sector = sb->first_fat_sector + (boot->bpb_num_fats * sb->fat_size) + sb->rootdir_sectors;
	kprintf("\tFirst Data Sector = %ld\n", sb->first_data_sector);

	/* check if disk is in fact FAT filesystem */      	

	/* valid sector size: 512, 1024, 2048, 4096 */
	if (sb->sectorsize != 512 && sb->sectorsize != 1024 && sb->sectorsize != 2048 && sb->sectorsize != 4096)
		invalid = TRUE;

	/* valid bpb_sect_per_clust: 1, 2, 4, 8, 16, 32, 64, 128 */
	if ((boot->bpb_sect_per_clust & (boot->bpb_sect_per_clust - 1)) != 0 || boot->bpb_sect_per_clust == 0 || boot->bpb_sect_per_clust > 128)
		invalid = TRUE;

	/* valid cluster size: 512, 1024, 2048, 4096, 8192, 16k, 32k, 64k */
	if (sb->clustersize > 64 * 1024)
		invalid = TRUE;

	if (sb->first_fat_sector == 0)
		invalid = TRUE;

	if (boot->bpb_num_fats == 0)
		invalid = TRUE;

	if (boot->bpb_media < 0xF0)
		invalid = TRUE;

	/* FAT "signature" */
	if (bh[510] != 0x55 || bh[511] != 0xAA)
		invalid = TRUE;
 
	if (invalid)
    {
		kprintf("\tInvalid FAT Boot Sector\n");
		FS_FreeMem(bh);
		return ERROR_NOT_A_DOS_DISK;
	}
 
	if ((err = InitDevice(glob->fssm, sb->sectorsize) != 0))
		return err;
 

	if (sb->clusters_count < 4085)
	{
		kprintf("\tFAT12 filesystem detected\n");
		sb->type = 12;
		sb->eoc_mark = 0x0FF8;
		sb->func_get_fat_entry = GetFat12Entry;
	}
	else if (sb->clusters_count < 65525)
	{
		kprintf("\tFAT16 filesystem detected\n");
		sb->type = 16;
		sb->eoc_mark = 0xFFF8;
		sb->func_get_fat_entry = GetFat16Entry;
	}
    else
	{
		kprintf("\tFAT32 filesystem detected\n");
		sb->type = 32;
		sb->eoc_mark = 0x0FFFFFF8;
		sb->func_get_fat_entry = GetFat32Entry;
	}

	if (sb->type != 32) /* FAT 12/16 */
	{
		/* setup volume id */
		sb->volume_id = LE32(boot->type.fat16.bs_volid);

		/* setup FAT */
		sb->fat = FS_AllocMem(sb->fat_size * sb->sectorsize);
		FS_GetBlocks(sb->first_fat_sector, sb->fat, sb->fat_size);
		
		/* setup rootdir extent */
		sb->first_rootdir_extent->sector = sb->first_rootdir_sector;
		sb->first_rootdir_extent->count = sb->rootdir_sectors;
		sb->first_rootdir_extent->offset = 0;
		sb->first_rootdir_extent->cur_cluster = 0;
		sb->first_rootdir_extent->next_cluster = sb->eoc_mark;
		sb->first_rootdir_extent->start_cluster = 0;
	}
	else
	{
		/* setup volume id */
		sb->volume_id = LE32(boot->type.fat32.bs_volid);
 
		/* setup FAT */
		sb->fat32_cachesize = 4096;
		sb->fat32_cachesize_bits = log2(sb->fat32_cachesize);
		sb->fat = FS_AllocMem(sb->fat32_cachesize);
		sb->fat32_cache_block = 0;
		FS_GetBlocks(sb->first_fat_sector, sb->fat, sb->fat32_cachesize >> sb->sectorsize_bits);
 
		/* setup rootdir extent */
		{
			struct Extent *ext = sb->first_rootdir_extent;
			ULONG block = LE32(boot->type.fat32.bpb_root_cluster);
			ULONG prev, count=0;

			ext->start_cluster = block;
			ext->cur_cluster = block;
			ext->sector = Cluster2Sector(sb, block);
			ext->offset = 0;
			do {
				prev = block;
				block = GetFat32Entry(sb, block);
				count++;
			} while (block == prev+1);
			ext->next_cluster = block;
			ext->count = count << sb->cluster_sectors_bits;
		}
    }
	kprintf("\tRootDir extent:\n");
	kprintf("\t\tsector = %ld\n", sb->first_rootdir_extent->sector);
	kprintf("\t\tcount = %ld\n", sb->first_rootdir_extent->count);
	kprintf("\t\toffset = %ld\n", sb->first_rootdir_extent->offset);
	kprintf("\t\tcur cluster = 0x%08lx\n", sb->first_rootdir_extent->cur_cluster);
	kprintf("\t\tnext cluster = 0x%08lx\n", sb->first_rootdir_extent->next_cluster);

	if (ReadVolumeName(sb, &sb->name[1]) != 0) /* generate volume name */
    {
		LONG i;
		UBYTE *uu = (void *)&sb->volume_id;

		for (i=1; i<10;)
		{
			int d;
			if (i==5)
				sb->name[i++]='-';
			d = (*uu) & 0x0f;
			sb->name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;
			d = ((*uu) & 0xf0)>>4;
			sb->name[i++] = (d < 10) ? '0' + d : 'A' - 10 + d;
			uu++;
		}
		sb->name[i] = '\0';
	}

	sb->name[0] = strlen(&(sb->name[1]));

	FS_FreeMem(bh);

	kprintf("\tFAT Filesystem succesfully detected.\n");
	return 0;
}

LONG ReadVolumeName(struct FSSuper *sb, UBYTE *dest)
{
	struct Extent ext;
	struct DirCache dc;
	struct DirEntry *de;
	ULONG entry = 0;
	LONG err;

	kprintf("\tReading volume name\n");

	err = SetupDirCache(sb, &dc, &ext, 0);

	while (err == 0 && entry < 65536)
    {
		if ((err = GetDirCacheEntry(sb, &dc, entry, &de)) != 0)
		{
			kprintf("\tGetDirCacheEntry error\n");
			break;
		}

		kprintf("\t\tEntry %ld, attr %2lx\n", entry, (LONG)de->attr);

		if ((de->attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID | ATTR_SYSTEM | ATTR_READ_ONLY | ATTR_HIDDEN)) == ATTR_VOLUME_ID)
		{
			int i;
			kprintf("\t\tFound entry %ld\n", entry);

			dest[0] = de->name[0];
			dest[11] = '\0';

			for (i=1; i<11; i++)
				dest[i] = tolower(de->name[i]);

			for (i=10; i>1; i--)
				if (dest[i] == ' ')
					dest[i] = '\0';

			kprintf("\t\tVolume name: %s\n", (LONG)dest);
            break;
		}
		if (de->name[0] == 0)
		{
			kprintf("\tDirEntry %ld - EOD Marker found\n", entry);
			err = ERROR_OBJECT_NOT_FOUND;
			break;
		}
		entry++;
	}
	FreeDirCache(sb, &dc);
	return err;
}


void FreeFATSuper(struct FSSuper *sb)
{
	kprintf("\tRemoving Super Block from memory\n");
	FS_FreeMem(sb->fat);
	sb->fat = NULL;
}

LONG CompareFATSuper(struct FSSuper *s1, struct FSSuper *s2)
{
	LONG res;

	if ((res = memcmp(s1->name, s2->name, s1->name[0])) != 0)
		return res;

	return s1->volume_id - s2->volume_id;
}


LONG InitExtent(struct FSSuper *sb, struct Extent *ext, ULONG start_cluster)
{
	ext->sector = 0;
	ext->count = 0;
	ext->offset = 0;
	ext->cur_cluster = 0;
	ext->last_cluster = 0;
	ext->start_cluster = start_cluster;
	ext->next_cluster = start_cluster;
	return NextExtent(sb, ext);
}
 
LONG NextExtent(struct FSSuper *sb, struct Extent *ext)
{
	ULONG block = ext->next_cluster;
	ULONG count = 0;
	ULONG prev;

	if (block >= sb->eoc_mark)
		return ERROR_OBJECT_NOT_FOUND;

	do {
		prev = block;
		block = GetFatEntry(block);
		count++;
	} while (block == prev+1);

	ext->cur_cluster = ext->next_cluster;
	ext->next_cluster = block;
	ext->last_cluster = prev;
	ext->sector = Cluster2Sector(sb, ext->cur_cluster);
	ext->offset += ext->count;
	ext->count = count << sb->cluster_sectors_bits;

	kprintf("\tNextExtent result: cluster %ld, count %ld, next %ld (%08lx), sector %ld, offset %ld\n", ext->cur_cluster, ext->count, block, block, ext->sector, ext->offset);
	return 0;
}

LONG SeekExtent(struct FSSuper *sb, struct Extent *ext, ULONG dst_sector)
{
	LONG err = 0;

	kprintf("\tSeekExtent - dst sector: %ld\n", dst_sector);

	if (dst_sector < ext->offset)
	{
		kprintf("\tReading FAT chain from beggining\n");
		ext->sector = 0;
		ext->count = 0;
		ext->offset = 0;
		ext->cur_cluster = 0;
		ext->last_cluster = 0;
		ext->next_cluster = ext->start_cluster;
	}

	while (err == 0 && dst_sector >= ext->offset + ext->count)
		err = NextExtent(sb, ext);

	return err;
}
