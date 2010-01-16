/* parthenope-install.c */

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../src/rdb.h"

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

static inline void calculate_checksum(struct AmigaBlock *blk)
{
        blk->amiga_ChkSum = blk->amiga_ChkSum - checksum(blk);
        return;
}

static struct AmigaBlock *read_block(FILE * dev,
				     struct AmigaBlock *blk, uint32_t block)
{
	uint32_t pos;

	pos = ftell(dev);

	fseek(dev, block * 512, SEEK_SET);

	fread(blk, 512, 1, dev);

	fseek(dev, pos, SEEK_SET);

	if (checksum(blk) != 0) {
/*		printf("parthenope-install: Bad checksum at block: %d!\n", block);*/
		return NULL;
	}

	return blk;
}

static struct RigidDiskBlock *read_rdb(FILE * dev,
				       struct RigidDiskBlock *blk,
				       uint32_t block)
{
	uint32_t pos;

	pos = ftell(dev);
	fseek(dev, block * 512, SEEK_SET);
	fread(blk, sizeof(struct RigidDiskBlock), 1, dev);
	fseek(dev, pos, SEEK_SET);

	return blk;
}

struct RigidDiskBlock *find_rdb(FILE * dev, uint32_t * rdb_block)
{
	uint32_t i;
	struct RigidDiskBlock *rdb;

	rdb = malloc(512);

	for (i = 0; i < RDB_LOCATION_LIMIT; i++) {
		if (read_block(dev, (struct AmigaBlock *)rdb, i) == NULL
		    || rdb->rdb_ID != IDNAME_RIGIDDISK)
			continue;
		if (checksum((struct AmigaBlock *)rdb) != 0) {
			printf("parthenope-install: Bad checksum at block "
			       "%d\n", i);
			continue;
		}
		*rdb_block = i;
		return read_rdb(dev, rdb, i);
	}

	free(rdb);
	return NULL;
}

int next_free_block(FILE * dev, uint32_t * block, uint32_t * free_pointer,
		    uint32_t nblocks)
{
	struct AmigaBlock *blk;

	blk = malloc(512);

	do {
		(*free_pointer)++;
		if (read_block(dev, blk, *free_pointer) == NULL)
			continue;
	} while (!IS_FREE(blk) && *free_pointer < nblocks);

	if (IS_FREE(blk)) {
		free(blk);
		*block = *free_pointer;
		return *block;
	}

	free(blk);

	return -1;
}

void write_block(FILE * dev, void *blk, uint32_t block)
{
	fseek(dev, block * 512, SEEK_SET);
	fwrite(blk, 1, 512, dev);
}

void erase_slb(FILE * dev, struct RigidDiskBlock *rdb)
{
	uint32_t block;
	struct AmigaBlock *blk;

	printf("parthenope-install: Erasing old slb ...");
	fflush(stdout);

	blk = malloc(512);
	for (block = rdb->rdb_BootBlockList;
	     block != IDNAME_FREE
	     && read_block(dev, blk, block) != NULL
	     && blk->amiga_ID == IDNAME_BOOT;
	     block = ((struct BootstrapCodeBlock *)blk)->bcb_Next) {
		blk->amiga_ID = IDNAME_FREE;
		calculate_checksum(blk);
		write_block(dev, blk, block);

	}

	printf(" done!\n");

}

void install_slb(FILE * dev, struct RigidDiskBlock *rdb, uint32_t * slb,
		 uint32_t slb_size)
{
	uint32_t offset;
	uint32_t block, next_block, free_block_pointer;
	struct BootstrapCodeBlock *blk;

	printf("parthenope-install: Writing new slb ...");
	fflush(stdout);

	free_block_pointer = 0;

	if (next_free_block(dev, &block, &free_block_pointer,
			    rdb->rdb_RDBBlocksHi - rdb->rdb_RDBBlocksLo) < 0) {
		printf("parthenope-install: Not enough free blocks!\n");
		return;
	}

	rdb->rdb_BootBlockList = block;

	blk = malloc(512);
	for (offset = 0; offset < slb_size; offset += 123) {
		if (next_free_block(dev, &next_block, &free_block_pointer,
				    rdb->rdb_RDBBlocksHi - rdb->rdb_RDBBlocksLo)
		    < 0) {

			printf("parthenope-install: Not enough free blocks!\n");

			return;

		}
		blk->bcb_ID = IDNAME_BOOT;
		blk->bcb_SummedLongs = 128;
		blk->bcb_HostID = 0;
		blk->bcb_Next = next_block;
		if (slb_size - offset <= 123) {
			blk->bcb_SummedLongs = 5 + slb_size - offset;
			blk->bcb_Next = IDNAME_FREE;
		}
		memmove(blk->bcb_LoadData, slb + offset, 123
			* sizeof(uint32_t));
		calculate_checksum((struct AmigaBlock *) blk);
		write_block(dev, blk, block);
		block = next_block;
	}

	printf(" done!\n");
}

int main(int argc, char **argv)
{
	FILE *dev, *f;
	struct RigidDiskBlock *rdb;
	uint32_t rdb_block, *slb, slb_size;
	struct stat st;

	if (argc != 2) {
		fprintf(stderr, "parthenope-install: Usage: %s "
			"/path/to/Parthenope!\n", argv[0]);
		return -1;
	}

	dev = fopen("/dev/sda", "w+");
	if (dev == NULL) {
		fprintf(stderr, "parthenope-install: Cannot open /dev/sda!\n");
		return -1;
	}

	rdb = find_rdb(dev, &rdb_block);
	if (rdb == NULL) {
		fprintf(stderr, "parthenope-install: Cannot find RDB!\n");
		fclose(dev);
		return -1;
	}

	if (lstat(argv[1], &st) < 0) {
		fprintf(stderr,
			"parthenope-install: Cannot stat %s!\n", argv[1]);
		free(rdb);
		fclose(dev);
		return -1;
	}

	f = fopen(argv[1], "r");
	if (f == NULL) {
		fprintf(stderr,
			"parthenope-install: Cannot open %s!\n", argv[1]);
		free(rdb);
		fclose(dev);
		return -1;
	}

	slb_size = st.st_size / sizeof(uint32_t);
	slb = malloc(st.st_size);

	fread(slb, 1, st.st_size, f);
	fclose(f);

	erase_slb(dev, rdb);
	install_slb(dev, rdb, slb, slb_size);
	calculate_checksum((struct AmigaBlock *) rdb);
	write_block(dev, rdb, rdb_block);
	free(rdb);
	fclose(dev);

	return 0;
}
