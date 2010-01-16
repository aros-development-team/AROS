/* sfs.h */

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

#ifndef _SFS_H
#define _SFS_H

#include "rdb.h"
#include "device.h"
#include "support.h"

struct SfsBlockHeader {
	uint32_t id;
	uint32_t checksum;
	uint32_t ownblock;
};
#define SBH(b) ((struct SfsBlockHeader *) (b))

struct SfsRootBlock {
	struct SfsBlockHeader bheader;

	uint16_t version;
	uint16_t sequencenumber;

	uint32_t datecreated;
	uint8_t bits;
	uint8_t pad1;
	uint16_t pad2;

	uint32_t reserved1[2];

	uint32_t firstbyteh;
	uint32_t firstbyte;

	uint32_t lastbyteh;
	uint32_t lastbyte;

	uint32_t totalblocks;
	uint32_t blocksize;

	uint32_t reserved2[2];
	uint32_t reserved3[8];

	uint32_t bitmapbase;
	uint32_t adminspacecontainer;
	uint32_t rootobjectcontainer;
	uint32_t extentbnoderoot;

	uint32_t reserved4[4];
};
#define SRB(b) ((struct SfsRootBlock *) (b))
#define SRB_ID 0x53465300

struct SfsObject {
	uint16_t owneruid;
	uint16_t ownergid;
	uint32_t objectnode;
	uint32_t protection;

	union {
		struct {
			uint32_t data;
			uint32_t size;
		} file;

		struct {
			uint32_t hashtable;
			uint32_t firstdirblock;
		} dir;
	} object;

	uint32_t datemodified;
	uint8_t bits;		/* see defines below */

	uint8_t name[0];
	uint8_t comment[0];
};
#define SO(b) ((struct SfsObject *) (b))
#define SO_ID 0x53465300

#define OTYPE_DELETED (32)
#define OTYPE_LINK    (64)
#define OTYPE_DIR     (128)

struct SfsObjectContainer {
	struct SfsBlockHeader bheader;
	uint32_t parent;
	uint32_t next;
	uint32_t previous;
	struct SfsObject object[1];
};
#define SOC(b) ((struct SfsObjectContainer *) (b))
#define SOC_ID (('O'<<24)|('B'<<16)|('J'<<8)|'C')

struct BNode {
	uint32_t key;
	uint32_t data;
};

struct BTreeContainer {
	uint16_t nodecount;
	uint8_t isleaf;
	uint8_t nodesize;

	struct BNode bnode[0];
};

struct SfsBNodeContainer {
	struct SfsBlockHeader bheader;
	struct BTreeContainer btc;
};
#define SBNC(b) ((struct SfsBNodeContainer *) (b))

struct SfsExtentBNode {
	uint32_t key;
	uint32_t next;
	uint32_t prev;
	uint16_t blocks;
};

boot_dev_t *sfs_create(struct RdbPartition *partition);

#endif /* _SFS_H */
