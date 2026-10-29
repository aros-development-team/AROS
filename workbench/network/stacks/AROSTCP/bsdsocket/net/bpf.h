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
 * The stack's part of the BPF interface: the descriptor and the kernel
 * functions. The userland part is the public <net/bpf.h>
 * (common/include/net/bpf.h), included here. Stack sources include this
 * file by its quoted name, as the sysroot copy wins an angle include
 * (Readme.md section 12).
 * $FreeBSD$
 */

#ifndef AROSTCP_NET_BPF_H
#define AROSTCP_NET_BPF_H

#include <net/bpf.h>

/*
 * BPF descriptor structure.
 * One per open bpf_open() handle.
 */
struct bpf_d {
	struct bpf_d	*bd_next;	/* global list link */

	/* Buffers: double-buffered store/hold scheme */
	caddr_t		bd_sbuf;	/* store buffer (receiving) */
	caddr_t		bd_hbuf;	/* hold buffer (ready to read) */
	int		bd_slen;	/* current length of store buffer */
	int		bd_hlen;	/* current length of hold buffer */
	int		bd_bufsize;	/* size of each buffer */

	/* Filter program */
	struct bpf_insn	*bd_filter;	/* filter code */
	u_int		bd_flen;	/* filter instruction count */

	/* Attached interface */
	struct ifnet	*bd_ifp;	/* interface pointer */
	int		bd_dlt;		/* link layer type */

	/* Statistics */
	u_int		bd_rcount;	/* packets received */
	u_int		bd_dcount;	/* packets dropped */
	u_int		bd_ccount;	/* packets captured */

	/* Flags and state */
	u_char		bd_promisc;	/* true if promiscuous mode */
	u_char		bd_immediate;	/* true if immediate mode */
	u_char		bd_seesent;	/* true if seeing sent packets */
	u_char		bd_hdrcmplt;	/* true if header complete on write */
	u_char		bd_inuse;	/* true if descriptor allocated */
	u_char		bd_waiting;	/* true if read is blocked */

	/* Amiga/AROS signal notification */
	ULONG		bd_notifymask;	/* signal mask for packet notification */
	ULONG		bd_intrmask;	/* signal mask for interrupts */
	struct Task	*bd_sigtask;	/* task to signal */

	/* Read timeout (0 = no timeout) */
	struct timeval	bd_rtimeout;
};

/* Maximum number of simultaneous BPF descriptors */
#define BPF_MAXDEVICES	16

/*
 * Kernel functions.
 */
#ifdef KERNEL

u_int	bpf_filter(const struct bpf_insn *, const u_char *, u_int, u_int);
int	bpf_validate(const struct bpf_insn *, int);
void	bpf_tap(struct ifnet *, u_char *, u_int, int);
void	bpf_mtap(struct ifnet *, struct mbuf *, int);
void	bpf_init(void);
void	bpf_cleanup(void);

/* Direction flags for bpf_tap/bpf_mtap */
#define BPF_D_IN	0	/* incoming packet */
#define BPF_D_OUT	1	/* outgoing packet */

#endif /* KERNEL */

#endif /* AROSTCP_NET_BPF_H */
