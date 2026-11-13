/*
 * Copyright (C) 2013, The AROS Development Team
 * All right reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef CHINON_H
#define CHINON_H

#include <exec/types.h>

/* Chinon O-658-2 24 CD Commands
 * 'msf' is [minutes seconds frame]
 *          ^- These are in BCD --^
 */

#define CHCD_RESET     0       /* Cmd =    [ 0x00 ] 
                                  Result = [ 0x00 ] */
#define CHCD_STOP      1       /* Cmd =    [ 0x01 0x00 ]
                                  Result = [ 0x01 errcode ] */
#define CHCD_PAUSE     2       /* Cmd =    [ 0x02 ]
                                  Result = [ 0x02 errcode ] */
#define CHCD_UNPAUSE   3       /* Cmd =    [ 0x03 ]
                                  Result = [ 0x03 errcode ] */
#define CHCD_MULTI     4       /* Cmd =    [ 0x04 start_msf end_msf subcmd flags ?? audio ?? ]
                                   subcmd: 0x80 - Data read
                                           0x03 - TOC read
                                   flags:  0x40 - Speed (0 = x1, 0x40 = x2)
                                   audio:  0x04 - Play audio
                                  Result = [ 0x04 0x01 ] // No Disk
                                           [ 0x04 0x02 ] // Read in progress
                                           [ 0x04 0x08 ] // Audio playing
                                           [ 0x04 0x80 ] // Error
                                           [ 0x04 0x00 ] // IO complete
                                */
#define CHCD_LED       5       /* Cmd =    [ 0x05 led ]
                                   led: 0x80 - Result wanted
                                        0x01 - LED on/off
                                  Result = [ 0x05 led ] (if led & 0x80)
                                 */
#define CHCD_SUBQ      6       /* Cmd =    [ 0x06 ]
                                  Result = [ 0x06 0x00 0x00 CtlAdr Track Index TrackPos_u24 DiskPos_u32 CurrPos_msf ] */
#define CHCD_STATUS    7       /* Cmd =    [ 0x07 ]
                                  Result = [ 0x07 code FirmwareString[18] ]
                                   code: 0x01 - Door closed, disk present
                                         0x80 - Door open
                                 */
#define CHCD_CMD8       8      /* Cmd =    [ 0x08 ?? ?? ?? ]
                                  Result = [ 0x08 ?? ?? ?? ]
                                */
#define CHCD_CMD9       9      /* Cmd =    [ 0x09 ]
                                  Result = [ 0x09 ?? ?? ?? ]
                                */
/* Received when there is a media change */
#define CHCD_MEDIA      10     /* Result = [ 0x0a status ]
                                *  status: 0x83 - Disk present
                                *          0x80 - Disk absent
                                */

#define CHERR_OK                0x00
#define CHERR_DISKPRESENT       0x01
#define CHERR_READING           0x02
#define CHERR_PLAYING           0x08
#define CHERR_BADCOMMAND        0x80
#define CHERR_CHECKSUM          0x88
#define CHERR_DRAWERSTUCK       0x90
#define CHERR_DISKUNREADABLE    0x98
#define CHERR_INVALIDADDRESS    0xa0
#define CHERR_WRONGDATA         0xa8
#define CHERR_FOCUS             0xc8
#define CHERR_SPINDLE           0xd0
#define CHERR_TRACKING          0xd8
#define CHERR_SLED              0xe0
#define CHERR_TRACKJUMP         0xe8
#define CHERR_ABNORMALSEEK      0xf0
#define CHERR_NODISK            0xf8

struct chcd_msf {
    BYTE minute;        /* In BCD (up to 99) */
    BYTE second;        /* In BCD (up to 59) */
    BYTE frame;         /* In BCD (up to 74) */
};

/* NOTE: There is a checksum following all commands,
 * regardless of length. The checksum is the inverse
 * of the sum of all the bytes of the command.
 */
struct chcd_cmd {
    BYTE cmd;
    union {
        struct {} noop;
        struct {
            BYTE resv;
        } stop;
        struct {} pause;
        struct {} unpause;
        struct {
            struct chcd_msf start;
            struct chcd_msf end;
            BYTE mode;  /* 0x80 for data read */
            BYTE flags; /* 0x40 for 150 frames/sec, 0x00 for 75 frames/sec */
            BYTE resv_1;
            BYTE audio; /* 0x04 to play audio */
            BYTE resv_2;
        } multi;
        struct {
            BYTE state; /* 0x01 to turn on, 0x00 to turn off */
                        /* 0x80 to force a result code */
        } led;
        struct { } subq;
        struct { } status;
    };
};


/* NOTE: There is a checksum following all results,
 * regardless of length. The checksum is the inverse
 * of the sum of all the bytes of the result.
 */
struct chcd_result {
    BYTE cmd;
    BYTE error;
    union {
        struct {} noop;
        struct {} stop;
        struct {} pause;
        struct {} unpause;
        struct {} multi;
        struct {} led;
        struct {
            BYTE resv;
            BYTE ctladr;
            BYTE track;
            BYTE index;
            struct chcd_msf track_position;
            BYTE resv_2;
            struct chcd_msf disk_position;
            BYTE resv_3;
            BYTE resv_4;
        } subq;
        struct {
            BYTE firmware[18];
        } status;
    };
};

#endif /* CHINON_H */
