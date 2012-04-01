/*
 * $Id$
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#define DEBUG 0

#include "context.h"
#include "device.h"
#include "uboot.h"
/* #include <debug.h> */

/* The iso9660 code from GRUB2 */
#define ISO9660_FSTYPE_DIR         0040000
#define ISO9660_FSTYPE_REG         0100000
#define ISO9660_FSTYPE_SYMLINK     0120000
#define ISO9660_FSTYPE_MASK        0170000

#define ISO9660_LOG2_BLKSZ         2
#define ISO9660_BLKSZ              2048

#define ISO9660_RR_DOT             2
#define ISO9660_RR_DOTDOT          4

#define ISO9660_MAX_DEPTH          8

/* The head of a volume descriptor.  */
struct iso9660_voldesc {
	uint8_t type;
	uint8_t magic[5];
	uint8_t version;
} __attribute__ ((packed));

/* A directory entry.  */
struct iso9660_dir {
	uint8_t len;
	uint8_t ext_sectors;
	uint32_t first_sector;
	uint32_t first_sector_be;
	uint32_t size;
	uint32_t size_be;
	uint8_t unused1[7];
	uint8_t flags;
	uint8_t unused2[6];
	uint8_t namelen;
} __attribute__ ((packed));

/* The primary volume descriptor.  Only little endian is used.  */
struct iso9660_primary_voldesc {
	struct iso9660_voldesc voldesc;
	uint8_t unused1[33];
	uint8_t volname[32];
	uint8_t unused2[60];
	uint32_t path_table_size;
	uint8_t unused3[4];
	uint32_t path_table;
	uint8_t unused4[12];
	struct iso9660_dir rootdir;
} __attribute__ ((packed));

/* A single entry in the path table.  */
struct iso9660_path {
	uint8_t len;
	uint8_t sectors;
	uint32_t first_sector;
	uint16_t parentdir;
	uint8_t name[0];
} __attribute__ ((packed));

/* An entry in the System Usage area of the directory entry.  */
struct iso9660_susp_entry {
	uint8_t sig[2];
	uint8_t len;
	uint8_t version;
	uint8_t data[0];
} __attribute__ ((packed));

/* The CE entry.  This is used to describe the next block where data
   can be found.  */
struct iso9660_susp_ce {
	struct iso9660_susp_entry entry;
	uint32_t blk;
	uint32_t blk_be;
	uint32_t off;
	uint32_t off_be;
	uint32_t len;
	uint32_t len_be;
} __attribute__ ((packed));

typedef struct cdrom_boot_dev {
	boot_dev_t dev;
	block_dev_desc_t *phys;
	context_t *ctx;
	struct iso9660_primary_voldesc voldesc;
	struct iso9660_dir rootdir;
	uint8_t tmpbuf[2048];
	uint32_t sector_in_buf;
	uint8_t rr;
	uint8_t susp_skip;
} cdrom_boot_dev_t;

static int cdrom_read(cdrom_boot_dev_t * boot, uint32_t block, uint32_t offset,
		      uint32_t length, char *dest)
{
	uint32_t len;
	uint32_t transferred = 0;

	while (offset >= 2048) {
		block++;
		offset -= 2048;
	}

	if (offset) {
		if (boot->sector_in_buf != block) {
			if (!boot->phys->
			    block_read(boot->phys->dev, block, 1, boot->tmpbuf))
				return -1;
			boot->sector_in_buf = block;
		}

		len = 2048 - offset;
		if (length < len)
			len = length;

		memcpy(dest, &boot->tmpbuf[offset], len);
		offset = 0;
		length -= len;
		dest += len;
		block++;
		transferred += len;
	}

	if (length / 2048) {
		uint32_t blocks_read = length / 2048;

		if (!boot->phys->
		    block_read(boot->phys->dev, block, blocks_read, dest))
			return -1;

		transferred += 2048 * blocks_read;
		block += blocks_read;
		dest += 2048 * blocks_read;
		length -= 2048 * blocks_read;
	}

	if (length) {
		if (boot->sector_in_buf != block) {
			if (!boot->phys->
			    block_read(boot->phys->dev, block, 1, boot->tmpbuf))
				return -1;

			boot->sector_in_buf = block;
		}

		len = 2048 - offset;
		if (length < len)
			len = length;

		memcpy(dest, boot->tmpbuf, len);
		transferred += len;
	}

	return 0;
}

