#ifndef DEVICES_BLUETOOTHHCI_H
#define DEVICES_BLUETOOTHHCI_H
/*
**	$VER: bluetoothhci.h 1.1 (22.12.2011)
**
**	standard bluetooth host controller interface device include file
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

#ifndef BLUETOOTH_HCI_H
#include "bluetooth/hci.h"
#endif

/* IO Request structure */

struct IOBTHCIReq
{
    struct IORequest iobt_Req;
    ULONG iobt_Actual;         /* Actual bytes transferred */
    ULONG iobt_Length;         /* Size of buffer */
    APTR  iobt_Data;           /* Pointer to in/out buffer */
    APTR  iobt_UserData;       /* private data, may not be touched by hardware driver,
                                  do not make assumptions about its contents */
};

/* BT HCI Event Message returned by HCI Driver */
struct BTHCIEventMsg
{
    struct Message    bem_Msg; /* message header */
    struct BTHCIEvent bem_Event; /* actual event */
};

/* non-standard commands */

#define BTCMD_QUERYDEVICE (CMD_NONSTD+0)
#define BTCMD_WRITEHCI    (CMD_NONSTD+1)
#define BTCMD_READEVENT   (CMD_NONSTD+2)
#define BTCMD_READACL     (CMD_NONSTD+3)
#define BTCMD_WRITEACL    (CMD_NONSTD+4)
#define BTCMD_SETUPSCO    (CMD_NONSTD+5)
#define BTCMD_READSCO     (CMD_NONSTD+6)
#define BTCMD_WRITESCO    (CMD_NONSTD+7)
#define BTCMD_ADDMSGPORT  (CMD_NONSTD+8)
#define BTCMD_REMMSGPORT  (CMD_NONSTD+9)

/* Error codes for io_Error field */

#define BTIOERR_NO_ERROR      0   /* No error occured */
#define BTIOERR_BTOFFLINE     1   /* USB non-operational */
#define BTIOERR_HOSTERROR     3   /* Unspecific host error */
#define BTIOERR_TIMEOUT       6   /* No acknoledge on packet */
#define BTIOERR_OVERFLOW      7   /* More data received than expected */
#define BTIOERR_BADPARAMS    11   /* Illegal parameters in request */
#define BTIOERR_OUTOFMEMORY  12   /* Out of auxiliary memory for the driver */

/* Tags for BTCMD_QUERYDEVICE */

#define BTA_Dummy          (TAG_USER  + 0x4711)
#define BTA_Author         (BTA_Dummy + 0x10)
#define BTA_ProductName    (BTA_Dummy + 0x11)
#define BTA_Version        (BTA_Dummy + 0x12)
#define BTA_Revision       (BTA_Dummy + 0x13)
#define BTA_Description    (BTA_Dummy + 0x14)
#define BTA_Copyright      (BTA_Dummy + 0x15)
#define BTA_DriverVersion  (BTA_Dummy + 0x20)

#endif	/* DEVICES_BLUETOOTHHCI_H */
