#ifndef DEVICES_PARALLEL_H
#define DEVICES_PARALLEL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Parallel device commands, structures, defintions etc.
    Lang: english
*/

#include "exec/io.h"

 struct  IOPArray {
	ULONG PTermArray0;
	ULONG PTermArray1;
};

struct   IOExtPar {
	struct	 IOStdReq IOPar;
	ULONG	io_PExtFlags;
	UBYTE	io_Status;
	UBYTE	io_ParFlags;
	struct	IOPArray io_PTermArray;
};

#define	PARB_SHARED	5
#define	PARF_SHARED	(1<<5)
#define PARB_SLOWMODE	4
#define PARF_SLOWMODE	(1<<4)
#define PARB_FASTMODE	3
#define PARF_FASTMODE	(1<<3)
#define PARB_RAD_BOOGIE	3
#define PARF_RAD_BOOGIE	(1<<3)

#define PARB_ACKMODE	2
#define PARF_ACKMODE	(1<<2)

#define PARB_EOFMODE	1
#define PARF_EOFMODE	(1<<1)

#define IOPARB_QUEUED	6
#define IOPARF_QUEUED	(1<<6)
#define	IOPARB_ABORT	5
#define	IOPARF_ABORT	(1<<5)
#define	IOPARB_ACTIVE	4
#define	IOPARF_ACTIVE	(1<<4)
#define	IOPTB_RWDIR	3
#define	IOPTF_RWDIR	(1<<3)
#define	IOPTB_PARSEL	2
#define	IOPTF_PARSEL	(1<<2)
#define	IOPTB_PAPEROUT 	1
#define	IOPTF_PAPEROUT 	(1<<1)
#define	IOPTB_PARBUSY	0
#define	IOPTF_PARBUSY	(1<<0)


#define PARALLELNAME		"parallel.device"

#define PDCMD_QUERY	(CMD_NONSTD)
#define PDCMD_SETPARAMS	(CMD_NONSTD+1)

#define ParErr_DevBusy		1
#define ParErr_BufTooBig	2
#define ParErr_InvParam		3
#define ParErr_LineErr		4
#define ParErr_NotOpen		5
#define ParErr_PortReset	6
#define ParErr_InitErr		7

#endif	/* DEVICES_PARALLEL_H */
