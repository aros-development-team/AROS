/*
 * ntfs.handler - New Technology FileSystem handler
 *
 * Copyright (C) 2025 The AROS Development Team
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the same terms as AROS itself.
 */

#ifndef NTFS_CONSTANTS_H
#define NTFS_CONSTANTS_H

/* MFT Record offsets */
#define MFT_RECORD_MAGIC_OFFSET         0x00
#define MFT_RECORD_USA_OFFSET           0x04
#define MFT_RECORD_USA_COUNT            0x06
#define MFT_RECORD_ATTRS_OFFSET         0x14
#define MFT_RECORD_FLAGS_OFFSET         0x16

/* Attribute offsets */
#define ATTR_TYPE_OFFSET                0x00
#define ATTR_LENGTH_OFFSET              0x04
#define ATTR_RESIDENT_FLAG_OFFSET       0x08
#define ATTR_NAME_LENGTH_OFFSET         0x09
#define ATTR_NAME_OFFSET_OFFSET         0x0A
#define ATTR_FLAGS_OFFSET               0x0C
#define ATTR_INSTANCE_OFFSET            0x0E

/* Resident attribute offsets */
#define ATTR_RESIDENT_VALUE_LENGTH      0x10
#define ATTR_RESIDENT_VALUE_OFFSET      0x14
#define ATTR_RESIDENT_FLAGS             0x16

/* Non-resident attribute offsets */
#define ATTR_NONRES_LOWEST_VCN          0x10
#define ATTR_NONRES_HIGHEST_VCN         0x18
#define ATTR_NONRES_MAPPING_OFFSET      0x20
#define ATTR_NONRES_COMPRESSION_UNIT    0x22
#define ATTR_NONRES_ALLOCATED_SIZE      0x28
#define ATTR_NONRES_DATA_SIZE           0x30
#define ATTR_NONRES_VALID_SIZE          0x38
#define ATTR_NONRES_ACTUAL_SIZE         0x40

/* Index entry offsets */
#define INDEX_ENTRY_MFT_REF             0x00
#define INDEX_ENTRY_LENGTH              0x08
#define INDEX_ENTRY_KEY_LENGTH          0x0A
#define INDEX_ENTRY_FLAGS               0x0C
#define INDEX_ENTRY_FILENAME_OFFSET     0x50
#define INDEX_ENTRY_NAME_LENGTH         0x50
#define INDEX_ENTRY_NAME_TYPE           0x51
#define INDEX_ENTRY_NAME_START          0x52
#define INDEX_ENTRY_FILE_FLAGS          0x48

/* Index allocation offsets */
#define INDEX_ROOT_OFFSET               0x10
#define INDEX_ROOT_ENTRIES_OFFSET       0x18
#define INDEX_HEADER_OFFSET             0x18

/* Filename attribute offsets */
#define FILENAME_PARENT_REF             0x00
#define FILENAME_CREATION_TIME          0x08
#define FILENAME_MODIFIED_TIME          0x10
#define FILENAME_MFT_CHANGED_TIME       0x18
#define FILENAME_ACCESSED_TIME          0x20
#define FILENAME_ALLOCATED_SIZE         0x28
#define FILENAME_REAL_SIZE              0x30
#define FILENAME_FLAGS                  0x38
#define FILENAME_REPARSE_VALUE          0x3C
#define FILENAME_NAME_LENGTH            0x40
#define FILENAME_NAME_TYPE              0x41
#define FILENAME_NAME_START             0x42

/* Standard Information attribute offsets */
#define STDINFO_CREATION_TIME           0x00
#define STDINFO_MODIFIED_TIME           0x08
#define STDINFO_MFT_CHANGED_TIME        0x10
#define STDINFO_ACCESSED_TIME           0x18
#define STDINFO_FILE_ATTRIBUTES         0x20
#define STDINFO_MAX_VERSIONS            0x24
#define STDINFO_VERSION_NUMBER          0x28
#define STDINFO_CLASS_ID                0x2C
#define STDINFO_OWNER_ID                0x30
#define STDINFO_SECURITY_ID             0x34
#define STDINFO_QUOTA_CHARGED           0x38
#define STDINFO_USN                     0x40

