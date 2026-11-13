#ifndef AFSBLOCKS_H
#define AFSBLOCKS_H

/*
    Copyright © 1995-2026, The AROS Development Team. All rights reserved.
    $Id$
*/

#define T_SHORT	2
#define T_DATA		8
#define T_LIST		16

#define MAX_NAME_LENGTH		31
#define MAX_COMMENT_LENGTH	91

#define BLOCK_SIZE(volume)						(volume->SizeBlock*4)

/*
 * Most header fields sit at a fixed distance from the end of the block.
 * BLKO_x is that distance in longs (negative), BLK_x(volume) the index from
 * the start of the block. BLK_TAILPTR() points just past a block, so
 * BLK_TAILPTR(buf, volume)[BLKO_x] is the same long as buf[BLK_x(volume)];
 * functions that touch several of these fields take that pointer once.
 */
#define BLK_TAILPTR(buf, volume)				((ULONG *)(buf) + (volume)->SizeBlock)

//all T_SHORT entries
#define BLK_PRIMARY_TYPE						0
#define BLK_CHECKSUM								5
#define BLK_TABLE_START							6
#define BLKO_TABLE_END					(-51)
#define BLK_TABLE_END(volume)					(volume->SizeBlock+BLKO_TABLE_END)
#define BLKO_SECONDARY_TYPE			(-1)
#define BLK_SECONDARY_TYPE(volume)			(volume->SizeBlock+BLKO_SECONDARY_TYPE)

// rootblock entries
#define BLK_TABLE_SIZE							3
#define BLKO_BITMAP_VALID_FLAG		(-50)
#define BLK_BITMAP_VALID_FLAG(volume)		(volume->SizeBlock+BLKO_BITMAP_VALID_FLAG)
#define BLKO_BITMAP_POINTERS_START	(-49)
#define BLK_BITMAP_POINTERS_START(volume)	(volume->SizeBlock+BLKO_BITMAP_POINTERS_START)
#define BLKO_BITMAP_POINTERS_END	(-25)
#define BLK_BITMAP_POINTERS_END(volume)	(volume->SizeBlock+BLKO_BITMAP_POINTERS_END)
#define BLKO_BITMAP_EXTENSION		(-24)
#define BLK_BITMAP_EXTENSION(volume)		(volume->SizeBlock+BLKO_BITMAP_EXTENSION)
#define BLKO_ROOT_DAYS					(-23)
#define BLK_ROOT_DAYS(volume)					(volume->SizeBlock+BLKO_ROOT_DAYS)
#define BLKO_ROOT_MINS					(-22)
#define BLK_ROOT_MINS(volume)					(volume->SizeBlock+BLKO_ROOT_MINS)
#define BLKO_ROOT_TICKS				(-21)
#define BLK_ROOT_TICKS(volume)				(volume->SizeBlock+BLKO_ROOT_TICKS)
#define BLKO_DISKNAME_START			(-20)
#define BLK_DISKNAME_START(volume)			(volume->SizeBlock+BLKO_DISKNAME_START)
#define BLKO_DISKNAME_END				(-13)
#define BLK_DISKNAME_END(volume)				(volume->SizeBlock+BLKO_DISKNAME_END)
#define BLKO_VOLUME_DAYS				(-10)
#define BLK_VOLUME_DAYS(volume)				(volume->SizeBlock+BLKO_VOLUME_DAYS)
#define BLKO_VOLUME_MINS				(-9)
#define BLK_VOLUME_MINS(volume)				(volume->SizeBlock+BLKO_VOLUME_MINS)
#define BLKO_VOLUME_TICKS				(-8)
#define BLK_VOLUME_TICKS(volume)				(volume->SizeBlock+BLKO_VOLUME_TICKS)
#define BLKO_CREATION_DAYS			(-7)
#define BLK_CREATION_DAYS(volume)			(volume->SizeBlock+BLKO_CREATION_DAYS)
#define BLKO_CREATION_MINS			(-6)
#define BLK_CREATION_MINS(volume)			(volume->SizeBlock+BLKO_CREATION_MINS)
#define BLKO_CREATION_TICKS			(-5)
#define BLK_CREATION_TICKS(volume)			(volume->SizeBlock+BLKO_CREATION_TICKS)

