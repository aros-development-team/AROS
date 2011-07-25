#ifndef _DEVICEIO_H
#define _DEVICEIO_H

#include <devices/scsidisk.h>

struct SCSI10Cmd {
  UBYTE Opcode;
  UBYTE Lun;
  ULONG LBA;
  UBYTE Reserved;
  UBYTE Length[2];
  UBYTE Control;
};

struct Read10Cmd {
  UBYTE Opcode;
  UBYTE Lun;
  ULONG LBA;
  UBYTE Reserved;
  UBYTE Length[2];
  UBYTE Control;
};

struct Write10Cmd {
  UBYTE Opcode;
  UBYTE Lun;
  ULONG LBA;
  UBYTE Reserved;
  UBYTE Length[2];
  UBYTE Control;
};

#define SCSICMD_READ10   0x28
#define SCSICMD_WRITE10  0x2a

#define DIO_READ  (1)
#define DIO_WRITE (2)

/* Errorcodes returned by device io functions, in addition to the
   errors which can be returned by the OpenDevice() call when
   calling initdeviceio(): */

#define ERROR_NO_64BIT_SUPPORT    (95)    /* Returned when the partition requires 64-bit support
                                             to function, but no 64-bit support is present. */
#define ERROR_OUTSIDE_PARTITION   (99)


/* Values returned by deviceapiused() */

#define DAU_NORMAL     (0)
#define DAU_NSD        (1)
#define DAU_TD64       (2)
#define DAU_SCSIDIRECT (3)

struct fsIORequest {
  struct fsIORequest *next;
  struct IOStdReq *ioreq;
  struct SCSICmd scsicmd;
  struct SCSI10Cmd scsi10cmd;
  UWORD action;
};

#endif // _DEVICEIO_H
