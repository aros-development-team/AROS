#ifndef _HARDWARE_ATA_H
#define _HARDWARE_ATA_H

/*
    Copyright © 2004-2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: ATA hardware register definitions
    Lang: English
*/

/* Registers */
#define ata_Error                               1
#define ata_Feature                             1
#define ata_Count                               2
#define ata_LBALow                              3
#define ata_Sector                              3
#define ata_LBAMid                              4
#define ata_CylinderLow                         4
#define ata_LBAHigh                             5
#define ata_CylinderHigh                        5
#define ata_DevHead                             6
#define ata_Status                              7
#define ata_Command                             7
#define ata_AltStatus                           0x2
#define ata_AltControl                          0x2

#define atapi_Error                             1
#define atapi_Features                          1
#define atapi_Reason                            2
#define atapi_ByteCntL                          4
#define atapi_ByteCntH                          5
#define atapi_DevSel                            6
#define atapi_Status                            7
#define atapi_Command                           7

/* Status bits */
#define ATAB_SLAVE                              4
#define ATAB_LBA                                6
#define ATAB_ATAPI                              7
#define ATAB_DATAREQ                            3
#define ATAB_ERROR                              0
#define ATAB_BUSY                               7

#define ATAF_SLAVE                              0x10
#define ATAF_LBA                                0x40
#define ATAF_ATAPI                              0x80
#define ATAF_DATAREQ                            0x08
#define ATAF_ERROR                              0x01
#define ATAF_BUSY                               0x80
#define ATAF_DRDY                               0x40

#define ATAPIF_CHECK                            0x01

/* Commands */
#define	ATA_RECALIBRATE                         0x10

#define ATA_IDENTIFY_DEVICE                     0xEC
#define ATA_CHECK_POWER_MODE                    0xE5
#define ATA_STANDBY                             0xE2
#define ATA_STANDBY_IMMED                       0xE0
#define	ATA_STANDBY_IMMEDIATE                   ATA_STANDBY_IMMED
#define ATA_IDLE_IMMED                          0xE1
#define	ATA_IDLE_IMMEDIATE                      ATA_IDLE_IMMED
#define ATA_IDLE                                0xE3
#define ATA_FLUSH_CACHE                         0xE7
#define ATA_FLUSH_CACHE_EXT                     0xEA
#define ATA_READ_DMA_EXT                        0x25
#define ATA_READ_DMA64                          ATA_READ_DMA_EXT
#define ATA_READ_DMA                            0xC8
#define ATA_READ_SECTORS_EXT                    0x24
#define ATA_READ64                              ATA_READ_SECTORS_EXT
#define ATA_READ_SECTORS                        0x20
#define ATA_READ                                ATA_READ_SECTORS
#define ATA_WRITE_DMA_EXT                       0x35
#define ATA_WRITE_DMA64                         ATA_WRITE_DMA_EXT
#define ATA_WRITE_DMA                           0xCA
#define ATA_WRITE_SECTORS_EXT                   0x34
#define ATA_WRITE64                             ATA_WRITE_SECTORS_EXT
#define ATA_WRITE_SECTORS                       0x30
#define ATA_WRITE                               ATA_WRITE_SECTORS
#define ATA_WRITE_UNCORRECTABLE                 0x45
#define ATA_READ_VERIFY_SECTORS                 0x40
#define ATA_READ_VERIFY_SECTORS_EXT             0x42
#define ATA_READ_BUFFER                         0xE4
#define ATA_WRITE_BUFFER                        0xE8
#define ATA_EXECUTE_DEVICE_DIAG                 0x90
#define ATA_EXECUTE_DIAG                        ATA_EXECUTE_DEVICE_DIAG
#define ATA_SET_FEATURES                        0xEF
#define ATA_SMART                               0xB0
#define ATA_PACKET_IDENTIFY                     0xA1
#define ATA_IDENTIFY_ATAPI                      ATA_PACKET_IDENTIFY
#define ATA_PACKET                              0xA0
#define ATA_READ_FPDMA                          0x60
#define ATA_WRITE_FPDMA                         0x61
#define ATA_READ_LOG_EXT                        0x2F
#define ATA_NOP                                 0x00
#define ATA_DEVICE_RESET                        0x08
#define ATA_MEDIA_EJECT                         0xED
#define ATA_SECURITY_UNLOCK                     0xF2
#define ATA_SECURITY_FREEZE_LOCK                0xF5
#define ATA_DATA_SET_MANAGEMENT                 0x06
#define ATA_DOWNLOAD_MICROCODE                  0x92
#define ATA_WRITE_STREAM_DMA_EXT                0x3A
#define ATA_READ_LOG_DMA_EXT                    0x47
#define ATA_READ_STREAM_DMA_EXT                 0x2A
#define ATA_WRITE_DMA_FUA                       0x3D
#define ATA_WRITE_LOG_DMA_EXT                   0x57
#define ATA_READ_DMA_QUEUED                     0xC7
#define ATA_READ_DMA_QUEUED_EXT                 0x26
#define ATA_WRITE_DMA_QUEUED                    0xCC
#define ATA_WRITE_DMA_QUEUED_EXT                0x36
#define ATA_WRITE_DMA_QUEUED_FUA_EXT            0x3E
#define ATA_SET_MULTIPLE                        0XC6
#define ATA_READ_MULTIPLE                       0xC4
#define ATA_READ_MULTIPLE_EXT                   0x29
#define ATA_READ_MULTIPLE64                     ATA_READ_MULTIPLE_EXT
#define ATA_WRITE_MULTIPLE                      0xC5
#define ATA_WRITE_MULTIPLE_EXT                  0x39
#define ATA_WRITE_MULTIPLE64                    ATA_WRITE_MULTIPLE_EXT
#define ATA_WRITE_MULTIPLE_FUA_EXT              0xCE

#define ATA_DEVICE_CONFIG_IDENTIFY              0xB1
#define ATA_DEVICE_CONFIG_ID_FEATURES           0xC2


/* SET_FEATURES sub-commands */
#define ATA_SET_FEATURES_ENABLE_CACHE           0x02
#define ATA_SET_FEATURES_DISABLE_CACHE          0x82
#define ATA_SET_FEATURES_DISABLE_READ_AHEAD     0x55
#define ATA_SET_FEATURES_ENABLE_READ_AHEAD      0xAA
#define ATA_SET_FEATURES_SET_TRANSFER_MODE      0x03

/* ATAPI reason flags */
#define ATAPIF_MASK                             0x03
#define ATAPIF_COMMAND                          0x01
#define ATAPIF_READ                             0x02
#define ATAPIF_WRITE                            0x00

/* AltControl bits */
#define ATACTLF_INT_DISABLE                     0x02
#define ATACTLF_RESET                           0x04

#endif
