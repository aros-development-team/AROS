/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef UDF_H
#define UDF_H

#include <exec/types.h>

/* UDF 1.5 support, based off of the OSTA subset of ECMA-167
 * Data structures for Plain, Spare, and VAT filesystems
 */

#ifndef __packed
#define __packed    __attribute__((__packed__))
#endif

typedef UBYTE   Uint8;
typedef BYTE    Int8;
typedef struct { USHORT le16; } Uint16;
typedef struct { USHORT le16; } Int16;
typedef struct { ULONG le32; } Uint32;
typedef struct { ULONG le32; } Int32;
typedef struct { UQUAD le32; } Uint64;
typedef struct { UQUAD le32; } Int64;

typedef struct osta_compressed {
    Uint8 CompressionID;
#define OSTA_COMPRESSIONID_8BIT    8   // UTF-8 stream
#define OSTA_COMPRESSIONID_16BIT   16  // UTF-16 stream
    UBYTE Stream[0];
} osta_compressed;

typedef struct charspec {
    Uint8   CharacterSetType;
#define UDF_CS0 0   /* UDF OSTA Compressed Unicode */
#define UDF_CS1 1   /* ECMA-6 (ISO/IEC 10646-1) */
#define UDF_CS2 2   /* ECMA-119 file identifiers:
                     *   0123456789._
                     *   ABCDEFGHIJKLMNOPQRSTUVWXYZ
                     */
#define UDF_CS3 3   /* ISO/IEC 9945-1:
                     *   0123456789._-
                     *   abcdefghijklmnopqrstuvwxyz
                     *   ABCDEFGHIJKLMNOPQRSTUVWXYZ
                     */
#define UDF_CS4 4   /* International ECMA-6 */
#define UDF_CS5 5   /* ECMA-94 (Latin-1) */
#define UDF_CS6 6   /* ECMA-35 + ECMA-48 */
#define UDF_CS7 7   /* EMCA-35 + ECMA-48 + their extension characters */
#define UDF_CS8 8   /* Portable set:
                     *   0123456789._-!#$%&'()^`{}~
                     *   abcdefghijklmnopqrstuvwxyz
                     *   ABCDEFGHIJKLMNOPQRSTUVWXYZ
                     */
    UBYTE   CharacterSetInfo[63];
#define UDF_CS0_OSTA_INFORMATION    "OSTA Compressed Unicode"
        /* CS0 - Valid graphic characters
         * CS1 - Valid graphic characters
         * CS2 - ZERO
         * CS3 - ZERO
         * CS4 - ZERO
         * CS5 - ZERO
         * CS6 - Valid graphic characters
         * CS7 - Valid graphic characters
         * CS8 - ZERO
         */
} charspec __packed;
// UDF 1.02 calls 'charspec' 'Charspec'
#define Charspec charspec

typedef struct dstring {
    Uint8   Length;
    UBYTE   Data[0];
} dstring __packed;

typedef struct timestamp {
    Uint16  TypeAndZone;
#define UDF_TZ_TYPE_of(x)   (((x) >> 12) & 0xf)
#define UDF_TZ_TYPE_UCT     0
#define UDF_TZ_TYPE_LOCAL   1
#define UDF_TZ_TYPE_UNKNOWN 2
#define UDF_TZ_of(x)        (((x) & 0x7ff) | (((x) >> 11) * 0xf800))
#define UDF_TZ_UNSPEC       -2047
    Int16   Year;           // 1 - 9999
    Uint8   Month;          // 1 - 12
    Uint8   Day;            // 1 - 31
    Uint8   Hour;           // 0 - 23
    Uint8   Minute;         // 0 - 59
    Uint8   Second;         // 0 - 59
    Uint8   Centiseconds;   // 0 - 99
    Uint8   HundredsofMicrosecs; // 0 - 99
    Uint8   Microseconds;   // 0 - 99
} timestamp __packed;