/* Attribute list entry offsets */
#define ATTRLIST_TYPE                   0x00
#define ATTRLIST_LENGTH                 0x04
#define ATTRLIST_NAME_LENGTH            0x06
#define ATTRLIST_NAME_OFFSET            0x07
#define ATTRLIST_LOWEST_VCN             0x08
#define ATTRLIST_MFT_REF                0x10
#define ATTRLIST_INSTANCE               0x18

/* Magic strings */
#define NTFS_MAGIC_FILE                 "FILE"
#define NTFS_MAGIC_INDX                 "INDX"
#define NTFS_MAGIC_BAAD                 "BAAD"
#define NTFS_OEM_ID                     "NTFS    "

/* Special MFT record numbers */
#define MFT_REC_MFT                     0
#define MFT_REC_MFTMIRR                 1
#define MFT_REC_LOGFILE                 2
#define MFT_REC_VOLUME                  3
#define MFT_REC_ATTRDEF                 4
#define MFT_REC_ROOT                    5
#define MFT_REC_BITMAP                  6
#define MFT_REC_BOOT                    7
#define MFT_REC_BADCLUS                 8
#define MFT_REC_SECURE                  9
#define MFT_REC_UPCASE                  10
#define MFT_REC_EXTEND                  11

/* Namespace types */
#define NAMESPACE_POSIX                 0
#define NAMESPACE_WIN32                 1
#define NAMESPACE_DOS                   2
#define NAMESPACE_WIN32_DOS             3

/* Index attribute name */
#define INDEX_I30_NAME_DWORD1           0x00490024  /* "$I" in Unicode */
#define INDEX_I30_NAME_DWORD2           0x00300033  /* "30" in Unicode */

/* Maximum name lengths */
#define NTFS_MAX_NAME_LENGTH            255
#define VOLUME_NAME_MAX_LENGTH          30
#define FIB_NAME_MAX_LENGTH             106

/* Sector and cluster constants */
#define SECTOR_SIZE_512                 512
#define SECTOR_SIZE_SHIFT_512           9

/* Update sequence array */
#define USA_OFFSET_MIN                  0x28
#define USA_OFFSET_MAX                  0x100

/* Validation limits */
#define MAX_REASONABLE_CLUSTER_SIZE     (4 * 1024 * 1024)  /* 4MB */
#define MAX_REASONABLE_MFT_SIZE         (16 * 1024)        /* 16KB */
#define MAX_REASONABLE_INDEX_SIZE       (16 * 1024)        /* 16KB */
#define MAX_UNICODE_CHAR                65535

/* Error return values */
#define NTFS_ERROR                      (~0UL)

/* Special file names in root */
#define SPECIAL_FILE_MFT                "$MFT"
#define SPECIAL_FILE_MFTMIRR            "$MFTMirr"
#define SPECIAL_FILE_LOGFILE            "$LogFile"
#define SPECIAL_FILE_VOLUME             "$Volume"
#define SPECIAL_FILE_ATTRDEF            "$AttrDef"
#define SPECIAL_FILE_ROOT               "."
#define SPECIAL_FILE_BITMAP             "$Bitmap"
#define SPECIAL_FILE_BOOT               "$Boot"
#define SPECIAL_FILE_BADCLUS            "$BadClus"
#define SPECIAL_FILE_SECURE             "$Secure"
#define SPECIAL_FILE_UPCASE             "$UpCase"
#define SPECIAL_FILE_EXTEND             "$Extend"
#define SPECIAL_FILE_RECYCLE            "$Recycle.Bin"

/* Recycle bin replacement name */
#define RECYCLE_BIN_NAME                "Recycle.Bin"

/* Cache parameters */
#define CACHE_ENTRIES                   64
#define CACHE_BLOCKS                    64

/* Size limits for 32-bit compatibility */
#define SIZE_32BIT_MAX                  0x7FFFFFFF

#endif /* NTFS_CONSTANTS_H */