//file and directory entries
#define BLK_OWN_KEY								1
#define BLKO_OWNER						(-49)
#define BLK_OWNER(volume)						(volume->SizeBlock+BLKO_OWNER)
#define BLKO_PROTECT					(-48)
#define BLK_PROTECT(volume)					(volume->SizeBlock+BLKO_PROTECT)
#define BLKO_COMMENT_START			(-46)
#define BLK_COMMENT_START(volume)			(volume->SizeBlock+BLKO_COMMENT_START)
#define BLKO_COMMENT_END				(-24)
#define BLK_COMMENT_END(volume)				(volume->SizeBlock+BLKO_COMMENT_END)
#define BLKO_DAYS						(-23)
#define BLK_DAYS(volume)						(volume->SizeBlock+BLKO_DAYS)
#define BLKO_MINS						(-22)
#define BLK_MINS(volume)						(volume->SizeBlock+BLKO_MINS)
#define BLKO_TICKS						(-21)
#define BLK_TICKS(volume)						(volume->SizeBlock+BLKO_TICKS)
#define BLKO_LINKCHAIN					(-10)
#define BLK_LINKCHAIN(volume)					(volume->SizeBlock+BLKO_LINKCHAIN)
#define BLKO_HASHCHAIN					(-4)
#define BLK_HASHCHAIN(volume)					(volume->SizeBlock+BLKO_HASHCHAIN)
#define BLKO_PARENT						(-3)
#define BLK_PARENT(volume)						(volume->SizeBlock+BLKO_PARENT)

//directory entries
#define BLKO_DIRECTORYNAME_START	(-20)
#define BLK_DIRECTORYNAME_START(volume)	(volume->SizeBlock+BLKO_DIRECTORYNAME_START)
#define BLKO_DIRECTORYNAME_END		(-13)
#define BLK_DIRECTORYNAME_END(volume)		(volume->SizeBlock+BLKO_DIRECTORYNAME_END)

//file entries
#define BLK_BLOCK_COUNT							2
#define BLK_FIRST_DATA							4
#define BLKO_BYTE_SIZE					(-47)
#define BLK_BYTE_SIZE(volume)					(volume->SizeBlock+BLKO_BYTE_SIZE)
#define BLKO_FILENAME_START			(-20)
#define BLK_FILENAME_START(volume)			(volume->SizeBlock+BLKO_FILENAME_START)
#define BLKO_FILENAME_END				(-13)
#define BLK_FILENAME_END(volume)				(volume->SizeBlock+BLKO_FILENAME_END)
#define BLKO_EXTENSION					(-2)
#define BLK_EXTENSION(volume)					(volume->SizeBlock+BLKO_EXTENSION)

//data blocks OFS
#define BLK_HEADER_KEY							1
#define BLK_SEQUENCE_NUMBER					2
#define BLK_DATA_SIZE							3
#define BLK_NEXT_DATA							4
#define BLK_DATA_START							6

//hardlinks
#define BLKO_HARDLINKNAME_START		(-20)
#define BLK_HARDLINKNAME_START(volume)		(volume->SizeBlock+BLKO_HARDLINKNAME_START)
#define BLKO_HARDLINKNAME_END		(-13)
#define BLK_HARDLINKNAME_END(volume)		(volume->SizeBlock+BLKO_HARDLINKNAME_END)
#define BLKO_ORIGINAL					(-11)
#define BLK_ORIGINAL(volume)					(volume->SizeBlock+BLKO_ORIGINAL)

//softlinks
#define BLK_SYMBOLICNAME_START				6
#define BLKO_SYMBOLICNAME_END		(-51)
#define BLK_SYMBOLICNAME_END(volume)		(volume->SizeBlock+BLKO_SYMBOLICNAME_END)
#define BLKO_SOFTLINKNAME_START		(-20)
#define BLK_SOFTLINKNAME_START(volume)		(volume->SizeBlock+BLKO_SOFTLINKNAME_START)
#define BLKO_SOFTLINKNAME_END		(-13)
#define BLK_SOFTLINKNAME_END(volume)		(volume->SizeBlock+BLKO_SOFTLINKNAME_END)

#endif