static int iterate_entries(cdrom_boot_dev_t * dev, uint32_t block,
			   uint32_t offset, uint32_t size,
			   int (*iter_func) (struct iso9660_susp_entry *,
					     void *userdata), void *userdata)
{
	char *sua = malloc(size);
	char *ptr;

	if (!sua)
		return -1;

	/* read the SUA into memory */
	if (cdrom_read(dev, block, offset, size, sua))
		return -1;

	ptr = sua;
	do {
		struct iso9660_susp_entry *e = (struct iso9660_susp_entry *)ptr;

		/* In case of CE entry get the new SUA and try again */
		while (e->sig[0] == 'C' && e->sig[1] == 'E') {
			struct iso9660_susp_ce *ce =
			    (struct iso9660_susp_ce *)e;
			block = ce->blk_be;
			offset = ce->off_be;
			size = ce->len_be;

			free(sua);
			sua = malloc(size);

			if (!sua)
				return -1;
			if (cdrom_read(dev, block, offset, size, sua))
				return -1;
			ptr = sua;
			e = (struct iso9660_susp_entry *)ptr;
		}

		/* ST entry means STOP NOW */
		if (e->sig[0] == 'S' && e->sig[1] == 'T') {
			free(sua);
			return 0;
		}

		/* Call the user function. If it returns non-zero value, break iteration */
		if (iter_func(e, userdata)) {
			free(sua);
			return 0;
		}

		ptr += e->len;
	} while (ptr < sua + size - 1);

	free(sua);

	return 0;
}

struct tmp_data {
	context_t *ctx;
	struct iso9660_susp_entry *entry;
	uint8_t sig[2];
};

static int my_iteration(struct iso9660_susp_entry *e, struct tmp_data *d)
{
	if (e->sig[0] == d->sig[0] && e->sig[1] == d->sig[1]) {
		d->entry = malloc(e->len);
		memcpy(d->entry, e, e->len);
		return 1;
	} else
		return 0;
}

static struct iso9660_susp_entry *get_entry(cdrom_boot_dev_t * dev,
					    uint32_t block, uint32_t offset,
					    uint32_t size, uint8_t sig[2])
{
	struct tmp_data d;
	context_t *ctx = dev->ctx;

	d.ctx = ctx;
	d.entry = NULL;
	d.sig[0] = sig[0];
	d.sig[1] = sig[1];

	iterate_entries(dev, block, offset, size,
			(int (*)(struct iso9660_susp_entry *, void *userdata))
			my_iteration, &d);

	return d.entry;
}

