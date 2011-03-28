/*
    Copyright © 1995-2007, The AROS Development Team. All rights reserved.
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
#include <resources/disk.h>

/* Stack size - 4096 longwords should be enough */
#define STACK_SIZE      16384

/* Maximum number of units */
#define TD_NUMUNITS 4

/* something */
#define TDUF_WRITE      (1<<0)

struct TaskData
{
    struct MsgPort      td_Port;                // MessagePort
    struct Task         td_Task;                // Task Structure
    APTR                td_Stack;   // Keep stack here
};

#define TDU_NODISK	0x00
#define TDU_DISK	0x01

#define TDU_READONLY	0x01
#define TDU_WRITABLE	0x00

struct TDU
{
    struct	TDU_PublicUnit pub;
    struct List	tdu_Listeners;
    UBYTE	tdu_UnitNum;		/* Unit number */
    UBYTE	tdu_DiskIn;			/* Disk in drive? */
    UBYTE	tdu_MotorOn;		/* Motor on? */
    UBYTE	tdu_ProtStatus;
    UBYTE	tdu_flags;
    BOOL    tdu_hddisk;			/* HD disk inserted */
    BOOL    tdu_broken;			/* recalibrate didn't find TRACK0, drive ignored */
    UBYTE	tdu_lastdir;		/* last step direction */
    UBYTE	tdu_sectors;		/* number of sectors per track */
    BOOL    tdu_selected;
};

struct TrackDiskBase
{
    struct Device           td_device;
    struct TaskData			*td_TaskData;
    struct TDU				*td_Units[TD_NUMUNITS];
    struct timerequest  	*td_TimerIO;
    struct timerequest  	*td_TimerIO2;
    struct MsgPort      	*td_TimerMP;
    struct MsgPort      	*td_TimerMP2;
    struct DiskBase 		*td_DiskBase;
    struct DiscResourceUnit td_dru;
    struct MsgPort			td_druport;
    struct Task				*td_task;
    APTR					td_DMABuffer;		/* Buffer for DMA accesses */
    UBYTE					*td_DataBuffer;
    ULONG					td_sectorbits;
    volatile struct Custom 	*custom;
    volatile struct CIA 	*ciaa;
    volatile struct CIA 	*ciab;
    ULONG					td_IntBit;
    ULONG					td_TmoBit;
    BOOL					td_nomount;
    BOOL					td_supportHD;
    WORD					td_buffer_track;
    BYTE					td_buffer_unit;		/* buffer contains this unit's track */
    BOOL					td_dirty;
};

#endif /* TRACKDISK_DEVICE_H */
