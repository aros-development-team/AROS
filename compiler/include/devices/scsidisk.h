#ifndef DEVICES_SCSIDISK_H
#define DEVICES_SCSIDISK_H

/*
    Copyright © 1995-2009, The AROS Development Team. All rights reserved.
    $Id$

    Desc: Definitions for scsi.device
    Lang: english
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif

#define HD_SCSICMD      28

#define HD_WIDESCSI      8    /* Used as part of unit number when a wide SCSI
                               * address is used */

struct SCSICmd
{
    UWORD       *scsi_Data;     /* Points to data used in data phase of command */
    ULONG       scsi_Length;    /* Length of Data */
    ULONG       scsi_Actual;
    UBYTE       *scsi_Command;  /* SCSI command */
    UWORD       scsi_CmdLength; /* length of SCSI command */
    UWORD       scsi_CmdActual;
    UBYTE       scsi_Flags;
    UBYTE       scsi_Status;
    UBYTE       *scsi_SenseData;
    UWORD       scsi_SenseLength;
    UWORD       scsi_SenseActual;
};

/* SCSI flags */

#define SCSIF_WRITE         0
#define SCSIF_READ          1
#define SCSIB_READ_WRITE    0

#define SCSIF_NOSENSE       0
#define SCSIF_AUTOSENSE     2
#define SCSIB_AUTOSENSE     1

#define SCSIF_OLDAUTOSENSE  6
#define SCSIB_OLDAUTOSENSE  2

/* SCSI io_Error values */

#define HFERR_SelfUnit      40
#define HFERR_DMA           41
#define HFERR_Phase         42
#define HFERR_Parity        43
#define HFERR_SelTimeout    44
#define HFERR_BadStatus     45

/* SCSI OpenDevice() io_Error values */

#define HFERR_NoBoard       50

#endif /* DEVICES_SCSIDISK_H */