static int fs_cdrom_find_file(cdrom_boot_dev_t * this, char *filename,
			      uint32_t * fblock, uint32_t * fsize)
{
	char *local_filename;
	char *elements[ISO9660_MAX_DEPTH + 1];
	int i, j, depth = 0;
	int block = 0;
	int offset = 0;
	int size = 0;
	char *fname = NULL;
	int fname_alloc = 0;
	int found = 0;

	struct iso9660_dir dirent;

	if (filename[0] == '/')
		filename++;

	local_filename = malloc(strlen(filename) + 1);
	memcpy(local_filename, filename, strlen(filename) + 1);

	/* Split the filename into separate elements */
	elements[depth++] = local_filename;
	for (i = 0; i < strlen(filename) - 1; i++) {
		if (local_filename[i] == '/') {
			local_filename[i] = 0;
			elements[depth++] = &local_filename[i + 1];
		}
	}

	block = this->voldesc.rootdir.first_sector_be;
	size = this->voldesc.rootdir.size_be;

	for (j = 0; j < depth; j++) {
		found = 0;

		while (!found && offset < size) {
			if (cdrom_read(this, block, offset, sizeof(dirent),
				       (char *)&dirent))
				return 0;

			if (!dirent.len) {
				offset = (offset / 2048 + 1) * 2048;
				continue;
			}

			{
				char *name = NULL;
				int nameoffset = offset + sizeof(dirent);
				int sua_off =
				    (sizeof(dirent) + dirent.namelen + 1 -
				     (dirent.namelen % 2));
				int sua_size = dirent.len - sua_off;

				sua_off += offset + this->susp_skip;

				fname = NULL;
				fname_alloc = 0;

				if (this->rr) {
					struct iso9660_susp_entry *entry =
					    get_entry(this, block,
						      sua_off,
						      sua_size,
						      (unsigned char *)"NM");
					if (entry) {
						if (entry->
						    data[0] & ISO9660_RR_DOT)
							fname = ".";
						else if (entry->
							 data[0] &
							 ISO9660_RR_DOTDOT)
							fname = "..";
						else {
							if (entry->len > 5) {
								int __size =
								    entry->len -
								    5;
								fname =
								    malloc
								    (__size +
								     1);
								if (!fname) {
									return
									    0;
								}
								fname_alloc = 1;
								memcpy(fname,
								       &entry->
								       data[1],
								       __size);
								fname[__size] =
								    '\0';
							}
						}
						free(entry);
					}
				}

				if (!fname) {
					name = malloc(dirent.namelen + 1);

					if (cdrom_read
					    (this, block, nameoffset,
					     dirent.namelen, name))
						return 0;

					name[dirent.namelen] = '\0';
					for (i = dirent.namelen; i; i--) {
						if (name[i] == ';') {
							name[i] = '\0';
							break;
						}
					}

					if (dirent.namelen == 1 && name[0] == 0)
						fname = ".";
					else if (dirent.namelen == 1
						 && name[0] == 1)
						fname = "..";
					else
						fname = name;

				}

				if (!strcasecmp(elements[j], fname)) {
					int type = dirent.flags & 3;

					if ((j + 1 < depth && type == 2)
					    || (j + 1 == depth && type != 2)) {
						found = 1;
					}
				}

				if (fname_alloc && fname)
					free(fname);

				if (name)
					free(name);
			}

			offset += dirent.len;
		}

		if (found) {
			/* Entry found. Go to the next level */
			block = dirent.first_sector_be;
			size = dirent.size_be;
			offset = 0;
		}
	}

	free(local_filename);

	if (found) {
		*fblock = block;
		*fsize = size;
//        cdrom_read(this, block, 0, size, buffer);
		//while(1);
		return size;
	}

	return 0;
}

#define D(x) x

static int fs_cdrom_destroy(cdrom_boot_dev_t * this)
{
	free(this);
	return 0;
}

static int fs_cdrom_load_file(cdrom_boot_dev_t * this, char *filename,
			      void *buffer)
{
	uint32_t block;
	uint32_t size = 0;

	if (fs_cdrom_find_file(this, filename, &block, &size)) {
		cdrom_read(this, block, 0, size, buffer);
	}

	return size;
}

