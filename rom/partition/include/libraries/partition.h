#ifndef PARTITION_H
#define PARTITION_H

/*
    Copyright © 2003-2011, The AROS Development Team. All rights reserved.
    $Id$
*/

#include <devices/trackdisk.h>
#include <dos/filehandler.h>
#include <utility/tagitem.h>
#include <exec/lists.h>
#include <exec/nodes.h>
#include <exec/types.h>
#include <exec/ports.h>

struct PartitionTableInfo
{
    ULONG pti_Type;
    STRPTR pti_Name;
};

struct PartitionBlockDevice
{
    struct MsgPort *port;
    struct IOExtTD *ioreq;
    ULONG cmdread;
    ULONG cmdwrite;
};

struct PartitionTableHandler
{
    ULONG type;
    struct List list;   /* list of partitions */
    void *handler;      /* the handler which handles this partition table */
    void *data;         /* private field for the table */
};

struct PartitionHandle
{
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
    UBYTE buffer[4096];
};

struct PartitionType
{
    UBYTE id[32];
    UWORD id_len;
};

/* partition table types */
#define PHPTT_UNKNOWN (0)
#define PHPTT_RDB     (1)
#define PHPTT_MBR     (2)
#define PHPTT_EBR     (3)
#define PHPTT_GPT     (4)

/* partition types */
//nothing defined here ...

/* commands for DoPartition() */

/* Tags for partition tables */
#define PTT_TYPE           (TAG_USER |  1L) /* ULONG - partition table type */
#define PTT_MAXLEADIN      (TAG_USER |  3L) /* ULONG - maximum number of
    reserved blocks that may be need to precede a partition for metadata */
#define PTT_RESERVED       (TAG_USER | 32L) /* ULONG - number of reserved blocks at
                                                       begining of a partition table */
#define PTT_MAX_PARTITIONS (TAG_USER | 33L) /* ULONG - max number of partitions in table */
#define PTT_STARTBLOCK	   (TAG_USER | 34L) /* ULONG - Absolute number of starting block of this logical disk */


/* Tags for partitions */
#define PT_GEOMETRY   (TAG_USER |  1L) /* struct DriveGeometry * - geometry of partition */
#define PT_DOSENVEC   (TAG_USER |  2L) /* struct DosEnvec * - partition layout info */
#define PT_TYPE       (TAG_USER |  3L) /* struct PartitionType * - type of partition */
#define PT_LEADIN     (TAG_USER |  4L) /* ULONG - number of reserved blocks
                                         preceding partition for metadata */
#define PT_POSITION   (TAG_USER | 32L) /* ULONG - 1st partition, 2nd ... (Linux: hdX0, hdX1, ... */
#define PT_ACTIVE     (TAG_USER | 33L) /* BOOL - MBR: set/get partition as active */
#define PT_NAME       (TAG_USER | 34L) /* STRPTR - name of partition */
#define PT_BOOTABLE   (TAG_USER | 35L) /* BOOL - partition is bootable */
#define PT_AUTOMOUNT  (TAG_USER | 36L) /* BOOL - partition will be auto mounted */
#define PT_STARTBLOCK (TAG_USER | 37L) /* ULONG - Number of partition's start block */
#define PT_ENDBLOCK   (TAG_USER | 38L) /* ULONG - Number of partition's end block */

/* Tags for filesystems */
#define FST_ID		(TAG_USER | 1L)	/* ULONG 		 - filesystem ID   	*/
#define FST_NAME	(TAG_USER | 2L)	/* STRPTR		 - filesystem name	*/
#define FST_FSENTRY	(TAG_USER | 3L) /* struct FileSysEntry * - fill in FileSysEntry */
#define FST_VERSION	(TAG_USER | 4L) /* ULONG		 - version number	*/

/* Attributes */

struct PartitionAttribute
{
    ULONG attribute;
    ULONG mode;
};

/* are attributes readable/writeable */
#define PLAM_READ  (1<<0)
#define PLAM_WRITE (1<<1)

/* Obsolete definitions. Don't use them in new code. */
#define PTTA_DONE           TAG_DONE
#define PTTA_TYPE           PTT_TYPE
#define PTTA_MAXLEADIN      PTT_MAXLEADIN
#define PTTA_RESERVED       PTT_RESERVED
#define PTTA_MAX_PARTITIONS PTT_MAX_PARTITIONS

#define PTA_DONE              TAG_DONE
#define PTA_GEOMETRY          PT_GEOMETRY
#define PTA_DOSENVEC          PT_DOSENVEC
#define PTA_LEADIN            PT_LEADIN
#define PTA_TYPE              PT_TYPE
#define PTA_POSITION          PT_POSITION
#define PTA_ACTIVE            PT_ACTIVE
#define PTA_NAME              PT_NAME
#define PTA_BOOTABLE          PT_BOOTABLE
#define PTA_AUTOMOUNT 	      PT_AUTOMOUNT

struct PartitionBase
{
    struct Library lib;
    struct PartitionTableInfo **tables;
};

#endif /* PARTITION_H */
