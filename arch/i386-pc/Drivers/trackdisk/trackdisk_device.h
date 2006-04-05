/*
    Copyright © 1995-2006, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Internal defs for trackdisk
    Lang: English
*/
#ifndef TRACKDISK_DEVICE_H
#define TRACKDISK_DEVICE_H

#include <exec/types.h>
#include <exec/devices.h>
#include <exec/semaphores.h>
#include <exec/interrupts.h>
#include <exec/tasks.h>

#include <devices/trackdisk.h>

/* Stack size - 4096 longwords should be enough */
#define STACK_SIZE      16384

/* Maximum number of units */
#define TD_NUMUNITS 2

/* something */
#define TDUF_WRITE      (1<<0)

struct TaskData
{
    struct MsgPort      td_Port;                // MessagePort
    BYTE                td_Flags;               // Flags, see unit flags
    BYTE                td_pad[3];
    struct Task         td_Task;                // Task Structure
    BYTE                td_Stack[STACK_SIZE];   // Keep stack here
};

#define TDU_NODISK	0x00
#define TDU_DISK	0x01

#define TDU_READONLY	0x01
#define TDU_WRITABLE	0x00

struct TDU
{
    struct	TDU_PublicUnit pub;
    struct List	tdu_Listeners;
    APTR	td_DMABuffer;		/* Buffer for DMA accesses */
    ULONG	tdu_ChangeNum;		/* Number of changes occured */
    BOOL	tdu_Busy;		/* Unit working? */
    UBYTE	tdu_UnitNum;		/* Unit number */
    UBYTE	tdu_DiskIn;		/* Disk in drive? */
    UBYTE	tdu_pcn;		/* Current track */
    UBYTE	tdu_ProtStatus;
    UBYTE	tdu_lastcyl;
    UBYTE	tdu_lasthd;
    UBYTE	tdu_flags;
};

struct TrackDiskBase
{
    struct Device           	td_device;
    struct ExecBase         	*sysbase;		/* Useless for native but... */
    struct TaskData		*td_TaskData;
    struct TDU			*td_Units[TD_NUMUNITS];
    struct timerequest  	*td_TimerIO;
    struct MsgPort      	*td_TimerMP;
    ULONG			td_IntBit;		/* Sigbit for floppyints */
    ULONG			td_TmoBit;		/* Used for timeout signaling */
    BOOL			td_click;		/* Check for diskchanges? */
    UBYTE			td_comsize;        	/* RAW command size */
    UBYTE			td_rawcom[9];      	/* RAW command to send */
    UBYTE			td_result[7];      	/* Last set of bytes */
    UBYTE			td_dor;			/* Digital Output Register */
    UBYTE			td_sr0;
    UBYTE			td_pcn;
    UBYTE			td_inttmo;
};

#endif /* TRACKDISK_DEVICE_H */
