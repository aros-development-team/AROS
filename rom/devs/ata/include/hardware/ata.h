#ifndef _HARDWARE_ATA_H
#define _HARDWARE_ATA_H

/*
    Copyright © 2004-2013, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATA hardware register definitions
    Lang: English
*/

/* Registers */
#define ata_Error           1
#define ata_Feature         1
#define ata_Count           2
#define ata_LBALow          3
#define ata_Sector          3
#define ata_LBAMid          4
#define ata_CylinderLow     4
#define ata_LBAHigh         5
#define ata_CylinderHigh    5
#define ata_DevHead         6
#define ata_Status          7
#define ata_Command         7
#define ata_AltStatus       0x2
#define ata_AltControl      0x2

#define atapi_Error         1
#define atapi_Features      1
#define atapi_Reason        2
#define atapi_ByteCntL      4
#define atapi_ByteCntH      5
#define atapi_DevSel        6
#define atapi_Status        7
#define atapi_Command       7

/* Status bits */
#define ATAB_SLAVE          4
#define ATAB_LBA            6
#define ATAB_ATAPI          7
#define ATAB_DATAREQ        3
#define ATAB_ERROR          0
#define ATAB_BUSY           7

#define ATAF_SLAVE          0x10
#define ATAF_LBA            0x40
#define ATAF_ATAPI          0x80
#define ATAF_DATAREQ        0x08
#define ATAF_ERROR          0x01
#define ATAF_BUSY           0x80
#define ATAF_DRDY           0x40

#define ATAPIF_CHECK        0x01

/* Commands */
#define ATA_SET_FEATURES    0xef
#define ATA_SET_MULTIPLE    0xc6
#define ATA_DEVICE_RESET    0x08
#define ATA_IDENTIFY_DEVICE 0xec
#define ATA_IDENTIFY_ATAPI  0xa1
#define ATA_NOP             0x00
#define ATA_EXECUTE_DIAG    0x90
#define ATA_PACKET          0xa0
#define ATA_READ_DMA        0xc8
#define ATA_READ_DMA64      0x25
#define ATA_READ            0x20
#define ATA_READ64          0x24
#define ATA_READ_MULTIPLE   0xc4
#define ATA_READ_MULTIPLE64 0x29
#define ATA_WRITE_DMA       0xca
#define ATA_WRITE_DMA64     0x35
#define ATA_WRITE           0x30
#define ATA_WRITE64         0x34
#define ATA_WRITE_MULTIPLE  0xc5
#define ATA_WRITE_MULTIPLE64 0x39
#define ATA_MEDIA_EJECT     0xed
#define ATA_SMART           0xB0
#define ATA_SET_FEATURES    0xEF

/* SET_FEATURES sub-commands */
#define ATA_SET_FEATURES_ENABLE_CACHE           0x02
#define ATA_SET_FEATURES_DISABLE_CACHE          0x82
#define ATA_SET_FEATURES_DISABLE_READ_AHEAD     0x55
#define ATA_SET_FEATURES_ENABLE_READ_AHEAD      0xAA
#define ATA_SET_FEATURES_SET_TRANSFER_MODE      0x3

/* ATAPI reason flags */
#define ATAPIF_MASK         0x03
#define ATAPIF_COMMAND      0x01
#define ATAPIF_READ         0x02
#define ATAPIF_WRITE        0x00

/* AltControl bits */
#define ATACTLF_INT_DISABLE 0x02
#define ATACTLF_RESET       0x04

#endif