struct regid {
    Uint8   Flags;
#define UDF_REGID_DIRTY     (1 << 0)
#define UDF_REGID_PROTECTED (1 << 1)
    UBYTE   Identifier[23];
    union {
        UBYTE  Suffix[8];
        struct DomainIdentifierSuffix {
            Uint16  UDFRevision;
            Uint8   DomainFlags;
#define UDF_REGID_DOMAINFLAGS_WPROTECT_HARD (1 << 0)
#define UDF_REGID_DOMAINFLAGS_WPROTECT_SOFT (1 << 1)
            UBYTE   Reserved[5];
        } DomainSuffix;
        struct UDFIdentifierSuffix {
            Uint16  UDFRevision;
            Uint8   OSClass;
#define UTF_OSCLASS_UNDEFINED   0
#define UTF_OSCLASS_DOS         1
#define UTF_OSCLASS_OS2         2
#define UTF_OSCLASS_MACOS       3
#define UTF_OSCLASS_UNIX        4
#define UTF_OSCLASS_WIN9X       5
#define UTF_OSCLASS_WINNT       6
#define UTF_OSCLASS_OS400       7
#define UTF_OSCLASS_BEOS        8
#define UTF_OSCLASS_WINCE       9
            Uint8   OSIdentifier;
            UBYTE   Reserved[4];
        } UDFSuffix;
        struct ImplentationIdentifierSuffix {
            Uint8   OSClass;
            Uint8   OSIdentifier;
            UBYTE   ImplementationUse[6];
        } ImplementationSuffix;
    };
} regid __packed;

// UDF 1.02 calls 'regid' 'EntityID' (following ISO 13346 1/7.4 naming)
#define EnityID regid


typedef struct extent_ad {
    Uint32  Length;
    Uint32  Location;
} extent_ad __packed;

typedef struct tag {
    Uint16  TagIdentifier;
#define UDF_TAG_PRIMARY_VOLUME      1
#define UDF_TAG_ANCHOR_VOLUME_PTR   2
#define UDF_TAG_VOLUME_PTR          3
//
#define UDF_TAG_PARTITION           5
#define UDF_TAG_LOGICAL_VOLUME      6
#define UDF_TAG_UNALLOCATED_SPACE   7
#define UDF_TAG_TERMINATING         8
#define UDF_TAG_LOGICAL_VOLUME_INT  9
    Uint16  DescriptorVersion;      /* 2 = NSR02, 3 = NSR03 */
    Uint8   TagChecksum;            /* Checksum of bytes 0-3 & 5-15 mod 256 */
    UBYTE   Reserved[1];
    Uint16  TagSerialNumber;
    Uint16  DescriptorCRC;          /* CRC-ITU-T (x^16 + x^12 + x^5 + 1) */
    Uint16  DescriptorCRCLength;
    UInt32  TagLocation;            /* Extent of this tag */
} tag __packed;

/****************** Volume & Partitions ******************/

