#ifndef DEVICES_CD_H
#define DEVICES_CD_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for cd.device and CD drivers
    Lang: english
*/

/* CD device error codes */

#ifndef EXEC_IO_H
#include <exec/io.h>
#endif
#ifndef DEVICES_TRACKDISK_H
#include <devices/trackdisk.h>
#endif

#define CDERR_OPENFAIL      (-1)
#define CDERR_ABORTED       (-2)
#define CDERR_NOCMD         (-3)
#define CDERR_BADLENGTH     (-4)
#define CDERR_BADADDRESS    (-5)
#define CDERR_UNITBUSY      (-6)
#define CDERR_SELFTEST      (-7)

#define CDERR_NotSpecified      20
#define CDERR_NoSecHdr          21
#define CDERR_BadSecPreamble    22
#define CDERR_BadSecID          23
#define CDERR_BadHdrSum         24
#define CDERR_BadSecSum         25
#define CDERR_TooFewSecs        26
#define CDERR_BadSecHdr         27
#define CDERR_WriteProt         28
#define CDERR_NoDisk            29
#define CDERR_SeekError         30
#define CDERR_NoMem             31
#define CDERR_BadUnitNum        32
#define CDERR_BadDriveType      33
#define CDERR_DriveInUse        34
#define CDERR_PostReset         35
#define CDERR_BadDataType       36
#define CDERR_InvalidState      37

#define CDERR_Phase             42
#define CDERR_NoBoard           50

#define CD_RESET        CMD_RESET
#define CD_READ         CMD_READ
#define CD_WRITE        CMD_WRITE
#define CD_UPDATE       CMD_UPDATE
#define CD_CLEAR        CMD_CLEAR
#define CD_STOP         CMD_STOP
#define CD_START        CMD_START
#define CD_FLUSH        CMD_FLUSH
#define CD_MOTOR        TD_MOTOR
#define CD_SEEK         TD_SEEK
#define CD_FORMAT       TD_FORMAT
#define CD_REMOVE       TD_REMOVE
#define CD_CHANGENUM    TD_CHANGENUM
#define CD_CHANGESTATE  TD_CHANGESTATE
#define CD_PROTSTATUS   TD_PROTSTATUS
#define CD_GETDRIVETYPE TD_GETDRIVETYPE
#define CD_GETNUMTRACKS TD_GETNUMTRACKS
#define CD_ADDCHANGEINT TD_ADDCHANGEINT
#define CD_REMCHANGEINT TD_REMCHANGEINT
#define CD_GETGEOMETRY  TD_GETGEOMETRY
#define CD_EJECT        TD_EJECT

#define CD_INFO         32      /* io_Data   - Pointer to struct CDInfo
                                 * io_Length - sizeof(struct CDInfo)
                                 */

struct CDInfo {
    UWORD       PlaySpeed;      /* In frames/second (75 = x1, 150 = x2, etc) */
    UWORD       ReadSpeed;      /* In frames/second */
    UWORD       ReadXLSpeed;    /* In frames/second */
    UWORD       SectorSize;     /* 2048 or 2328 */
    UWORD       XLECC;          /* 1 = CDXL ECC enabled, 0 = CDXL ECC disabled */
    UWORD       EjectReset;     /* 1 = Reset on eject */
    UWORD       Reserved1[4];
    UWORD       MaxSpeed;       /* Maximum frames/second for the drive */
    UWORD       AudioPrecision; /* 0 = no atten., 1 = mute only, N = levels */
    UWORD       Status;         /* See CDSTSB_* flags */
    UWORD       Reserved2[4];
};

#define CDSTSB_CLOSED   0       /* Drive closed */
#define CDSTSB_DISK     1       /* Disk present */
#define CDSTSB_SPIN     2       /* Disk spinning */
#define CDSTSB_TOC      3       /* TOC read */
#define CDSTSB_CDROM    4       /* Track 1 is data Mode 1 */
#define CDSTSB_PLAYING  5       /* Playing audio track */
#define CDSTSB_PAUSED   6       /* Playing paused */
#define CDSTSB_SEARCH   7       /* Playing fast forward/fast reverse */
#define CDSTSB_DIRECTION 8      /* Playing forward (0) or reverse (1) */

#define CDSTSF_CLOSED   (1 << CDSTSB_CLOSED)
#define CDSTSF_DISK     (1 << CDSTSB_DISK)
#define CDSTSF_SPIN     (1 << CDSTSB_SPIN)
#define CDSTSF_TOC      (1 << CDSTSB_TOC)
#define CDSTSF_CDROM    (1 << CDSTSB_CDROM)
#define CDSTSF_PLAYING  (1 << CDSTSB_PLAYING)
#define CDSTSF_PAUSED   (1 << CDSTSB_PAUSED)
#define CDSTSF_SEARCH   (1 << CDSTSB_SEARCH)
#define CDSTSF_DIRECTION (1 << CDSTSB_DIRECTION)

#define CD_CONFIG       33      /* io_Data   - Pointer to TagItem array
                                 * io_Length - 0
                                 */
