/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright © 2012 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 *
 * $Id $
 */

#ifndef NTFS_STRUCT_H
#define NTFS_STRUCT_H

#define FILE_MFT      0
#define FILE_MFTMIRR  1
#define FILE_LOGFILE  2
#define FILE_VOLUME   3
#define FILE_ATTRDEF  4
#define FILE_ROOT     5
#define FILE_BITMAP   6
#define FILE_BOOT     7
#define FILE_BADCLUS  8
#define FILE_QUOTA    9
#define FILE_UPCASE  10

#define AT_STANDARD_INFORMATION	0x10
#define AT_ATTRIBUTE_LIST	0x20
#define AT_FILENAME		0x30
#define AT_OBJECT_ID		0x40
#define AT_SECURITY_DESCRIPTOR	0x50
#define AT_VOLUME_NAME		0x60
#define AT_VOLUME_INFORMATION	0x70
#define AT_DATA			0x80
#define AT_INDEX_ROOT		0x90
#define AT_INDEX_ALLOCATION	0xA0
#define AT_BITMAP		0xB0
#define AT_REPARSE_POINT	0xC0
#define AT_EA_INFORMATION	0xD0
#define AT_EA			0xE0
#define AT_PROPERTY_SET		0xF0
#define AT_LOGGED_UTILITY_STRM	0x100
#define AT_USER			0x1000
#define AT_END			0xFFFFFFFF

#define ATTR_READ_ONLY		0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_DOSDIRECTORY	0x10
#define ATTR_ARCHIVE		0x20
#define ATTR_DEVICE		0x40
#define ATTR_NORMAL		0x80
#define ATTR_TEMPORARY		0x100
#define ATTR_SPARSE		0x200
#define ATTR_REPARSE		0x400
#define ATTR_COMPRESSED		0x800
#define ATTR_OFFLINE		0x1000
#define ATTR_NOT_INDEXED	0x2000
#define ATTR_ENCRYPTED		0x4000
#define ATTR_DIRECTORY		0x10000000
#define ATTR_INDEX_VIEW		0x20000000

#define FLAG_COMPRESSED		1
#define FLAG_ENCRYPTED		0x4000
#define FLAG_SPARSE		0x8000

#define AF_ALST		1
#define AF_MMFT		2
#define AF_GPOS		4

#define COM_LEN		4096
#define COM_LOG_LEN	12
#define COM_SEC		(COM_LEN >> 9)

#define INDEX_ENTRY_END 2

#define MFTREF_MASK 0x0000ffffffffffffULL

#define ATTR_RESIDENT_FORM 0x00 
#define ATTR_NONRESIDENT_FORM 0x01 

#define FILERECORD_SEGMENT_IN_USE (0x0001)
#define FILERECORD_NAME_INDEX_PRESENT (0x0002)

struct NTFSBootSector {
    UBYTE jmp_boot[3];
    UBYTE oem_name[8];
    
    UWORD bytes_per_sector;           /* Size of a sector in bytes. */
    UBYTE sectors_per_cluster;        /* Size of a cluster in sectors. */
    UBYTE reserved0[7];
    UBYTE media_type;
    UWORD reserved1;
    UWORD sectors_per_track;          /* irrelevant */
    UWORD heads;                      /* irrelevant */
    ULONG hidden_sectors;             /* zero */
    ULONG reserved2[2];
    QUAD number_of_sectors;
    QUAD mft_lcn;                     /* Cluster location of mft data. */
    QUAD mftmirr_lcn;                 /* Cluster location of copy of mft. */
    BYTE clusters_per_mft_record;     /* Mft record size in clusters. */
    UBYTE reserved3[3];
    BYTE  clusters_per_index_record;  /* Index block size in clusters. */
    UBYTE  reserved4[3];
    UQUAD volume_serial_number;       /* Irrelevant (serial number). */
    ULONG checksum;                   /* Boot sector checksum. */
} __attribute__ ((__packed__));

/* MultiSectorHeader */
struct MFTRecordMSH
{
    UBYTE magic[4];				// 0x00
    UWORD usa_offset;				// 0x04
    UWORD usa_count;				// 0x06
} __attribute__ ((__packed__));

struct MFTSegmentReference
{
    ULONG seg_low;				// 0x00
    UWORD seg_high;				// 0x04
    UWORD seg_sn;				// 0x06
} __attribute__ ((__packed__));

struct MFTRecordEntry
{
    struct MFTRecordMSH header;
    UQUAD lsn;					// 0x08
    UWORD sequence_number;		// 0x10
    UWORD link_count;				// 0x12
    UWORD attrs_offset;			// 0x14
    UWORD flags;					// 0x16
    ULONG bytes_in_use;			// 0x18
    ULONG bytes_allocated;			// 0x1C
    struct MFTSegmentReference base_mft_record;			// 0x20
    UWORD next_attr_instance;		// 0x28
    UWORD reserved;				// 0x2A
    ULONG mft_record_number;		// 0x2C
} __attribute__ ((__packed__));


struct MFTAttr {
    ULONG type;
    ULONG length;
    UBYTE residentflag;
    UBYTE attrname_length; /* UTF32 size of attrname (0 = unnamed) */
    UWORD attrname_offset;
    UWORD attrflags;
    UWORD instance;
    union {
	struct {
	    ULONG value_length;
	    UWORD value_offset;
	    UBYTE flags;
	    UBYTE reserved;
	} __attribute__ ((__packed__)) resident;
	struct {
	    UQUAD lowest_vcn;
	    UQUAD highest_vcn;
	    UQUAD mapping_pairs_offset;
	    UBYTE compression_unit;
	    UBYTE reserved[5];
	    UQUAD allocated_size;
	    UQUAD data_size;
	    UQUAD valid_size;
	    UQUAD actual_size;
		} __attribute__ ((__packed__)) non_resident;
	} __attribute__ ((__packed__)) data;
} __attribute__ ((__packed__));

struct MFTIndexHeader {
    union {
	struct {
	    UQUAD indexed_file;
	} __attribute__ ((__packed__)) dir;
	struct {
	    UWORD data_offset;
	    UWORD data_length;
	    ULONG reserved00;
	} __attribute__ ((__packed__)) vi;
    } __attribute__ ((__packed__)) data;
    UWORD length;
    UWORD key_length;
    UWORD flags;
    UWORD reserved;
} __attribute__ ((__packed__));

struct MFTIndexEntry {
    union {
	struct {
	    UQUAD indexed_file;
	} __attribute__ ((__packed__)) dir;
	struct {
	    UWORD data_offset;
	    UWORD data_length;
	    ULONG reserved00;
	} __attribute__ ((__packed__)) vi;
    } __attribute__ ((__packed__)) data;
    UWORD length;
    UWORD key_length;
    UWORD flags;
    UWORD reserved;
/*    union {
	UQUAD file_name;
	UWORD sii;
	UWORD sdh;
	UQUAD object_id;
	UQUAD reparse;
	uuid_t sid;
	ULONG owner_id;
    } __attribute__ ((__packed__)) key;*/
} __attribute__ ((__packed__));

#endif /* NTFS_STRUCT_H */
