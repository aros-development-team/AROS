#ifndef DEVICES_SERIAL_H
#define DEVICES_SERIAL_H

/*
    Copyright © 1995-2001, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Serial device commands, structures, defintions etc.
    Lang: english
*/

#include "exec/io.h"

struct IOTArray 
{
  ULONG TermArray0;
  ULONG TermArray1;
};

#define SER_DEFAULT_CTLCHAR 0x11130000

struct IOExtSer 
{
  struct  IOStdReq IOSer;
  ULONG   io_CtlChar;
  ULONG   io_RBufLen;
  ULONG   io_ExtFlags;
  ULONG   io_Baud;
  ULONG   io_BrkTime;
  struct  IOTArray io_TermArray;
  UBYTE   io_ReadLen;
  UBYTE   io_WriteLen;
  UBYTE   io_StopBits;
  UBYTE   io_SerFlags;
  UWORD   io_Status;
};

#define   SDCMD_QUERY		CMD_NONSTD
#define   SDCMD_BREAK	       (CMD_NONSTD+1)
#define   SDCMD_SETPARAMS      (CMD_NONSTD+2)

/*
** The follwoing flags(bits) are to be set in IOExtSer->IOSer.io_Flags
*/

#define	SERB_PARTY_ON	0
#define	SERF_PARTY_ON	(1<<0)
#define	SERB_PARTY_ODD	1
#define	SERF_PARTY_ODD	(1<<1)
#define	SERB_7WIRE	2
#define	SERF_7WIRE	(1<<2)
#define	SERB_QUEUEDBRK	3
#define	SERF_QUEUEDBRK	(1<<3)
#define SERB_RAD_BOOGIE 4
#define SERF_RAD_BOOGIE (1<<4)
#define	SERB_SHARED	5
#define	SERF_SHARED	(1<<5)
#define	SERB_EOFMODE	6
#define	SERF_EOFMODE	(1<<6)
#define SERB_XDISABLED	7
#define SERF_XDISABLED	(1<<7)

/*
** The following flags(bits) are to be found in IOExtSer->io_Status
*/

#define	IO_STATB_OVERRUN    8
#define	IO_STATF_OVERRUN    (1<<8)
#define	IO_STATB_WROTEBREAK 9
#define	IO_STATF_WROTEBREAK (1<<9)
#define	IO_STATB_READBREAK  10
#define	IO_STATF_READBREAK  (1<<10)
#define	IO_STATB_XOFFWRITE  11
#define	IO_STATF_XOFFWRITE  (1<<11)
#define	IO_STATB_XOFFREAD   12
#define	IO_STATF_XOFFREAD   (1<<12) 




#define	SEXTB_MARK	0
#define	SEXTF_MARK	(1<<0)
#define	SEXTB_MSPON	1
#define	SEXTF_MSPON	(1<<1)


/*
** The follwoing error codes are to be found in IOExtSer->IOSer.io_Error
*/

#define SerErr_DevBusy	       1
#define SerErr_BaudMismatch    2
#define SerErr_BufErr	       4
#define SerErr_InvParam        5
#define SerErr_LineErr	       6
#define SerErr_ParityErr       9
#define SerErr_TimerErr       11
#define SerErr_BufOverflow    12
#define SerErr_NoDSR	      13
#define SerErr_DetectedBreak  15

#define SERIALNAME     "serial.device"

/*
**  Anything below this point is obsolete.
*/

#ifdef DEVICES_SERIAL_H_OBSOLETE
#define SerErr_InvBaud	       3
#define SerErr_NotOpen	       7
#define SerErr_PortReset       8
#define SerErr_InitErr	      10
#define SerErr_NoCTS	      14


#define	IOSTB_OVERRUN	 0
#define	IOSTF_OVERRUN	 (1<<0)
#define	IOSTB_WROTEBREAK 1
#define	IOSTF_WROTEBREAK (1<<1)
#define	IOSTB_READBREAK  2
#define	IOSTF_READBREAK  (1<<2)
#define	IOSTB_XOFFWRITE  3
#define	IOSTF_XOFFWRITE  (1<<3)
#define	IOSTB_XOFFREAD	 4
#define	IOSTF_XOFFREAD	 (1<<4)

#define	IOSERB_ACTIVE	4
#define	IOSERF_ACTIVE	(1<<4)
#define	IOSERB_ABORT	5
#define	IOSERF_ABORT	(1<<5)
#define	IOSERB_QUEUED	6
#define	IOSERF_QUEUED	(1<<6)
#define	IOSERB_BUFRREAD 7
#define	IOSERF_BUFRREAD (1<<7)
#endif


#endif  /*  DEVICES_SERIAL_H  */
