/*
 * Copyright (C) 2020, The AROS Development Team.  All rights reserved.
 */

#ifndef _HARDWARE_NVME_H
#define _HARDWARE_NVME_H

#include <exec/types.h>

struct nvme_registers {
    UQUAD			cap;	/* Controller Capabilities */
    ULONG			vs;	/* Version */
    ULONG			intms;	/* Interrupt Mask Set */
    ULONG			intmc;	/* Interrupt Mask Clear */
    ULONG			cc;	/* Controller Configuration */
    ULONG			rsvd1;	/* Reserved */
    ULONG			csts;	/* Controller Status */
    ULONG			nssr;	/* NVM Subsystem Reset */
    ULONG			aqa;	/* Admin Queue Attributes */
    UQUAD			asq;	/* Admin Submission Queue Base Address */
    UQUAD			acq;	/* Admin Completion Queue Base Address */
};

struct nvme_id_power_state {
    UWORD			max_power;	/* centiwatts */
    UWORD			rsvd2;
    ULONG			entry_lat;	/* microseconds */
    ULONG			exit_lat;	/* microseconds */
    UBYTE			read_tput;
    UBYTE			read_lat;
    UBYTE			write_tput;
    UBYTE			write_lat;
    UBYTE			rsvd16[16];
};

struct nvme_id_ctrl {
    UWORD			vid;
    UWORD			ssvid;
    char			sn[20];
    char			mn[40];
    char			fr[8];
    UBYTE			rab;
    UBYTE			ieee[3];
    UBYTE			mic;
    UBYTE			mdts;
    UBYTE                       cntlid[2];
    UBYTE			rsvd78[176];
    UWORD			oacs;
    UBYTE			acl;
    UBYTE			aerl;
    UBYTE			frmw;
    UBYTE			lpa;
    UBYTE			elpe;
    UBYTE			npss;
    UBYTE                       avscc;
    UBYTE                       apsta;
    UBYTE			rsvd264[246];
    UBYTE			sqes;
    UBYTE			cqes;
    UBYTE			rsvd514[2];
    ULONG			nn;
    UWORD			oncs;
    UWORD			fuses;
    UBYTE			fna;
    UBYTE			vwc;
    UWORD			awun;
    UWORD			awupf;
    UBYTE			rsvd530[1518];
    struct nvme_id_power_state	psd[32];
    UBYTE			vs[1024];
};

/* i/o commands */
enum nvme_opcode {
    nvme_cmd_flush		= 0x00,
    nvme_cmd_write		= 0x01,
    nvme_cmd_read		= 0x02,
    nvme_cmd_write_uncor	= 0x04,
    nvme_cmd_compare	        = 0x05,
    nvme_cmd_dsm		= 0x09,
};

/* admin commands */
enum nvme_admin_opcode {
    nvme_admin_delete_sq        = 0x00,
    nvme_admin_create_sq        = 0x01,
    nvme_admin_get_log_page     = 0x02,
    nvme_admin_delete_cq        = 0x04,
    nvme_admin_create_cq        = 0x05,
    nvme_admin_identify		= 0x06,
    nvme_admin_abort_cmd        = 0x08,
    nvme_admin_set_features     = 0x09,
    nvme_admin_get_features     = 0x0a,
    nvme_admin_async_event      = 0x0c,
    nvme_admin_activate_fw      = 0x10,
    nvme_admin_download_fw      = 0x11,
    nvme_admin_format_nvm       = 0x80,
    nvme_admin_security_send	= 0x81,
    nvme_admin_security_recv	= 0x82,
};

#define NVME_CAP_TIMEOUT(cap)	(((cap) >> 24) & 0xff)
#define NVME_CAP_STRIDE(cap)	(((cap) >> 32) & 0xf)

enum {
	NVME_CC_ENABLE		= 1 << 0,
	NVME_CC_CSS_NVM		= 0 << 4,
	NVME_CC_MPS_SHIFT	= 7,
	NVME_CC_ARB_RR		= 0 << 11,
	NVME_CC_ARB_WRRU	= 1 << 11,
	NVME_CC_ARB_VS		= 7 << 11,
	NVME_CC_SHN_NONE	= 0 << 14,
	NVME_CC_SHN_NORMAL	= 1 << 14,
	NVME_CC_SHN_ABRUPT	= 2 << 14,
	NVME_CC_IOSQES		= 6 << 16,
	NVME_CC_IOCQES		= 4 << 20,
	NVME_CSTS_RDY		= 1 << 0,
	NVME_CSTS_CFS		= 1 << 1,
	NVME_CSTS_SHST_NORMAL	= 0 << 2,
	NVME_CSTS_SHST_OCCUR	= 1 << 2,
	NVME_CSTS_SHST_CMPLT	= 2 << 2,
};

struct nvme_completion {
    ULONG	result;		/* Used by admin commands to return data */
    ULONG	rsvd;
    UWORD	sq_head;	/* how much of this queue may be reclaimed */
    UWORD	sq_id;		/* submission queue that generated this entry */
    UWORD	command_id;	/* of the command which completed */
    UWORD	status;		/* did the command fail, and if so, why? */
};

struct nvme_op {
    UBYTE			opcode;
    UBYTE			flags;
    UWORD			command_id;
};

