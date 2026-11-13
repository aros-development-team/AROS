/*
 * Copyright (c) 1990, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
 * Copyright (C) 2005-2026 The AROS Dev Team
 *
 * This code is derived from the Stanford/CMU enet packet filter,
 * (net/enet.h) distributed as part of 4.3BSD, and code contributed
 * to Berkeley by Steven McCanne and Van Jacobson both of Lawrence
 * Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.
 *
 * @(#)bpf.h	8.1 (Berkeley) 6/10/93
 *
 * The userland interface of the BPF calls in bsdsocket.library (bpf_open(),
 * bpf_ioctl(), bpf_read(), ...): the records a reader receives, the filter
 * program, the BIOC* requests and the data link types. The stack's own
 * descriptor and functions are in its local net/bpf.h.
 * $FreeBSD$
 */

#ifndef _NET_BPF_H_
#define _NET_BPF_H_

#include <sys/types.h>
#include <sys/time.h>	/* struct timeval in struct bpf_hdr */

/*
 * Alignment macros.  BPF_WORDALIGN rounds up to the next even multiple
 * of BPF_ALIGNMENT.
 */
#define BPF_ALIGNMENT	sizeof(long)
#define BPF_WORDALIGN(x) (((x) + (BPF_ALIGNMENT - 1)) & ~(BPF_ALIGNMENT - 1))

#define BPF_MAXINSNS	512
#define BPF_MAXBUFSIZE	0x8000
#define BPF_DFLTBUFSIZE	(4096)
#define BPF_MINBUFSIZE	32

/*
 * Structure prepended to each packet delivered to a BPF reader.
 */
struct bpf_hdr {
	struct timeval	bh_tstamp;	/* time stamp */
	u_int32_t	bh_caplen;	/* length of captured portion */
	u_int32_t	bh_datalen;	/* original length of packet */
	u_short		bh_hdrlen;	/* length of bpf header (this struct
					   plus alignment padding) */
};

/*
 * Data link level type codes (DLT_*).
 */
#define DLT_NULL	0	/* BSD loopback encapsulation */
#define DLT_EN10MB	1	/* Ethernet (10Mb) */
#define DLT_EN3MB	2	/* Experimental Ethernet (3Mb) */
#define DLT_AX25	3	/* Amateur Radio AX.25 */
#define DLT_PRONET	4	/* Proteon ProNET Token Ring */
#define DLT_CHAOS	5	/* Chaos */
#define DLT_IEEE802	6	/* 802.5 Token Ring */
#define DLT_ARCNET	7	/* ARCNET, with BSD-style header */
#define DLT_SLIP	8	/* Serial Line IP */
#define DLT_PPP		9	/* Point-to-point Protocol */
#define DLT_FDDI	10	/* FDDI */
#define DLT_ATM_RFC1483	11	/* LLC/SNAP encapsulated atm */
#define DLT_RAW		12	/* raw IP */
#define DLT_SLIP_BSDOS	15	/* BSD/OS Serial Line IP */
#define DLT_PPP_BSDOS	16	/* BSD/OS Point-to-point Protocol */
#define DLT_IEEE802_11	105	/* IEEE 802.11 wireless */
#define DLT_LOOP	108	/* OpenBSD loopback */
#define DLT_LINUX_SLL	113	/* Linux cooked sockets */

/*
 * BPF ioctl commands — Roadshow-compatible encoding.
 * Uses _IOR/_IOW/_IOWR macros with group 'B', matching Roadshow's
 * public net/bpf.h exactly.
 */
#include <sys/ioctl.h>

#define	BIOCGBLEN	_IOR('B', 102, u_int)
#define	BIOCSBLEN	_IOWR('B', 102, u_int)
#define	BIOCSETF	_IOW('B', 103, struct bpf_program)
#define	BIOCFLUSH	_IO('B', 104)
#define	BIOCPROMISC	_IO('B', 105)
#define	BIOCGDLT	_IOR('B', 106, u_int)
#define	BIOCGETIF	_IOR('B', 107, struct ifreq)
#define	BIOCSETIF	_IOW('B', 108, struct ifreq)
#define	BIOCSRTIMEOUT	_IOW('B', 109, struct timeval)
#define	BIOCGRTIMEOUT	_IOR('B', 110, struct timeval)
#define	BIOCGSTATS	_IOR('B', 111, struct bpf_stat)
#define	BIOCIMMEDIATE	_IOW('B', 112, u_int)
#define	BIOCVERSION	_IOR('B', 113, struct bpf_version)