/* NOTE: TAG_IGNORE, TAG_MORE, and TAG_SKIP are illegal in this command! */
#define TAGCD_DONE              0       /* Last tag */
#define TAGCD_PLAYSPEED         1       /* in frames/second */
#define TAGCD_READSPEED         2       /* in frames/second */
#define TAGCD_READXLSPEED       3       /* in frames/second */
#define TAGCD_SECTORSIZE        4       /* 2048 or 2328 */
#define TAGCD_XLECC             5       /* 0 or 1 */
#define TAGCD_EJECTRESET        6       /* 0 or 1 */

#define CD_TOCMSF       34      
#define CD_TOCLSN       35      /* io_Data   - union CDTOC array
                                 * io_Length - # of CDTOC array entries.
                                 * io_Offset - Starting entry (0 for summary)
                                 */

union LSNMSF {
    struct RMSF {
        UBYTE   Reserved;       /* Always 0 */
        UBYTE   Minute;         /* Minutes (0..99) */
        UBYTE   Second;         /* Seconds (0..59) */
        UBYTE   Frame;          /* Frame   (0..74) */
    } MSF;
    ULONG LSN;                  /* Logical sector number */
};

union CDTOC {
    struct TOCSummary {         /* Entry 0 */
        UBYTE   FirstTrack;     /* ID of first track (almost always 1) */
        UBYTE   LastTrack;      /* Last track on the disk */
        union LSNMSF LeadOut;   /* Beginning of lead-out */
    } Summary;
    struct TOCEntry {           /* All other entries */
        UBYTE   CtlAdr;         /* Q-Code */
        UBYTE   Track;          /* Track Number */
        union LSNMSF Position;
    } Entry;
};

#define CD_READXL       36      /* io_Data   - Pointer to CDXL node's List
                                 * io_Length - Maximum transfer length (or 0)
                                 * io_Offset - Byte offset into Mode 1 Track 1
                                 */

#define CD_READXL_INTP(n, cdxl, data) \
    AROS_UFP3(VOID, n, \
            AROS_UFPA(struct CDXL *, cdxl, A2), \
            AROS_UFPA(APTR, data, A4), \
            AROS_UFPA(struct ExecBase *, __cxdl_SysBase, A6))

#define CD_READXL_INTC(n, cdxl, data) \
    AROS_UFC3(VOID, n, \
            AROS_UFCA(struct CDXL *, cdxl, A2), \
            AROS_UFCA(APTR, data, A4), \
            AROS_UFCA(struct ExecBase *, SysBase, A6))

#define CD_READXL_INTH(n, cdxl, data) \
    AROS_UFH3(VOID, n, \
            AROS_UFHA(struct CDXL *, cdxl, A2), \
            AROS_UFHA(APTR, data, A4), \
            AROS_UFHA(struct ExecBase *, SysBase, A6))

#define CD_READXL_INTFUNC_INIT  AROS_USERFUNC_INIT
#define CD_READXL_INTFUNC_EXIT  AROS_USERFUNC_EXIT

struct CDXL {
    struct MinNode      Node;
    BYTE               *Buffer;
    LONG                Length;
    LONG                Actual;
    APTR                IntData;        /* 'data' parameter */
    APTR                IntCode;        /* CD_READXL_INTH() function */
};
    
#define CD_PLAYTRACK    37      /* io_Length - Number of track to play
                                 * io_Offset - Starting track
                                 */
#define CD_PLAYMSF      38      /* io_Length - MSF length to play
                                 * io_Offset - MSF start
                                 */
#define CD_PLAYLSN      39      /* io_Length - Frames to play
                                 * io_Offset - Starting frame
                                 */
#define CD_PAUSE        40      /* io_Length - 0 = unpause, 1 = pause */

#define CD_SEARCH       41      /* io_Length - 0 (normal), 1 (FFW), 2 (FREV) */

/* Modes for CD_SEARCH's io_Length parameter: */
#define CDMODE_NORMAL   0       /* Play normally */
#define CDMODE_FFWD     1       /* Play fast forward */
#define CDMODE_FREV     2       /* Play fast reverse */

#define CD_QCODEMSF     42
#define CD_QCODELSN     43      /* io_Data - Pointer to struct QCode
                                 * io_Length - sizeof(struct QCode)
                                 */
struct QCode {
    UBYTE       CtlAdr;
    UBYTE       Track;
    UBYTE       Index;
    UBYTE       Zero;
    union LSNMSF TrackPosition;
    union LSNMSF DiskPosition;
};

#define CD_ATTENUATE    44      /* io_Length - Duration of fade in frames 
                                 * io_Offset - Target volume (0 - 0x7FFF, or -1)
                                 * -> io_Actual - Current volume
                                 */

#define CD_ADDFRAMEINT  45      /* io_Data   - struct Interrupt
                                 * io_Length - sizeof(struct Interrupt)
                                 */
#define CD_REMFRAMEINT  46


#endif /* DEVICES_CD_H */
