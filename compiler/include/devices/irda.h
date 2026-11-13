#ifndef DEVICES_IRDA_H
#define DEVICES_IRDA_H
/*
**	$VER: irda.h 1.1 (22.12.2011)
**
**	standard irda host controller interface device include file
**
**	(C) Copyright 2005 Chris Hodges
**	(C) Copyright 2011 AROS Development Team
**	    All Rights Reserved
*/

#ifndef EXEC_IO_H
#include "exec/io.h"
#endif

#ifndef EXEC_ERRORS_H
#include <exec/errors.h>
#endif

/* IO Request structure */

struct IOIrDAReq
{
    struct IORequest ioir_Req;
    ULONG ioir_Actual;         /* Actual bytes transferred */
    ULONG ioir_Length;         /* Size of buffer */
    APTR  ioir_Data;           /* Pointer to in/out buffer */
    ULONG ioir_Baud;           /* IrDA baud rate requested  */
    UWORD ioir_NumBOFs;        /* Number of BOFs in SIR */
    UBYTE ioir_Address;        /* LAP Address field */
    UBYTE ioir_Control;        /* LAP Control field */
    APTR  ioir_UserData;       /* private data, may not be touched by hardware driver,
                                  do not make assumptions about its contents */
};

/* non-standard commands */

#define IRCMD_QUERYDEVICE (CMD_NONSTD+0)

/* Error codes for io_Error field */

#define IRIOERR_NO_ERROR      0   /* No error occured */
#define IRIOERR_IRDAOFFLINE   1   /* IrDA non-operational */
#define IRIOERR_HOSTERROR     3   /* Unspecific host error */
#define IRIOERR_TIMEOUT       6   /* No acknoledge on packet */
#define IRIOERR_OVERFLOW      7   /* More data received than expected */
#define IRIOERR_BADPARAMS    11   /* Illegal parameters in request */
#define IRIOERR_OUTOFMEMORY  12   /* Out of auxiliary memory for the driver */

/* Tags for IRCMD_QUERYDEVICE */

#define IRA_Dummy          (TAG_USER  + 0x4711)
#define IRA_SuppBaudRate   (IRA_Dummy + 0x01)
#define IRA_SuppDataSize   (IRA_Dummy + 0x02)
#define IRA_Author         (IRA_Dummy + 0x10)
#define IRA_ProductName    (IRA_Dummy + 0x11)
#define IRA_Version        (IRA_Dummy + 0x12)
#define IRA_Revision       (IRA_Dummy + 0x13)
#define IRA_Description    (IRA_Dummy + 0x14)
#define IRA_Copyright      (IRA_Dummy + 0x15)
#define IRA_DriverVersion  (IRA_Dummy + 0x20)

#endif	/* DEVICES_IRDA_H */
