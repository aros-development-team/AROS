#ifndef DEVICES_TRACKDISK_H
#define DEVICES_TRACKDISK_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for trackdisk.device
    Lang: english
*/

#ifndef EXEC_DEVICE_H
#   include <exec/devices.h>
#endif
#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif

#define TD_NAME "trackdisk.device"

/* OpenDevice() Flags */
#define TDB_ALLOW_NON_3_5     0
#define TDF_ALLOW_NON_3_5 (1<<0)

struct IOExtTD
{
    struct IOStdReq iotd_Req;
    ULONG           iotd_Count;
    ULONG           iotd_SecLabel;
};

/* io_Flags */
#define IOTDB_INDEXSYNC     4
#define IOTDF_INDEXSYNC (1<<4)
#define IOTDB_WORDSYNC      5
#define IOTDF_WORDSYNC  (1<<5)

/* trackdisk.device specific commands */
#define TD_MOTOR        (CMD_NONSTD + 0)
#define TD_SEEK         (CMD_NONSTD + 1)
#define TD_FORMAT       (CMD_NONSTD + 2)
#define TD_REMOVE       (CMD_NONSTD + 3)
#define TD_CHANGENUM    (CMD_NONSTD + 4)
#define TD_CHANGESTATE  (CMD_NONSTD + 5)
#define TD_PROTSTATUS   (CMD_NONSTD + 6)
#define TD_RAWREAD      (CMD_NONSTD + 7)
#define TD_RAWWRITE     (CMD_NONSTD + 8)
#define TD_GETDRIVETYPE (CMD_NONSTD + 9) /* see below */
#define TD_GETNUMTRACKS (CMD_NONSTD + 10)
#define TD_ADDCHANGEINT (CMD_NONSTD + 11)
#define TD_REMCHANGEINT (CMD_NONSTD + 12)
#define TD_GETGEOMETRY  (CMD_NONSTD + 13) /* returns (DriveGeometry *) */
#define TD_EJECT        (CMD_NONSTD + 14)
#define TD_LASTCOMM     (CMD_NONSTD + 15)

/* extended commands */
#define TDF_EXTCOM (1<<15)
#define ETD_READ     (CMD_READ     | TDF_EXTCOM)
#define ETD_WRITE    (CMD_WRITE    | TDF_EXTCOM)
#define ETD_UPDATE   (CMD_UPDATE   | TDF_EXTCOM)
#define ETD_CLEAR    (CMD_CLEAR    | TDF_EXTCOM)
#define ETD_MOTOR    (TD_MOTOR     | TDF_EXTCOM)
#define ETD_SEEK     (TD_SEEK      | TDF_EXTCOM)
#define ETD_FORMAT   (TD_FORMAT    | TDF_EXTCOM)
#define ETD_RAWREAD  (TD_RAWREAD   | TDF_EXTCOM)
#define ETD_RAWWRITE (TD_RAWWRITE  | TDF_EXTCOM)

/* TD_GETDRIVETYPE */
#define DRIVE3_5        1
#define DRIVE3_25       2
#define DRIVE3_5_150RPM 3

struct DriveGeometry
{
    ULONG dg_SectorSize;
    ULONG dg_TotalSectors;
    ULONG dg_Cylinders;
    ULONG dg_CylSectors;
    ULONG dg_Heads;
    ULONG dg_TrackSectors;
    ULONG dg_BufMemType;
    UBYTE dg_DeviceType;   /* see below */
    UBYTE dg_Flags;        /* see below */
    UWORD dg_Reserved;
};

/* dg_DeviceType */
#define DG_DIRECT_ACCESS     0
#define DG_SEQUENTIAL_ACCESS 1
#define DG_PRINTER           2
#define DG_PROCESSOR         3
#define DG_WORM              4
#define DG_CDROM             5
#define DG_SCANNER           6
#define DG_OPTICAL_DISK      7
#define DG_MEDIUM_CHANGER    8
#define DG_COMMUNICATION     9
#define DG_UNKNOWN           31

/* dg_Flags */
#define DGB_REMOVABLE     0
#define DGF_REMOVABLE (1<<0)

struct TDU_PublicUnit
{
    struct Unit tdu_Unit;

    UWORD tdu_Comp01Track;
    UWORD tdu_Comp10Track;
    UWORD tdu_Comp11Track;
    ULONG tdu_StepDelay;
    ULONG tdu_SettleDelay;
    UBYTE tdu_RetryCnt;
    UBYTE tdu_PubFlags;       /* see below */
    UWORD tdu_CurrTrk;
    ULONG tdu_CalibrateDelay;
    ULONG tdu_Counter;
};

/* tdu_PubFlags */
#define TDPB_NOCLICK      0
#define TDPF_NOCLICK (1L<<0)

#define TD_LABELSIZE 16
#define TD_SECTOR 512
#define TD_SECSHIFT 9

#define TDERR_NotSpecified   20
#define TDERR_NoSecHdr       21
#define TDERR_BadSecPreamble 22
#define TDERR_BadSecID       23
#define TDERR_BadHdrSum      24
#define TDERR_BadSecSum      25
#define TDERR_TooFewSecs     26
#define TDERR_BadSecHdr      27
#define TDERR_WriteProt      28
#define TDERR_DiskChanged    29
#define TDERR_SeekError      30
#define TDERR_NoMem          31
#define TDERR_BadUnitNum     32
#define TDERR_BadDriveType   33
#define TDERR_DriveInUse     34
#define TDERR_PostReset      35

#endif /* DEVICES_TRACKDISK_H */