static block_dev_desc_t *get_dev()
{
	block_dev_desc_t *bdev = NULL;
	SCAN_HANDLE hnd;
	uint32_t blocksize;

	start_unit_scan(NULL, &blocksize);

	for (hnd = start_unit_scan(get_scan_list(), &blocksize);
	     hnd != NULL; hnd = next_unit_scan(hnd, &blocksize)) {
		if (hnd->ush_device.type == DEV_TYPE_CDROM) {
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

boot_dev_t *cdrom_create()
{
	char *sp;
	block_dev_desc_t *dev = get_dev();

	if (dev == NULL)
		return NULL;

	cdrom_boot_dev_t *boot = malloc(sizeof(cdrom_boot_dev_t));

	asm volatile ("mr %0,%%r1":"=r" (sp));

	if (boot) {
		int sua_pos;
		int sua_size;
		char *sua;
		struct iso9660_susp_entry *entry;

		char huge_array[1000];
		huge_array[1] = 1;
		huge_array[999] = 2;
		(void)huge_array; /* Placeholder to eat stack */

		boot->phys = dev;
		boot->dev.load_file =
		    (int (*)(void *, char *, void *))fs_cdrom_load_file;
		boot->dev.destroy = (void (*)(void *))fs_cdrom_destroy;
		boot->rr = 0;

		if (cdrom_read(boot, 16, 0, sizeof(boot->voldesc),
			       (char *)&boot->voldesc)) {
			goto fail;
		}
//        if (!phys->block_read(phys->dev, 16, 1, boot->tmpbuf))
//        {
//            D(printf("[BOOT:CD] Cannot read device!\n"));
//            goto fail;
//        }
//        boot->sector_in_buf = 16;
//        memcpy(&boot->voldesc, boot->tmpbuf, sizeof(boot->voldesc));

		if (strncmp((char *)boot->voldesc.voldesc.magic, "CD001", 5)) {
			goto fail;
		}

		/* Read the system use area and test it to see if SUSP is
		   supported.  */
		if (cdrom_read(boot, boot->voldesc.rootdir.first_sector_be, 0,
			       sizeof(boot->rootdir), (char *)&boot->rootdir)) {
			goto fail;
		}
//        if (!phys->block_read(phys->dev, boot->voldesc.rootdir.first_sector_be, 1, boot->tmpbuf))
//        {
//            D(printf("[BOOT:CD] Not an iso9660 filesystem\n"));
//            goto fail;
//        }
//        boot->sector_in_buf = boot->voldesc.rootdir.first_sector_be;
//        memcpy(&boot->rootdir, boot->tmpbuf, sizeof(boot->rootdir));

		sua_pos = (sizeof(boot->rootdir) + boot->rootdir.namelen
			   + (boot->rootdir.namelen % 2) - 1);
		sua_size = boot->rootdir.len - sua_pos;

		sua = malloc(sua_size);
//        memcpy(sua, &boot->tmpbuf[sua_pos], sua_size);
		cdrom_read(boot, boot->voldesc.rootdir.first_sector_be, sua_pos,
			   sua_size, sua);

		entry = (struct iso9660_susp_entry *)sua;

		if (entry->sig[0] == 'S' && entry->sig[1] == 'P') {

			boot->susp_skip = entry->data[2];
			entry = (struct iso9660_susp_entry *)(sua + entry->len);

			/* Iterate through the SUSP entries and look for the RockRidge marker */

			entry =
			    get_entry(boot,
				      boot->voldesc.rootdir.first_sector_be,
				      sua_pos, sua_size, (unsigned char *)"RR");
			if (entry) {
				boot->rr = 1;
				free(entry);
			}
//            
//            do
//            {
//                D(printf("%c%c(%d) ", entry->sig[0], entry->sig[1], entry->len));
//                
//                if (entry->sig[0] == 'R' && entry->sig[1] == 'R')
//                {
//                    boot->rr = 1;
//                    break;
//                }
//                
//                if (!strncmp(entry->sig, "ST", 2))
//                    break;
//                
//                if (!strncmp(entry->sig, "CE", 2))
//                {
//                    struct iso9660_susp_ce *ce = (struct iso9660_susp_ce *)entry;
//                    sua_size = ce->len_be;
//                    sua_pos = ce->off_be;
//                    free(sua);
//                    sua = malloc(sua_size);
//                    phys->block_read(phys->dev, ce->blk_be, 1, boot->tmpbuf);
//                    memcpy(sua, &boot->tmpbuf[sua_pos], sua_size);
//                    entry = (struct iso9660_susp_entry *)sua;
//                }
//                else
//                    entry = (struct iso9660_susp_entry *)((char*)entry + entry->len);
//            } while ((char *)entry < (char*)sua + sua_size - 1);
//            
/*             if (boot->rr) */
/*                 D(printf("[BOOT:CD] The CD supports RockRidge extensions\n")); */
		}
		free(sua);
	}

	return &boot->dev;

fail:
	boot->dev.destroy(boot);
	return NULL;
}
