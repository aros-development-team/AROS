#ifndef AFSBLOCKS_H
#define AFSBLOCKS_H

/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$
*/

#define T_SHORT	2
#define T_DATA		8
#define T_LIST		16

#define MAX_NAME_LENGTH		31
#define MAX_COMMENT_LENGTH	91

#define BLOCK_SIZE(volume)						(volume->SizeBlock*4)

//all T_SHORT entries
#define BLK_PRIMARY_TYPE						0
#define BLK_CHECKSUM								5
#define BLK_TABLE_START							6
#define BLK_TABLE_END(volume)					(volume->SizeBlock-51)
#define BLK_SECONDARY_TYPE(volume)			(volume->SizeBlock-1)

// rootblock entries
#define BLK_TABLE_SIZE							3
#define BLK_BITMAP_VALID_FLAG(volume)		(volume->SizeBlock-50)
#define BLK_BITMAP_POINTERS_START(volume)	(volume->SizeBlock-49)
#define BLK_BITMAP_POINTERS_END(volume)	(volume->SizeBlock-25)
#define BLK_BITMAP_EXTENSION(volume)		(volume->SizeBlock-24)
#define BLK_ROOT_DAYS(volume)					(volume->SizeBlock-23)
#define BLK_ROOT_MINS(volume)					(volume->SizeBlock-22)
#define BLK_ROOT_TICKS(volume)				(volume->SizeBlock-21)
#define BLK_DISKNAME_START(volume)			(volume->SizeBlock-20)
#define BLK_DISKNAME_END(volume)				(volume->SizeBlock-13)
#define BLK_VOLUME_DAYS(volume)				(volume->SizeBlock-10)
#define BLK_VOLUME_MINS(volume)				(volume->SizeBlock-9)
#define BLK_VOLUME_TICKS(volume)				(volume->SizeBlock-8)
#define BLK_CREATION_DAYS(volume)			(volume->SizeBlock-7)
#define BLK_CREATION_MINS(volume)			(volume->SizeBlock-6)
#define BLK_CREATION_TICKS(volume)			(volume->SizeBlock-5)

//file and directory entries
#define BLK_OWN_KEY								1
#define BLK_OWNER(volume)						(volume->SizeBlock-49)
#define BLK_PROTECT(volume)					(volume->SizeBlock-48)
#define BLK_COMMENT_START(volume)			(volume->SizeBlock-46)
#define BLK_COMMENT_END(volume)				(volume->SizeBlock-24)
#define BLK_DAYS(volume)						(volume->SizeBlock-23)
#define BLK_MINS(volume)						(volume->SizeBlock-22)
#define BLK_TICKS(volume)						(volume->SizeBlock-21)
#define BLK_LINKCHAIN(volume)					(volume->SizeBlock-10)
#define BLK_HASHCHAIN(volume)					(volume->SizeBlock-4)
#define BLK_PARENT(volume)						(volume->SizeBlock-3)

//directory entries
#define BLK_DIRECTORYNAME_START(volume)	(volume->SizeBlock-20)
#define BLK_DIRECTORYNAME_END(volume)		(volume->SizeBlock-13)

//file entries
#define BLK_BLOCK_COUNT							2
#define BLK_FIRST_DATA							4
#define BLK_BYTE_SIZE(volume)					(volume->SizeBlock-47)
#define BLK_FILENAME_START(volume)			(volume->SizeBlock-20)
#define BLK_FILENAME_END(volume)				(volume->SizeBlock-13)
#define BLK_EXTENSION(volume)					(volume->SizeBlock-2)

//data blocks OFS
#define BLK_HEADER_KEY							1
#define BLK_SEQUENCE_NUMBER					2
#define BLK_DATA_SIZE							3
#define BLK_NEXT_DATA							4
#define BLK_DATA_START							6

//hardlinks
#define BLK_HARDLINKNAME_START(volume)		(volume->SizeBlock-20)
#define BLK_HARDLINKNAME_END(volume)		(volume->SizeBlock-13)
#define BLK_ORIGINAL(volume)					(volume->SizeBlock-11)

//softlinks
#define BLK_SYMBOLICNAME_START				6
#define BLK_SYMBOLICNAME_END(volume)		(volume->SizeBlock-51)
#define BLK_SOFTLINKNAME_START(volume)		(volume->SizeBlock-20)
#define BLK_SOFTLINKNAME_END(volume)		(volume->SizeBlock-13)

#endif