struct nvme_common_command {
    struct nvme_op              op;
    ULONG			nsid;
    ULONG			cdw2[2];
    UQUAD			metadata;
    UQUAD			prp1;
    UQUAD			prp2;
    ULONG			cdw10[6];
};

struct nvme_rw_command {
    struct nvme_op              op;
    ULONG			nsid;
    UQUAD			rsvd2;
    UQUAD			metadata;
    UQUAD			prp1;
    UQUAD			prp2;
    UQUAD			slba;
    UWORD			length;
    UWORD			control;
    ULONG			dsmgmt;
    ULONG			reftag;
    UWORD			apptag;
    UWORD			appmask;
};

struct nvme_identify {
    struct nvme_op              op;
    ULONG			nsid;
    UQUAD			rsvd2[2];
    UQUAD			prp1;
    UQUAD			prp2;
    ULONG			cns;
    ULONG			rsvd11[5];
};

enum {
    NVME_QUEUE_PHYS_CONTIG	= (1 << 0),
    NVME_CQ_IRQ_ENABLED	= (1 << 1),
    NVME_SQ_PRIO_URGENT	= (0 << 1),
    NVME_SQ_PRIO_HIGH	        = (1 << 1),
    NVME_SQ_PRIO_MEDIUM	= (2 << 1),
    NVME_SQ_PRIO_LOW	        = (3 << 1),
    NVME_FEAT_ARBITRATION	= 0x01,
    NVME_FEAT_POWER_MGMT	= 0x02,
    NVME_FEAT_LBA_RANGE	        = 0x03,
    NVME_FEAT_TEMP_THRESH	= 0x04,
    NVME_FEAT_ERR_RECOVERY	= 0x05,
    NVME_FEAT_VOLATILE_WC	= 0x06,
    NVME_FEAT_NUM_QUEUES	= 0x07,
    NVME_FEAT_IRQ_COALESCE	= 0x08,
    NVME_FEAT_IRQ_CONFIG	= 0x09,
    NVME_FEAT_WRITE_ATOMIC	= 0x0a,
    NVME_FEAT_ASYNC_EVENT	= 0x0b,
    NVME_FEAT_SW_PROGRESS	= 0x0c,
};

struct nvme_features {
    struct nvme_op              op;
    ULONG			nsid;
    UQUAD			rsvd2[2];
    UQUAD			prp1;
    UQUAD			prp2;
    ULONG			fid;
    ULONG			dword11;
    ULONG			rsvd12[4];
};

struct nvme_create_cq {
    struct nvme_op              op;
    ULONG			rsvd1[5];
    UQUAD			prp1;
    UQUAD			rsvd8;
    UWORD			cqid;
    UWORD			qsize;
    UWORD			cq_flags;
    UWORD			irq_vector;
    ULONG			rsvd12[4];
};

struct nvme_create_sq {
    struct nvme_op              op;
    ULONG			rsvd1[5];
    UQUAD			prp1;
    UQUAD			rsvd8;
    UWORD			sqid;
    UWORD			qsize;
    UWORD			sq_flags;
    UWORD			cqid;
    ULONG			rsvd12[4];
};

struct nvme_delete_queue {
    struct nvme_op              op;
    ULONG			rsvd1[9];
    UWORD			qid;
    UWORD			rsvd10;
    ULONG			rsvd11[5];
};

struct nvme_download_firmware {
    struct nvme_op              op;
    ULONG			rsvd1[5];
    UQUAD			prp1;
    UQUAD			prp2;
    ULONG			numd;
    ULONG			offset;
    ULONG			rsvd12[4];
};

struct nvme_command {
    union {
        struct nvme_common_command      common;
        struct nvme_rw_command          rw;
        struct nvme_identify            identify;
        struct nvme_features            features;
        struct nvme_create_cq           create_cq;
        struct nvme_create_sq           create_sq;
        struct nvme_delete_queue        delete_queue;
        struct nvme_download_firmware   dlfw;
    };
};

struct nvme_lbaf {
    UWORD			ms;
    UBYTE			ds;
    UBYTE			rp;
};

struct nvme_id_ns {
    UQUAD			nsze;
    UQUAD			ncap;
    UQUAD			nuse;
    UBYTE			nsfeat;
    UBYTE			nlbaf;
    UBYTE			flbas;
    UBYTE			mc;
    UBYTE			dpc;
    UBYTE			dps;
    UBYTE			rsvd30[98];
    struct nvme_lbaf	lbaf[16];
    UBYTE			rsvd192[192];
    UBYTE			vs[3712];
};

struct nvme_lba_range_type {
    UBYTE			type;
    UBYTE			attributes;
    UBYTE			rsvd2[14];
    UQUAD			slba;
    UQUAD			nlb;
    UBYTE			guid[16];
    UBYTE			rsvd48[16];
};

enum {
    NVME_LBART_TYPE_FS	        = 0x01,
    NVME_LBART_TYPE_RAID	= 0x02,
    NVME_LBART_TYPE_CACHE	= 0x03,
    NVME_LBART_TYPE_SWAP	= 0x04,

    NVME_LBART_ATTRIB_TEMP	= 1 << 0,
    NVME_LBART_ATTRIB_HIDE	= 1 << 1,
};

#endif
