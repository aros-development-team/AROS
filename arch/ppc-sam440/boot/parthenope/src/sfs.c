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
#include "sfs.h"

typedef struct {
	int (*load_file) (void *this, char *filename, void *buffer);
	void (*destroy) (void *this);
	struct RdbPartition *partition;
	struct SfsObject *root;
	uint32_t extentbnoderoot;
	uint32_t blocksize;
} sfs_boot_dev_t;

uint32_t calcchecksum(struct SfsBlockHeader *block, uint32_t blocksize)
{
	uint32_t *data = (uint32_t *) block;
	uint32_t checksum = 1;

	block->checksum = 0;

	while (blocksize > 0) {
		checksum += *data++;
		blocksize -= 4;
	}

	return -checksum;
}

static int sfs_loadsector(sfs_boot_dev_t *self, uint32_t block, void *buffer)
{
	return loadsector(self->partition->info->start
			  + (block * (self->blocksize / 512)),
			  self->partition->info->blksz, self->blocksize / 512, 
			  buffer);
}

static struct SfsObject *sfs_nextobject(struct SfsObject *o)
{
	int i;
	uint8_t *p = o->name;

	for (i = 2; i > 0; p++)
		if (*p == '\0')
			i--;
	if ((p - (uint8_t *) o) & 0x01)
		p++;

	return ((struct SfsObject *)p);
}

static struct SfsObject *sfs_findobject(sfs_boot_dev_t * self,
					struct SfsObject *cwd,
					char *dirname, uint8_t type)
{
	void *buffer;
	uint32_t next;
	struct SfsObject *o;

	buffer = malloc(self->blocksize);
	next = cwd->object.dir.firstdirblock;

	while (next != 0) {
		sfs_loadsector(self, next, buffer);
		if (!calcchecksum(SBH(buffer), self->blocksize)) {
			printf("Invalid checksum!\n");
			return NULL;
		}
		if (SBH(buffer)->id != SOC_ID)
			break;
		o = SOC(buffer)->object;
		while (o->objectnode > 0) {
			if (strcasecmp((char *)o->name, dirname) == 0
			    && (!type || o->bits & type))
				return o;
			o = sfs_nextobject(o);
		}
		next = SOC(buffer)->next;
	}
	free(buffer);
	return NULL;
}

static struct SfsObject *sfs_find(sfs_boot_dev_t * self, char *filename)
{
	char *directory;
	char *p;
	struct SfsObject *root;

	if (*filename == '/')
		filename++;

	directory = malloc(strlen(filename) + 1);
	root = self->root;
	while (root && (p = strchr(filename, '/')) != NULL) {
		memmove(directory, filename, strlen(filename) - strlen(p));
		directory[strlen(filename) - strlen(p)] = 0;
		filename += strlen(directory) + 1;
		root = sfs_findobject(self, root, directory, OTYPE_DIR);
	}

	free(directory);

	if (root == NULL)
		return NULL;

	return sfs_findobject(self, root, filename, 0);
}

static struct BNode *get_bnode(sfs_boot_dev_t * self, uint32_t key)
{
	void *buffer;
	struct BNode *bnode, *tmp;
	uint32_t next;
	uint16_t i;

	bnode = NULL;
	buffer = malloc(self->blocksize);
	next = self->extentbnoderoot;
	while (sfs_loadsector(self, next, buffer)) {
		if (!calcchecksum(SBH(buffer), self->blocksize)) {
			printf("Invalid checksum!\n");
			return NULL;
		}
		bnode = NULL;
		for (i = SBNC(buffer)->btc.nodecount - 1; i >= 0; i--) {
			tmp = (struct BNode *)(((uint8_t *) SBNC(buffer)->btc.
						bnode) +
					       i * SBNC(buffer)->btc.nodesize);
			if (tmp->key <= key) {
				bnode = tmp;
				break;
			}
		}
		if (bnode == NULL)
			return NULL;
		if (SBNC(buffer)->btc.isleaf)
			break;
		next = bnode->data;
	}
	return bnode;
}

static int sfs_loadfile(sfs_boot_dev_t * self, char *filename, void *filebuffer)
{
	struct SfsObject *o;
	void *buffer;
	uint32_t i, next, readed, size, tocopy;
	struct SfsExtentBNode *sebn;

	if ((o = sfs_find(self, filename)) == NULL) {
		printf("%s not found!\n", filename);
		return -1;
	}

	buffer = malloc(self->blocksize);

	next = o->object.file.data;
	size = o->object.file.size;
	readed = 0;
	while (next != 0 && readed < size) {
		sebn = (struct SfsExtentBNode *)get_bnode(self, next);
		for (i = 0; i < sebn->blocks; i++) {
			sfs_loadsector(self, next + i, buffer);
			tocopy = (size - readed >= self->blocksize 
				  ? self->blocksize : size - readed);
			if (filebuffer != NULL)
				memmove(filebuffer + readed, buffer, tocopy);
			readed += tocopy;
		}
		next = sebn->next;
	}
	free(buffer);
	return readed;
}

static int sfs_destroy(sfs_boot_dev_t * this)
{
	free(this);
	return 0;
}

boot_dev_t *sfs_create(struct RdbPartition *partition)
{
	sfs_boot_dev_t *boot;
	void *buffer;


	boot = malloc(sizeof(sfs_boot_dev_t));
	boot->partition = partition;

	buffer = malloc(64 * 512);
	loadsector(boot->partition->info->start, boot->partition->info->blksz, 
		   64, buffer);
	if (SBH(buffer)->id != SRB_ID) {
		printf("** Bad sfs partition or disk - %d:%d **\n",
		       boot->partition->disk, boot->partition->partition);
		free(buffer);
		free(boot);
		return NULL;
	}

	boot->extentbnoderoot = SRB(buffer)->extentbnoderoot;
	boot->blocksize = SRB(buffer)->blocksize;

	sfs_loadsector(boot, SRB(buffer)->rootobjectcontainer, buffer);

	if (SBH(buffer)->id != SOC_ID) {
		printf("** Root Object Container not found - %d:%d **\n",
		       boot->partition->disk, boot->partition->partition);
		free(buffer);
		free(boot);
		return NULL;
	}

	printf("Found sfs partition! Name: %s\n",
	       (char *)SOC(buffer)->object->name);

	boot->root = malloc(sizeof(struct SfsObject));
	memmove(boot->root, SOC(buffer)->object, sizeof(struct SfsObject));
	free(buffer);

	boot->load_file = (int (*)(void *, char *, void *))sfs_loadfile;
	boot->destroy = (void (*)(void *))sfs_destroy;

	return (boot_dev_t *) boot;
}

