/*
    Copyright � 1995-2010, The AROS Development Team. All rights reserved.
    $Id$
*/

/*
 * -date------ -name------------------- -description-----------------------------
 * 02-jan-2008 [Tomasz Wiszkowski]      created disk validation procedures
 * 04-jan-2008 [Tomasz Wiszkowski]      corrected tabulation
 * 07-jan-2008 [Tomasz Wiszkowski]      partitioned procedures to prepare non-recursive scan
 */

#ifndef VALIDATOR_H
#define VALIDATOR_H

#include "os.h"

/*
 * call this function to initiate validation
 */
LONG launchValidator(struct AFSBase *afsbase, struct Volume *volume);
LONG checkValid(struct AFSBase *afs, struct Volume *vol);

#ifdef __AROS__

/********************************************************
 * structures
 ********************************************************/
typedef struct
{
	struct AFSBase            *afs;
	struct Volume             *vol;
	void*                      bitmap;
	LONG                       flags;
	ULONG                     *bm_blocks;
	LONG                       bm_lastblk;
	ULONG                     *bme_blocks;
	LONG                       bme_lastblk;

	/* elements regarding to currently validated file */
	ULONG                      max_file_len;
   ULONG                      file_blocks;
} DiskStructure;


/********************************************************
 * enums
 ********************************************************/
typedef enum
{
	vr_OK,
	vr_NoAccess,
	vr_ReadError,
	vr_UnknownDiskType,
	vr_InvalidChksum,
	vr_StructureDamaged,
	vr_OutOfMemory,
	vr_BlockUsedTwice,
	vr_Aborted,
	vr_BlockOutsideDisk,
} ValidationResult;

typedef enum
{
	st_OK,
	st_AlreadyInUse,
	st_OutOfRange
} BitmapResult;

typedef enum
{
	ValFlg_DisableReq_MaybeNotAFS      = 1,  /* consider these FLAGS */
	ValFlg_DisableReq_DataLossImminent = 2,  /* next value = 4       */
} ValidationFlags;

/*
 * validation entry point for new process
 */
LONG validate(struct AFSBase*, struct Volume*);

/*
 * initiate validation once structures are initially ready.
 * walks through all structures and collects used blocks
 */
ValidationResult start_superblock(DiskStructure *ds);

/*
 * verifies checksum. 
 * returns 0 if checksum is ok, otherwise a new checksum.
 * a new checksum is recorded to the structure automatically,
 * just not recorded
 */
ULONG verify_checksum(DiskStructure *ds, ULONG* block);
ULONG verify_bm_checksum(DiskStructure *ds, ULONG* block);

/*
 * various per-block operations:
 * - check_block does the most generic checks (checksum, block type, range and stuff like that)
 * - collect_bitmap is called with root block where we begin root block acquisition - nonrecursive
 */
ValidationResult check_block(DiskStructure* ds, struct BlockCache* block);
ValidationResult collect_bitmap(DiskStructure* ds, struct BlockCache* block);
ValidationResult collect_file_extensions(DiskStructure* ds, struct BlockCache* block);

/*
 * collect all bitmap blocks, starting with first extension block
 * all root blocks are already collected
 */
ValidationResult collect_directory_blocks(DiskStructure *ds, ULONG ext);

/*
 * record bitmap back to disk
 */
void record_bitmap(DiskStructure* ds);

/*
 * call to allocate bitmap in disk structure.
 * returns 0 if all went ok, otherwise other value
 * (just in case someone needed error handling here)
 */
LONG bm_allocate_bitmap(DiskStructure *ds);
void bm_free_bitmap(DiskStructure *ds);

/*
 * bitmap operations ;]
 */
BitmapResult bm_mark_block(DiskStructure *ds, ULONG block);

/*
 * 
 */
void bm_add_bitmap_block(DiskStructure *ds, ULONG blk);
void bm_add_bitmap_extension_block(DiskStructure *ds, ULONG blk);

#endif /* __AROS__ */
#endif /* VALIDATOR_H */
