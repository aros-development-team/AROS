#ifndef PARTITION_H
#define PARTITION_H

#include <devices/trackdisk.h>
#include <dos/filehandler.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/ports.h>

struct PartitionTableInfo {
	ULONG pti_Type;
	STRPTR pti_Name;
};

struct PartitionBlockDevice {
	struct MsgPort *port;
	struct IOExtTD *ioreq;
	ULONG cmdread;
	ULONG cmdwrite;
};

struct PartitionTableHandler {
	ULONG type;
	struct List list;   /* list of partitions */
	void *handler; /* the handler which handles this partition table */
	void *data;    /* private field for the table */
};

struct PartitionHandle {
	struct Node ln;
	struct PartitionHandle *root;    /* root partition handle */
	struct PartitionBlockDevice *bd; /* block device info */
	ULONG flags;
	struct PartitionTableHandler *table;
	void *data;         /* a private field for the partition */
	struct DosEnvec de; /* info about HD/partition */
};

struct PartitionType {
	UBYTE id[32];
	UWORD id_len;
};

/* partition table types */
#define PHPTT_UNKNOWN (0)
#define PHPTT_RDB     (1)
#define PHPTT_MBR     (2)

/* partition types */
//nothing defined here ...

/* commands for DoPartition() */

/* Tags for partition tables */
#define PTT_DOSENVEC         (1)
#define PTT_TYPE             (2)  /* returns partition table type */
#define PTT_RESERVED         (32) /* returns number of reserved blocks
                                    at begining of a partition table */
#define PTT_MAX_PARTITIONS   (33) /* max number of partitions in table */


/* Tags for partitions */
#define PT_DOSENVEC PTT_DOSENVEC
#define PT_TYPE     PTT_TYPE /* arg is of struct PartitionType */
#define PT_POSITION  (32)  /* 1st partition, 2nd ... (Linux: hdX0, hdX1, ... */
#define PT_ACTIVE    (33)  /* MBR: set/get partition as active */
#define PT_NAME      (34)  /* name of partition */
#define PT_BOOTABLE  (35)  /* partition is bootable */
#define PT_AUTOMOUNT (36)  /* partition will be auto mounted */

/* partition table attributes */
#define PTTA_DOSENVEC         1 /* seems to be obsolete because only LowCyl,
                                   HighCyl, Surfaces, BlocksPerTrack,
                                   SizeBlock are of interrest in a table */
#define PTTA_GEOMETRY         2 /* because of above comment ... */
#define PTTA_TYPE           100 /* partition table type */
#define PTTA_RESERVED       101 /* reserved blocks */
#define PTTA_MAX_PARTITIONS 102 /* max numbers of partitions in table */

/* partition attributes */
#define PTA_DOSENVEC   1 /* whole struct DosEnvec support */
#define PTA_GEOMETRY   2 /* only geometry info in DosEnvec
                            (LowCyl, HighCyl, Surfaces, BlocksPerTrack,
                             SizeBlock) */
#define PTA_TYPE      100 /* type of partition */
#define PTA_POSITION  101 /* position of table within partition table */
#define PTA_ACTIVE    102 /* make partition active (whatever that means ;) */
#define PTA_NAME      103 /* device name support */
#define PTA_BOOTABLE  104
#define PTA_AUTOMOUNT 105

struct PartitionBase {
	struct Library lib;
	struct PartitionTableInfo **tables;
};

#endif /* PARTITION_H */

