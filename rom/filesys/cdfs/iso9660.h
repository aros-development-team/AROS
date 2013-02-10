/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef ISO9660_H
#define ISO9660_H

#include <exec/types.h>
#include <dos/filehandler.h>

#ifndef __packed
#define __packed    __attribute__((__packed__))
#endif

typedef UBYTE   int8;
typedef TEXT    strA;   /* ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ 
                         * !"%&'()*+,-./:;<=>?
                         */
typedef TEXT    strD;   /* ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_ */
typedef struct { ULONG LSB; ULONG MSB; } int32LM;
typedef struct { UWORD LSB; UWORD MSB;} int16LM;
typedef ULONG   int32L;
typedef ULONG   int32M;
typedef ULONG   int32;
typedef UWORD   int16;
typedef struct {
    strD Year[4];
    strD Month[2];
    strD Day[2];
    strD Hour[2];
    strD Minute[2];
    strD Second[2];
    strD Hundredth[2];
    int8 TimeZone;  /* 0 = GMT-12, 1 = GMT-11:45, 2 = GMT-11:30, etc */
} ascDate;

typedef struct {
    int8    Years;  /* 1900 + Year */
    int8    Month;  /* Base 1 */
    int8    Day;    /* Base 1 */
    int8    Hour;   /* Base 0 */
    int8    Minute; /* Base 0 */
    int8    Second; /* Base 0 */
    int8    TimeZone;  /* 0 = GMT-12, 1 = GMT-11:45, 2 = GMT-11:30, etc */
} binDate;

struct isoVolume {
    int8    Type;
#define ISO_Volume_Boot             0
#define ISO_Volume_Primary          1
#define ISO_Volume_Supplementary    2
#define ISO_Volume_Partition        3
#define ISO_Volume_Terminator       255
    strA    Identifier[5];
    int8    Version;
    union {
        UBYTE Data[2041];
        struct isoVolumeBoot {
            strA    BootSystemIdentifier[32];
            strA    BootIdentifier[32];
            UBYTE   BootSystemUse[1977];
        } __packed Boot;
        struct isoVolumePrimary {
            UBYTE   Unused_1[1];
            strA    SystemIdentifier[32];
            strD    VolumeIdentifier[32];
            UBYTE   Unused_2[8];
            int32LM VolumeSpaceSize;
            UBYTE   Unused_3[32];
            int16LM VolumeSetSize;
            int16LM VolumeSequenceNumber;
            int16LM LogicalBlockSize;
            int32LM PathTableSize;
            int32L  TypeLPathTable;
            int32L  TypeLOptionalPathTable;
            int32M  TypeMPathTable;
            int32M  TypeMOptionalPathTable;
            UBYTE   RootDirectoryEntry[34];
            strD    VolumeSetIdentifier[128];
            strA    PublisherIdentifier[128];
            strA    DataPreparerIdentifier[128];
            strA    ApplicationIdentifier[128];
            strD    CopyrightFileIdentifier[38];
            strD    AbstractFileIdentifier[36];
            strD    BibliographicFileIdentifier[37];
            ascDate VolumeCreation;
            ascDate VolumeModification;
            ascDate VolumeExpiration;
            ascDate VolumeEffective;
            int8    FileStructureVersion;
            UBYTE   Unused_4[1];
            UBYTE   Application[512];
            UBYTE   Reserved[653];
        } __packed Primary;
        struct {
        } __packed Terminator;
    };
};

struct isoPathTable {
    int8    DirectoryIdentifierLength;
    int8    ExtendedAttributeLength;
    int32   ExtentLocation;         /* LBA */
    int16   ParentDirectory;
    strD    DirectoryIdentifier[0];  /* Padded to an even number */
} __packed;

struct isoDirectory {
    int8    DirectoryLength;
    int8    ExtendedAttributeLength;
    int32LM ExtentLocation;         /* LBA */
    int32LM DataLength;             /* In bytes */
    binDate RecordingDate;
    int8    Flags;
#define ISO_Directory_HIDDEN    (1 << 0)    /* Hidden file */
#define ISO_Directory_ISDIR     (1 << 1)    /* Is a directrory node */
#define ISO_Directory_ASSOC     (1 << 2)    /* Associated File */
#define ISO_Directory_EXTFORMAT (1 << 3)    /* Extended Attribute has format info */
#define ISO_Directory_EXTPERM   (1 << 4)    /* Extended Attribute has permission info */
#define ISO_Directory_CONTINUED (1 << 5)    /* File continued in another entry */
    int8    InterleaveSize;
    int8    InterleaveGap;
    int16LM VolumeSequence;
    int8    FileIdentifierLength;
    strD    FileIdentifier[0];   /* Padded to an even length */
    /* System Use area follows the FileIdentifier */
} __packed;

/* Rock-Ridge and Amiga extensions */

