/* rdb.c */

/* <project_name> -- <project_description>
 *
 * Copyright (C) 2006 - 2007
 *     Giuseppe Coviello <cjg@cruxppc.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "context.h"
#include "rdb.h"

static struct RdbPartition table[RDB_LOCATION_LIMIT];

static int checksum(struct AmigaBlock *_header)
{
	struct AmigaBlock *header;
	header = malloc(512);
	memmove(header, _header, 512);
	int32_t *block = (int32_t *) header;
	uint32_t i;
	int32_t sum = 0;
	if (header->amiga_SummedLongss > 512)
		header->amiga_SummedLongss = 512;
	for (i = 0; i < header->amiga_SummedLongss; i++)
		sum += *block++;
	free(header);
	return sum;
}

static block_dev_desc_t *get_dev(int dev)
{
	block_dev_desc_t *bdev = NULL;
	SCAN_HANDLE hnd;
	uint32_t blocksize;

	for (hnd = start_unit_scan(get_scan_list(), &blocksize);
	     hnd != NULL; hnd = next_unit_scan(hnd, &blocksize)) {
		if (hnd->ush_device.type == DEV_TYPE_HARDDISK) {
			bdev = malloc(sizeof(block_dev_desc_t));
			memmove(bdev, &hnd->ush_device,
				sizeof(block_dev_desc_t));
			break;
		}
	}

	end_unit_scan(hnd);
	end_global_scan();

	return bdev;
}

static struct RigidDiskBlock *get_rdb(block_dev_desc_t * dev_desc)
{
	int i;
	char *block_buffer = malloc(dev_desc->blksz);

	for (i = 0; i < RDB_LOCATION_LIMIT; i++) {
		unsigned res = dev_desc->block_read(dev_desc->dev, i, 1,
						    (unsigned *)block_buffer);
		if (res == 1) {
			struct RigidDiskBlock *trdb =
			    (struct RigidDiskBlock *)block_buffer;
			if (trdb->rdb_ID == IDNAME_RIGIDDISK) {
				if (checksum((struct AmigaBlock *)trdb) != 0)
					continue;
				return trdb;
			}
		}
	}
	printf("Done scanning, no RDB found\n");
	return NULL;
}

static int get_partition_info(block_dev_desc_t * dev_desc, 
			      struct RigidDiskBlock *rdb,
			      int part, disk_partition_t * info)
{
	char *block_buffer;
	struct PartitionBlock *p;
	struct AmigaPartitionGeometry *g;
	unsigned block, disk_type;

	block = rdb->rdb_PartitionList;
	block_buffer = malloc(dev_desc->blksz);
	p = NULL;
	while (block != 0xFFFFFFFF) {
		if (dev_desc->block_read(dev_desc->dev, block, 1,
					 (unsigned *)block_buffer)) {
			p = (struct PartitionBlock *)block_buffer;
			if (p->pb_ID == IDNAME_PARTITION) {
				if (checksum((struct AmigaBlock *)p) != 0)
					continue;
				if (part-- == 0)
					break;
				block = p->pb_Next;
			} else
				block = 0xFFFFFFFF;
			p = NULL;
		} else
			block = 0xFFFFFFFF;
	}

	if (p == NULL)
		return -1;

	g = (struct AmigaPartitionGeometry *)&(p->pb_Environment[0]);
	info->start = g->apg_LowCyl * g->apg_BlockPerTrack * g->apg_Surfaces;
	info->size = (g->apg_HighCyl - g->apg_LowCyl + 1)
	    * g->apg_BlockPerTrack * g->apg_Surfaces - 1;
	info->blksz = rdb->rdb_BlockBytes;
	strcpy((char *)info->name, p->pb_DriveName);

	disk_type = g->apg_DosType;

	info->type[0] = (disk_type & 0xFF000000) >> 24;
	info->type[1] = (disk_type & 0x00FF0000) >> 16;
	info->type[2] = (disk_type & 0x0000FF00) >> 8;
	info->type[3] = '\\';
	info->type[4] = (disk_type & 0x000000FF) + '0';
	info->type[5] = 0;

	return 0;
}

void RdbPartitionTable_init(void)
{
	block_dev_desc_t *dev_desc;
	struct RigidDiskBlock *rdb;
	disk_partition_t *info;
	int i;

	for(i = 0; i < RDB_LOCATION_LIMIT; i++)
		table[i].name = NULL;

	dev_desc = get_dev(0);
	if (dev_desc == NULL) {
		printf("** Block device not supported\n");
		return;
	}

	rdb = get_rdb(dev_desc);
	if(rdb == NULL)
		return;

	for(i = 0; i < RDB_LOCATION_LIMIT; i++) {
		info = malloc(sizeof(disk_partition_t));
		if (get_partition_info(dev_desc, rdb, i, info)) {
			free(info);
			continue;
		}
		table[i].name = (char *) info->name + 1;
		table[i].disk = 0;
		table[i].partition = i;
		table[i].dev_desc = dev_desc;
		table[i].info = info;
	}
}

struct RdbPartition *RdbPartitionTable_get(uint8_t disk, uint8_t partition)
{
	if(partition >= RDB_LOCATION_LIMIT || table[partition].name == NULL)
		return NULL;
	return &table[partition];
}

struct RdbPartition *RdbPartitionTable_getbyname(char *name)
{
	int i;
	for(i = 0; i < RDB_LOCATION_LIMIT && table[i].name != NULL; i++) {
		if(strcasecmp(name, table[i].name) == 0)
			return &table[i];
	}
	return NULL;
}

void RdbPartitionTable_dump(void)
{
	int i;
	for(i = 0; i < RDB_LOCATION_LIMIT && table[i].name != NULL; i++) {
		printf("%2d. %s\n", i, table[i].name);
	}
}

