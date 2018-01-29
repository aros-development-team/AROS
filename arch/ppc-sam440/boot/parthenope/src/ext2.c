/* ext2.c */

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
#include "ext2.h"

typedef struct {
	int (*load_file) (void *this, char *filename, void *buffer);
	void (*destroy) (void *this);
	int discno;
	int partno;
	unsigned part_length;
} ext2_boot_dev_t;

static int load_file(ext2_boot_dev_t * this, char *filename, void *buffer)
{
	int filelen;

	if (!ext2fs_mount(this->part_length)) {
		printf("** Bad ext2 partition or disk - %d:%d **\n",
		       this->discno, this->partno);
		ext2fs_close();
		return -4;
	}

	filelen = ext2fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext2fs_close();
		return -5;
	}

	if (ext2fs_read((char *)buffer, filelen) != filelen) {
		printf("\n** Unable to read \"%s\" from %d:%d **\n",
		       filename, this->discno, this->partno);
		ext2fs_close();
		return -6;
	}

	ext2fs_close();

	return filelen;
}

static int destroy(ext2_boot_dev_t * this)
{
	free(this);
	return 0;
}

boot_dev_t *ext2_create(struct RdbPartition *partition)
{
	ext2_boot_dev_t *boot;
	unsigned part_length;

	if ((part_length = ext2fs_set_blk_dev_full(partition->dev_desc,
						   partition->info)) == 0) {
		printf("** Bad partition - %d:%d **\n", partition->disk, 
		       partition->partition);
		ext2fs_close();
		return NULL;
	}

	if (!ext2fs_mount(part_length)) {
		printf("** Bad ext2 partition or disk - %d:%d **\n",
		       partition->disk, partition->partition);
		ext2fs_close();
		return NULL;
	}

	ext2fs_close();

	boot = malloc(sizeof(ext2_boot_dev_t));
	boot->load_file = (int (*)(void *, char *, void *))load_file;
	boot->destroy = (void (*)(void *))destroy;
	boot->discno = partition->disk;
	boot->partno = partition->partition;
	boot->part_length = part_length;

	return (boot_dev_t *) boot;
}

