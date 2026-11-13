/*
 * Copyright (C) 2012-2020, The AROS Development Team.  All rights reserved.
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
    UBYTE resv;
    UBYTE length;	/* number of VPD entries */
#define SVPD_SUPPORTED_PAGES_SIZE 251
    UBYTE list[SVPD_SUPPORTED_PAGES_SIZE];
};

struct scsi_vpd_unit_serial_number {
    UBYTE device;
    UBYTE page_code;
#define SVPD_UNIT_SERIAL_NUMBER 0x80
    UBYTE resv;
    UBYTE length; /* serial number length */
#define SVPD_SERIAL_NUM_SIZE 251
    UBYTE serial_num[SVPD_SERIAL_NUM_SIZE];
};

struct scsi_vpd_unit_devid {
    UBYTE device;
    UBYTE page_code;
#define SVPD_UNIT_DEVID 0x83
	u_int8_t reserved;
	u_int8_t length;
	/* extended by variable array of scsi_vpd_devid_hdr */
};

struct scsi_generic {
    UBYTE opcode;
    UBYTE resv[11];
};
struct scsi_inquiry {
    UBYTE opcode;
    UBYTE byte2;
#define SI_EVPD (1 << 0)
    UBYTE page_code;
    UBYTE reserved;
    UBYTE length;
    UBYTE control;
};

/*
 * Device Types
 */
#define	T_DIRECT	0x00
#define	T_SEQUENTIAL	0x01
#define	T_PRINTER	0x02
#define	T_PROCESSOR	0x03
#define	T_WORM		0x04
#define	T_CDROM		0x05
#define	T_SCANNER	0x06
#define	T_OPTICAL 	0x07
#define	T_CHANGER	0x08
#define	T_COMM		0x09
#define	T_ASC0		0x0a
#define	T_ASC1		0x0b
#define	T_STORARRAY	0x0c
#define	T_ENCLOSURE	0x0d
#define	T_RBC		0x0e
#define	T_OCRW		0x0f
#define	T_OSD		0x11
#define	T_ADC		0x12
#define	T_NODEVICE	0x1f
#define	T_ANY		0xff	/* Used in Quirk table matches */

#define T_REMOV		1
#define	T_FIXED		0

/*
 * This length is the initial inquiry length used by the probe code, as    
 * well as the legnth necessary for scsi_print_inquiry() to function 
 * correctly.  If either use requires a different length in the future, 
 * the two values should be de-coupled.
 */
#define	SHORT_INQUIRY_LENGTH	36

struct scsi_inquiry_data {
    UBYTE device;
#define	SID_TYPE(inq_data) ((inq_data)->device & 0x1f)
#define	SID_QUAL(inq_data) (((inq_data)->device & 0xE0) >> 5)
#define	SID_QUAL_LU_CONNECTED	0x00	/*
					 * The specified peripheral device
					 * type is currently connected to
					 * logical unit.  If the target cannot
					 * determine whether or not a physical
					 * device is currently connected, it
					 * shall also use this peripheral
					 * qualifier when returning the INQUIRY
					 * data.  This peripheral qualifier
					 * does not mean that the device is
					 * ready for access by the initiator.
					 */
#define	SID_QUAL_LU_OFFLINE	0x01	/*
					 * The target is capable of supporting
					 * the specified peripheral device type
					 * on this logical unit; however, the
					 * physical device is not currently
					 * connected to this logical unit.
					 */
#define SID_QUAL_RSVD		0x02
#define	SID_QUAL_BAD_LU		0x03	/*
					 * The target is not capable of
					 * supporting a physical device on
					 * this logical unit. For this
					 * peripheral qualifier the peripheral
					 * device type shall be set to 1Fh to
					 * provide compatibility with previous
					 * versions of SCSI. All other
					 * peripheral device type values are
					 * reserved for this peripheral
					 * qualifier.
					 */
#define	SID_QUAL_IS_VENDOR_UNIQUE(inq_data) ((SID_QUAL(inq_data) & 0x08) != 0)
    UBYTE dev_qual2;
#define	SID_QUAL2	0x7F
#define	SID_IS_REMOVABLE(inq_data) (((inq_data)->dev_qual2 & 0x80) != 0)
    UBYTE version;
#define SID_ANSI_REV(inq_data) ((inq_data)->version & 0x07)
#define		SCSI_REV_0		0
#define		SCSI_REV_CCS		1
#define		SCSI_REV_2		2
#define		SCSI_REV_SPC		3
#define		SCSI_REV_SPC2		4
#define		SCSI_REV_SPC3		5

#define SID_ECMA	0x38
#define SID_ISO		0xC0
    UBYTE response_format;
#define SID_AENC	0x80
#define SID_TrmIOP	0x40
    UBYTE additional_length;
#define	SID_ADDITIONAL_LENGTH(iqd)					\
	((iqd)->additional_length +					\
	offsetof(struct scsi_inquiry_data, additional_length) + 1)
    UBYTE reserved;
    UBYTE spc2_flags;
#define SPC2_SID_MChngr 	0x08
#define SPC2_SID_MultiP 	0x10
#define SPC2_SID_EncServ	0x40
#define SPC2_SID_BQueue		0x80

#define INQ_DATA_TQ_ENABLED(iqd)				\
    ((SID_ANSI_REV(iqd) < SCSI_REV_SPC2)? ((iqd)->flags & SID_CmdQue) :	\
    (((iqd)->flags & SID_CmdQue) && !((iqd)->spc2_flags & SPC2_SID_BQueue)) || \
    (!((iqd)->flags & SID_CmdQue) && ((iqd)->spc2_flags & SPC2_SID_BQueue)))