/* FreeBSD extensions (not in Roadshow but useful) */
#define	BIOCGHDRCMPLT	_IOR('B', 114, u_int)
#define	BIOCSHDRCMPLT	_IOW('B', 115, u_int)
#define	BIOCGSEESENT	_IOR('B', 116, u_int)
#define	BIOCSSEESENT	_IOW('B', 117, u_int)

/*
 * BPF statistics structure.
 */
struct bpf_stat {
	u_int	bs_recv;	/* packets received */
	u_int	bs_drop;	/* packets dropped */
};

/*
 * BPF version.
 */
struct bpf_version {
	u_short	bv_major;
	u_short	bv_minor;
};

#define BPF_MAJOR_VERSION	1
#define BPF_MINOR_VERSION	1

/*
 * BPF filter program structure.
 * A filter program is an array of instructions.
 */
struct bpf_insn {
	u_short		code;
	u_char		jt;	/* jump-true offset */
	u_char		jf;	/* jump-false offset */
	u_int32_t	k;	/* generic multiuse field */
};

struct bpf_program {
	u_int		bf_len;
	struct bpf_insn	*bf_insns;
};

/*
 * Macros for insn.code (instruction classes and subfields).
 */

/* Instruction classes */
#define BPF_CLASS(code)	((code) & 0x07)
#define	BPF_LD		0x00
#define	BPF_LDX		0x01
#define	BPF_ST		0x02
#define	BPF_STX		0x03
#define	BPF_ALU		0x04
#define	BPF_JMP		0x05
#define	BPF_RET		0x06
#define	BPF_MISC	0x07

/* LD/LDX fields: size */
#define BPF_SIZE(code)	((code) & 0x18)
#define	BPF_W		0x00	/* 32-bit word */
#define	BPF_H		0x08	/* 16-bit half-word */
#define	BPF_B		0x10	/* 8-bit byte */

/* LD/LDX fields: mode */
#define BPF_MODE(code)	((code) & 0xe0)
#define	BPF_IMM		0x00
#define	BPF_ABS		0x20
#define	BPF_IND		0x40
#define	BPF_MEM		0x60
#define	BPF_LEN		0x80
#define	BPF_MSH		0xa0

/* ALU/JMP fields: operation */
#define BPF_OP(code)	((code) & 0xf0)
#define	BPF_ADD		0x00
#define	BPF_SUB		0x10
#define	BPF_MUL		0x20
#define	BPF_DIV		0x30
#define	BPF_OR		0x40
#define	BPF_AND		0x50
#define	BPF_LSH		0x60
#define	BPF_RSH		0x70
#define	BPF_NEG		0x80
#define	BPF_MOD		0x90
#define	BPF_XOR		0xa0

/* JMP operations */
#define	BPF_JA		0x00
#define	BPF_JEQ		0x10
#define	BPF_JGT		0x20
#define	BPF_JGE		0x30
#define	BPF_JSET	0x40

/* Source operand */
#define BPF_SRC(code)	((code) & 0x08)
#define	BPF_K		0x00	/* constant */
#define	BPF_X		0x08	/* index register */

/* RET source */
#define BPF_RVAL(code)	((code) & 0x18)
#define	BPF_A		0x10

/* MISC operations */
#define BPF_MISCOP(code) ((code) & 0xf8)
#define	BPF_TAX		0x00
#define	BPF_TXA		0x80

/* Number of scratch memory words */
#define BPF_MEMWORDS	16

/*
 * Macros for constructing filter instructions.
 */
#define BPF_STMT(code, k) { (u_short)(code), 0, 0, k }
#define BPF_JUMP(code, k, jt, jf) { (u_short)(code), jt, jf, k }

#endif /* _NET_BPF_H_ */