struct rrSystemUse {
    int16   Signature;          /* Always treat as MSB */
    int8    Length;
    int8    Version;
    union {
#define RR_SystemUse_AS            0x4153  /* Amiga */
#define RR_SystemUse_AS_VERSION    1
        struct {
            int8    Flags;
#define RR_AS_PROTECTION          (1 << 0)    /* Protection bits present */
#define RR_AS_COMMENT             (1 << 1)    /* Comment field present */
#define RR_AS_COMMENT_CONTINUE    (1 << 2)    /* Comment continues in next SUA */
            UBYTE   Data[];
        } __packed AS;
#define RR_SystemUse_PX            0x5058  /* POSIX */
#define RR_SystemUse_PX_VERSION    1
        struct {
            int32LM st_mode;
#define S_IRUSR     0000400
#define S_IWUSR     0000200
#define S_IXUSR     0000100
#define S_IRGRP     0000040
#define S_IWGRP     0000020
#define S_IXGRP     0000010
#define S_IROTH     0000004
#define S_IWOTH     0000002
#define S_IXOTH     0000001
#define S_ISUID     0004000
#define S_ISGID     0002000
#define S_ENFMT     0002000
#define S_ISVTX     0001000
#define S_IFSOCK    0140000
#define S_IFLINK    0120000
#define S_IFREG     0100000
#define S_IFBLK     0060000
#define S_IFCHR     0020000
#define S_IFDIR     0040000
#define S_IFIFO     0010000
            int32LM st_nlink;
            int32LM st_uid;
            int32LM st_gid;
            int32LM st_ino;
        } __packed PX;
#define RR_SystemUse_PN            0x504e
#define RR_SystemUse_PN_VERISON    1
        struct {
            int32LM dev_t_High;
            int32LM dev_t_Low;
        } __packed PN;
#define RR_SystemUse_SL            0x534c
#define RR_SystemUse_SL_VERSION    1
        struct {
            int8    Flags;
#define RR_SL_CONTINUE              (1 << 0)
            union {
                TEXT    Component[0];
                struct rrSL_Component {
                    int8    Flags;
#define RR_SL_COMPONENT_CONTINUE    (1 << 0)
#define RR_SL_COMPONENT_CURRENT     (1 << 1)
#define RR_SL_COMPONENT_PARENT      (1 << 2)
#define RR_SL_COMPONENT_ROOT        (1 << 3)
#define RR_SL_COMPONENT_VOLUME      (1 << 4)    /* Obsolete */
#define RR_SL_COMPONENT_HOSTNAME    (1 << 5)    /* Obsolete */
                    int8    ContentLength;
                    TEXT    Content[0];
                } __packed SL_Component;
            };
        } __packed SL;
#define RR_SystemUse_NM            0x4e4d
#define RR_SystemUse_NM_VERSION    1
        struct {
            int8    Flags;
#define RR_NM_COMPONENT_CONTINUE    (1 << 0)
#define RR_NM_COMPONENT_CURRENT     (1 << 1)
#define RR_NM_COMPONENT_PARENT      (1 << 2)
            TEXT Content[0];
        } __packed NM;
#define RR_SystemUse_CL            0x434c
#define RR_SystemUse_CL_VERSION    1
        struct {
            int32LM ChildDirectory;      /* LBA */
        } __packed CL;
#define RR_SystemUse_PL            0x504c
#define RR_SystemUse_PL_VERSION    1
        struct {
            int32LM ParentDirectory;    /* LBA */
        } __packed PL;
#define RR_SystemUse_RE            0x524c
#define RR_SystemUse_RE_VERSION    1
        struct {
        } __packed RE;
#define RR_SystemUse_TF            0x5446
#define RR_SystemUse_TF_VERSION    1
        struct {
            int8    Flags;
#define RR_TF_CREATION              (1 << 0)
#define RR_TF_MODIFY                (1 << 1)
#define RR_TF_ACCESS                (1 << 2)
#define RR_TF_ATTRIBUTES            (1 << 3)
#define RR_TF_BACKUP                (1 << 4)
#define RR_TF_EXPIRATION            (1 << 5)
#define RR_TF_EFFECTIVE             (1 << 6)
#define RR_TF_LONG_FORM             (1 << 7)
            UBYTE   Data[]; /* Array of ascDate or binDate format */
        } __packed TF;
#define RR_SystemUse_SF            0x5346
#define RR_SystemUse_SF_VERSION    1
        struct {
            int32LM st_size_High;
            int32LM st_size_Low;
            int8    TableDepth;
        } __packed SF;
    };
};


struct rrSparseFileTable {
    int32LM TableEntry[256];
#define RR_SFT_BLOCK(x)     ((x) & 0x00ffffff)  /* LBA */
#define RR_SFT_TABLE        (1 << 30)
#define RR_SFT_EMPTY        (1 << 31)
} __packed;



struct SystemUseASProtection {
    UBYTE   User;
    UBYTE   Zero;
    UBYTE   MultiUser;
    UBYTE   Protection;
} __packed;

struct SystemUseASComment {
    UBYTE   Length;
    TEXT    Comment[];
} __packed;

extern const struct CDFSOps ISO9660_Ops;

#endif /* ISO9660_H */
