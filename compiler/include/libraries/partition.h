#ifndef PARTITION_H
#define PARTITION_H

#include <devices/trackdisk.h>
#include <dos/filehandler.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/ports.h>
#include <libcore/base.h>

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
	void *handler;      /* the handler which handles this partition table */
	void *data;         /* private field for the table */
};

struct PartitionHandle {
	struct Node ln;
	struct PartitionHandle *root;    /* root partition handle */
	struct PartitionBlockDevice *bd; /* block device info */
	ULONG flags;
	struct PartitionTableHandler *table;
	void *data;                      /* a private field for the partition */
	struct DriveGeometry dg;         /* geometry of whole partition */
	struct DosEnvec de;              /* info about HD/partition including */
                                    /* position within root->dg */
                                    /* (de_Surfaces==root->dg.Heads,...!!!) */
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
#define PTT_TYPE             (1)  /* ULONG - partition table type */
#define PTT_RESERVED         (32) /* ULONG - number of reserved blocks at
                                             begining of a partition table */
#define PTT_MAX_PARTITIONS   (33) /* ULONG - max number of partitions in table */


/* Tags for partitions */
#define PT_GEOMETRY   (1) /* struct DriveGeometry - geometry of partition */
#define PT_DOSENVEC   (2) /* struct DosEnvec - partition layout info */
#define PT_TYPE       (3) /* struct PartitionType - type of partition */
#define PT_POSITION  (32) /* ULONG - 1st partition, 2nd ... (Linux: hdX0, hdX1, ... */
#define PT_ACTIVE    (33) /* BOOL - MBR: set/get partition as active */
#define PT_NAME      (34) /* STRPTR - name of partition */
#define PT_BOOTABLE  (35) /* BOOL - partition is bootable */
#define PT_AUTOMOUNT (36) /* BOOL - partition will be auto mounted */


/* Attributes */

struct PartitionAttribute {
	ULONG attribute;
	ULONG mode;
};

/* are attributes readable/writeable */
#define PLAM_READ  (1<<0)
#define PLAM_WRITE (1<<1)

/* partition table attributes */
#define PTTA_DONE             0 /* no more attributes */
#define PTTA_TYPE           100 /* partition table type */
#define PTTA_RESERVED       101 /* reserved blocks */
#define PTTA_MAX_PARTITIONS 102 /* max numbers of partitions in table */

/* partition attributes */
#define PTA_DONE      PTTA_DONE /* no more attributes */
#define PTA_GEOMETRY          1 /* geometry of partition (virtual HD) */
#define PTA_DOSENVEC          2 /* whole struct DosEnvec support */
#define PTA_DOSENVEC_GEOMETRY 3 /* only low/high cyl, sizeblock support in struct DosEnvec */
#define PTA_TYPE            100 /* type of partition */
#define PTA_POSITION        101 /* position of table within partition table */
#define PTA_ACTIVE          102 /* make partition active (whatever that means ;) */
#define PTA_NAME            103 /* device name support */
#define PTA_BOOTABLE        104 /* bootable flag support */
#define PTA_AUTOMOUNT 105

struct PartitionBase {
	struct LibHeader lh;
	struct PartitionTableInfo **tables;
};

#endif /* PARTITION_H */

