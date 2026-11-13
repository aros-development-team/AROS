#ifndef _HARDWARE_SCSI_H
#define _HARDWARE_SCSI_H

/*
    Copyright © 2019, The AROS Development Team. All rights reserved.
    $Id$

    Desc: SCSI hardware register definitions
    Lang: English
*/

/* Registers */
#define scsi_Error                               1
#define scsi_Feature                             1
#define scsi_Count                               2
#define scsi_LBALow                              3
#define scsi_Sector                              3
#define scsi_LBAMid                              4
#define scsi_CylinderLow                         4
#define scsi_LBAHigh                             5
#define scsi_CylinderHigh                        5
#define scsi_DevHead                             6
#define scsi_Status                              7
#define scsi_Command                             7
#define scsi_AltStatus                           0x2
#define scsi_AltControl                          0x2

#define scsi_pi_Error                             1
#define scsi_pi_Features                          1
#define scsi_pi_Reason                            2
#define scsi_pi_ByteCntL                          4
#define scsi_pi_ByteCntH                          5
#define scsi_pi_DevSel                            6
#define scsi_pi_Status                            7
#define scsi_pi_Command                           7

/* Status bits */
#define SCSIB_SLAVE                              4
#define SCSIB_LBA                                6
#define SCSIB_ATAPI                              7
#define SCSIB_DATAREQ                            3
#define SCSIB_ERROR                              0
#define SCSIB_BUSY                               7

#define SCSIF_SLAVE                              0x10
#define SCSIF_LBA                                0x40
#define SCSIF_ATAPI                              0x80
#define SCSIF_DATAREQ                            0x08
#define SCSIF_ERROR                              0x01
#define SCSIF_BUSY                               0x80
#define SCSIF_DRDY                               0x40

#define SCSIPIF_CHECK                            0x01

/* Commands */
#define	SCSI_RECALIBRATE                         0x10

#define SCSI_IDENTIFY_DEVICE                     0xEC
#define SCSI_CHECK_POWER_MODE                    0xE5
#define SCSI_STANDBY                             0xE2
#define SCSI_STANDBY_IMMED                       0xE0
#define	SCSI_STANDBY_IMMEDIATE                   SCSI_STANDBY_IMMED
#define SCSI_IDLE_IMMED                          0xE1
#define	SCSI_IDLE_IMMEDIATE                      SCSI_IDLE_IMMED
#define SCSI_IDLE                                0xE3
#define SCSI_FLUSH_CACHE                         0xE7
#define SCSI_FLUSH_CACHE_EXT                     0xEA
#define SCSI_READ_DMA_EXT                        0x25
#define SCSI_READ_DMA64                          SCSI_READ_DMA_EXT
#define SCSI_READ_DMA                            0xC8
#define SCSI_READ_SECTORS_EXT                    0x24
#define SCSI_READ64                              SCSI_READ_SECTORS_EXT
#define SCSI_READ_SECTORS                        0x20
#define SCSI_READ                                SCSI_READ_SECTORS
#define SCSI_WRITE_DMA_EXT                       0x35
#define SCSI_WRITE_DMA64                         SCSI_WRITE_DMA_EXT
#define SCSI_WRITE_DMA                           0xCA
#define SCSI_WRITE_SECTORS_EXT                   0x34
#define SCSI_WRITE64                             SCSI_WRITE_SECTORS_EXT
#define SCSI_WRITE_SECTORS                       0x30
#define SCSI_WRITE                               SCSI_WRITE_SECTORS
#define SCSI_WRITE_UNCORRECTABLE                 0x45
#define SCSI_READ_VERIFY_SECTORS                 0x40
#define SCSI_READ_VERIFY_SECTORS_EXT             0x42
#define SCSI_READ_BUFFER                         0xE4
#define SCSI_WRITE_BUFFER                        0xE8
#define SCSI_EXECUTE_DEVICE_DIAG                 0x90
#define SCSI_EXECUTE_DIAG                        SCSI_EXECUTE_DEVICE_DIAG
#define SCSI_SET_FEATURES                        0xEF
#define SCSI_SMART                               0xB0
#define SCSI_PACKET_IDENTIFY                     0xA1
#define SCSI_IDENTIFY_ATAPI                      SCSI_PACKET_IDENTIFY
#define SCSI_PACKET                              0xA0
#define SCSI_READ_FPDMA                          0x60
#define SCSI_WRITE_FPDMA                         0x61
#define SCSI_READ_LOG_EXT                        0x2F
#define SCSI_NOP                                 0x00
#define SCSI_DEVICE_RESET                        0x08
#define SCSI_MEDIA_EJECT                         0xED
#define SCSI_SECURITY_UNLOCK                     0xF2
#define SCSI_SECURITY_FREEZE_LOCK                0xF5
#define SCSI_DATA_SET_MANAGEMENT                 0x06
#define SCSI_DOWNLOAD_MICROCODE                  0x92
#define SCSI_WRITE_STREAM_DMA_EXT                0x3A
#define SCSI_READ_LOG_DMA_EXT                    0x47
#define SCSI_READ_STREAM_DMA_EXT                 0x2A
#define SCSI_WRITE_DMA_FUA                       0x3D
#define SCSI_WRITE_LOG_DMA_EXT                   0x57
#define SCSI_READ_DMA_QUEUED                     0xC7
#define SCSI_READ_DMA_QUEUED_EXT                 0x26
#define SCSI_WRITE_DMA_QUEUED                    0xCC
#define SCSI_WRITE_DMA_QUEUED_EXT                0x36
#define SCSI_WRITE_DMA_QUEUED_FUA_EXT            0x3E
#define SCSI_SET_MULTIPLE                        0XC6
#define SCSI_READ_MULTIPLE                       0xC4
#define SCSI_READ_MULTIPLE_EXT                   0x29
#define SCSI_READ_MULTIPLE64                     SCSI_READ_MULTIPLE_EXT
#define SCSI_WRITE_MULTIPLE                      0xC5
#define SCSI_WRITE_MULTIPLE_EXT                  0x39
#define SCSI_WRITE_MULTIPLE64                    SCSI_WRITE_MULTIPLE_EXT
#define SCSI_WRITE_MULTIPLE_FUA_EXT              0xCE

#define SCSI_DEVICE_CONFIG_IDENTIFY              0xB1
#define SCSI_DEVICE_CONFIG_ID_FEATURES           0xC2


/* SET_FEATURES sub-commands */
#define SCSI_SET_FEATURES_ENABLE_CACHE           0x02
#define SCSI_SET_FEATURES_DISABLE_CACHE          0x82
#define SCSI_SET_FEATURES_DISABLE_READ_AHEAD     0x55
#define SCSI_SET_FEATURES_ENABLE_READ_AHEAD      0xAA
#define SCSI_SET_FEATURES_SET_TRANSFER_MODE      0x03

/* ATAPI reason flags */
#define ATAPIF_MASK                             0x03
#define ATAPIF_COMMAND                          0x01
#define ATAPIF_READ                             0x02
#define ATAPIF_WRITE                            0x00

/* AltControl bits */
#define SCSICTLF_INT_DISABLE                     0x02
#define SCSICTLF_RESET                           0x04

#endif
