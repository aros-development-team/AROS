#ifndef DEVICES_FIDO_H
#define DEVICES_FIDO_H
/*
    Copyright (C) 2026, The AROS Development Team. All rights reserved.

    Desc: fido.device - raw access to the FIDO (CTAPHID) interface of a USB
          security key, as fido.class (Poseidon) provides it: one unit per
          key, 64-byte HID reports in and out. The CTAPHID protocol on top
          of it (channels, framing, U2F/CTAP2 messages) is the user's
          business; see the FIDO Alliance's CTAP specification.

          CMD_WRITE: io_Data/io_Length (at most the report size) is sent
                     as one report, padded with zeroes.
          CMD_READ:  the next report the key sends is copied to io_Data
                     (io_Length bytes at most, io_Actual the report size).
                     Several reads may be queued; a read waits until a
                     report arrives, or it is aborted (AbortIO()), or the
                     key is unplugged (IOERR_ABORTED).
          CMD_RESET: abort and drop every queued request.
          CMD_FLUSH: drop the queued requests.
          FIDOCMD_UNITINFO: fill the struct FidoUnitInfo at io_Data.
*/

#include <exec/io.h>

#define FIDO_DEVNAME        "fido.device"
#define FIDO_REPORTSIZE     64

#define FIDOCMD_UNITINFO    (CMD_NONSTD + 0)

struct FidoUnitInfo
{
    UWORD   VendorID;
    UWORD   ProductID;
    UWORD   ReportSize;         /* bytes per report in and out                  */
    UWORD   Flags;              /* FIDOUF_#?                                    */
    char    Product[64];        /* the key's product string, if it has one      */
};

#define FIDOUF_NOOUTENDPOINT    (1 << 0)    /* reports go out by SET_REPORT on endpoint 0 */

#endif /* DEVICES_FIDO_H */