struct udfVolume {
    Uint8   Type;       /*  0       0                    0    */
    UBYTE   Id[5];      /* BEA01, BOOT2, NSR02, NSR03, TEA01 */
    Uint8   Version;    /*  1       1                    1    */
    union {
        UBYTE   Data[2041];
        struct UDF_VolumeDescriptor_BOOT2 {
            UBYTE   Reserved[1];
            regid       ArchitectureType;
            regid       BootIdentifier;
            Uint32      BootExtentLocation;
            Uint32      BootExtentLength;
            Uint64      LoadAddress;
            Uint64      StartAddress;
            timestamp   CreationTime;
            Uint16      Flags;
#define UDF_BOOT2_IGNORE    (1 << 0)
            UBYTE       Reserved[32];
            UBYTE       BootUse[1906];
        } BOOT2;
        struct { } BEA01;
        struct { } TEA01;
        struct { } NSR02;
        struct { } NSR03;
} __packed;

struct udfPrimaryVolume {
    tag         Tag;
    Uint32      VolumeDescriptorSequenceNumber;
    Uint32      PrimaryVolumeDescriptorNumber;
    union {
        dstring VolumeIdentifier;
        UBYTE   VolumeIdentifier_[32];
    };
    Uint16      VolumeSequenceNumber;
    Uint16      MaximumVolumeSequenceNumber;
    Uint16      InterchangeLevel;
#define UDF_VOLUME_INTERCHANGE_SINGLE   2
#define UDF_VOLUME_INTERCHANGE_MULTI    3
    Uint16      MaximumInterchangeLevel;
    Uint32      CharacterSetList;
#define UDF_VOLUME_CHARACTERSETLIST     (1 << 0)    // This is the ONLY value allowed!
    Uint32      MaximumCharacterSetList;
    union {
        dstring VolumeSetIdentifier;
        UBYTE   VolumeSetIdentifier_[128];
    };
    charspec    DescriptorCharacterSet;
    charspec    ExplanatoryCharacterSet;
    extent_ad   VolumeAbstract;
    extent_ad   VolumeCopyright;
    regid       ApplicationIdentifier;
    timestamp   RecordingDateandTime;
    regid       ImplementationIdentifier;
    UBYTE       ImplementationUse[64];
    Uint32      PredecessorVolumeDescriptorSequenceLocation;
    Uint16      Flags;
#define UDF_PRIMARY_VOLUME_COMMON   (1 << 0)
    UBYTE       Reserved[22];
} __packed;

struct udfAnchorVolumePointer {
    tag         Tag;
    extent_ad   MainVolumeDescriptorSequenceExtent;
    extent_ad   ReserveVolumeDescriptorSequenceExtent;
    UBYTE       Reserved[480];
} __packed;

struct udfVolumePointer {
    tag         Tag;
    Uint32      VolumeSequenceNumber;
    extent_ad   NextVolumeExtent;
    UBYTE       Reserved[484];
} __packed;

struct udfPartition {
    tag         Tag;
    Uint32      VolumeSequenceNumber;
    Uint16      Flags;
#define UDF_PARTITION_FLAGS_ALLOCATED (1 << 0)
    Uint16      Number;
    regid       Contents;   /* "+FDC01" - ECMA-107
                             * "+CD001" - ECMA-119
                             * "+CDW02" - ECMA-168
                             * "+NSR02" - ECMA-167/2
                             * "+NSR03" - ECMA-167
                             */
    UBYTE       ContentsUse[128];
    Uint32      AccessType;
#define UDF_PARTITION_ACCESSTYPE_RO     1   /* Read only */
#define UDF_PARTITION_ACCESSTYPE_WORM   2   /* Write once */
#define UDF_PARTITION_ACCESSTYPE_REW    3   /* Read/Erase/Write */
#define UDF_PARTITION_ACCESSTYPE_RW     4   /* Read/Write */
    Uint32      StartingLocation;
    Uint32      Length;
    regid       ImplementationIdentifier;
    UBYTE       ImplementationUse[128];
    UBYTE       Reserved[156];
} __packed;

struct udfLogicalVolume {
    tag         Tag;
    Uint32      VolumeDescriptorSequenceNumber;
    charspec    DescriptorCharacterSet;
    union {
        dstring Identifier;
        UBYTE   Identifier_[128];
    };
    Uint32      LogicalBlockSize;
    regid       DomainIdentifier;
    UBYTE       LogicalVolumeContentsUse[16];
    Uint32      MapTableLength;         /* MT_L */
    Uint32      NumberOfParititionMaps;
    regid       ImplementationIdentifier;
    UBYTE       ImplementationUse[128];
    extent_ad   IntegritySequenceExtent;
    UBYTE       PartitionMaps[0];
} __packed;

struct udfPartitionMap {
    Uint8   Type;
#define UDF_PARTITIONMAP_TYPE_1 1
#define UDF_PARTITIONMAP_TYPE_2 2
    Uint8   Length;
    union {
        UBYTE   Data[0];
        struct {
            Uint16  VolumeSequenceNumber;
            Uint16  PartitionNumber;
        } Type_1;
        struct {
            UBYTE   Reserved[2];
            regid   PartitionTypeIdentifier;
            Uint16  VolumeSequenceNumber;
            Uint16  PartitionNumber;
            UBYTE   Reserved[24];
        } Type_2;
    };
} __packed;

struct udfSparingTable {
    tag     Tag;
    regid   Identifier;
    Uint16  MapEntries;
    UBYTE   Reserved[2];
    Uint32  SequenceNumber;
    struct udfSparingMapEntry {
        Uint32  OriginalLocation;
#define UDF_MAPENTRY_AVAILABLE  0xffffffff
#define UDF_MAPENTRY_DEFECTIVE  0xfffffff0
        Uint32  MappedLocation;
    } MapEntry[0];
};

struct udfUnallocatedSpaceDesc {
    tag         Tag;
    Uint32      VolumeDescriptorSequenceNumber;
    Uint32      NumberofAlloctionDescriptors;
    extent_ad   AlloctionDescriptor[0];
} __packed;

struct udfTerminating {
    tag         Tag;
    UBYTE       Reserved[496];
} __packed;

struct udfLogicalVolumeIntegrityDesc {
    tag         Tag;
    timestamp   RecordingDateandTime;
    Uint32      IntegrityType;
#define UDF_LOGICALVOLUMEINTEGRITY_TYPE_OPEN    0
#define UDF_LOGICALVOLUMEINTEGRITY_TYPE_CLOSE   1
    extent_ad   NextIntegrityExtent;
    UBYTE       LogicalVolumeContentsUse[32];
    Uint32      NumberOfPartitions;
    Uint32      LengthOfImplementationUse;
    Uint32      Data[0];    /* Free Space Table, Size Table, Impl. Use */
} __packed;

struct udfLogicalVolumeIntegrityImplementationUse {
    regid       ImplementationID;
    Uint32      NumberofFiles;
    Uint32      NumberofDirectories;
    Uint16      MinimumUDFReadRevision;
    Uint16      MinimumUDFWriteRevision;
    Uint16      MaximumUDFWriteRevision;
    UBYTE       ImplementationUse[0];
} __packed;

struct udfImpUseVolume {
    tag         Tag;
    Uint32      VolumeDescriptorSequenceNumber;
    regid       ImplementationIdentifier;
    union {
        UBYTE       ImplementationUse[460];
        struct LVInformation {
            charspec    LVICharset;
            union {
                dstring     LogicalVolumeIdentifier;
                UBYTE       LogicalVolumeIdentifier_[128];
            };
            union {
                dstring     LVInfo1;
                UBYTE       LVInfo1_[36];
            };
            union {
                dstring     LVInfo2;
                UBYTE       LVInfo2_[36];
            };
            union {
                dstring     LVInfo3;
                UBYTE       LVInfo3_[36];
            };
            regid       ImplementationId;
            UBYTE       ImplementationUse[128];
        } LVInformation;
    };
} __packed;



/****************** Filesystem ***************************/

typedef struct lb_addr {
    Uint32      LogicalBlock;
    Uint16      Partition;
} lb_addr __packed;

typedef struct short_ad {
    Uint32      TypeLength;
#define UDF_SHORT_AD_LENGTH_of(x)    ((x) & 0x3fffffff)
#define UDF_SHORT_AD_TYPE_of(x)      (((x) >> 30) & 3)
#define   UDF_SHORT_AD_TYPE_RECORDED    0
#define   UDF_SHORT_AD_TYPE_ALLOCATED   1
#define   UDF_SHORT_AD_TYPE_FREE        2
#define   UDF_SHORT_AD_TYPE_NEXT        3
    Uint32      ExtentLocation;
} short_ad __packed;

typedef struct long_ad {
    Uint32      ExtentLength;
    lb_addr     ExtentLocation;
    union {
        UBYTE       ImplementationUse[6];
        struct ADImpUse {
            Uint16  flags;
            UBYTE   impUse[4];
        } ADImpUse;
    };
} long_ad __packed;

/* NOTE: This is here for completeness
 *       ext_ad structures ARE NOT USED in UDF <= 2.60
 */
typedef struct ext_ad {
    Uint32      ExtentLength;
    Uint32      RecordedLength;
    Uint32      InformationLength;
    lb_addr     ExtentLocation;
    UBYTE       ImplementationUse[2];
} ext_ad __packed;

#define UDF_TAG_FILE_SET                256
#define UDF_TAG_FILE_IDENTIFIER         257
#define UDF_TAG_ALLOCATION_EXTENT       258
#define UDF_TAG_INDIRECT_ENTRY          259
#define UDF_TAG_TERMINAL_ENTRY          260
#define UDF_TAG_FILE_ENTRY              261
#define UDF_TAG_EXTENDED_ATTRIBUTE      262
#define UDF_TAG_UNALLOCATED_SPACE       263
#define UDF_TAG_SPACE_BITMAP            264
#define UDF_TAG_PARTITION_INTEGRITY     265
#define UDF_TAG_EXTENDED_FILE           266

// ECMA-167 4/14.1
struct udfFileSet {
    tag         Tag;
    timestamp   RecordingDateandTime;
    Uint16      InterchangeLevel;
    Uint16      MaximumInterchangeLevel;
    Uint32      CharacterSetList;
    Uint32      MaximumCharacterSetList;
    Uint32      FileSetNumber;
    Uint32      FileSetDescriptorNumber;
    charspec    LogicalVolumeIdentifierCharacterSet;
    union {
        dstring LogicalVolumeIdentifier;
        UBYTE   LogicalVolumeIdentifier_[128];
    };
    charspec    FileSetCharacterSet;
    union {
        dstring Identifier;
        UBYTE   Identifier_[32];
    };
    union {
        dstring CopyrightFileIdentifier;
        UBYTE   CopyrightFileIdentifier_[32];
    };
    union {
        dstring AbstractFileIdentifier;
        UBYTE   AbstractFileIdentifier_[32];
    };
    long_ad     RootDirectoryICB;
    regid       DomainIdentifier;
    long_ad     NextExtent;
    UBYTE       Reserved[48];
} __packed;

// ECMA-167 4/14.3
struct udfPartitionHeader {
    short_ad    UnallocatedSpaceTable;
    short_ad    UnallocatedSpaceBitmap;
    short_ad    PartitionIntegrityTable;
    short_ad    FreedSpaceTable;
    short_ad    FreedSpaceBitmap;
    UBYTE       Reserved[88];
} __packed;

// ECMA-167 4/14.4
struct udfFileIdentifier {
    tag         Tag;
    Uint16      FileVersionNumber;
    Uint8       FileCharacteristics;
#define UDF_FILEID_FLAGS_HIDDEN     (1 << 0)
#define UDF_FILEID_FLAGS_DIRECTORY  (1 << 1)
#define UDF_FILEID_FLAGS_DELETED    (1 << 2)
#define UDF_FILEID_FLAGS_PARENT     (1 << 3)
#define UDF_FILEID_FLAGS_METADATA   (1 << 4)
    Uint8       FileIdentifierLength;     /* L_FI */
    long_ad     ICB;
    Uint16      ImplementationUseLength;  /* L_IU */
    UBYTE       Data[0];    /* L_IU + L_FI */
} __packed;

// ECMA-167 4/14.5
struct udfAllocationExtent {
    tag         Tag;
    Uint32      PreviousAllocationExtentLocation;   // Must be 0
    Uint32      AllocationDescriptorLength;
    // allocation descriptors follow
} __packed;

// ECMA-167 4/14.6
typedef struct icbtag {
    Uint32      PriorRecordedNumberofDirectEntries;
    Uint16      StrategyType;
#define UDF_ICBTAG_STRATEGY_1       // ECMA-167 4/A.2    Unused in UDF
#define UDF_ICBTAG_STRATEGY_2       // ECMA-167 4/A.3    Unused in UDF
#define UDF_ICBTAG_STRATEGY_3       // ECMA-167 4/A.4    Unused in UDF
#define UDF_ICBTAG_STRATEGY_4       // ECMA-167 4/A.5    UDF <= 2.60
#define UDF_ICBTAG_STRATEGY_4096    //                   UDF specific
    UBYTE       StrategyParameter[2];
    Uint16      NumberofEntries;
    UBYTE       Reserved[1];
    Uint8       FileType;
#define UDF_FILETYPE_UNALLOCATED_SPACE      1
#define UDF_FILETYPE_PARTITION_INTEGRITY    2
#define UDF_FILETYPE_INDIRECT_ENTRY         3
#define UDF_FILETYPE_DIRECTORY              4
#define UDF_FILETYPE_FILE                   5
#define UDF_FILETYPE_DEVICE_BLOCK           6
#define UDF_FILETYPE_DEVICE_CHAR            7
#define UDF_FILETYPE_EXTENDED_ATTRIBUTE     8
#define UDF_FILETYPE_DEVICE_FIFO            9
#define UDF_FILETYPE_DEVICE_SOCKET          10
#define UDF_FILETYPE_TERMINAL_ENTRY         11
#define UDF_FILETYPE_SYMLINK                12
#define UDF_FILETYPE_STREAM_DIRECTORY       13
    lb_addr     ParentICBLocation;
    Uint16      Flags;
#define UDF_ICBTAG_FLAGS_ALLOCTYPE_of(x)    ((x) & 7)
#define   UDF_ALLOCTYPE_SHORT_AD                0
#define   UDF_ALLOCTYPE_LONG_AD                 1
#define   UDF_ALLOCTYPE_EXTENDED_AD             2   // NOT USED IN UDF!
#define   UDF_ALLOCTYPE_TINY                    3
#define UDF_ICBTAG_FLAGS_DIRECTORY_SORT     (1 << 3)
#define UDF_ICBTAG_FLAGS_NON_RELOCATABLE    (1 << 4)
#define UDF_ICBTAG_FLAGS_ARCHIVE            (1 << 5)
#define UDF_ICBTAG_FLAGS_SETUID             (1 << 6)
#define UDF_ICBTAG_FLAGS_SETGID             (1 << 7)
#define UDF_ICBTAG_FLAGS_STICKY             (1 << 8)
#define UDF_ICBTAG_FLAGS_CONTIGUOUS         (1 << 9)
#define UDF_ICBTAG_FLAGS_SYSTEM             (1 << 10)
#define UDF_ICBTAG_FLAGS_TRANSFORMED        (1 << 11)
#define UDF_ICBTAG_FLAGS_MULTIVERSION       (1 << 12)
#define UDF_ICBTAG_FLAGS_STREAM             (1 << 13)
} icbtag __packed;

// ECMA-167 4/14.7
struct udfICBIndirect {
    tag         Tag;
    icbtag      ICBTag;
    long_ad     IndirectICB;
} __packed;

// ECMA-167 4/14.8
struct udfICBTerminal {
    tag         Tag;
    icbtag      ICBTag;
} __packed;

// ECMA-167 4/14.9
struct udfICBFileEntry {
    tag         Tag;
    icbtag      ICBTag;
    Uint32      Uid;
    Uint32      Gid;
    Uint32      Permissions;
#define UDF_FILEPERM_OX         (1 <<  0)
#define UDF_FILEPERM_OW         (1 <<  1)
#define UDF_FILEPERM_OR         (1 <<  2)
#define UDF_FILEPERM_OA         (1 <<  3)
#define UDF_FILEPERM_OD         (1 <<  4)
#define UDF_FILEPERM_GX         (1 <<  5)
#define UDF_FILEPERM_GW         (1 <<  6)
#define UDF_FILEPERM_GR         (1 <<  7)
#define UDF_FILEPERM_GA         (1 <<  8)
#define UDF_FILEPERM_GD         (1 <<  9)
#define UDF_FILEPERM_UX         (1 << 10)
#define UDF_FILEPERM_UW         (1 << 11)
#define UDF_FILEPERM_UR         (1 << 12)
#define UDF_FILEPERM_UA         (1 << 13)
#define UDF_FILEPERM_UD         (1 << 14)
    Uint16      FileLinkCount;
    Uint8       RecordFormat;
#define UDF_RECORDFORMAT_BIN    0   /* We only support this for now */
    Uint8       RecordDisplayAttributes;
#define UDF_RECORDATTR_BIN      0   /* We only support this for now */
    Uint32      RecordLength;
    Uint64      InformationLength;
    Uint64      LogicalBlocksRecorded;
    timestamp   AccessTime;
    timestamp   ModificationTime;
    timestamp   AttributeTime;
    Uint32      Checkpoint;
    long_ad     ExtendedAttributeICB;
    regid       ImplementationIdentifier;
    Uint64      UniqueId;
    Uint32      ExtendedAttributeLength;    // L_EA
    Uint32      AllocationDescriptorLength; // L_AD
    UBYTE       Data[0];    // L_EA + L_AD
} __packed;

// ECMA-167 4/14.10 - Extended Attributes are ignored

// ECMA-167 4/14.11 - Unallocated Space
struct udfICBUnallocatedSpace {
    tag         Tag;
    icbtag      ICBTag;
    Uint32      AllocationDescriptorLength; // L_AD
    UBYTE       Data[0];    // L_AD
} __packed;

// ECMA-167 4/14.12 - Space Bitmap Descriptor
struct udfSpaceBitmap {
    tag         Tag;
    Uint32      Bits;       // N_BT
    Uint32      Bytes;      // N_B
    UBYTE       Bitmap[0];  // N_B
} __packed;

// ECMA-167 4/14.13 - Partition Integrity
struct udfICBPartitionIntegrity {
    tag         Tag;
    icbtag      ICBTag;
    timestamp   RecordingTime;
    Uint8       IntegrityType;
#define UDF_PARTITIONINTEGRITY_OPEN     0
#define UDF_PARTITIONINTEGRITY_CLOSE    1
#define UDF_PARTITIONINTEGRITY_STABLE   2
    UBYTE       Reserved[175];
    regid       ImplementationIdentifier;
    UBYTE       ImplementationUse[256];
} __packed;

// ECMA-167 4/14.16 - Pathname
struct udfPathComponent {
    Uint8       Type;
#define UDF_PATHTYPE_ROOT       1
#define UDF_PATHTYPE_ROOT_OF    2   /* ":" */
#define UDF_PATHTYPE_PARENT_OF  3   /* "/" */
#define UDF_PATHTYPE_DIR_OF     4   /* "" */
#define UDF_PATHTYPE_FILE       5
    Uint8       IdentifierLength;       // L_CI
    Uint16      FileVersion;            // Shall be 0
    UBYTE       Identifier[0];          // L_CI bytes 
} __packed;

#endif /* UDF_H */
