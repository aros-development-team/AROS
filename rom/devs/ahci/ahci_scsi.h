/*
 * Copyright (C) 2012, The AROS Development Team.  All rights reserved.
 * Author: Jason S. McMullan <jason.mcmullan@gmail.com>
 *
 * Licensed under the AROS PUBLIC LICENSE (APL) Version 1.1
 */

#ifndef AHCI_SCSI_H
#define AHCI_SCSI_H

#include <exec/io.h>

#include <scsi/commands.h>

struct scsi_vpd_supported_page_list {
    UBYTE device;
    UBYTE page_code;
#define SVPD_SUPPORTED_PAGE_LIST  0
#define SVPD_SUPPORTED_PAGES_HDR_LEN 4
    UBYTE resv;
    UBYTE length;
#define SVPD_SUPPORTED_PAGES_SIZE 251
    UBYTE list[SVPD_SUPPORTED_PAGES_SIZE];
};

struct scsi_vpd_unit_serial_number {
    UBYTE device;
    UBYTE page_code;
#define SVPD_UNIT_SERIAL_NUMBER 0x80
    UBYTE resv;
    UBYTE length;
#define SVPD_SERIAL_NUM_SIZE 251
    UBYTE serial_num[SVPD_SERIAL_NUM_SIZE];
};

struct scsi_vpd_unit_devid {
    UBYTE device;
    UBYTE page_code;
#define SVPD_UNIT_DEVID 0x83
#define SVPD_UNIT_DEVID_MAX_SIZE 252
    UBYTE length[2];
    UBYTE desc_list[];
};

#define SHORT_INQUIRY_LENGTH 36

struct scsi_generic {
    UBYTE opcode;
    UBYTE resv[5];
};
struct scsi_inquiry {
    UBYTE opcode;
    UBYTE byte2;
#define SI_EVPD (1 << 0)
    UBYTE page_code;
    UBYTE length;
    UBYTE control;
};

struct scsi_inquiry_data {
#define T_DIRECT        0x00
    UBYTE device;
    UBYTE dev_qual2;
    UBYTE version;
#define SCSI_REV_SPC2   4
    UBYTE response_format;
    UBYTE additional_length;
    UBYTE spc3_flags;
    UBYTE spc2_flags;
    UBYTE flags;
#define SID_VENDOR_SIZE 8
    UBYTE vendor[SID_VENDOR_SIZE];
#define SID_PRODUCT_SIZE 16
    UBYTE product[SID_PRODUCT_SIZE];
#define SID_REVISION_SIZE 4
    UBYTE revision[SID_REVISION_SIZE];
    UBYTE spi3data;
    UBYTE reserved2;
    UBYTE version1[2];
    UBYTE version2[2];
    UBYTE version3[2];
    UBYTE version4[2];
    UBYTE version5[2];
    UBYTE version6[2];
    UBYTE version7[2];
    UBYTE version8[2];
    UBYTE reserved3[22];
#define SID_VENDOR_SPECIFIC_1_SIZE 160
    UBYTE vendor_specific[SID_VENDOR_SPECIFIC_1_SIZE];
};

struct scsi_sense_data {
    UBYTE error_code;
#define SSD_ERRCODE                     0x7f
#define         SSD_CURRENT_ERROR       0x70
#define         SSD_DEFERRED_ERROR      0x71
#define SSD_ERRCODE_VALID               0x80
#define SSD_FULL_SIZE 252
    UBYTE segment;
    UBYTE flags;
#define SSD_KEY                 0x0f
#define         SSD_KEY_MEDIUM_ERROR    0x01
#define         SSD_KEY_NOT_READY       0x02
#define         SSD_KEY_ILLEGAL_REQUEST 0x05
#define SSD_ILI                 0x20
#define SSD_EOM                 0x40
#define SSD_FILEMARK            0x80
    UBYTE info[4];
    UBYTE extra_len;
    UBYTE cmd_spec_info[4];
    UBYTE add_sense_code;
    UBYTE add_sense_qual;
    UBYTE fru;
    UBYTE sense_key_spec[3];
    UBYTE extra_bytes[14];
};

/* Commands */
struct scsi_sense {
    UBYTE opcode;
    UBYTE byte2;
    UBYTE unused[2];
    UBYTE length;
    UBYTE control;
};

struct scsi_rw_6 {
    UBYTE opcode;
    UBYTE addr[3];
#define SRW_TOPADDR 0x1f
    UBYTE length;
    UBYTE control;
};

struct scsi_rw_10 {
    UBYTE opcode;
    UBYTE byte2;
    UBYTE addr[4];
    UBYTE reserved;
    UBYTE length[2];
    UBYTE control;
};

struct scsi_rw_12 {
    UBYTE opcode;
    UBYTE byte2;
    UBYTE addr[4];
    UBYTE length[4];
    UBYTE resv;
    UBYTE control;
};

struct scsi_rw_16 {
    UBYTE opcode;
    UBYTE byte2;
    UBYTE addr[8];
    UBYTE length[4];
    UBYTE resv;
    UBYTE control;
};

struct scsi_read_capacity_data {
    UBYTE addr[4];
    UBYTE length[4];
};

struct scsi_read_capacity {
    UBYTE opcode;
    UBYTE byte2;
#define SRC_RELADR      0x01
    UBYTE addr[4];
    UBYTE unused[2];
    UBYTE pmi;
#define SRC_PMI         0x01
    UBYTE control;
};

struct scsi_read_capacity_16 {
    UBYTE opcode;
#define SRC16_SERVICE_ACTION 0x10
    UBYTE service_action;
    UBYTE addr[8];
    UBYTE alloc_len[4];
#define SRC16_PMI       0x01
#define SRC16_RELADR    0x02
    UBYTE reladr;
    UBYTE control;
};

typedef union scsi_cdb {
    struct scsi_generic generic;
    struct scsi_inquiry inquiry;
    struct scsi_read_capacity read_capacity;
    struct scsi_rw_6 rw_6;
    struct scsi_rw_10 rw_10;
    struct scsi_rw_12 rw_12;
    struct scsi_rw_16 rw_16;
    struct scsi_sense sense;
} *scsi_cdb_t;


BOOL ahci_scsi_disk_io(struct IORequest *io, struct SCSICmd *scsi);
BOOL ahci_scsi_atapi_io(struct IORequest *io, struct SCSICmd *scsi);

#endif /* AHCI_SCSI_H */