    UBYTE flags;
#define	SID_SftRe	0x01
#define	SID_CmdQue	0x02
#define	SID_Linked	0x08
#define	SID_Sync	0x10
#define	SID_WBus16	0x20
#define	SID_WBus32	0x40
#define	SID_RelAdr	0x80
#define SID_VENDOR_SIZE   8
    UBYTE vendor[SID_VENDOR_SIZE];
#define SID_PRODUCT_SIZE 16
    UBYTE product[SID_PRODUCT_SIZE];
#define SID_REVISION_SIZE 4
    UBYTE revision[SID_REVISION_SIZE];
	/*
	 * The following fields were taken from SCSI Primary Commands - 2
	 * (SPC-2) Revision 14, Dated 11 November 1999
	 */
#define	SID_VENDOR_SPECIFIC_0_SIZE	20
    UBYTE vendor_specific0[SID_VENDOR_SPECIFIC_0_SIZE];
	/*
	 * An extension of SCSI Parallel Specific Values
	 */
#define	SID_SPI_IUS		0x01
#define	SID_SPI_QAS		0x02
#define	SID_SPI_CLOCK_ST	0x00
#define	SID_SPI_CLOCK_DT	0x04
#define	SID_SPI_CLOCK_DT_ST	0x0C
#define	SID_SPI_MASK		0x0F
    UBYTE spi3data;
    UBYTE reserved2;
	/*
	 * Version Descriptors, stored 2 byte values.
	 */
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
#define	SSD_ERRCODE			0x7F
#define		SSD_CURRENT_ERROR	0x70
#define		SSD_DEFERRED_ERROR	0x71
#define	SSD_ERRCODE_VALID	0x80
    UBYTE segment;
    UBYTE flags;
#define	SSD_KEY				0x0F
#define		SSD_KEY_NO_SENSE	0x00
#define		SSD_KEY_RECOVERED_ERROR	0x01
#define		SSD_KEY_NOT_READY	0x02
#define		SSD_KEY_MEDIUM_ERROR	0x03
#define		SSD_KEY_HARDWARE_ERROR	0x04
#define		SSD_KEY_ILLEGAL_REQUEST	0x05
#define		SSD_KEY_UNIT_ATTENTION	0x06
#define		SSD_KEY_DATA_PROTECT	0x07
#define		SSD_KEY_BLANK_CHECK	0x08
#define		SSD_KEY_Vendor_Specific	0x09
#define		SSD_KEY_COPY_ABORTED	0x0a
#define		SSD_KEY_ABORTED_COMMAND	0x0b		
#define		SSD_KEY_EQUAL		0x0c
#define		SSD_KEY_VOLUME_OVERFLOW	0x0d
#define		SSD_KEY_MISCOMPARE	0x0e
#define		SSD_KEY_RESERVED	0x0f			
#define	SSD_ILI		0x20
#define	SSD_EOM		0x40
#define	SSD_FILEMARK	0x80
    UBYTE info[4];
    UBYTE extra_len;
    UBYTE cmd_spec_info[4];
    UBYTE add_sense_code;
    UBYTE add_sense_code_qual;
    UBYTE fru;
    UBYTE sense_key_spec[3];
#define	SSD_SCS_VALID		0x80
#define SSD_FIELDPTR_CMD	0x40
#define SSD_BITPTR_VALID	0x08
#define SSD_BITPTR_VALUE	0x07
#define SSD_MIN_SIZE 18
    UBYTE extra_bytes[14];
#define SSD_FULL_SIZE sizeof(struct scsi_sense_data)
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
/* only 5 bits are valid in the MSB address byte */
#define SRW_TOPADDR 0x1f
    UBYTE length;
    UBYTE control;
};

struct scsi_rw_10 {
    UBYTE opcode;
#define	SRW10_RELADDR	0x01
/* EBP defined for WRITE(10) only */
#define SRW10_EBP	0x04
#define SRW10_FUA	0x08
#define	SRW10_DPO	0x10
    UBYTE byte2;
    UBYTE addr[4];
    UBYTE reserved;
    UBYTE length[2];
    UBYTE control;
};

struct scsi_rw_12 {
    UBYTE opcode;
#define	SRW12_RELADDR	0x01
#define SRW12_FUA	0x08
#define	SRW12_DPO	0x10
    UBYTE byte2;
    UBYTE addr[4];
    UBYTE length[4];
    UBYTE resv;
    UBYTE control;
};

struct scsi_rw_16 {
    UBYTE opcode;
#define        SRW16_RELADDR   0x01
#define        SRW16_FUA       0x08
#define        SRW16_DPO       0x10
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
    UBYTE addr[4];
    UBYTE unused[3];
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
